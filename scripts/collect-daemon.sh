#!/usr/bin/env bash
set -e

# collect-daemon.sh — 将守护进程产物复制到主程序输出目录
# 用法: bash collect-daemon.sh <守护进程输出目录> <目标目录> <守护进程二进制名> <重命名目标名>

DAEMON_DIR="${1:?缺少守护进程输出目录参数}"
TARGET_DIR="${2:?缺少目标目录参数}"
DAEMON_BIN="${3:?缺少守护进程二进制文件名}"
RENAMED_BIN="${4:?缺少重命名后的文件名}"

mkdir -p "$TARGET_DIR"

# 复制守护进程可执行文件并重命名
if [ -f "$DAEMON_DIR/$DAEMON_BIN" ]; then
    cp -f "$DAEMON_DIR/$DAEMON_BIN" "$TARGET_DIR/$RENAMED_BIN"
    echo "[INFO] 已复制: $RENAMED_BIN"
fi

# 复制动态库 (.so / .dylib / .dll / .sys)
for ext in so dylib dll sys; do
    for f in "$DAEMON_DIR/"*."$ext"; do
        if [ -f "$f" ]; then
            cp -f "$f" "$TARGET_DIR/"
            echo "[INFO] 已复制: $(basename "$f")"
        fi
    done
done

echo "[INFO] 守护进程集成完成"
