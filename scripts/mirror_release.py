#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
将 GitHub 仓库的 Releases（含预发布版）镜像同步到 CNB 仓库。

本脚本与 easytier-ffi-bin 仓库中的 CNB Release 同步脚本同源同向：GitHub 位于海外，
国内访问不便。当 release 发布在 GitHub 侧时，可在此脚本把 GitHub 的 release
（含预发布版）及全部附件镜像同步到 CNB 仓库，方便国内用户直接下载。

流程：
1. （可选，--wait-tag）先轮询等待 GitHub 出现指定 tag 的 release：
   用于 v* 分支发布流程——CNB 推送 vX.Y.Z 分支并同步代码到 GitHub 后，
   GitHub Actions 构建发布对应版本 release，本脚本每 2 分钟轮询一次、
   最长等待 2 小时，出现后立即开始同步。
2. 通过 GitHub API 获取源仓库全部 release（分页，公开仓库无需认证）。
3. 通过 CNB API 获取目标仓库已有的 release tag 集合（分页）。
4. 对每个「GitHub 有而 CNB 没有」的非草稿 release：
    下载其全部附件 -> 在 CNB 创建同名 release -> 依次上传附件（三步式）。
5. make_latest 判定：仅当该版本是 GitHub 侧最新正式版，且 CNB 侧不存在不晚于它的
   正式版（含 CNB 直接发布的版本）时，才标记为 CNB 最新版，避免抢占 CNB 已有发布。
6. 幂等：已存在的版本自动跳过，可重复触发。

环境变量：
    CNB_TOKEN     必填，CNB 访问令牌（云原生构建流水线自动注入）
    GITHUB_TOKEN  可选，GitHub 访问令牌（公开仓库无需）
    CNB_API_BASE  可选，默认 https://api.cnb.cool

依赖：仅 Python 3.8+ 标准库，无需第三方包。
"""

import argparse
import json
import os
import shutil
import sys
import tempfile
import time
import urllib.error
import urllib.parse
import urllib.request

API_GITHUB = "https://api.github.com"
API_CNB = os.environ.get("CNB_API_BASE", "https://api.cnb.cool")
DEFAULT_GITHUB_REPO = "qteasytier/qt-easy-tier"
DEFAULT_CNB_REPO = "myqfeng/qteasytier/qt-easy-tier"
USER_AGENT = "qt-easy-tier-cnb-mirror/1.0"
GITHUB_PAGE_SIZE = 100
CNB_PAGE_SIZE = 100


class ApiError(RuntimeError):
    def __init__(self, status, url, body):
        self.status = status
        self.url = url
        self.body = body
        super().__init__("HTTP %d %s: %s" % (status, url, (body or "")[:500]))


def _gh_headers(token):
    headers = {"Accept": "application/vnd.github+json", "User-Agent": USER_AGENT}
    if token:
        headers["Authorization"] = "Bearer " + token
    return headers


def _cnb_headers(token):
    headers = {
        "Accept": "application/vnd.cnb.api+json",
        "Content-Type": "application/json",
        "User-Agent": USER_AGENT,
    }
    if token:
        headers["Authorization"] = "Bearer " + token
    return headers


def _sleep_backoff(attempt):
    time.sleep(min(2 ** attempt, 30))


def http_json(url, headers=None, method="GET", payload=None, retries=4, timeout=120):
    """发送 JSON 请求并解析响应。payload 为 dict 时以 JSON 编码。"""
    hdrs = dict(headers or {})
    data = None
    if payload is not None:
        data = json.dumps(payload).encode("utf-8")
        hdrs.setdefault("Content-Type", "application/json")

    last_err = None
    for attempt in range(retries):
        req = urllib.request.Request(url, data=data, headers=hdrs, method=method)
        try:
            with urllib.request.urlopen(req, timeout=timeout) as resp:
                body = resp.read()
            return json.loads(body.decode("utf-8")) if body else None
        except urllib.error.HTTPError as e:
            body = e.read().decode("utf-8", "ignore")
            status = e.code
            if status == 429 or status >= 500:
                # 限流或服务端错误：等待后重试
                retry_after = e.headers.get("Retry-After")
                if retry_after and retry_after.isdigit():
                    time.sleep(min(int(retry_after), 60))
                else:
                    _sleep_backoff(attempt)
                last_err = ApiError(status, url, body)
                continue
            raise ApiError(status, url, body)
        except urllib.error.URLError as e:
            last_err = e
            _sleep_backoff(attempt)
    raise last_err


def download_file(url, dest, headers=None, retries=4, timeout=300):
    """下载文件到 dest，失败重试。"""
    hdrs = dict(headers or {})
    hdrs.setdefault("User-Agent", USER_AGENT)
    last_err = None
    for attempt in range(retries):
        try:
            req = urllib.request.Request(url, headers=hdrs)
            with urllib.request.urlopen(req, timeout=timeout) as resp, open(dest, "wb") as f:
                shutil.copyfileobj(resp, f)
            return True
        except urllib.error.HTTPError as e:
            body = e.read().decode("utf-8", "ignore")
            status = e.code
            last_err = ApiError(status, url, body)
            if status != 429 and status < 500:
                break
            retry_after = e.headers.get("Retry-After")
            if retry_after and retry_after.isdigit():
                time.sleep(min(int(retry_after), 60))
            else:
                _sleep_backoff(attempt)
        except (urllib.error.URLError, OSError) as e:
            last_err = e
            _sleep_backoff(attempt)
    raise last_err


# ---------- GitHub（源） ----------

def gh_list_releases(repo, token=""):
    """分页获取 GitHub 仓库全部 release。"""
    releases = []
    page = 1
    while True:
        url = "%s/repos/%s/releases?per_page=%d&page=%d" % (
            API_GITHUB, repo, GITHUB_PAGE_SIZE, page)
        data = http_json(url, headers=_gh_headers(token))
        if not data:
            break
        releases.extend(data)
        if len(data) < GITHUB_PAGE_SIZE:
            break
        page += 1
    return releases


def gh_get_release_by_tag(repo, token, tag):
    """按 tag 查询 GitHub release；不存在时抛 ApiError(404)，用于轮询等待。"""
    url = "%s/repos/%s/releases/tags/%s" % (
        API_GITHUB, repo, urllib.parse.quote(tag, safe=""))
    return http_json(url, headers=_gh_headers(token), retries=2)


def gh_download_asset(asset, token, dest):
    """下载单个 GitHub 附件（browser_download_url 为公开直链）。"""
    a_url = asset.get("browser_download_url")
    if not a_url:
        raise ApiError(0, "", "附件无下载地址: %s" % asset.get("name"))
    download_file(a_url, dest, headers=_gh_headers(token))
    return True


# ---------- CNB（目标） ----------

def cnb_list_releases(repo, token):
    """分页获取 CNB 仓库全部 release。"""
    releases = []
    page = 1
    while True:
        url = "%s/%s/-/releases?page=%d&page_size=%d" % (
            API_CNB, repo, page, CNB_PAGE_SIZE)
        data = http_json(url, headers=_cnb_headers(token))
        if not data:
            break
        releases.extend(data)
        if len(data) < CNB_PAGE_SIZE:
            break
        page += 1
    return releases


def cnb_get_release_by_tag(repo, token, tag):
    """按 tag 查询 CNB release（用于幂等补全）。"""
    url = "%s/%s/-/releases/tags/%s" % (
        API_CNB, repo, urllib.parse.quote(tag, safe=""))
    return http_json(url, headers=_cnb_headers(token), retries=2)


def cnb_create_release(repo, token, payload):
    """创建 CNB release，返回响应 dict。"""
    url = "%s/%s/-/releases" % (API_CNB, repo)
    return http_json(url, headers=_cnb_headers(token), method="POST", payload=payload)


def cnb_upload_asset(repo, token, release_id, file_path):
    """三步式上传附件：申请 URL -> PUT 文件 -> 确认。"""
    asset_name = os.path.basename(file_path)
    file_size = os.path.getsize(file_path)

    # 1. 申请预签名上传 URL
    url = "%s/%s/-/releases/%s/asset-upload-url" % (API_CNB, repo, release_id)
    info = http_json(
        url,
        headers=_cnb_headers(token),
        method="POST",
        payload={"asset_name": asset_name, "overwrite": True, "size": file_size},
    )
    if not info or "upload_url" not in info:
        raise ApiError(0, url, "asset-upload-url 未返回 upload_url: %s" % json.dumps(info, ensure_ascii=False))

    # 2. PUT 文件二进制
    upload_headers = {
        "Accept": "application/json",
        "Authorization": "Bearer " + token if token else "",
        "Content-Type": "application/octet-stream",
        "Content-Length": str(file_size),
        "User-Agent": USER_AGENT,
    }
    with open(file_path, "rb") as f:
        file_data = f.read()
    req = urllib.request.Request(info["upload_url"], data=file_data, headers=upload_headers, method="PUT")
    status = 0
    try:
        with urllib.request.urlopen(req, timeout=600) as resp:
            status = resp.status
            resp.read()
    except urllib.error.HTTPError as e:
        raise ApiError(e.code, info["upload_url"], e.read().decode("utf-8", "ignore"))
    if status >= 300:
        raise ApiError(status, info["upload_url"], "PUT 上传附件失败")

    # 3. 确认上传（若返回 verify_url）
    verify_url = info.get("verify_url")
    if verify_url:
        http_json(verify_url, headers=_cnb_headers(token), method="POST", timeout=120)
    return True


# ---------- 主流程 ----------

def pick_make_latest(gh_releases, cnb_releases, gh_release):
    """判断某待镜像的 GitHub release 是否应标记为 CNB 最新正式版。

    GitHub 列表按创建时间倒序，第一个正式版（非预发布、非草稿）即 GitHub 最新正式版。
    仅当满足以下两个条件时才返回 True：
    1. 该版本是 GitHub 侧最新的正式版。
    2. CNB 侧不存在不晚于该版本发布的正式版（含 CNB 直接发布的版本），
       避免镜像同步抢占 CNB 已有的最新版标记。
    其余情况（预发布版、非最新正式版、CNB 已有更新版本）返回 False。
    """
    tag = gh_release.get("tag_name")
    if gh_release.get("prerelease") or gh_release.get("draft"):
        return False
    # 按发布时间倒序取 GitHub 最新正式版，不依赖 API 列表顺序
    ordered = sorted(
        (r for r in gh_releases
         if not r.get("prerelease") and not r.get("draft") and r.get("tag_name")),
        key=lambda r: (r.get("published_at") or ""),
        reverse=True)
    if not ordered or ordered[0].get("tag_name") != tag:
        return False
    published = gh_release.get("published_at") or ""
    cnb_stable = [r for r in cnb_releases
                  if not r.get("prerelease") and not r.get("draft") and r.get("tag_name")]
    for r in cnb_stable:
        if (r.get("published_at") or r.get("created_at") or "") >= published:
            return False
    return True


def wait_for_release(repo, token, tag, timeout_minutes=120, interval_minutes=2):
    """轮询等待 GitHub 出现指定 tag 的 release（v* 分支发布流程使用）。

    每 interval_minutes 分钟查询一次 releases/tags/{tag}，最长等待 timeout_minutes 分钟；
    出现后返回，超时抛出 RuntimeError。
    """
    print("== 0/4 等待 GitHub release: %s（每 %.0f 分钟轮询一次，最长 %.0f 分钟）" % (
        tag, interval_minutes, timeout_minutes))
    deadline = time.time() + timeout_minutes * 60
    while True:
        try:
            gh_get_release_by_tag(repo, token, tag)
            print("   已找到 release %s，开始镜像同步" % tag)
            return
        except ApiError as e:
            if e.status != 404:
                raise
            remaining_min = (deadline - time.time()) / 60.0
            if remaining_min <= 0:
                raise RuntimeError("等待超时（%.0f 分钟），GitHub 尚未发布 release %s" % (
                    timeout_minutes, tag))
            sleep_min = min(interval_minutes, remaining_min)
            print("   尚未发布 %s，%.0f 分钟后重试（剩余 %.0f 分钟）" % (
                tag, sleep_min, remaining_min))
            time.sleep(max(sleep_min * 60, 1))


def main():
    parser = argparse.ArgumentParser(description="将 GitHub Releases 镜像同步到 CNB")
    parser.add_argument("--github-repo", default=os.environ.get("GITHUB_REPO", DEFAULT_GITHUB_REPO),
                        help="GitHub 源仓库，默认 %s" % DEFAULT_GITHUB_REPO)
    parser.add_argument("--cnb-repo", default=os.environ.get("CNB_REPO", DEFAULT_CNB_REPO),
                        help="CNB 目标仓库（格式 org/repo），默认 %s" % DEFAULT_CNB_REPO)
    parser.add_argument("--github-token", default=os.environ.get("GITHUB_TOKEN", ""),
                        help="GitHub Token（可选，公开仓库无需）")
    parser.add_argument("--wait-tag", default="",
                        help="先轮询等待 GitHub 出现该 tag 的 release（v* 分支发布流程使用），"
                             "出现后再开始镜像同步")
    parser.add_argument("--wait-timeout", type=float, default=120,
                        help="等待超时分钟数，默认 120（配合 --wait-tag 使用）")
    parser.add_argument("--wait-interval", type=float, default=2,
                        help="轮询间隔分钟数，默认 2（配合 --wait-tag 使用）")
    parser.add_argument("--dry-run", action="store_true", help="仅对比版本，不下载不上传")
    args = parser.parse_args()

    token = os.environ.get("CNB_TOKEN", "")
    if not token:
        print("[错误] 缺少环境变量 CNB_TOKEN，无法调用 CNB API。"
              "云原生构建流水线会自动注入，本地调试请手动设置。", file=sys.stderr)
        sys.exit(2)

    if args.wait_tag:
        try:
            wait_for_release(args.github_repo, args.github_token, args.wait_tag,
                             args.wait_timeout, args.wait_interval)
        except RuntimeError as e:
            print("[错误] %s" % e, file=sys.stderr)
            sys.exit(1)

    print("== 1/4 获取 GitHub releases: %s" % args.github_repo)
    gh_releases = gh_list_releases(args.github_repo, args.github_token)
    print("   GitHub 共 %d 个 release" % len(gh_releases))

    print("== 2/4 获取 CNB 现有 releases: %s" % args.cnb_repo)
    cnb_releases = cnb_list_releases(args.cnb_repo, token)
    cnb_tags = {r.get("tag_name") for r in cnb_releases if r.get("tag_name")}
    print("   CNB 共 %d 个 release" % len(cnb_releases))

    # 待同步：GitHub 有、CNB 无、且非草稿
    pending = []
    for r in gh_releases:
        if r.get("draft"):
            continue
        tag = r.get("tag_name")
        if not tag:
            continue
        if tag not in cnb_tags:
            pending.append(r)
    pending.sort(key=lambda r: (r.get("published_at") or ""), reverse=True)

    print("== 3/4 待同步 %d 个版本" % len(pending))
    if not pending:
        print("   全部已是最新，无需同步。")
        return 0
    for r in pending:
        pre = " [预发布]" if r.get("prerelease") else ""
        print("   - %s%s (%d 个附件)" % (
            r.get("tag_name"), pre, len(r.get("assets") or [])))

    if args.dry_run:
        print("[dry-run] 仅对比，不执行同步。")
        return 0

    print("== 4/4 开始同步")
    workdir = tempfile.mkdtemp(prefix="cnb_mirror_")
    ok_count = 0
    fail_count = 0
    try:
        for i, r in enumerate(pending, 1):
            tag = r.get("tag_name")
            name = r.get("name") or tag
            body = r.get("body") or ""
            html_url = r.get("html_url") or ("https://github.com/%s/releases/tag/%s" % (args.github_repo, tag))
            make_latest = pick_make_latest(gh_releases, cnb_releases, r)
            print("\n[%d/%d] 同步 %s%s" % (i, len(pending), tag,
                  " [预发布]" if r.get("prerelease") else ""))
            try:
                # 3.1 下载全部附件
                assets = r.get("assets") or []
                asset_files = []
                rel_dir = os.path.join(workdir, tag.replace("/", "_"))
                os.makedirs(rel_dir, exist_ok=True)
                for a in assets:
                    a_name = a.get("name")
                    a_url = a.get("browser_download_url")
                    if not a_name or not a_url:
                        print("   跳过无下载地址的附件: %s" % a_name)
                        continue
                    dest = os.path.join(rel_dir, a_name)
                    print("   下载 %s (%d bytes)" % (a_name, a.get("size", 0)))
                    gh_download_asset(a, args.github_token, dest)
                    asset_files.append(dest)

                # 3.2 创建 CNB release
                mirror_body = (
                    "> 本版本由 CNB 云原生构建从 GitHub 自动镜像同步。\n"
                    "> 源发布: %s\n\n%s" % (html_url, body)
                ).strip()
                payload = {
                    "tag_name": tag,
                    "name": name,
                    "body": mirror_body,
                    "draft": False,
                    "prerelease": bool(r.get("prerelease")),
                    "make_latest": make_latest,
                }
                print("   创建 CNB release (make_latest=%s)" % make_latest)
                try:
                    created = cnb_create_release(args.cnb_repo, token, payload)
                    release_id = str(created.get("id"))
                    print("   release_id=%s" % release_id)
                except ApiError as e:
                    # 创建失败时尝试幂等补全：若该 tag 已存在 release 则复用之
                    if e.status in (400, 409, 422):
                        existing = cnb_get_release_by_tag(args.cnb_repo, token, tag)
                        if existing:
                            release_id = str(existing.get("id"))
                            print("   tag 已存在 release（%s），继续补全附件" % release_id)
                        else:
                            raise
                    else:
                        raise

                # 3.3 上传附件
                for j, f in enumerate(asset_files, 1):
                    print("   上传 [%d/%d] %s" % (j, len(asset_files), os.path.basename(f)))
                    cnb_upload_asset(args.cnb_repo, token, release_id, f)
                ok_count += 1
                print("   ✅ 完成")
            except Exception as e:  # noqa: BLE001
                fail_count += 1
                print("   ❌ 失败: %s" % e)
    finally:
        shutil.rmtree(workdir, ignore_errors=True)

    print("\n===== 同步汇总 =====")
    print("成功: %d 个版本" % ok_count)
    print("失败: %d 个版本" % fail_count)
    return 1 if fail_count else 0


if __name__ == "__main__":
    sys.exit(main())
