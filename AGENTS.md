# QtEasyTier 代理开发速查

Qt 6.8+ / C++20 / QML 桌面应用；C++ 承担业务和持久化，QML 主要做展示与绑定。QML 模块 URI 是 `QtEasyTier`，入口在 `src/main.cpp`，由 `engine.load(QUrl(QStringLiteral("qrc:/QtEasyTier/Main.qml")))` 加载。

## 构建、运行、验证

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/Output/appQtEasyTier
```

- 所有可执行文件和库输出到 `${CMAKE_BINARY_DIR}/Output`，不是 `build/` 根目录。
- 单测可聚焦运行：`ctest --test-dir build -R tst_network_conf --output-on-failure` 或 `./build/Output/tst_network_conf`。
- 仓库未发现 CI workflow、formatter、linter、pre-commit 或 task runner 配置；完成代码改动时以 CMake 构建和 CTest 为主要验证。
- 根 `CMakeLists.txt` 会在存在 `importedcontent/CMakeLists.txt` 时自动 `add_subdirectory(importedcontent)`；这是 Figma/Qt 导入内容的可选入口。
- 默认构建需要 `git` 和网络：CMake 会自动调用 `scripts/build_daemon.sh` 从 GitHub 克隆并编译 `qtet-daemon`，然后由 `scripts/collect-daemon.sh` 把产物复制到 `Output/`。如需禁用后端构建，配置时传入 `-DBUILD_WITH_DAEMON=OFF`；如需从 Gitee 克隆后端，传入 `-DCLONE_DAEMON_FROM_GITEE=ON`；如果无网络且仍开启该选项，需手动把 `qtet-daemon` 放到输出目录。

## CMake 与新增文件

- 根 `CMakeLists.txt` 负责全局配置、app target、QML 模块和 daemon 构建开关；生产模块 target 拆分到对应源码目录的 `CMakeLists.txt`。
- `appQtEasyTier` 只编译 `src/main.cpp` 和 `assets/resources.qrc`，并链接 `qtet_appsupport`；不要把生产 `.cpp` 重新堆到 app target。
- 新 C++ 源文件加入所属模块 target；测试 target 链接模块 target，不要在测试里重复列生产 `.cpp`。
- 新 QML 文件加入根 `qt_add_qml_module(appQtEasyTier ... QML_FILES ...)`；新资源加入 `assets/resources.qrc`。
- Qt 组件以根 `CMakeLists.txt` 为准：Core、Sql、Network、Test、Quick、Widgets、Concurrent、Svg、QuickDialogs2。

## 代码边界

- `src/app` 是装配层：`AppLaunchManager` 解析启动参数并管理前端单实例 socket，`AppServices` 创建并持有服务/ViewModel，`QmlSingletonRegistrar` 集中 `qmlRegisterSingletonType`。
- `src/core/config`：`NetworkConf`、TOML 序列化、校验、URL 编解码。
- `src/core/repository`：SQLite repository；`DatabaseConnection::open()` 负责幂等建表/迁移。
- `src/core/service`：`qtet-daemon` IPC、JSON-RPC、自定义帧协议和 `DaemonApi`。
- `src/core/application`：配置命令/导入导出、日志 sink、设置存储、自启动、公共服务器 provider、运行状态类型。
- `src/core/viewmodel`：注册给 QML 的 facade 和 Qt model；QML 不应直接依赖 repository、daemon client 或平台实现。
- `src/core/vpn_manager`：VPN 生命周期与运行状态；`VpnManager` 暴露 `nodeInfoModel`、`runtimeLogModel`，内部由 `VpnController`/`StatusMonitor` 维护缓存与刷新。
- `src/core/system_tray`：系统托盘管理器、托盘消息分发与类型；`SystemTrayManager` 在 `main.cpp` 绑定主窗口并控制关闭到托盘行为，不注册为 QML singleton。
- `src/core/util` 放 `AutoStartHelper`、`DaemonRegisterHelper`、`FontHelper`、`LogHelper` 等工具；当前没有 `src/core/platform/` 目录，`qtet_platform` target 编译的是 `src/core/util/` 下的平台相关实现。

## QML 与对象生命周期

- `AppServices services(db.database(), &engine)` 以 `QQmlApplicationEngine` 作为服务对象父级；所有预创建 QML singleton 通过 `QQmlEngine::setObjectOwnership(..., CppOwnership)` 保持 C++ 所有权。不要改成 `QApplication` 父对象或 `setContextProperty`。
- C++ 暴露给 QML 统一走 `QmlSingletonRegistrar.cpp` 的 `qmlRegisterSingletonType`；不要把注册逻辑散落到 `main.cpp`。
- `Card.qml` 的间距属性叫 `contentSpacing`，不要改成 `spacing`，因为 Qt 6.7+ `Frame.spacing` 是 FINAL。
- 通用 UI 颜色优先用 `palette.xxx`；状态色从 `Theme.qml` 的 `statusGreen/statusOrange/statusRed/statusBlue` 取。
- `PageContainer.qml` 通过 `State` + `Transition` 做 opacity/y 切换，并用 `visible: opacity > 0.01` 防止隐藏页面接收事件。

## 数据、资源、运行时

- `src/main.cpp` 固定 `organizationName = qteasytier`、`applicationName = QtEasyTier`；Linux 下 `AppConfigLocation` 通常为 `~/.config/qteasytier/QtEasyTier/`。
- 默认 SQLite 数据库：`~/.config/qteasytier/QtEasyTier/qteasytier.db`。
- 全局设置走 `SettingsStore`/`SettingsViewModel` 的 `settings3.json`，不走 SQLite。
- 应用默认连接 Unix socket `qtet-daemon.sock`；`tst_daemon_client` 使用内存 `QLocalServer` 模拟 daemon，不需要真实后台。
- `assets/publicservers.json` 通过 `assets/resources.qrc` 进入 `:/publicservers.json`，由 `PublicServerProvider` 读取。
- 系统托盘可用时应用默认关闭到托盘（`QApplication::setQuitOnLastWindowClosed(false)`）；自启动且托盘可用时启动不闪现主窗口。

## 测试清单

- 测试注册在 `tests/CMakeLists.txt`，当前目标：`tst_network_conf`、`tst_config_url_codec`、`tst_sqlite_repository`、`tst_config_list_model`、`tst_favorite_node_repository`、`tst_daemon_client`、`tst_daemon_register_helper`、`tst_autostart_helper`、`tst_settings_store`、`tst_autostart_service`、`tst_public_server_provider`、`tst_import_nodes_viewmodel`、`tst_app_services`、`tst_log_repository`、`tst_log_helper`、`tst_tray_message_dispatcher`、`tst_system_tray_manager`、`tst_app_launch_manager`。
- 新测试使用 `add_core_test(name file.cpp)`，再按职责链接 `qtet_*` 模块 target。

## 仓库杂项

- `.gitignore` 忽略 `/build*/`、`/cmake-build*/`、`/.qtcreator/`、`/.idea/`、`/.opencode/`、`/.worktrees/`、`/example/`、`/docs/` 和运行期 `configs*` 数据库文件；不要把这些作为源码提交。
- `CONTRIBUTING.md` 有更长的架构说明；本文件只保留未来 agent 容易遗漏的执行约束。
