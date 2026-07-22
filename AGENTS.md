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
- Default daemon clone is GitHub; use `-DCLONE_DAEMON_FROM_GITEE=ON` for Gitee. If daemon build is enabled, configure needs `git` and network unless `build/qtet-daemon` is already present.
- Windows is documented for MinGW64/UCRT, not MSVC. With daemon enabled, configure also copies `ThirdParty/WinSW/DaemonInstaller.xml` to `Output/DaemonInstaller.xml` and downloads WinSW as `Output/DaemonInstaller.exe`.
- Focused test loop: `cmake --build build --target tst_network_conf` then `ctest --test-dir build -R tst_network_conf --output-on-failure`; test executables also live in `build/Output/`.
- Use `QT_QPA_PLATFORM=offscreen` for headless CTest runs; GitHub Actions sets it only on the test step.
- CI workflows under `.github/workflows/` use Qt 6.8.3 + Ninja, default `BUILD_WITH_DAEMON=ON`, and upload/package `build/Output`. `build-release.yml` runs from branches named `vX.Y.Z` and requires that version to match root `project(... VERSION ...)`.
- No formatter, linter, pre-commit, task runner, lockfile, or repo-local OpenCode config is present; use CMake build plus CTest as the source of truth.

## CMake And Files

- Root `CMakeLists.txt` defines app target, `QTET_QML_FILES`, Qt components, output dirs, and daemon options; production modules each have their own `src/**/CMakeLists.txt`.
- `appQtEasyTier` only compiles `src/main.cpp`, `assets/resources.qrc`, and generated Windows rc; production `.cpp` files belong in module targets such as `qtet_config`, `qtet_repository`, `qtet_viewmodel`, or `qtet_appsupport`.
- New C++ files go into the owning module target. Tests use `add_core_test(name file.cpp)` in `tests/CMakeLists.txt` and link the relevant `qtet_*` target; do not duplicate production `.cpp` files in tests.
- New QML files must be added to root `QTET_QML_FILES`; new non-QML resources must be added to `assets/resources.qrc`.
- If `importedcontent/CMakeLists.txt` exists, root CMake automatically adds it; this is the optional Figma/Qt import hook.

## Architecture Boundaries

- `src/app`: composition and startup support. `AppServices` owns long-lived services/ViewModels; `QmlSingletonRegistrar` is the only place for QML singleton registration.
- `src/core/config`: `NetworkConf`, TOML import/export, validation, URL codec. Keep DHCP/static IP semantics here: when `dhcp` is true, TOML export must omit `ipv4` even if the in-memory value is retained.
- `src/core/repository`: SQLite repositories; `DatabaseConnection::open()` is responsible for idempotent schema creation/migration.
- `src/core/service`: `qtet-daemon` local-socket IPC, JSON-RPC framing, and `DaemonApi`.
- `src/core/application`: app services for config commands/import/export, settings, autostart, logs, public servers, and run-state types.
- `src/core/viewmodel`: QML-facing facades and Qt models. QML should not call repositories, daemon clients, or platform helpers directly.
- `src/core/vpn_manager`: VPN lifecycle and runtime status; `VpnManager` exposes `nodeInfoModel` and `runtimeLogModel`.
- `src/core/system_tray`: tray manager and message dispatcher. `SystemTrayManager` is bound to the main window in `main.cpp`; it is not a QML singleton.
- `src/core/util` contains platform helpers; there is no `src/core/platform/` directory even though the target is named `qtet_platform`.

## QML And Lifetime

- `AppServices services(db.database(), &engine)` intentionally uses `QQmlApplicationEngine` as parent. Pre-created QML singletons use `QQmlEngine::setObjectOwnership(..., CppOwnership)`; do not replace this with `QApplication` parenting or `setContextProperty`.
- `Card.qml` uses `contentSpacing`; do not add/rename it to `spacing` because `Frame.spacing` is FINAL in Qt 6.7+.
- Prefer `palette.*` colors in QML; status colors come from `Theme.qml` (`statusGreen`, `statusOrange`, `statusRed`, `statusBlue`).
- `PageContainer.qml` uses a single `Loader`; switching pages destroys the old page instance, so page-local state and open dialogs are not preserved across navigation.

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
