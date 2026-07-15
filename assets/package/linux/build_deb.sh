#!/usr/bin/env bash
set -euo pipefail

VERSION=""
SOURCE_DIR=""

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
        --source-dir)
            if [[ $# -lt 2 ]]; then
                echo "错误: $1 需要一个参数"
                exit 1
            fi
            SOURCE_DIR="$2"
            shift 2
            ;;
        *)
            echo "未知参数: $1"
            exit 1
            ;;
    esac
done

if [[ -z "$VERSION" ]]; then
    echo "用法: $(basename "$0") -v <版本号> [--source-dir <部署根目录>]"
    echo "示例: $(basename "$0") -v 3.0.0 --source-dir /path/to/Deploy"
    exit 1
fi

OUTPUT_DIR="$(pwd)"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ASSETS_DIR="$(cd "$SCRIPT_DIR/../../" && pwd)"

PACKAGE_NAME="qteasytier"
BUILD_DIR="$OUTPUT_DIR/${PACKAGE_NAME}_amd64"
DEB_NAME="${PACKAGE_NAME}_v${VERSION}_linux_amd64.deb"

echo "[INFO] 输出目录: $OUTPUT_DIR"
echo "[INFO] 版本号: $VERSION"
echo "[INFO] 包名: $DEB_NAME"
if [[ -n "$SOURCE_DIR" ]]; then
    SOURCE_DIR="$(cd "$SOURCE_DIR" && pwd)"
    echo "[INFO] 部署源目录: $SOURCE_DIR"
fi

rm -rf "$BUILD_DIR"

mkdir -p "$BUILD_DIR/DEBIAN"
mkdir -p "$BUILD_DIR/opt/qteasytier"
mkdir -p "$BUILD_DIR/usr/share/applications"
mkdir -p "$BUILD_DIR/etc/systemd/system"

if [[ -n "$SOURCE_DIR" ]]; then
    echo "[INFO] 复制部署目录..."
    cp -a "$SOURCE_DIR/." "$BUILD_DIR/"
else
    echo "[INFO] 复制程序文件..."
    for f in "$OUTPUT_DIR"/*; do
        if [ -f "$f" ]; then
            basename_f=$(basename "$f")
            if [[ "$basename_f" == *.a ]] || [[ "$basename_f" == tst* ]]; then
                echo "[INFO] 跳过: $basename_f"
                continue
            fi
            cp -a "$f" "$BUILD_DIR/opt/qteasytier/"
        fi
    done
fi

echo "[INFO] 复制图标..."
if [[ ! -f "$BUILD_DIR/opt/qteasytier/qtet.png" ]]; then
    cp -a "$ASSETS_DIR/favicon/qtet.png" "$BUILD_DIR/opt/qteasytier/"
fi

echo "[INFO] 复制控制文件..."
sed "s/^Version: .*/Version: ${VERSION}/" "$SCRIPT_DIR/DEBIAN/control" > "$BUILD_DIR/DEBIAN/control"
cp -a "$SCRIPT_DIR/DEBIAN/postinst" "$BUILD_DIR/DEBIAN/postinst"
cp -a "$SCRIPT_DIR/DEBIAN/prerm" "$BUILD_DIR/DEBIAN/prerm"

cp -a "$SCRIPT_DIR/qteasytier.desktop" "$BUILD_DIR/usr/share/applications/"
cp -a "$SCRIPT_DIR/qtet-daemon.service" "$BUILD_DIR/etc/systemd/system/"

chmod 755 "$BUILD_DIR/DEBIAN/postinst"
chmod 755 "$BUILD_DIR/DEBIAN/prerm"
chmod 644 "$BUILD_DIR/DEBIAN/control"

find "$BUILD_DIR/opt/qteasytier" -type d -exec chmod 755 {} +
find "$BUILD_DIR/opt/qteasytier" -type f -exec chmod 644 {} +
find "$BUILD_DIR/opt/qteasytier" -type f \( -name "*.so" -o -name "*.so.*" \) -exec chmod 755 {} +
for executable in appQtEasyTier qteasytier qtet-daemon; do
    if [[ -f "$BUILD_DIR/opt/qteasytier/$executable" ]]; then
        chmod 755 "$BUILD_DIR/opt/qteasytier/$executable"
    fi
done
chmod 644 "$BUILD_DIR/usr/share/applications/qteasytier.desktop"
chmod 644 "$BUILD_DIR/etc/systemd/system/qtet-daemon.service"

echo "[INFO] 构建 Debian 包..."
dpkg-deb --build "$BUILD_DIR" "$OUTPUT_DIR/$DEB_NAME"

rm -rf "$BUILD_DIR"

echo "[INFO] 打包完成: $OUTPUT_DIR/$DEB_NAME"
