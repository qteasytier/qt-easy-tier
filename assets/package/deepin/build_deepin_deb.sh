#!/usr/bin/env bash
set -euo pipefail

# QtEasyTier Deepin (DDE) 打包脚本
# 用法: build_deepin_deb.sh -v <版本号> [-a <架构>]
# 产物: qteasytier-dde_v<版本号>_deepin_<架构>.deb
#
# 与 assets/package/linux/build_deb.sh 的区别：
#   - 额外收集 DDE 托盘插件（Output/Plugins/libqtetDdeTrayPlugin.so，与 dcc-setting 图标，安装到 dde-dock 标准目录；
#   - DEBIAN/control 的 Depends 补充 DTK6 / dde-tray-loader 运行时依赖；
#   - desktop 文件带 X-Deepin-Vendor 字段。

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
    echo "示例: $(basename "$0") -v 3.2.0 -a amd64"
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
PLUGIN_DIR="$OUTPUT_DIR/Plugins"

PACKAGE_NAME="qteasytier-dde"
BUILD_DIR="$OUTPUT_DIR/${PACKAGE_NAME}_deepin_${ARCH}"
DEB_NAME="${PACKAGE_NAME}_v${VERSION}_deepin_${ARCH}.deb"

# dde-dock 插件安装目录（与 src/dde_tray_plugin/CMakeLists.txt 的
# DDE_TRAY_PLUGIN_INSTALL_DIR / DDE_DCC_ICON_INSTALL_DIR 保持一致）
DDE_PLUGIN_INSTALL_DIR="usr/lib/dde-dock/plugins/system-trays"
DDE_DCC_ICON_INSTALL_DIR="usr/share/dde-dock/icons/dcc-setting"
# dde-tray-loader 固定读取 dcc-setting/<pluginName>.svg，文件名不可随意更改
DDE_DCC_ICON_NAME="qteasytier-dde-tray-plugin.svg"

echo "[INFO] 输出目录: $OUTPUT_DIR"
echo "[INFO] 版本号: $VERSION"
echo "[INFO] 架构: $ARCH"
echo "[INFO] 包名: $DEB_NAME"

rm -rf "$BUILD_DIR"

mkdir -p "$BUILD_DIR/DEBIAN"
mkdir -p "$BUILD_DIR/opt/qteasytier"
mkdir -p "$BUILD_DIR/usr/share/applications"
mkdir -p "$BUILD_DIR/etc/systemd/system"
mkdir -p "$BUILD_DIR/$DDE_PLUGIN_INSTALL_DIR"
mkdir -p "$BUILD_DIR/$DDE_DCC_ICON_INSTALL_DIR"

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

echo "[INFO] 复制 DDE 托盘插件..."
if [[ ! -f "$PLUGIN_DIR/libqtetDdeTrayPlugin.so" ]]; then
    echo "错误: DDE 托盘插件不存在: $PLUGIN_DIR/libqtetDdeTrayPlugin.so" >&2
    echo "请确认使用 -DBUILD_WITH_DDE_TRAY_PLUGIN=ON 配置并完成构建" >&2
    exit 1
fi
# 插件元数据（tray-plugin.json）已通过 Q_PLUGIN_METADATA 嵌入 .so，
# 此处只复制动态库，避免多余 json 文件与同目录官方插件混淆
cp -a "$PLUGIN_DIR/libqtetDdeTrayPlugin.so" "$BUILD_DIR/$DDE_PLUGIN_INSTALL_DIR/"
cp -a "$ASSETS_DIR/favicon/qtet.svg" "$BUILD_DIR/$DDE_DCC_ICON_INSTALL_DIR/$DDE_DCC_ICON_NAME"

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
chmod 755 "$BUILD_DIR/$DDE_PLUGIN_INSTALL_DIR/libqtetDdeTrayPlugin.so"
chmod 644 "$BUILD_DIR/$DDE_DCC_ICON_INSTALL_DIR/$DDE_DCC_ICON_NAME"
chmod 644 "$BUILD_DIR/usr/share/applications/qteasytier.desktop"
chmod 644 "$BUILD_DIR/etc/systemd/system/qtet-daemon.service"

echo "[INFO] 构建 Debian 包..."
dpkg-deb --build "$BUILD_DIR" "$OUTPUT_DIR/$DEB_NAME"

rm -rf "$BUILD_DIR"

echo "[INFO] 打包完成: $OUTPUT_DIR/$DEB_NAME"
