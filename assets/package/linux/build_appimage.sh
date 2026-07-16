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

SKIPPED_PLUGINS_DIR="$OUTPUT_DIR/appimage-skipped-plugins"
DEPLOY_LOG="$OUTPUT_DIR/appimage-deploy.log"

restore_skipped_plugins() {
    if [[ ! -d "$SKIPPED_PLUGINS_DIR" ]]; then
        return
    fi

    local qt_plugins_dir=""
    if [[ -n "${Qt6_DIR:-}" ]]; then
        qt_plugins_dir="${Qt6_DIR}/plugins"
    elif [[ -n "${QTDIR:-}" ]]; then
        qt_plugins_dir="${QTDIR}/plugins"
    fi

    if [[ -z "$qt_plugins_dir" || ! -d "$qt_plugins_dir" ]]; then
        return
    fi

    while IFS= read -r -d '' skipped_plugin; do
        local relative_path="${skipped_plugin#$SKIPPED_PLUGINS_DIR/}"
        local restore_path="$qt_plugins_dir/$relative_path"
        mkdir -p "$(dirname "$restore_path")"
        mv "$skipped_plugin" "$restore_path"
        echo "[INFO] 已恢复跳过的 Qt 插件: $relative_path"
    done < <(find "$SKIPPED_PLUGINS_DIR" -type f -print0)
}

trap restore_skipped_plugins EXIT

extract_missing_library() {
    sed -nE 's/.*did not find library ([^[:space:]]+).*/\1/p' "$DEPLOY_LOG" | head -n 1
}

skip_plugins_requiring_library() {
    local missing_library="$1"
    local qt_plugins_dir=""
    if [[ -n "${Qt6_DIR:-}" ]]; then
        qt_plugins_dir="${Qt6_DIR}/plugins"
    elif [[ -n "${QTDIR:-}" ]]; then
        qt_plugins_dir="${QTDIR}/plugins"
    fi

    if [[ -z "$qt_plugins_dir" || ! -d "$qt_plugins_dir" ]]; then
        echo "错误: 无法定位 Qt 插件目录，不能自动跳过依赖 $missing_library 的插件"
        return 1
    fi

    local skipped_count=0
    while IFS= read -r -d '' plugin_file; do
        if readelf -d "$plugin_file" 2>/dev/null | grep -F "[$missing_library]" >/dev/null; then
            local relative_path="${plugin_file#$qt_plugins_dir/}"
            local skipped_path="$SKIPPED_PLUGINS_DIR/$relative_path"
            mkdir -p "$(dirname "$skipped_path")"
            mv "$plugin_file" "$skipped_path"
            echo "[INFO] 自动跳过缺失依赖 $missing_library 的 Qt 插件: $relative_path"
            skipped_count=$((skipped_count + 1))
        fi
    done < <(find "$qt_plugins_dir" -type f -print0)

    if [[ "$skipped_count" -eq 0 ]]; then
        echo "错误: 未在 Qt 插件目录中找到依赖 $missing_library 的插件"
        return 1
    fi
}

deploy_with_missing_plugin_skip() {
    local desktop_file="$1"
    local max_attempts=10
    local attempt=1

    while [[ "$attempt" -le "$max_attempts" ]]; do
        echo "[INFO] 运行 go-appimage deploy，第 $attempt 次..."
        set +e
        "$APP_IMAGE_TOOL" deploy "$desktop_file" 2>&1 | tee "$DEPLOY_LOG"
        local deploy_status=${PIPESTATUS[0]}
        set -e

        if [[ "$deploy_status" -eq 0 ]]; then
            return 0
        fi

        local missing_library
        missing_library="$(extract_missing_library)"
        if [[ -z "$missing_library" ]]; then
            echo "错误: go-appimage deploy 失败，且未发现可自动跳过的缺失库信息"
            return "$deploy_status"
        fi

        echo "[INFO] go-appimage deploy 缺失库: $missing_library"
        skip_plugins_requiring_library "$missing_library"
        attempt=$((attempt + 1))
    done

    echo "错误: go-appimage deploy 自动跳过插件后仍失败，已达到最大重试次数 $max_attempts"
    return 1
}

rm -rf "$APP_DIR"
rm -rf "$SKIPPED_PLUGINS_DIR"
rm -f "$DEPLOY_LOG"

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
deploy_with_missing_plugin_skip "$APP_DIR/usr/share/applications/qteasytier.desktop"

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
