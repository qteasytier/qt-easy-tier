#!/usr/bin/env bash
set -e

# 克隆并构建 qtet-daemon

USE_GITEE=false

for arg in "$@"; do
    case "$arg" in
        --gitee)
            USE_GITEE=true
            ;;
        *)
            echo "[ERROR] 未知参数: $arg"
            echo "用法: $0 [--gitee]"
            exit 1
            ;;
    esac
done

if [ -n "${REPO_URL:-}" ]; then
    REPO_URL="$REPO_URL"
elif [ "$USE_GITEE" = true ]; then
    REPO_URL="https://gitee.com/qteasytier/qtet-daemon.git"
else
    REPO_URL="https://github.com/qteasytier/qtet-daemon.git"
fi
CLONE_DIR="${CLONE_DIR:-qtet-daemon}"
BUILD_TYPE="${BUILD_TYPE:-Release}"

echo "[INFO] 依赖检查..."
command -v git >/dev/null 2>&1 || { echo "[ERROR] 未找到 git"; exit 1; }
command -v cmake >/dev/null 2>&1 || { echo "[ERROR] 未找到 cmake"; exit 1; }

echo "[INFO] 仓库: $REPO_URL"
echo "[INFO] 目录: $CLONE_DIR"

if [ ! -d "$CLONE_DIR" ]; then
    echo "[INFO] 克隆源码..."
    git clone "$REPO_URL" "$CLONE_DIR"
else
    echo "[WARN] 目录 $CLONE_DIR 已存在，跳过克隆"
fi

cd "$CLONE_DIR"

echo "[INFO] CMake 配置（$BUILD_TYPE）..."
cmake -B build -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

echo "[INFO] 开始编译..."
cmake --build build -j"$(nproc)"

echo "[INFO] 构建完成"
echo "  daemon:     build/Output/qtet-daemon"
echo "  cli-client: build/Output/cli-client"
echo "  测试:       build/Output/tst_*"
