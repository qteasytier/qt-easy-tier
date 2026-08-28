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
- qtet-daemon is version-managed: configure clones the tag named by root `QTET_DAEMON_VER` (`--branch ${QTET_DAEMON_VER} --depth 1`); if `build/qtet-daemon` source already exists it is force re-aligned to that tag on every configure (`git fetch --tags --force` + `git checkout --detach --force`).
- Windows is documented for MinGW64/UCRT, not MSVC. qtet-daemon self-registers the Windows service via `qtet-daemon.exe --install/--start/--stop/--uninstall` (each needs admin; the app elevates via `runElevated`); there is no WinSW download or `DaemonInstaller.exe` anymore. The Windows service name is `qtet-daemon.sock`.
- Focused test loop: `cmake --build build --target tst_network_conf` then `ctest --test-dir build -R tst_network_conf --output-on-failure`; test executables also live in `build/Output/`.
- Use `QT_QPA_PLATFORM=offscreen` for headless CTest runs; GitHub Actions sets it only on the test step.
- CI workflows under `.github/workflows/` use Qt 6.8.3 + Ninja, default `BUILD_WITH_DAEMON=ON`, and upload/package `build/Output`. `build-release.yml` runs from branches named `vX.Y.Z` and requires that version to match root `project(... VERSION ...)`.
- No formatter, linter, pre-commit, task runner, lockfile, or repo-local OpenCode config is present; use CMake build plus CTest as the source of truth.

## OpenSSL (Static)

- The app uses a **static OpenSSL** for X25519 secure-mode key pairs (`src/core/config/X25519KeyHelper.*`, linked via `qtet_config` → `OpenSSL::Crypto`).
- The static OpenSSL path is **not hardcoded** in CMake. Root `CMakeLists.txt` sets `OPENSSL_USE_STATIC_LIBS TRUE` and calls `find_package(OpenSSL REQUIRED)`; the static prefix is supplied at configure time via `CMAKE_PREFIX_PATH`:
  ```bash
  cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DBUILD_WITH_DAEMON=OFF \
        -DCMAKE_PREFIX_PATH=/path/to/openssl-3.5.7
  ```
- If the static OpenSSL is **not found** on this system (e.g. configure fails under static mode, or only a shared `libcrypto.so` is available), **ask the user to provide the CMake search prefix of their static OpenSSL install** — the directory that contains `include/openssl/` plus `lib/libcrypto.a` and `lib/cmake/OpenSSL/OpenSSLConfig.cmake` — then configure with `-DCMAKE_PREFIX_PATH=<that prefix>`.
- A successful lookup prints `-- Found OpenSSL: .../libcrypto.a (found version "X.Y.Z")`.

## CMake And Files

- Root `CMakeLists.txt` defines app target, `QTET_QML_FILES`, Qt components, output dirs, and daemon options; production modules each have their own `src/**/CMakeLists.txt`.
- `appQtEasyTier` only compiles `src/main.cpp`, `assets/resources.qrc`, and generated Windows rc; production `.cpp` files belong in module targets such as `qtet_config`, `qtet_repository`, `qtet_viewmodel`, or `qtet_appsupport`.
- New C++ files go into the owning module target. Tests use `add_core_test(name file.cpp)` in `tests/CMakeLists.txt` and link the relevant `qtet_*` target; do not duplicate production `.cpp` files in tests.
- New QML files must be added to root `QTET_QML_FILES`; new non-QML resources must be added to `assets/resources.qrc`.
- If `importedcontent/CMakeLists.txt` exists, root CMake automatically adds it; this is the optional Figma/Qt import hook.

## Architecture Boundaries

- `src/app`: composition and startup support. `AppServices` owns long-lived services/ViewModels; `QmlSingletonRegistrar` is the only place for QML singleton registration.
- `src/core/config`: `NetworkConf`, TOML import/export, validation, URL codec, `ConfigPayloadBuilder` (daemon `cfg_str` payload), and the shared `ConfigRunState` enum. Keep DHCP/static IP semantics here: when `dhcp` is true, TOML export must omit `ipv4` even if the in-memory value is retained.
- `src/core/repository`: SQLite repositories; `DatabaseConnection::open()` is responsible for idempotent schema creation/migration. `NetworkConfigRepository::generateUniqueInstanceName()` owns the `QtET-<UUID>` naming rule.
- `src/core/service`: `qtet-daemon` local-socket IPC, JSON-RPC framing, and `DaemonApi`.
- `src/app_service`: app-service layer bridging UI and basic services: config commands/import-export, `VpnRuntimeService` (the application-level VPN runtime coordinator owning instance lifecycle, heartbeat sync, and `NodeInfoModel`/`RuntimeLogModel`, which live in `app_service/runtime/`), settings (`SettingsStore`/`AutoStartService`/`SettingsBackendService`), autostart, logs, `DangerousOperationService`, favorite import/export. Links `qtet_vpn`; basic services must NOT include `app_service` or `viewmodels` headers.
- `src/viewmodels`: QML-facing facades and Qt models. QML and ViewModels should call application services, not repositories/daemon clients/`VpnRuntimeService` internals directly (known exceptions: `LogViewModel`→`LogRepository`, `FavoriteNodeViewModel`→`FavoriteNodeRepository`, `BackendStatusViewModel`→`DaemonClient`, `ImportNodesViewModel`→`FavoriteNodeJsonCodec` — historical thin-shell debt, don't expand).
- `src/core/vpn_manager`: single-instance lifecycle state machine (`VpnController`) and daemon status parsing (`StatusMonitor`). Multi-instance coordination, heartbeat sync, external-instance discovery, and display-model population all live in the application-level `VpnRuntimeService` (`app_service/runtime/`); QML binds `VpnRuntimeService`, never `VpnController`.
- `src/core/system_tray`: tray manager and message dispatcher. `SystemTrayManager` is bound to the main window in `main.cpp`; it is not a QML singleton.
- `src/platform`: platform helpers (`AutoStartHelper`, `DaemonRegisterHelper`, `FontHelper`) live here, built into the `qtet_platform` target. There is no `src/core/platform/` directory.

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
