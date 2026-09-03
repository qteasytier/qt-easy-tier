# QtEasyTier Agent Notes

Qt 6.8+ / C++20 / QML desktop app. C++ owns business logic, persistence, daemon IPC, and platform helpers; QML should stay as UI and binding code. QML module URI is `QtEasyTier`; `src/main.cpp` loads `qrc:/QtEasyTier/Main.qml`.

## Build And Verify

**You should not build daemon by default.**

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DBUILD_WITH_DAEMON=OFF
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/Output/appQtEasyTier
```

- All binaries and libraries go under `${CMAKE_BINARY_DIR}/Output`, not the build root.
- For offline/frontend-only work, configure with `-DBUILD_WITH_DAEMON=OFF`; default configure builds and collects `qtet-daemon` immediately via `cmake/QtEasyTierDaemon.cmake` and `cmake/scripts/*.cmake`.
- Default daemon clone is GitHub (`CLONE_DAEMON_FROM=GITHUB`); set `-DCLONE_DAEMON_FROM=GITEE` or `-DCLONE_DAEMON_FROM=CNB` to clone from Gitee or cnb.cool instead. If daemon build is enabled, configure needs `git` and network unless `build/qtet-daemon` is already present.
- qtet-daemon is not tag-pinned: `QTET_DAEMON_VER` is no longer defined in root `CMakeLists.txt`, so configure clones the remote **default branch** (`git clone --depth 1`); if `build/qtet-daemon` source already exists it is force re-aligned to the remote default branch on every configure (`git fetch --force origin` + `git remote set-head origin -a` + `git checkout --detach --force origin/HEAD`). Defining `QTET_DAEMON_VER` (e.g. `-DQTET_DAEMON_VER=1.0.0`) restores tag-pinned behavior (daemon 1.0 did not support the multi-frontend connection model).
- Windows is documented for MinGW64/UCRT, not MSVC. qtet-daemon self-registers the Windows service via `qtet-daemon.exe --install/--start/--stop/--uninstall` (each needs admin; the app elevates via `runElevated`); there is no WinSW download or `DaemonInstaller.exe` anymore. The Windows service name is `qtet-daemon.sock`.
- Focused test loop: `cmake --build build --target tst_network_conf` then `ctest --test-dir build -R tst_network_conf --output-on-failure`; test executables also live in `build/Output/`.
- `BUILD_WITH_DDE_TRAY_PLUGIN` defaults to ON and requires the `dde-tray-loader-dev` headers at the hardcoded `/usr/include/dde-dock` (configure `FATAL_ERROR`s if missing). It is a Deepin-only feature: pass `-DBUILD_WITH_DDE_TRAY_PLUGIN=OFF` on non-Deepin/CI builds (CI does not set it and is not Deepin — keep it OFF there).
- Use `QT_QPA_PLATFORM=offscreen` for headless CTest runs; GitHub Actions sets it only on the test step.
- CI workflows under `.github/workflows/` use Qt 6.8.3 + Ninja, default `BUILD_WITH_DAEMON=ON`, and upload/package `build/Output`. `build-release.yml` runs from branches named `vX.Y.Z` and requires that version to match root `project(... VERSION ...)`.
- No formatter, linter, pre-commit, task runner, lockfile, or repo-local OpenCode config is present; use CMake build plus CTest as the source of truth.

## OpenSSL (Static)

- The app uses a **static OpenSSL** for X25519 secure-mode key pairs (`src/config/X25519KeyHelper.*`, linked via `qtet_config` → `OpenSSL::Crypto`).
- The static OpenSSL path is **not hardcoded** in CMake. Root `CMakeLists.txt` sets `OPENSSL_USE_STATIC_LIBS TRUE` and calls `find_package(OpenSSL REQUIRED)`; the static prefix is supplied at configure time via `CMAKE_PREFIX_PATH`:
  ```bash
  cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DBUILD_WITH_DAEMON=OFF \
        -DCMAKE_PREFIX_PATH=/path/to/openssl-3.5.7
  ```
- If the static OpenSSL is **not found** on this system (e.g. configure fails under static mode, or only a shared `libcrypto.so` is available), **ask the user to provide the CMake search prefix of their static OpenSSL install** — the directory that contains `include/openssl/` plus `lib/libcrypto.a` and `lib/cmake/OpenSSL/OpenSSLConfig.cmake` — then configure with `-DCMAKE_PREFIX_PATH=<that prefix>`.
- A successful lookup prints `-- Found OpenSSL: .../libcrypto.a (found version "X.Y.Z")`.

## CMake And Files

- Root `CMakeLists.txt` defines the app target, output dirs, and daemon options, and aggregates the QML file lists collected per-directory under `src/qml/` (`QTET_QML_FILES` plus the `QTET_QML_SHARED_FILES`/`QTET_QML_DDE_FILES` frontend-tree sets); production modules each have their own `src/**/CMakeLists.txt`. No `QQuickStyle` is injected from C++ anywhere — the shared frontend is SwbControls-based with its own `SwbTheme`, the DDE frontend uses `org.deepin.dtk` controls directly (no Chameleon style injection either).
- **Frontend tree selection (replaces the old same-name masking)**: `BUILD_WITH_DDE=ON` compiles the complete standalone DTK tree `src/qml/dde/**` (DdeMain + its own `components/`, `pages/`, `config_form/`); `BUILD_WITH_DDE=OFF` compiles `Main.qml` + the shared `components/`/`pages/` tree. The two trees contain same-named types (`NetworkPage`, `Card`, `Sidebar`, `FormField`, …) which must never be mixed into one build (duplicate type names in the module). `components/Theme.qml` (pure QtObject status colors) and `components/PageContainer.qml` (pure QtQuick page container with animations) are frontend-agnostic and always compiled; `PageContainer` references pages by type name, so under DDE it resolves to `dde/pages/` automatically. In DDE builds nothing imports `SwbControls`: the `ThirdParty/SWB-QML-UI` subdirectory, its `theme.qrc`, and the `SwbControls`/`SwbControlsplugin` links are all wrapped in `if(NOT BUILD_WITH_DDE)`.
- `appQtEasyTier` only compiles `src/main.cpp`, `assets/resources.qrc`, and generated Windows rc; production `.cpp` files belong in module targets such as `qtet_config`, `qtet_sqlite_repository`, `qtet_appcore`, or `qtet_appsupport`.
- New C++ files go into the owning module target. Tests use `add_core_test(name file.cpp)` in `tests/CMakeLists.txt` and link the relevant `qtet_*` target; do not duplicate production `.cpp` files in tests.
- New QML files must be registered in the CMakeLists.txt of their owning directory under `src/qml/` (`components/`, `components/config_form/`, `pages/`, `dde/`, `dde/components/`, `dde/pages/`, `dde/config_form/`); the root `CMakeLists.txt` aggregates these lists (`QTET_QML_FILES` always-on set, plus `QTET_QML_SHARED_FILES`/`QTET_QML_DDE_FILES` selected by `BUILD_WITH_DDE`). List entries are paths relative to `src/qml/` and double as `QT_RESOURCE_ALIAS` (so qrc paths stay `qrc:/QtEasyTier/<relative path>`). New non-QML resources must be added to `assets/resources.qrc`.
- QML files that reference types living in a *different* subdirectory of the module (e.g. `config_form/` components using `Theme`/`IconToolButton` from `components/`) must have an explicit `import QtEasyTier`; same-directory types resolve implicitly, cross-directory ones do not.
- If `importedcontent/CMakeLists.txt` exists, root CMake automatically adds it; this is the optional Figma/Qt import hook.
- `ThirdParty/SWB-QML-UI/` is the vendored shadcn-style pure-QML control library (MIT, upstream <https://github.com/xxmzwf/SWB-QML-UI>), built via `add_subdirectory` as module URI `SwbControls` and linked **statically** into `appQtEasyTier` (root CMake sets `BUILD_SHARED_LIBS OFF` before the `add_subdirectory`; link both `SwbControls` and `SwbControlsplugin` per upstream's static-integration recipe — `libSwbControls.so` is no longer produced/shipped); QML files use it with `import SwbControls`. All local modifications to upstream are recorded in `ThirdParty/SWB-QML-UI/PATCHES.md` — the key ones: upstream requires Qt 6.10 but is patched down to 6.8, and `SwbIconLabel.qml` is patched to drop `QtQuick.VectorImage` entirely (deepin v25 ships the vectorimage libs but no QML module packages, so the import would fail at runtime; SVG icons render via `IconImage` instead). Component API reference lives at `ThirdParty/SWB-QML-UI/CONTROLS-Chinese.md`. Its controls carry their own `SwbTheme` (light/dark follows the system); status colors still come from `Theme.qml`. No extra CI modules or deb `Depends` entries are needed for it.
- `src/dde_tray_plugin/` builds two things: the `qtet_dde_tray_status` static lib (`TrayStatusService`/`TrayStatusTypes`, linked like any module) and the `qtetDdeTrayPlugin` MODULE loaded by dde-tray-loader. Install destinations are hardcoded absolute paths, overridable via `-DDDE_TRAY_PLUGIN_INSTALL_DIR` (`/usr/lib/dde-dock/plugins/system-trays`) and `-DDDE_DCC_ICON_INSTALL_DIR` (`/usr/share/dde-dock/icons/dcc-setting`); they ignore `CMAKE_INSTALL_PREFIX`. `QTET_DDE_TRAY_HAS_DTK` (optional `find_package(DTKGui)`) switches `icon()` to `DciIcon`.

## Architecture Boundaries

- `src/app`: composition and startup support. `AppServices` owns long-lived services/ViewModels; `QmlSingletonRegistrar` is the only place for QML singleton registration.
- `src/config`: `NetworkConf`, TOML import/export, validation, URL codec, `ConfigPayloadBuilder` (daemon `cfg_str` payload), and the shared `ConfigRunState` enum. Keep DHCP/static IP semantics here: when `dhcp` is true, TOML export must omit `ipv4` even if the in-memory value is retained.
- `src/sqlite_repository`: SQLite repositories (target `qtet_sqlite_repository`); `DatabaseConnection::open()` is responsible for idempotent schema creation/migration. `NetworkConfigRepository::generateUniqueInstanceName()` owns the `QtET-<UUID>` naming rule. The `FavoriteNode` value type lives here as the favorite-nodes table record; the JSON codec lives in `core/favorite/`.
- `src/daemon_service`: `qtet-daemon` local-socket IPC, JSON-RPC framing, and `DaemonApi` (target `qtet_daemon_service`).
- `src/core`: the application core layer bridging UI and basic services: config commands/import-export, `VpnRuntimeService` (the application-level VPN runtime coordinator owning instance lifecycle, heartbeat sync, and `NodeInfoModel`/`RuntimeLogModel`/`VpnController`/`StatusMonitor`, which live in `core/runtime/`), settings (`SettingsStore`/`UpdateCheckService`/`DangerousOperationService`), autostart, logs, favorite import/export (incl. `FavoriteNodeJsonCodec`). `DangerousOperationService` is a QML-facing service registered under the compatibility name `DangerousOperationViewModel`. `qtet_appcore` also depends on `qtet_config`, `qtet_sqlite_repository`, `qtet_daemon_service`, `qtet_platform`, `qtet_log`; basic services must NOT include `core/` or `core/viewmodels/` headers.
- `src/core/viewmodels`: QML-facing facades and Qt models. `src/core` and its `viewmodels/` subdirectory are compiled into the single `qtet_appcore` target. QML and ViewModels should call application services, not repositories/daemon clients/`VpnRuntimeService` internals directly (known exceptions: `LogViewModel`→`LogRepository`, `FavoriteNodeViewModel`→`FavoriteNodeRepository`, `BackendStatusViewModel`→`DaemonClient`, `ImportNodesViewModel`→`FavoriteNodeJsonCodec`, `SettingsViewModel`→`AutoStartHelper` for system autostart state — historical thin-shell debt, don't expand). `SettingsViewModel` directly owns auto-reconnect and update-check async state, using injected `DaemonApi`/`UpdateCheckService` (non-owning) and `AutoStartHelper`.
- `NetworkOptions.qml` is a data-driven thin shell (both frontends have one: `pages/` Swb version and `dde/pages/` DTK version): `ConfigEditorViewModel::formSections` (CONSTANT metadata: card groups, field key/title/type, combo options, spin ranges) is rendered by `FormField.qml` which dispatches to per-type renderers (`components/config_form/` Swb set, `dde/config_form/` DTK set); values are read via `ConfigEditorViewModel[fieldKey]` and written via `setFieldValue(key, value)` (QMetaObject reflection onto the named properties — all named setters/tests stay authoritative). Field-interlock disabling (dhcp→ipv4, whitelist switch→its text field) is deliberately QML-side (`fieldEnabledByKey` in the page shell). `tst_network_options_smoke` loads the shared shell QML with a real `AppServices` assembly and is only registered in non-DDE builds (DDE builds have no shared `NetworkOptions` in resources and must not instantiate DTK renderers on non-DTK hosts) — note `AppServices` conditionally creates config-related ViewModels, so pass a real opened `DatabaseConnection` (an invalid `QSqlDatabase` yields null singletons).
- `src/system_tray`: tray manager and message dispatcher. `SystemTrayManager` is bound to the main window in `main.cpp`; it is not a QML singleton.
- `src/platform`: platform helpers (`AutoStartHelper`, `DaemonRegisterHelper`, `FontHelper`) live here, built into the `qtet_platform` target. System autostart state (Windows registry / XDG Autostart) is the single source of truth; `settings3.json` does NOT persist `autoStart`. There is no `src/core/platform/` directory.
- `src/dde_tray_plugin`: Deepin tray plugin loaded by dde-tray-loader into its own `trayplugin-loader` process — separate from the app, no QML/ViewModel involvement. `TrayStatusService` is a lightweight parallel of `VpnRuntimeService` (own 3s heartbeat, same `qtet-daemon.sock`, same `qteasytier.db` read-only via `TrayStatusService::resolveDatabasePath`); keep their semantics in sync (e.g. node counts exclude the local node). The plugin must not call app-core services; it links `qtet_config`/`qtet_sqlite_repository`/`qtet_daemon_service` directly. Widgets returned to the loader (`m_icon`/`m_tips`/`m_popup`) are reparented by the loader, so members are `QPointer` — never assume ownership. `message()` must answer `Dock::MSG_GET_SUPPORT_FLAG`.

## QML And Lifetime

- `AppServices services(db.database(), &engine)` intentionally uses `QQmlApplicationEngine` as parent. Pre-created QML singletons use `QQmlEngine::setObjectOwnership(..., CppOwnership)`; do not replace this with `QApplication` parenting or `setContextProperty`.
- `Card.qml` uses `contentSpacing` and an optional `title` (bold, rendered by the card itself); do not add/rename `contentSpacing` to `spacing` because `Frame.spacing` is FINAL in Qt 6.7+. The shared (non-DDE) `Card`/`IconToolButton`/`ErrorDialog` are Swb-based but keep their original external API (`title`, `borderColor`, `iconSource`/`iconTint`). Tabs are plain `SwbTabBar` + `SwbTabButton` + `StackLayout` (the old `QtETabWidget` wrapper is gone; when stretching buttons, bind their width to the page root width, not `tabBar.width`, to avoid a binding loop through the bar's implicitWidth).
- Prefer `palette.*` colors in QML; status colors come from `Theme.qml` (`statusGreen`, `statusOrange`, `statusRed`, `statusBlue`). Pages/components restyled with SwbControls use `SwbTheme` tokens (`background`/`popover`/`foreground`/`mutedForeground`/`border`/`secondary`) instead of `palette.*` — do not mix the two systems inside one file. When referencing `Theme.qml` from QML, instantiate it as `Theme { id: appTheme }` — **never `id: theme`**: every Swb control carries a `theme` property (an `SwbStyle`), which shadows the id inside that control's bindings and yields `undefined` colors (e.g. `theme.statusGreen` on a `SwbLabel` breaks).
- `PageContainer.qml` uses a single `Loader`; switching pages destroys the old page instance, so page-local state and open dialogs are not preserved across navigation.
- DDE/DTK frontend conventions (everything under `src/qml/dde/`, controls from `import org.deepin.dtk` with `QtQuick.Controls` aliased as `QQC` to avoid name clashes):
  - Colors come from `palette.*` plus the shared `Theme.qml` status colors; never mix `SwbTheme` into a DDE file. Tabs are the self-drawn `dde/components/TabHeader.qml` (underline style) + `StackLayout` — do not use QQC `TabBar` or Chameleon style injection.
  - All dialogs are **modal `DialogWindow`s** (`modality: Qt.ApplicationModal`, `header: DialogTitleBar`, right-aligned button row). `DialogWindow` has no `standardButtons`/`onOpened`/`onAccepted`: pages implement `open()` (reset state → `visible = true` + `requestActivate()`), `close()`, and emit a compat `closed()` via the hadShown flag in `onVisibleChanged`. Simple confirm/input dialogs derive from `dde/components/ConfirmDialog.qml` (`message`, `inputMode`, `danger`, `accepted`/`rejected`). Validation-failing dialogs (list editors, node editor) keep the window open instead of closing on accept.
  - `DialogWindow`'s default content property only accepts `Item`: `Connections`/`Timer`/`Theme` must live inside the content `ColumnLayout` or be named properties; nested dialog windows must be mounted as named properties (see `revokeConfirmDialog` in `dde/components/CredentialManageDialog.qml`). Content uses `anchors.left/right: parent` with margins (`Layout.*` attached properties only work inside layouts, not against the window).
  - Scrolling uses `Flickable` + DTK `ScrollBar.vertical: ScrollBar {}`; multiline text uses `QQC.TextArea` with a self-drawn DTK-styled background (`org.deepin.dtk` has no `TextArea`); lists use `QQC.ListView`.
  - DDE `Sidebar` keeps the language button (DTK `Menu` + `LanguageController`) but has **no theme button** — DDE theming is owned by the title-bar DTK `ThemeMenu` (system theme), and `SettingsViewModel.themeMode` has no DDE UI entry.

## Runtime Data

- `src/main.cpp` fixes `organizationName = qteasytier` and `applicationName = QtEasyTier`; Linux `AppConfigLocation` is typically `~/.config/qteasytier/QtEasyTier/`.
- Default SQLite database is `qteasytier.db` in `AppConfigLocation`; global settings are `settings3.json` via `SettingsStore`, not SQLite.
- The app connects to `qtet-daemon.sock` by default. `tst_daemon_client` uses an in-memory `QLocalServer`; it does not require a real daemon.
- `assets/publicservers.json` is embedded as `:/publicservers.json` and read through `FavoriteNodeJsonCodec`.
- When a system tray is available, closing the window keeps the app running; autostart launches hidden if tray is available.

## Repo Hygiene

- `.gitignore` excludes `/build*/`, `/.qtcreator/`, `/.idea/`, `/.opencode/`, `/.worktrees/`, `/example/`, `/docs/`, and runtime `configs*` database files; do not treat these as source.
- `README.md` and `CONTRIBUTING.md` are broader prose docs. If they conflict with CMake or source, trust executable config and update this file only with verified gotchas.

## Output Language Requirements

- You should talk with me in **Chinese**.
- You should write the Doxygen comments in **Chinese** in this repo's code clearly.

## Coding Requirements

- Before you edit this repo, please read the [CONTRIBUTING.md](CONTRIBUTING.md) file.
- You should use the skills `superpowers` to analyze the repo's code before you make any changes if the skills are available.
- You should not usually use the Test-Driven Development (TDD) approach, for example, when the feature is simple and the test is trivial.
