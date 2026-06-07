#!/bin/bash
set -euo pipefail

PROJECT_DIR="${1:-$(pwd)}"
VERSION="${2:-}"

if [ -z "$VERSION" ]; then
    CMAKE_FILE="$PROJECT_DIR/CMakeLists.txt"
    if [ ! -f "$CMAKE_FILE" ]; then
        echo "Error: missing CMakeLists.txt: $CMAKE_FILE"
        exit 1
    fi
    VERSION=$(sed -nE 's/.*project\([^)]*VERSION ([0-9.]+).*/\1/p' "$CMAKE_FILE" || true)
    if [ -z "$VERSION" ]; then
        echo "Error: unable to read project version from CMakeLists.txt"
        exit 1
    fi
fi

APP_NAME="QtEasyTier"
APP_IDENTIFIER="io.github.qteasytier.QtEasyTier"
APP_PATH="$PROJECT_DIR/Install/${APP_NAME}.app"
APP_EXECUTABLE="$APP_PATH/Contents/MacOS/$APP_NAME"
CIDR_EXECUTABLE="$APP_PATH/Contents/MacOS/CIDRCalculator"
HELPER_EXECUTABLE="$APP_PATH/Contents/MacOS/QtEasyTierHelper"
HELPER_LAUNCHD_PLIST="$APP_PATH/Contents/Library/LaunchDaemons/io.github.qteasytier.QtEasyTier.Helper.plist"
APP_FRAMEWORKS_DIR="$APP_PATH/Contents/Frameworks"
SVG_ICON_PLUGIN="$APP_PATH/Contents/PlugIns/iconengines/libqsvgicon.dylib"
SVG_IMAGE_PLUGIN="$APP_PATH/Contents/PlugIns/imageformats/libqsvg.dylib"
SIGN_IDENTITY="${MACOS_SIGN_IDENTITY:--}"
DMG_SIGN_IDENTITY="${MACOS_DMG_SIGN_IDENTITY:-$SIGN_IDENTITY}"
APP_ENTITLEMENTS="${MACOS_APP_ENTITLEMENTS:-$PROJECT_DIR/assets/macos/QtEasyTier.entitlements}"
HELPER_ENTITLEMENTS="${MACOS_HELPER_ENTITLEMENTS:-$PROJECT_DIR/assets/macos/QtEasyTierHelper.entitlements}"
NOTARIZE="${MACOS_NOTARIZE:-0}"
NOTARY_PROFILE="${MACOS_NOTARY_PROFILE:-}"
REQUIRE_RELEASE_CHECKS="${MACOS_REQUIRE_RELEASE_CHECKS:-0}"
ALLOW_PROTOTYPE_HELPER="${MACOS_ALLOW_PROTOTYPE_HELPER:-0}"
COMMUNITY_BUILD="${MACOS_COMMUNITY_BUILD:-0}"
echo "=============================="
echo "Version: $VERSION"
echo "Project: $PROJECT_DIR"
echo "Signing identity: $SIGN_IDENTITY"
echo "Notarize: $NOTARIZE"
echo "Community build: $COMMUNITY_BUILD"
echo "=============================="
require_file() {
    local path="$1"
    local message="$2"
    if [ ! -f "$path" ]; then
        echo "Error: $message: $path"
        exit 1
    fi
}
require_executable() {
    local path="$1"
    local message="$2"
    if [ ! -x "$path" ]; then
        echo "Error: $message: $path"
        exit 1
    fi
}
require_developer_id_identity() {
    if [[ "$SIGN_IDENTITY" != Developer\ ID\ Application:* ]]; then
        echo "Error: release checks require MACOS_SIGN_IDENTITY to be a Developer ID Application identity"
        exit 1
    fi
    if [[ "$DMG_SIGN_IDENTITY" != Developer\ ID\ Application:* ]]; then
        echo "Error: release checks require MACOS_DMG_SIGN_IDENTITY to be a Developer ID Application identity"
        exit 1
    fi
}
codesign_field() {
    local target="$1"
    local field="$2"
    codesign -dvv "$target" 2>&1 | sed -nE "s/^${field}=//p" | head -n 1
}
check_release_preflight() {
    if [ "$REQUIRE_RELEASE_CHECKS" != "1" ]; then
        return
    fi
    require_developer_id_identity
    if [ "$NOTARIZE" != "1" ]; then
        echo "Error: release checks require MACOS_NOTARIZE=1"
        exit 1
    fi
    if [ -z "$NOTARY_PROFILE" ]; then
        echo "Error: release checks require MACOS_NOTARY_PROFILE"
        exit 1
    fi
    if [ "$ALLOW_PROTOTYPE_HELPER" != "0" ]; then
        echo "Error: release checks do not allow prototype helper fallback"
        exit 1
    fi
}
check_release_preflight

if [ ! -d "$APP_PATH" ]; then
    echo "Error: missing app bundle: $APP_PATH"
    echo "Run cmake --install before packaging."
    exit 1
fi

require_executable "$APP_EXECUTABLE" "missing app executable"
require_executable "$CIDR_EXECUTABLE" "missing CIDRCalculator"
require_executable "$HELPER_EXECUTABLE" "missing QtEasyTierHelper"
require_file "$HELPER_LAUNCHD_PLIST" "missing helper LaunchDaemon plist"
require_file "$APP_FRAMEWORKS_DIR/libeasytier_ffi.dylib" "missing EasyTier FFI dylib"
require_file "$SVG_ICON_PLUGIN" "missing SVG icon plugin"
require_file "$SVG_IMAGE_PLUGIN" "missing SVG image plugin"
if [ "$SIGN_IDENTITY" != "-" ]; then
    require_file "$APP_ENTITLEMENTS" "missing app entitlements"
    require_file "$HELPER_ENTITLEMENTS" "missing helper entitlements"
fi

if command -v plutil >/dev/null 2>&1; then
    plutil -lint "$HELPER_LAUNCHD_PLIST"
fi
if /usr/libexec/PlistBuddy -c 'Print :MachServices:io.github.qteasytier.QtEasyTier.Helper' "$HELPER_LAUNCHD_PLIST" >/dev/null 2>&1; then
    :
else
    echo "Error: helper LaunchDaemon plist is missing the XPC MachServices entry"
    exit 1
fi
if /usr/libexec/PlistBuddy -c 'Print :ProgramArguments' "$HELPER_LAUNCHD_PLIST" | grep -q -- '--socket'; then
    echo "Error: production helper LaunchDaemon plist must use XPC, not --socket"
    exit 1
fi

if [ "$REQUIRE_RELEASE_CHECKS" = "1" ]; then
    if strings "$APP_EXECUTABLE" | grep -q 'QTEASYTIER_FORCE_PROTOTYPE_HELPER'; then
        echo "Error: release app binary contains prototype helper fallback strings"
        echo "Reconfigure with -DQTEASYTIER_ENABLE_PROTOTYPE_HELPER=OFF and rebuild."
        exit 1
    fi
    if ! strings "$HELPER_EXECUTABLE" | grep -q 'QTEASYTIER_PROTOTYPE_HELPER_DISABLED'; then
        echo "Error: release helper does not contain the prototype-disabled marker"
        echo "Reconfigure with -DQTEASYTIER_ENABLE_PROTOTYPE_HELPER=OFF and rebuild."
        exit 1
    fi
fi
if [ "$COMMUNITY_BUILD" = "1" ]; then
    if strings "$HELPER_EXECUTABLE" | grep -q 'QTEASYTIER_PROTOTYPE_HELPER_DISABLED'; then
        echo "Error: community macOS build requires prototype helper support."
        echo "Reconfigure with -DQTEASYTIER_ENABLE_PROTOTYPE_HELPER=ON and rebuild."
        exit 1
    fi
fi

if ! find "$APP_FRAMEWORKS_DIR" -maxdepth 1 -name 'Qt*.framework' -type d | grep -q .; then
    echo "Error: missing Qt frameworks. Run cmake --install and macdeployqt first."
    exit 1
fi

ARCH=$(lipo -archs "$APP_EXECUTABLE" | tr ' ' '-' || true)
if [ -z "$ARCH" ]; then
    echo "Error: unable to read app architecture: $APP_EXECUTABLE"
    exit 1
fi

is_macho_file() {
    file "$1" | grep -q 'Mach-O'
}

bundle_dependency_ref() {
    local dep="$1"
    if [[ "$dep" == *".framework/"* ]]; then
        printf '%s\n' "$dep" | sed -E 's|^.*\/([^\/]+\.framework\/.*)$|@rpath/\1|'
    else
        printf '@rpath/%s\n' "$(basename "$dep")"
    fi
}

macho_install_names() {
    local target="$1"
    otool -D "$target" 2>/dev/null | awk '
        NR == 1 { next }
        /:$/ { next }
        NF { print }
    '
}

fix_local_dependency_paths() {
    local scan_root="$1"
    local macho install_name refs dep new_ref

    while IFS= read -r -d '' macho; do
        if ! is_macho_file "$macho"; then
            continue
        fi

        while IFS= read -r install_name; do
            if [ -z "$install_name" ]; then
                continue
            fi
            if printf '%s\n' "$install_name" | grep -Eq '/opt/homebrew|/usr/local|/Users/'; then
                new_ref=$(bundle_dependency_ref "$install_name")
                echo "Fix install name: $macho"
                echo "  $install_name -> $new_ref"
                install_name_tool -id "$new_ref" "$macho"
            fi
        done < <(macho_install_names "$macho")

        refs=$(otool -L "$macho" 2>/dev/null \
            | awk 'substr($0, 1, 1) == "\t" { print $1 }' \
            | grep -E '/opt/homebrew|/usr/local|/Users/' || true)

        while IFS= read -r dep; do
            if [ -z "$dep" ]; then
                continue
            fi
            new_ref=$(bundle_dependency_ref "$dep")
            echo "Fix dependency: $macho"
            echo "  $dep -> $new_ref"
            install_name_tool -change "$dep" "$new_ref" "$macho"
        done <<< "$refs"
    done < <(find "$scan_root" -type f -print0)
}

check_no_local_dependency_paths() {
    local scan_root="$1"
    local bad_deps=0
    local macho install_name refs

    while IFS= read -r -d '' macho; do
        if ! is_macho_file "$macho"; then
            continue
        fi

        while IFS= read -r install_name; do
            if [ -z "$install_name" ]; then
                continue
            fi
            if printf '%s\n' "$install_name" | grep -Eq '/opt/homebrew|/usr/local|/Users/'; then
                echo "Error: local absolute install name in $macho"
                printf '%s\n' "$install_name"
                bad_deps=1
            fi
        done < <(macho_install_names "$macho")

        refs=$(otool -L "$macho" \
            | awk 'substr($0, 1, 1) == "\t" { print $1 }' \
            | grep -E '/opt/homebrew|/usr/local|/Users/' || true)
        if [ -n "$refs" ]; then
            echo "Error: local absolute dependency path in $macho"
            printf '%s\n' "$refs"
            bad_deps=1
        fi
    done < <(find "$scan_root" -type f -print0)

    return "$bad_deps"
}

sign_file() {
    local target="$1"
    shift
    if [ "$SIGN_IDENTITY" = "-" ]; then
        codesign --force --sign - "$target"
    else
        codesign --force --timestamp --sign "$SIGN_IDENTITY" "$@" "$target"
    fi
}
sign_macho() {
    local target="$1"
    local runtime="${2:-0}"
    local entitlements="${3:-}"
    if [ "$SIGN_IDENTITY" = "-" ]; then
        sign_file "$target"
        return
    fi
    if [ "$runtime" = "1" ] && [ -n "$entitlements" ]; then
        sign_file "$target" --options runtime --entitlements "$entitlements"
    elif [ "$runtime" = "1" ]; then
        sign_file "$target" --options runtime
    elif [ -n "$entitlements" ]; then
        sign_file "$target" --entitlements "$entitlements"
    else
        sign_file "$target"
    fi
}
sign_nested_items() {
    local item
    while IFS= read -r -d '' item; do
        if is_macho_file "$item"; then
            sign_macho "$item" 1
        fi
    done < <(find "$APP_FRAMEWORKS_DIR" -type f -print0)
    if [ -d "$APP_PATH/Contents/PlugIns" ]; then
        while IFS= read -r -d '' item; do
            if is_macho_file "$item"; then
                sign_macho "$item" 1
            fi
        done < <(find "$APP_PATH/Contents/PlugIns" -type f -print0)
    fi
    sign_macho "$CIDR_EXECUTABLE" 1
    sign_macho "$HELPER_EXECUTABLE" 1 "$HELPER_ENTITLEMENTS"
    sign_macho "$APP_EXECUTABLE" 1 "$APP_ENTITLEMENTS"
    if [ "$SIGN_IDENTITY" = "-" ]; then
        codesign --force --sign - "$APP_PATH"
    else
        if [ -n "$APP_ENTITLEMENTS" ]; then
            codesign --force --options runtime --timestamp --sign "$SIGN_IDENTITY" \
                --entitlements "$APP_ENTITLEMENTS" "$APP_PATH"
        else
            codesign --force --options runtime --timestamp --sign "$SIGN_IDENTITY" "$APP_PATH"
        fi
    fi
}
fix_local_dependency_paths "$APP_PATH"
check_no_local_dependency_paths "$APP_PATH"

echo "Signing app bundle..."
sign_nested_items

codesign --verify --deep --strict --verbose=2 "$APP_PATH"

if [ "$REQUIRE_RELEASE_CHECKS" = "1" ]; then
    app_identifier="$(codesign_field "$APP_PATH" "Identifier")"
    app_team="$(codesign_field "$APP_PATH" "TeamIdentifier")"
    helper_team="$(codesign_field "$HELPER_EXECUTABLE" "TeamIdentifier")"
    if [ "$app_identifier" != "$APP_IDENTIFIER" ]; then
        echo "Error: signed app identifier mismatch: expected $APP_IDENTIFIER, got ${app_identifier:-<empty>}"
        exit 1
    fi
    if [ -z "$app_team" ] || [ -z "$helper_team" ] || [ "$app_team" != "$helper_team" ]; then
        echo "Error: app/helper TeamIdentifier mismatch or missing TeamIdentifier"
        echo "App TeamIdentifier: ${app_team:-<empty>}"
        echo "Helper TeamIdentifier: ${helper_team:-<empty>}"
        exit 1
    fi
    prototype_probe_dir="${TMPDIR:-/tmp}/qteasytier-release-helper-check-$$"
    rm -rf "$prototype_probe_dir"
    mkdir -p "$prototype_probe_dir"
    "$HELPER_EXECUTABLE" \
        --daemon \
        --socket "$prototype_probe_dir/helper.sock" \
        --token-file "$prototype_probe_dir/helper.token" \
        --owner-uid "$(id -u)" \
        > "$prototype_probe_dir/helper.out" \
        2> "$prototype_probe_dir/helper.err" &
    prototype_probe_pid=$!
    prototype_probe_waits=0
    while kill -0 "$prototype_probe_pid" >/dev/null 2>&1 && [ "$prototype_probe_waits" -lt 30 ]; do
        sleep 0.1
        prototype_probe_waits=$((prototype_probe_waits + 1))
    done
    if kill -0 "$prototype_probe_pid" >/dev/null 2>&1; then
        kill "$prototype_probe_pid" >/dev/null 2>&1 || true
        wait "$prototype_probe_pid" >/dev/null 2>&1 || true
        rm -rf "$prototype_probe_dir"
        echo "Error: release helper accepted local socket prototype mode"
        echo "Reconfigure with -DQTEASYTIER_ENABLE_PROTOTYPE_HELPER=OFF and rebuild."
        exit 1
    fi
    set +e
    wait "$prototype_probe_pid"
    prototype_probe_rc=$?
    set -e
    if [ "$prototype_probe_rc" -eq 0 ] || ! grep -q 'local socket prototype mode is disabled' "$prototype_probe_dir/helper.err"; then
        echo "Error: release helper did not clearly reject local socket prototype mode"
        cat "$prototype_probe_dir/helper.err"
        rm -rf "$prototype_probe_dir"
        exit 1
    fi
    rm -rf "$prototype_probe_dir"
fi

if [ "$COMMUNITY_BUILD" = "1" ]; then
    DMG_NAME="${APP_NAME}_v${VERSION}_macos_${ARCH}_community"
else
    DMG_NAME="${APP_NAME}_v${VERSION}_macos_${ARCH}"
fi
DMG_PATH="$PROJECT_DIR/Install/${DMG_NAME}.dmg"
DMG_STAGE="$PROJECT_DIR/Install/${DMG_NAME}-stage"
echo "Creating DMG..."
rm -rf "$DMG_STAGE"
mkdir -p "$DMG_STAGE"
cp -R "$APP_PATH" "$DMG_STAGE/"
ln -s /Applications "$DMG_STAGE/Applications"

hdiutil create \
    -volname "$APP_NAME" \
    -srcfolder "$DMG_STAGE" \
    -ov \
    -format UDZO \
    "$DMG_PATH"

rm -rf "$DMG_STAGE"
if [ "$DMG_SIGN_IDENTITY" != "-" ]; then
    codesign --force --timestamp --sign "$DMG_SIGN_IDENTITY" \
        -i "${APP_IDENTIFIER}.dmg" \
        "$DMG_PATH"
fi
if [ "$NOTARIZE" = "1" ]; then
    if [ -z "$NOTARY_PROFILE" ]; then
        echo "Error: MACOS_NOTARY_PROFILE is required when MACOS_NOTARIZE=1"
        exit 1
    fi
    xcrun notarytool submit "$DMG_PATH" --keychain-profile "$NOTARY_PROFILE" --wait
    xcrun stapler staple "$DMG_PATH"
    xcrun stapler validate "$DMG_PATH"
fi
if command -v spctl >/dev/null 2>&1; then
    if [ "$REQUIRE_RELEASE_CHECKS" = "1" ]; then
        spctl --assess --type execute --verbose=4 "$APP_PATH"
        spctl --assess --type open --context context:primary-signature --verbose=4 "$DMG_PATH"
    else
        spctl --assess --type execute --verbose=4 "$APP_PATH" || true
        spctl --assess --type open --context context:primary-signature --verbose=4 "$DMG_PATH" || true
    fi
fi
echo "=============================="
echo "DMG created: $DMG_PATH"
echo "=============================="
