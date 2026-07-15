#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "用法: $(basename "$0") --build-dir <build目录> --qml-dir <QML源码目录> --linuxdeployqt <linuxdeployqt路径> --version <版本号>"
}

BUILD_DIR=""
QML_DIR=""
LINUXDEPLOYQT=""
VERSION=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir)
            BUILD_DIR="${2:-}"
            shift 2
            ;;
        --qml-dir)
            QML_DIR="${2:-}"
            shift 2
            ;;
        --linuxdeployqt)
            LINUXDEPLOYQT="${2:-}"
            shift 2
            ;;
        --version)
            VERSION="${2:-}"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "未知参数: $1"
            usage
            exit 1
            ;;
    esac
done

if [[ -z "$BUILD_DIR" || -z "$QML_DIR" || -z "$LINUXDEPLOYQT" || -z "$VERSION" ]]; then
    usage
    exit 1
fi

OUTPUT_DIR="$BUILD_DIR/Output"
APP_BIN="$OUTPUT_DIR/appQtEasyTier"
APPIMAGE_WORK_DIR="$BUILD_DIR/AppImage"
APPDIR="$APPIMAGE_WORK_DIR/AppDir"
APPDIR_USR="$APPDIR/usr"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
ASSETS_DIR="$PROJECT_DIR/assets"
APPIMAGE_NAME="qteasytier_v${VERSION}_linux_amd64.AppImage"

if [[ ! -x "$APP_BIN" ]]; then
    echo "未找到主程序: $APP_BIN"
    exit 1
fi

if [[ ! -x "$LINUXDEPLOYQT" ]]; then
    echo "linuxdeployqt 不可执行: $LINUXDEPLOYQT"
    exit 1
fi

QMAKE_BIN="$(command -v qmake || true)"
if [[ -z "$QMAKE_BIN" ]]; then
    QMAKE_BIN="$(command -v qmake6 || true)"
fi
if [[ -z "$QMAKE_BIN" ]]; then
    echo "未找到 qmake/qmake6"
    exit 1
fi

QT_VERSION="$($QMAKE_BIN -query QT_VERSION)"
QT_LIBS_DIR="$($QMAKE_BIN -query QT_INSTALL_LIBS)"
QT_PREFIX="$($QMAKE_BIN -query QT_INSTALL_PREFIX)"

echo "[INFO] Qt 版本: $QT_VERSION"
echo "[INFO] Qt 安装目录: $QT_PREFIX"
echo "[INFO] Qt 库目录: $QT_LIBS_DIR"

rm -rf "$APPIMAGE_WORK_DIR"
mkdir -p "$APPDIR_USR/bin" "$APPDIR_USR/share/applications" "$APPDIR_USR/share/icons/hicolor/256x256/apps"

cp -a "$APP_BIN" "$APPDIR_USR/bin/"
cat > "$APPDIR_USR/share/applications/qteasytier.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=QtEasyTier
Comment=QtEasyTier is a remote networking tool based on EasyTier and Qt.
Exec=appQtEasyTier
Icon=qteasytier
Terminal=false
Categories=Utility;Qt;Web;Network;Internet;
EOF
cp -a "$ASSETS_DIR/favicon/qtet.png" "$APPDIR_USR/share/icons/hicolor/256x256/apps/qteasytier.png"

rm -f "$OUTPUT_DIR"/*.AppImage "$APPIMAGE_WORK_DIR"/*.AppImage

echo "[INFO] 使用 linuxdeployqt 构建 AppImage..."
(
    cd "$APPIMAGE_WORK_DIR"
    LD_LIBRARY_PATH="$QT_LIBS_DIR:${LD_LIBRARY_PATH:-}" \
        "$LINUXDEPLOYQT" "$APPDIR_USR/share/applications/qteasytier.desktop" \
            -qmake="$QMAKE_BIN" \
            -qmldir="$QML_DIR" \
            -appimage \
            -verbose=2 \
            -unsupported-allow-new-glibc
)

APPIMAGE_PATH=""
for candidate in "$APPIMAGE_WORK_DIR"/*.AppImage; do
    if [[ -f "$candidate" ]]; then
        APPIMAGE_PATH="$candidate"
        break
    fi
done

if [[ -z "$APPIMAGE_PATH" ]]; then
    echo "未找到 linuxdeployqt 生成的 AppImage"
    exit 1
fi

mv "$APPIMAGE_PATH" "$OUTPUT_DIR/$APPIMAGE_NAME"
chmod 755 "$OUTPUT_DIR/$APPIMAGE_NAME"

echo "[INFO] AppImage 已生成: $OUTPUT_DIR/$APPIMAGE_NAME"
