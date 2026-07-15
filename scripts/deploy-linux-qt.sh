#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "用法: $(basename "$0") --build-dir <build目录> --deploy-dir <部署目录> --qml-dir <QML源码目录> --linuxdeployqt <linuxdeployqt路径>"
}

BUILD_DIR=""
DEPLOY_DIR=""
QML_DIR=""
LINUXDEPLOYQT=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir)
            BUILD_DIR="${2:-}"
            shift 2
            ;;
        --deploy-dir)
            DEPLOY_DIR="${2:-}"
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

if [[ -z "$BUILD_DIR" || -z "$DEPLOY_DIR" || -z "$QML_DIR" || -z "$LINUXDEPLOYQT" ]]; then
    usage
    exit 1
fi

OUTPUT_DIR="$BUILD_DIR/Output"
APP_BIN="$OUTPUT_DIR/appQtEasyTier"
APPDIR="$BUILD_DIR/AppDir"
APPDIR_USR="$APPDIR/usr"
OPT_DIR="$DEPLOY_DIR/opt/qteasytier"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
PACKAGE_DIR="$PROJECT_DIR/assets/package/linux"
ASSETS_DIR="$PROJECT_DIR/assets"

if [[ ! -x "$APP_BIN" ]]; then
    echo "未找到主程序: $APP_BIN"
    exit 1
fi

if [[ ! -x "$LINUXDEPLOYQT" ]]; then
    echo "linuxdeployqt 不可执行: $LINUXDEPLOYQT"
    exit 1
fi

QMAKE_BIN="$(command -v qmake6 || true)"
if [[ -z "$QMAKE_BIN" ]]; then
    QMAKE_BIN="$(command -v qmake || true)"
fi
if [[ -z "$QMAKE_BIN" ]]; then
    echo "未找到 qmake6/qmake"
    exit 1
fi

QT_VERSION="$($QMAKE_BIN -query QT_VERSION)"
QT_LIBS_DIR="$($QMAKE_BIN -query QT_INSTALL_LIBS)"
QT_QML_DIR="$($QMAKE_BIN -query QT_INSTALL_QML)"
QT_PLUGINS_DIR="$($QMAKE_BIN -query QT_INSTALL_PLUGINS)"

echo "[INFO] Qt 版本: $QT_VERSION"
echo "[INFO] Qt 库目录: $QT_LIBS_DIR"
echo "[INFO] Qt QML 目录: $QT_QML_DIR"
echo "[INFO] Qt 插件目录: $QT_PLUGINS_DIR"

rm -rf "$APPDIR" "$DEPLOY_DIR"
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

echo "[INFO] 使用 linuxdeployqt 收集 Qt 运行时..."
"$LINUXDEPLOYQT" "$APPDIR_USR/share/applications/qteasytier.desktop" \
    -qmake="$QMAKE_BIN" \
    -qmldir="$QML_DIR" \
    -verbose=2 \
    -unsupported-allow-new-glibc

mkdir -p "$OPT_DIR"

if [[ -d "$APPDIR_USR/bin" ]]; then
    cp -a "$APPDIR_USR/bin/." "$OPT_DIR/"
fi

for dir in lib plugins qml; do
    if [[ -d "$APPDIR_USR/$dir" ]]; then
        cp -a "$APPDIR_USR/$dir" "$OPT_DIR/"
    fi
done

if [[ -d "$APPDIR/plugins" && ! -d "$OPT_DIR/plugins" ]]; then
    cp -a "$APPDIR/plugins" "$OPT_DIR/"
fi
if [[ -d "$APPDIR/qml" && ! -d "$OPT_DIR/qml" ]]; then
    cp -a "$APPDIR/qml" "$OPT_DIR/"
fi

if [[ ! -d "$OPT_DIR/qml/QtQuick" && -d "$QT_QML_DIR/QtQuick" ]]; then
    mkdir -p "$OPT_DIR/qml"
    cp -a "$QT_QML_DIR/." "$OPT_DIR/qml/"
fi

if [[ ! -f "$OPT_DIR/lib/libQt6Core.so.6" && -d "$QT_LIBS_DIR" ]]; then
    mkdir -p "$OPT_DIR/lib"
    cp -a "$QT_LIBS_DIR"/libQt6*.so* "$OPT_DIR/lib/"
fi

for plugin_dir in platforms imageformats sqldrivers iconengines tls xcbglintegrations wayland-decoration-client wayland-graphics-integration-client wayland-shell-integration; do
    if [[ ! -d "$OPT_DIR/plugins/$plugin_dir" && -d "$QT_PLUGINS_DIR/$plugin_dir" ]]; then
        mkdir -p "$OPT_DIR/plugins"
        cp -a "$QT_PLUGINS_DIR/$plugin_dir" "$OPT_DIR/plugins/"
    fi
done

if [[ -f "$OUTPUT_DIR/qtet-daemon" ]]; then
    cp -a "$OUTPUT_DIR/qtet-daemon" "$OPT_DIR/"
fi
if [[ -f "$OUTPUT_DIR/libeasytier_ffi.so" ]]; then
    cp -a "$OUTPUT_DIR/libeasytier_ffi.so" "$OPT_DIR/"
fi

cp -a "$PACKAGE_DIR/qteasytier" "$OPT_DIR/qteasytier"
cp -a "$ASSETS_DIR/favicon/qtet.png" "$OPT_DIR/qtet.png"

chmod 755 "$OPT_DIR/appQtEasyTier" "$OPT_DIR/qteasytier"
if [[ -f "$OPT_DIR/qtet-daemon" ]]; then
    chmod 755 "$OPT_DIR/qtet-daemon"
fi

echo "[INFO] 部署目录已生成: $DEPLOY_DIR"
