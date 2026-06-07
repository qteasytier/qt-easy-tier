# macOS Release Checklist

This checklist is for a Developer ID distribution build. Do not use ad-hoc
signing results as release evidence.

For the current GitHub/Gitee community testing artifact, use
`docs/macos-community-build.md` instead.

## Prerequisites

- Developer ID Application certificate is installed in the signing keychain.
- `xcrun notarytool store-credentials` has created a keychain profile.
- Xcode command line tools are installed.
- Accessibility permission is granted for GUI smoke tools when running AX
  smoke.

## Build

```sh
cmake -S . -B build-release \
  -DCMAKE_BUILD_TYPE=Release \
  -DQTEASYTIER_ENABLE_PROTOTYPE_HELPER=OFF \
  -DCMAKE_INSTALL_PREFIX="$PWD/Install"
cmake --build build-release --target QtEasyTier QtEasyTierHelper -j4
cmake --install build-release
plutil -lint Install/QtEasyTier.app/Contents/Library/LaunchDaemons/io.github.qteasytier.QtEasyTier.Helper.plist
```

The existing development build may keep `QTEASYTIER_ENABLE_PROTOTYPE_HELPER=ON`
for prototype smoke tests. Do not use that build for distribution signing.

## Developer Build

```sh
cmake --build build-helper --target QtEasyTier QtEasyTierHelper -j4
cmake --install build-helper
```

## Sign, DMG, Notarize

```sh
export MACOS_SIGN_IDENTITY='Developer ID Application: <name> (<TEAMID>)'
export MACOS_DMG_SIGN_IDENTITY="$MACOS_SIGN_IDENTITY"
export MACOS_NOTARIZE=1
export MACOS_NOTARY_PROFILE='<notarytool-profile>'
export MACOS_REQUIRE_RELEASE_CHECKS=1
package/mac/build_mac.sh "$PWD"
```

Expected:

- `codesign --verify --deep --strict --verbose=2 Install/QtEasyTier.app`
  passes.
- `xcrun stapler validate <dmg>` passes.
- `spctl --assess` passes for app and DMG.

## Helper Lifecycle Smoke

```sh
zsh package/mac/smoke_smappservice_lifecycle.zsh
```

If macOS requires approval, approve QtEasyTier in System Settings, then rerun
the script. The Settings helper status should show client identity verification
enabled after the service-managed XPC helper is running.

## Prototype Regression Smoke

```sh
sudo -v
python3 package/mac/smoke_helper_ipc.py
QTEASYTIER_GUI_SMOKE_HELPER=1 zsh package/mac/smoke_gui_ax.zsh
```

Prototype smoke must continue to use `/tmp/qteasytier-$uid` and
`--allow-shutdown`. Those flags are not used by the production LaunchDaemon.
Run these tests from a development build configured with
`QTEASYTIER_ENABLE_PROTOTYPE_HELPER=ON`.

## Clean Machine

On a clean Mac:

1. Download the notarized DMG.
2. Verify Gatekeeper accepts opening the DMG and launching the app.
3. Install/approve the helper from Settings.
4. Start and stop one Network TUN instance.
5. Start and stop one OneClick host TUN instance.
6. Quit and relaunch QtEasyTier; helper-owned state should restore.
7. Uninstall the helper from Settings.
8. Confirm `launchctl print system/io.github.qteasytier.QtEasyTier.Helper`
   fails after uninstall.
