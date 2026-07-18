#!/usr/bin/env bash
set -euo pipefail

VERSION=""
ARCH="amd64"

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
        -a|--arch)
            if [[ $# -lt 2 ]]; then
                echo "错误: $1 需要一个参数"
                exit 1
            fi
            ARCH="$2"
            shift 2
            ;;
        *)
            echo "未知参数: $1"
            exit 1
            ;;
    esac
done

if [[ -z "$VERSION" ]]; then
    echo "用法: $(basename "$0") -v <版本号> [-a <架构>]"
    echo "示例: $(basename "$0") -v 3.0.0 -a amd64"
    exit 1
fi

case "$ARCH" in
    amd64|arm64)
        ;;
    *)
        echo "错误: 不支持的架构: $ARCH"
        echo "支持的架构: amd64, arm64"
        exit 1
        ;;
esac

OUTPUT_DIR="$(pwd)"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ASSETS_DIR="$(cd "$SCRIPT_DIR/../../" && pwd)"

PACKAGE_NAME="qteasytier"
BUILD_DIR="$OUTPUT_DIR/${PACKAGE_NAME}_${ARCH}"
DEB_NAME="${PACKAGE_NAME}_v${VERSION}_linux_${ARCH}.deb"

echo "[INFO] 输出目录: $OUTPUT_DIR"
echo "[INFO] 版本号: $VERSION"
echo "[INFO] 架构: $ARCH"
echo "[INFO] 包名: $DEB_NAME"

rm -rf "$BUILD_DIR"

mkdir -p "$BUILD_DIR/DEBIAN"
mkdir -p "$BUILD_DIR/opt/qteasytier"
mkdir -p "$BUILD_DIR/usr/share/applications"
mkdir -p "$BUILD_DIR/etc/systemd/system"

echo "[INFO] 复制程序文件..."
for f in "$OUTPUT_DIR"/*; do
    if [ -f "$f" ]; then
        basename_f=$(basename "$f")
        if [[ "$basename_f" == *.a ]] || [[ "$basename_f" == tst* ]] || [[ "$basename_f" == *.AppImage ]]; then
            echo "[INFO] 跳过: $basename_f"
            continue
        fi
        cp -a "$f" "$BUILD_DIR/opt/qteasytier/"
    fi
done

echo "[INFO] 复制图标..."
cp -a "$ASSETS_DIR/favicon/qtet.png" "$BUILD_DIR/opt/qteasytier/"

echo "[INFO] 复制控制文件..."
sed \
    -e "s/^Version: .*/Version: ${VERSION}/" \
    -e "s/^Architecture: .*/Architecture: ${ARCH}/" \
    "$SCRIPT_DIR/DEBIAN/control" > "$BUILD_DIR/DEBIAN/control"
cp -a "$SCRIPT_DIR/DEBIAN/postinst" "$BUILD_DIR/DEBIAN/postinst"
cp -a "$SCRIPT_DIR/DEBIAN/prerm" "$BUILD_DIR/DEBIAN/prerm"

cp -a "$SCRIPT_DIR/qteasytier.desktop" "$BUILD_DIR/usr/share/applications/"
cp -a "$SCRIPT_DIR/qtet-daemon.service" "$BUILD_DIR/etc/systemd/system/"

chmod 755 "$BUILD_DIR/DEBIAN/postinst"
chmod 755 "$BUILD_DIR/DEBIAN/prerm"
chmod 644 "$BUILD_DIR/DEBIAN/control"

find "$BUILD_DIR/opt/qteasytier" -type f -exec chmod 755 {} +
find "$BUILD_DIR/opt/qteasytier" -type d -exec chmod 755 {} +
chmod 644 "$BUILD_DIR/usr/share/applications/qteasytier.desktop"
chmod 644 "$BUILD_DIR/etc/systemd/system/qtet-daemon.service"

echo "[INFO] 构建 Debian 包..."
dpkg-deb --build "$BUILD_DIR" "$OUTPUT_DIR/$DEB_NAME"

rm -rf "$BUILD_DIR"

echo "[INFO] 打包完成: $OUTPUT_DIR/$DEB_NAME"
