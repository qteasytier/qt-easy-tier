#!/usr/bin/env bash
set -euo pipefail

VERSION=""
APP_IMAGE_TOOL="${APP_IMAGE_TOOL:-appimagetool}"

while [[ $# -gt 0 ]]; do
    case "$1" in
        -v|--version)
            if [[ $# -lt 2 ]]; then
                echo "错误: $1 需要一个参数"
                exit 1
            fi
            VERSION="$2"
            shift 2
            ;;
        --app-image-tool)
            if [[ $# -lt 2 ]]; then
                echo "错误: $1 需要一个参数"
                exit 1
            fi
            APP_IMAGE_TOOL="$2"
            shift 2
            ;;
        *)
            echo "未知参数: $1"
            exit 1
            ;;
    esac
done

if [[ -z "$VERSION" ]]; then
    echo "用法: $(basename "$0") -v <版本号>"
    echo "示例: $(basename "$0") -v 3.0.0"
    exit 1
fi

OUTPUT_DIR="$(pwd)"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ASSETS_DIR="$(cd "$SCRIPT_DIR/../../" && pwd)"

APP_NAME="qteasytier"
APP_DIR="$OUTPUT_DIR/AppDir"
APPIMAGE_NAME="${APP_NAME}_v${VERSION}_linux_amd64.AppImage"

DESKTOP_SRC="$SCRIPT_DIR/qteasytier.desktop"
ICON_SRC="$ASSETS_DIR/favicon/qtet.png"

if [[ ! -f "$OUTPUT_DIR/appQtEasyTier" ]]; then
    echo "错误: 在 $OUTPUT_DIR 中未找到 appQtEasyTier"
    exit 1
fi

if [[ ! -f "$DESKTOP_SRC" ]]; then
    echo "错误: 未找到 desktop 文件: $DESKTOP_SRC"
    exit 1
fi

if [[ ! -f "$ICON_SRC" ]]; then
    echo "错误: 未找到图标文件: $ICON_SRC"
    exit 1
fi

if ! command -v "$APP_IMAGE_TOOL" >/dev/null 2>&1; then
    echo "错误: 未找到 appimagetool: $APP_IMAGE_TOOL"
    exit 1
fi

echo "[INFO] 输出目录: $OUTPUT_DIR"
echo "[INFO] 版本号: $VERSION"
echo "[INFO] AppImage 名称: $APPIMAGE_NAME"
echo "[INFO] appimagetool: $APP_IMAGE_TOOL"

rm -rf "$APP_DIR"

mkdir -p "$APP_DIR/usr/bin"
mkdir -p "$APP_DIR/usr/share/applications"
mkdir -p "$APP_DIR/usr/share/icons/hicolor/256x256/apps"

echo "[INFO] 复制主程序..."
cp -a "$OUTPUT_DIR/appQtEasyTier" "$APP_DIR/usr/bin/"

echo "[INFO] 复制图标..."
cp -a "$ICON_SRC" "$APP_DIR/usr/share/icons/hicolor/256x256/apps/qtet.png"
cp -a "$ICON_SRC" "$APP_DIR/qtet.png"
ln -s "qtet.png" "$APP_DIR/.DirIcon"

echo "[INFO] 生成 AppImage 专用 desktop 文件..."
sed \
    -e "s|^Exec=.*|Exec=appQtEasyTier|" \
    -e "s|^Icon=.*|Icon=qtet|" \
    "$DESKTOP_SRC" > "$APP_DIR/usr/share/applications/qteasytier.desktop"
cp -a "$APP_DIR/usr/share/applications/qteasytier.desktop" "$APP_DIR/qteasytier.desktop"

echo "[INFO] 部署 Qt 依赖..."
"$APP_IMAGE_TOOL" deploy "$APP_DIR/usr/share/applications/qteasytier.desktop"

echo "[INFO] 构建 AppImage..."
VERSION="$VERSION" "$APP_IMAGE_TOOL" "$APP_DIR"
GENERATED_APPIMAGE="$APP_DIR-x86_64.AppImage"
if [[ -f "$GENERATED_APPIMAGE" ]]; then
    mv "$GENERATED_APPIMAGE" "$OUTPUT_DIR/$APPIMAGE_NAME"
else
    GENERATED_APPIMAGE=$(ls -t "$OUTPUT_DIR"/*.AppImage 2>/dev/null | head -n 1 || true)
    if [[ -z "$GENERATED_APPIMAGE" ]]; then
        echo "错误: 未找到生成的 AppImage 文件"
        exit 1
    fi
    mv "$GENERATED_APPIMAGE" "$OUTPUT_DIR/$APPIMAGE_NAME"
fi

rm -rf "$APP_DIR"

echo "[INFO] 打包完成: $OUTPUT_DIR/$APPIMAGE_NAME"
