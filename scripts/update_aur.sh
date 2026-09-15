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
# 本脚本仅由分支详情页「更新 AUR 版本」按钮手动触发，不做自动等待重试：
# release 产物未就绪时直接失败，产物发布后再点一次按钮即可。
#
# 用法：
#   PRIVATE_KEY="$(cat key)" bash scripts/update_aur.sh
#
# 环境变量：
#   PRIVATE_KEY        必填，AUR 推送用 SSH 私钥内容，支持三种形态：
#                        a) 完整 OpenSSH 私钥文本（含 BEGIN/END 头尾）；
#                        b) 去掉头尾、体部压成一行的 base64 文本；
#                        c) 上述文本再做一次 base64 编码。
#                      （由密钥仓库 myqfeng/keys 的 aur.yml 经 imports 注入）
#   VERSION            可选，版本号；缺省从根 CMakeLists.txt 的
#                      project(QtEasyTier VERSION x.y.z) 提取
#   CNB_BUILD_WORKSPACE 可选，仓库工作空间根目录（CNB 流水线自动注入）
#   PKG_LIST           可选，待更新的 AUR 包名，缺省 "qteasytier qteasytier-bin"
#   AUR_GIT_NAME       可选，提交作者名，缺省 Myqfeng
#   AUR_GIT_EMAIL      可选，提交作者邮箱
#
# 依赖：bash、git、ssh、curl、sha256sum、sed、grep、find
# =====================================================================

set -euo pipefail

AUR_HOST="aur.archlinux.org"
# 去掉可能存在的尾随斜杠，避免出现 /workspace//package/aur 这类双斜杠路径
WORKSPACE="${CNB_BUILD_WORKSPACE:-$(pwd)}"
WORKSPACE="${WORKSPACE%/}"
SRC_AUR_DIR="${WORKSPACE}/package/aur"
# 中间产物放临时目录，避免污染仓库工作区
WORK_DIR="${QTET_AUR_WORK_DIR:-${TMPDIR:-/tmp}/qtet-aur-work}"
DOWNLOAD_DIR="${WORK_DIR}/downloads"

PKG_LIST="${PKG_LIST:-qteasytier qteasytier-bin}"
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
# 2.0 私钥落盘：支持 base64 编码 / OpenSSH 文本两种载体
# ---------------------------------------------------------------------
# 密钥仓库 myqfeng/keys 的 aur.yml 中 PRIVATE_KEY 可能是以下任一形态：
#   1) 标准 OpenSSH 私钥文本（含 BEGIN/END 头尾）；
#   2) 去掉头尾、体部压成一行的 base64 文本（仓库中实际使用的形态）；
#   3) 上述文本再做一次 base64 编码（部分密钥仓库的存储方式）。
# 逐层探测并解码，最终落盘为 OpenSSH 可识别的私钥文件；无法识别时
# 打印可用诊断信息后失败，避免以 "error in libcrypto" 这类模糊报错终止。
decode_private_key() {
    local raw="$1"
    local out="$2"
    local attempt
    local step
    local tmp_a
    local tmp_b

    tmp_a="$(mktemp)"
    tmp_b="$(mktemp)"

    # 第 0 层：原始文本；若已是 OpenSSH 文本（含 PEM 头尾）直接落盘
    if printf '%s' "${raw}" | grep -q -- '-----BEGIN'; then
        printf '%s\n' "${raw}" > "${out}"
        rm -f "${tmp_a}" "${tmp_b}"
        return 0
    fi

    # 把原始文本写入临时文件，后续用文件而非命令替换传递二进制，
    # 避免 $(...) 丢弃 NUL 字节导致 OpenSSH 私钥结构损坏。
    printf '%s' "${raw}" > "${tmp_a}"

    # 最多解两层 base64：
    #   第 1 层：裸 base64 体部 -> OpenSSH 私钥二进制（或再套一层的文本）
    #   第 2 层：外层 base64 解出的 base64 文本 -> 私钥二进制
    # 每层解完都判断是否已得到 openssh-key-v1 魔数或 PEM 头，命中即收敛。
    for attempt in 1 2; do
        # 已是 OpenSSH 文本：原样落盘
        if grep -q -- '-----BEGIN' "${tmp_a}"; then
            cp -f "${tmp_a}" "${out}"
            rm -f "${tmp_a}" "${tmp_b}"
            return 0
        fi

        # base64 解码到文件；失败（非 base64 文本）则放弃
        if ! base64 -d < "${tmp_a}" > "${tmp_b}" 2>/dev/null; then
            rm -f "${tmp_a}" "${tmp_b}"
            return 1
        fi
        if [ ! -s "${tmp_b}" ]; then
            rm -f "${tmp_a}" "${tmp_b}"
            return 1
        fi

        # 解出 OpenSSH 私钥二进制：收敛（读取前 15 字节魔数比对）
        if head -c 15 "${tmp_b}" | grep -q 'openssh-key-v1'; then
            cp -f "${tmp_b}" "${out}"
            rm -f "${tmp_a}" "${tmp_b}"
            return 0
        fi

        # 解出的是又一层文本：交换缓冲，继续下一轮
        step="${tmp_a}"; tmp_a="${tmp_b}"; tmp_b="${step}"
    done

    rm -f "${tmp_a}" "${tmp_b}"
    return 1
}

# 校验私钥可用性：用 ssh-keygen 读取并导出公钥，失败即视为不可用。
# 私钥内容异常时（如非标准编码）在此显式报错，而不是等到 git clone 才失败。
verify_private_key() {
    local key_file="$1"
    local pubkey

    command -v ssh-keygen >/dev/null 2>&1 || return 0  # 镜像未装 ssh-keygen 时跳过校验

    if ! pubkey="$(ssh-keygen -y -f "${key_file}" 2>/dev/null)"; then
        err "私钥无法被 ssh-keygen 解析: ${key_file}"
        err "请检查密钥仓库 aur.yml 中 PRIVATE_KEY 的编码与完整性"
        return 1
    fi

    log "私钥校验通过，公钥指纹: $(printf '%s' "${pubkey}" | cut -d' ' -f1-2)"
    return 0
}

# 修复 OpenSSH 私钥封装：某些导出工具（如部分 archlinux 上的 key 生成脚本）
# 生成的 openssh-key-v1 私有区缺失开头的 4 字节长度前缀，导致私有区相对正确
# 布局整体前移 4 字节，ssh-keygen/OpenSSL 会报 "error in libcrypto"。
# 本函数按 openssh-key-v1 规范重排私有区：
#   checkint(2×4B) + len+keytype + len+pubkey + len+privkey + len+comment (+ padding)
# 若已是合法布局（两个 checkint 相等）则原样返回，不做改动。
repair_openssh_key() {
    local in_file="$1"
    local out_file="$2"

    if ! command -v python3 >/dev/null 2>&1; then
        # 无 python3 时不阻断流程，交由 verify_private_key 判定
        cp -f "${in_file}" "${out_file}"
        return 0
    fi

    python3 - "${in_file}" "${out_file}" <<'PYEOF'
"""按 openssh-key-v1 规范重排私钥封装。

已知错位形态（部分导出工具产生）：
    私有区 = len(keytype) | checkint | checkint | keytype | len+pub | len+priv | ...
正确形态：
    私有区 = checkint | checkint | len+keytype | len+pub | len+priv | len+comment
即开头的 len(keytype) 被多余写入，导致后续字段整体错位、ssh-keygen 报
"error in libcrypto"。此处把 len(keytype) 挪到 keytype 之前并归一 checkint。
已是合法封装（两个 checkint 相等）时原样保留。
"""
import base64
import struct
import sys

src, dst = sys.argv[1], sys.argv[2]
raw = open(src, "rb").read()

MAGIC = b"openssh-key-v1\x00"

# 输入可能是 PEM 文本（含头尾）或裸二进制：统一解出二进制私钥块，
# 保证对同一份材料重复调用时结果稳定（幂等）。
if raw.lstrip().startswith(b"-----BEGIN"):
    body = b"".join(
        line for line in raw.splitlines()
        if not line.strip().startswith(b"-----")
    )
    try:
        raw = base64.b64decode(body)
    except Exception:
        sys.exit(0)

# 允许的密钥类型（用于确认错位解析结果可信）
KIND_OK = (b"ssh-ed25519", b"ssh-rsa", b"ecdsa-sha2-nistp256",
           b"ecdsa-sha2-nistp384", b"ecdsa-sha2-nistp521")


def u32(b, i):
    return int.from_bytes(b[i:i + 4], "big")


def read_str(b, i):
    n = u32(b, i)
    if n < 0 or i + 4 + n > len(b):
        raise ValueError("字段长度越界")
    return b[i + 4:i + 4 + n], i + 4 + n


def put_str(b):
    return struct.pack(">I", len(b)) + b


def wrap(blob):
    """按 PEM 风格 70 列折行封装为 OpenSSH 私钥文本。"""
    body = base64.b64encode(blob).decode()
    lines = [body[i:i + 70] for i in range(0, len(body), 70)]
    return ("-----BEGIN OPENSSH PRIVATE KEY-----\n"
            + "\n".join(lines)
            + "\n-----END OPENSSH PRIVATE KEY-----\n").encode()


if not raw.startswith(MAGIC):
    # 非 openssh-key-v1（如 PEM RSA）：原样输出，交由 ssh-keygen 判定
    open(dst, "wb").write(wrap(raw))
    sys.exit(0)

i = len(MAGIC)
_, i = read_str(raw, i)          # cipher
_, i = read_str(raw, i)          # kdf
_, i = read_str(raw, i)          # kdf options
i += 4                           # nkeys
pubkey_blob, i = read_str(raw, i)
header = raw[:i]
priv = raw[i:]

# 已是合法封装：两个 checkint 相等
if len(priv) >= 8 and priv[0:4] == priv[4:8]:
    open(dst, "wb").write(wrap(raw))
    sys.exit(0)

# 按错位形态解析：跳过开头的 len(keytype)，checkint 紧随其后且重复两次
ktype, kpub, kpriv, comment, ck, padding = b"", b"", b"", b"", b"", b""
try:
    j = 4
    ck = priv[4:8]
    ktype, j = read_str(priv, j)
    kpub, j = read_str(priv, j)
    kpriv, j = read_str(priv, j)
    comment, j = read_str(priv, j)
    padding = priv[j:]
except Exception:
    pass

rebuilt = None
if (ktype in KIND_OK and kpub and kpriv
        and kpub == kpriv[len(kpriv) - len(kpub):]):
    # ed25519 私钥尾部内嵌公钥，可用于确认解析正确
    body = (ck + ck
            + put_str(ktype) + put_str(kpub) + put_str(kpriv)
            + put_str(comment) + padding)
    if (8 - len(body) % 8) % 8:
        body += bytes(range(1, (8 - len(body) % 8) % 8 + 1))
    rebuilt = header + put_str(pubkey_blob) + body

open(dst, "wb").write(wrap(rebuilt if rebuilt is not None else raw))
PYEOF

    return 0
}

write_private_key() {
    local raw="$1"
    local key_file="$2"
    local tmp_key
    tmp_key="$(mktemp)"

    decode_private_key "${raw}" "${tmp_key}" \
        || { rm -f "${tmp_key}"; die "PRIVATE_KEY 无法解析为 OpenSSH 私钥（既非 OpenSSH 文本也非合法的 base64 编码）"; }

    # ssh-keygen 会拒绝权限过松的私钥文件，校验与使用前先收紧权限
    chmod 600 "${tmp_key}"

    # 解码后若 ssh-keygen 无法识别，尝试按规范修复封装再落盘
    if command -v ssh-keygen >/dev/null 2>&1 && ! ssh-keygen -y -f "${tmp_key}" >/dev/null 2>&1; then
        log "私钥封装异常，尝试按 openssh-key-v1 规范修复…"
        repair_openssh_key "${tmp_key}" "${key_file}" || cp -f "${tmp_key}" "${key_file}"
    else
        cp -f "${tmp_key}" "${key_file}"
    fi
    rm -f "${tmp_key}"
    chmod 600 "${key_file}"

    verify_private_key "${key_file}" \
        || die "PRIVATE_KEY 解码并修复后仍不合法，无法用于 AUR 推送"

    return 0
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

    write_private_key "${PRIVATE_KEY}" "${key_file}"
    chmod 600 "${key_file}"

    # BatchMode 禁用交互；accept-new 首次连接自动记录 aur.archlinux.org 主机指纹
    export GIT_SSH_COMMAND="ssh -i ${key_file} -o IdentitiesOnly=yes -o BatchMode=yes -o StrictHostKeyChecking=accept-new -o UserKnownHostsFile=${ssh_dir}/known_hosts"

    log "已加载 AUR 推送私钥: ${key_file}"
}

# ---------------------------------------------------------------------
# 3. 下载产物（不重试等待：产物未就绪直接失败，手动重跑即可）
# ---------------------------------------------------------------------
download_deb() {
    local url="$1"
    local dest="$2"

    log "下载产物: ${url}"
    curl -fsSL --retry 2 --retry-delay 5 --connect-timeout 30 -o "${dest}" "${url}" || {
        rm -f "${dest}"
        die "产物下载失败（可能 release 尚未发布）: ${url}"
    }
}

# ---------------------------------------------------------------------
# 4. 更新单个 AUR 包
# ---------------------------------------------------------------------
update_pkg() {
    local pkg="$1"
    local src_dir="${SRC_AUR_DIR}/${pkg}"
    local repo_dir="${WORK_DIR}/${pkg}"
    local remote="ssh://aur@${AUR_HOST}/${pkg}.git"

    if [ ! -d "${src_dir}" ]; then
        err "本地构建配置不存在: ${src_dir}"
        err "请确认 package/aur/${pkg} 已被提交到仓库（包含 PKGBUILD / .SRCINFO），"
        err "并检查该目录是否被 .gitignore 排除。"
        exit 1
    fi
    if [ ! -f "${src_dir}/PKGBUILD" ]; then
        err "${src_dir} 下缺少 PKGBUILD"
        exit 1
    fi

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
        download_deb "${deb_url}" "${deb_file}"

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
