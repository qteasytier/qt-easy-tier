#!/bin/bash
set -e

PROJECT_DIR="${1:-$(pwd)}"
VERSION="$2"

# 从 CMakeLists.txt 提取版本号
if [ -z "$VERSION" ]; then
    CMAKE_FILE="$PROJECT_DIR/CMakeLists.txt"
    if [ ! -f "$CMAKE_FILE" ]; then
        echo "错误: 未找到 CMakeLists.txt: $CMAKE_FILE"
        exit 1
    fi
    VERSION=$(sed -nE 's/.*project\([^)]*VERSION ([0-9.]+).*/\1/p' "$CMAKE_FILE" || true)
    if [ -z "$VERSION" ]; then
        echo "错误: 无法从 CMakeLists.txt 提取版本号"
        exit 1
    fi
    echo "从 CMakeLists.txt 提取版本号: $VERSION"
fi

echo "=============================="
echo "版本号: $VERSION"
echo "项目目录: $PROJECT_DIR"
echo "=============================="

APP_NAME="QtEasyTier"
DMG_NAME="${APP_NAME}_v${VERSION}_macos_amd64"
APP_PATH="$PROJECT_DIR/Install/${APP_NAME}.app"
DMG_PATH="$PROJECT_DIR/Install/${DMG_NAME}.dmg"

if [ ! -d "$APP_PATH" ]; then
    echo "错误: 未找到 .app Bundle: $APP_PATH"
    echo "请先运行 cmake --install 生成 Bundle"
    exit 1
fi

echo "创建 DMG 镜像..."
hdiutil create \
    -volname "$APP_NAME" \
    -srcfolder "$APP_PATH" \
    -ov \
    -format UDZO \
    "$DMG_PATH"

echo "=============================="
echo "DMG 创建完成!"
echo "输出: $DMG_PATH"
echo "=============================="
