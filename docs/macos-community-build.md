# macOS Community Testing Build

This build target is for open-source release artifacts when Apple Developer ID
credentials are not available. It is intentionally not notarized.

GitHub/Gitee macOS artifacts should use this checklist unless a Developer ID
certificate and notary profile are available.

## Build

```sh
cmake -S . -B build-macos-community \
  -DCMAKE_BUILD_TYPE=Release \
  -DQTEASYTIER_ENABLE_PROTOTYPE_HELPER=ON \
  -DCMAKE_INSTALL_PREFIX="$PWD/Install"
cmake --build build-macos-community --target QtEasyTier QtEasyTierHelper -j4
cmake --install build-macos-community
MACOS_COMMUNITY_BUILD=1 package/mac/build_mac.sh "$PWD"
```

Expected artifact:

```text
Install/QtEasyTier_v<version>_macos_<arch>_community.dmg
```

## Validation

```sh
plutil -lint Install/QtEasyTier.app/Contents/Library/LaunchDaemons/io.github.qteasytier.QtEasyTier.Helper.plist
codesign --verify --deep --strict --verbose=2 Install/QtEasyTier.app
hdiutil verify Install/QtEasyTier_v*_macos_*_community.dmg
```

For real TUN smoke on a development Mac:

```sh
sudo -v
QTEASYTIER_APP=Install/QtEasyTier.app python3 package/mac/smoke_helper_ipc.py
QTEASYTIER_APP=Install/QtEasyTier.app QTEASYTIER_GUI_SMOKE_HELPER=1 \
  zsh package/mac/smoke_gui_ax.zsh
```

The GUI smoke uses `peekaboo` for Accessibility automation. The script looks for
`peekaboo` on `PATH`; if it is installed elsewhere, set
`QTEASYTIER_PEEK=/path/to/peekaboo`.

## User Notes

Because this build is not notarized, macOS may block the first launch. Users can
right-click the app and choose Open, or clear the quarantine attribute after
copying the app to `/Applications`:

```sh
xattr -dr com.apple.quarantine /Applications/QtEasyTier.app
```

The first TUN network start asks for administrator authorization. The helper is
a temporary local helper for the active user session, not a permanently
installed LaunchDaemon.

Community builds prefer the prototype helper runtime even if a stale
SMAppService/LaunchDaemon registration exists on the test machine. The
service-managed helper path is reserved for the future Developer ID release
target.

## Not Covered

- Gatekeeper acceptance through `spctl`.
- Notarization and stapling.
- Developer ID signed SMAppService/XPC helper approval.
- Clean-machine installation without manual security prompts.
