#!/usr/bin/env bash
# =====================================================================
# QtEasyTier AUR 版本更新脚本
#
# 把本仓库 package/aur 下的两个 AUR 包配置（qteasytier / qteasytier-bin）
# 同步推送到 AUR 官方仓库，并完成版本号与校验和的刷新。
#
# 每个包的更新流程：
#   1. git clone ssh://aur@aur.archlinux.org/<pkg>.git
#   2. 清空仓库内全部文件（保留 .git）
#   3. 复制 package/aur/<pkg> 下的构建配置（PKGBUILD / .SRCINFO / *.install）
#   4. 把配置中的占位版本号 0.0.0 替换为真实版本号
#   5. bin 包：按 PKGBUILD 中替换后的 source 地址下载 .deb，计算 sha256，
#      替换 sha256sums 中的 SHA256SUMS 占位符
#   6. 提交并以 --force 推送（AUR 侧历史可能与本地分叉，强推保证一致）
#
# 用法：
#   PRIVATE_KEY="$(cat key)" bash scripts/update_aur.sh
#
# 环境变量：
#   PRIVATE_KEY        必填，AUR 推送用 SSH 私钥内容
#                      （由密钥仓库 myqfeng/keys 的 aur.yml 经 imports 注入）
#   VERSION            可选，版本号；缺省从根 CMakeLists.txt 的
#                      project(QtEasyTier VERSION x.y.z) 提取
#   CNB_BUILD_WORKSPACE 可选，仓库工作空间根目录（CNB 流水线自动注入）
#   PKG_LIST           可选，待更新的 AUR 包名，缺省 "qteasytier qteasytier-bin"
#   DEB_WAIT_SECONDS   可选，bin 包等待 .deb 产物出现的总时长，缺省 1800 秒
#   DEB_WAIT_INTERVAL  可选，bin 包轮询间隔，缺省 30 秒
#   AUR_GIT_NAME       可选，提交作者名，缺省 Myqfeng
#   AUR_GIT_EMAIL      可选，提交作者邮箱
#
# 依赖：bash、git、ssh、curl、sha256sum、sed、grep、find
# =====================================================================

set -euo pipefail

AUR_HOST="aur.archlinux.org"
WORKSPACE="${CNB_BUILD_WORKSPACE:-$(pwd)}"
SRC_AUR_DIR="${WORKSPACE}/package/aur"
# 中间产物放临时目录，避免污染仓库工作区
WORK_DIR="${QTET_AUR_WORK_DIR:-${TMPDIR:-/tmp}/qtet-aur-work}"
DOWNLOAD_DIR="${WORK_DIR}/downloads"

PKG_LIST="${PKG_LIST:-qteasytier qteasytier-bin}"
DEB_WAIT_SECONDS="${DEB_WAIT_SECONDS:-1800}"
DEB_WAIT_INTERVAL="${DEB_WAIT_INTERVAL:-30}"
AUR_GIT_NAME="${AUR_GIT_NAME:-Myqfeng}"
AUR_GIT_EMAIL="${AUR_GIT_EMAIL:-viagrahuang@outlook.com}"

log() { echo "[aur] $*"; }
err() { echo "[aur][ERROR] $*" >&2; }
die() { err "$*"; exit 1; }

# ---------------------------------------------------------------------
# 1. 确定版本号：优先取外部传入，否则从 CMakeLists.txt 提取
# ---------------------------------------------------------------------
resolve_version() {
    if [ -n "${VERSION:-}" ]; then
        log "使用外部传入的版本号: ${VERSION}"
    else
        cd "${WORKSPACE}"
        VERSION="$(sed -n -E 's/^project\(QtEasyTier VERSION ([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' CMakeLists.txt | head -n 1)"
    fi

    if ! printf '%s' "${VERSION}" | grep -qE '^[0-9]+\.[0-9]+\.[0-9]+$'; then
        die "版本号非法: '${VERSION}'（期望 x.y.z 形式）"
    fi

    log "目标版本号: ${VERSION}"
}

# ---------------------------------------------------------------------
# 2. 准备 AUR 推送用 SSH 私钥
# ---------------------------------------------------------------------
setup_ssh_key() {
    [ -n "${PRIVATE_KEY:-}" ] || die "缺少环境变量 PRIVATE_KEY（应由密钥仓库 myqfeng/keys 的 aur.yml 注入）"

    local ssh_dir="${HOME}/.ssh"
    local key_file="${ssh_dir}/aur_key"

    mkdir -p "${ssh_dir}"
    chmod 700 "${ssh_dir}"

    # 私钥内容来自密钥仓库，按原样落盘；OpenSSH 可容忍尾部空行
    printf '%s\n' "${PRIVATE_KEY}" > "${key_file}"
    chmod 600 "${key_file}"

    # BatchMode 禁用交互；accept-new 首次连接自动记录 aur.archlinux.org 主机指纹
    export GIT_SSH_COMMAND="ssh -i ${key_file} -o IdentitiesOnly=yes -o BatchMode=yes -o StrictHostKeyChecking=accept-new -o UserKnownHostsFile=${ssh_dir}/known_hosts"

    log "已加载 AUR 推送私钥: ${key_file}"
}

# ---------------------------------------------------------------------
# 3. 下载产物（带重试等待：v* 分支触发时 release 可能尚未发布到 CNB）
# ---------------------------------------------------------------------
download_with_wait() {
    local url="$1"
    local dest="$2"
    local waited=0

    while :; do
        if curl -fsSL --retry 2 --retry-delay 5 --connect-timeout 30 -o "${dest}" "${url}"; then
            return 0
        fi
        rm -f "${dest}"

        if [ "${waited}" -ge "${DEB_WAIT_SECONDS}" ]; then
            die "等待 ${DEB_WAIT_SECONDS}s 后仍无法下载: ${url}"
        fi

        log "产物暂不可下载（已等待 ${waited}s），${DEB_WAIT_INTERVAL}s 后重试"
        sleep "${DEB_WAIT_INTERVAL}"
        waited=$((waited + DEB_WAIT_INTERVAL))
    done
}

# ---------------------------------------------------------------------
# 4. 更新单个 AUR 包
# ---------------------------------------------------------------------
update_pkg() {
    local pkg="$1"
    local src_dir="${SRC_AUR_DIR}/${pkg}"
    local repo_dir="${WORK_DIR}/${pkg}"
    local remote="ssh://aur@${AUR_HOST}/${pkg}.git"

    [ -d "${src_dir}" ] || die "本地构建配置不存在: ${src_dir}"

    log "======== 更新 ${pkg} ========"

    # 4.1 拉取 AUR 仓库
    rm -rf "${repo_dir}"
    log "克隆 ${remote}"
    git clone --quiet "${remote}" "${repo_dir}"

    # 4.2 清空仓库内全部文件（保留 .git）
    find "${repo_dir}" -mindepth 1 -maxdepth 1 ! -name '.git' -exec rm -rf {} +

    # 4.3 复制本地构建配置（含 .SRCINFO 等隐藏文件）
    cp -a "${src_dir}/." "${repo_dir}/"

    # 4.4 占位版本号 0.0.0 -> 真实版本号
    sed -i "s/0\.0\.0/${VERSION}/g" "${repo_dir}/PKGBUILD"
    if [ -f "${repo_dir}/.SRCINFO" ]; then
        sed -i "s/0\.0\.0/${VERSION}/g" "${repo_dir}/.SRCINFO"
    fi

    # 4.5 二进制包：下载 deb 并替换 SHA256SUMS 占位符
    if grep -q 'SHA256SUMS' "${repo_dir}/PKGBUILD"; then
        local deb_url
        local deb_file
        local sha

        deb_url="$(grep -oE 'https://[^"]+\.deb' "${repo_dir}/PKGBUILD" | head -n 1)"
        [ -n "${deb_url}" ] || die "${pkg}: 未能从 PKGBUILD 中解析出 .deb 下载地址"

        # PKGBUILD 的 source 通常写作 ${pkgver} 由 makepkg 展开，
        # 此处直接下载需要按真实版本号展开后再使用
        deb_url="${deb_url//'${pkgver}'/${VERSION}}"
        deb_url="${deb_url//'$pkgver'/${VERSION}}"

        case "${deb_url}" in
            *'$'*) die "${pkg}: 下载地址中仍存在未展开的变量: ${deb_url}" ;;
        esac
        log "源码包地址: ${deb_url}"

        mkdir -p "${DOWNLOAD_DIR}"
        deb_file="${DOWNLOAD_DIR}/$(basename "${deb_url}")"
        rm -f "${deb_file}"
        download_with_wait "${deb_url}" "${deb_file}"

        sha="$(sha256sum "${deb_file}" | awk '{print $1}')"
        [ -n "${sha}" ] || die "${pkg}: 计算 sha256 失败"
        log "sha256: ${sha}"

        sed -i "s/SHA256SUMS/${sha}/g" "${repo_dir}/PKGBUILD"
        if [ -f "${repo_dir}/.SRCINFO" ]; then
            sed -i "s/SHA256SUMS/${sha}/g" "${repo_dir}/.SRCINFO"
        fi
    fi

    # 4.6 提交并强推（AUR 提交无需签名，显式关闭 GPG 签名避免环境干扰）
    git -C "${repo_dir}" config user.name "${AUR_GIT_NAME}"
    git -C "${repo_dir}" config user.email "${AUR_GIT_EMAIL}"
    git -C "${repo_dir}" config commit.gpgsign false
    git -C "${repo_dir}" add -A

    if git -C "${repo_dir}" diff --cached --quiet; then
        log "${pkg}: 内容无变化，跳过提交"
    else
        git -C "${repo_dir}" commit --quiet -m "update to ${VERSION}"
    fi

    log "${pkg}: 推送到 ${remote}（--force）"
    git -C "${repo_dir}" push --force --quiet origin "HEAD:master"

    log "${pkg}: 更新完成"
}

# ---------------------------------------------------------------------
# 主流程
# ---------------------------------------------------------------------
mkdir -p "${WORK_DIR}"
resolve_version
setup_ssh_key

for pkg in ${PKG_LIST}; do
    update_pkg "${pkg}"
done

log "全部完成（版本 ${VERSION}）: ${PKG_LIST}"
