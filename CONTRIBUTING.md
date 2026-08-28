# QtEasyTier 开发者指南

QtEasyTier 是一个 Qt 6.8+ / C++20 / QML 桌面应用。C++ 后端承担主要业务逻辑，QML 只负责界面与绑定。QML 模块 URI 为 `QtEasyTier`，入口在 `src/main.cpp`。

本文档只说明程序架构与设计思路，不逐条复述实现细节；具体原理以源码为准，开始改代码前建议先读对应模块。

## 快速构建与测试

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/Output/appQtEasyTier
```

- 默认构建会同时克隆、构建并收集 `qtet-daemon`。只做前端/离线验证时加 `-DBUILD_WITH_DAEMON=OFF`；后端源码来源由 `-DCLONE_DAEMON_FROM=GITEE`（或 `CNB`，默认 `GITHUB`）指定。
- Windows 仅适配 MinGW64 工具链，不面向 MSVC；且始终只构建前端（跳过 daemon）。
- 无 formatter / linter / 预提交钩子，以 CMake 构建 + CTest 为准。

## 总体架构

### 目录结构

```text
src/
├── main.cpp                         应用入口
├── app/                             应用装配层（composition root）
├── core/                            应用核心业务层
│   ├── config/  credential/  favorite/  logging/
│   ├── runtime/  settings/
│   └── viewmodels/                  ViewModel / Model
├── config/                          配置值模型、TOML、校验、URL codec、daemon 载荷
├── sqlite_repository/               SQLite 持久化
├── daemon_service/                  daemon IPC、JSON-RPC、DaemonApi
├── system_tray/                     系统托盘与托盘消息
├── log/                             日志基础设施
├── platform/                        平台能力（自启动、daemon 注册、字体）
└── qml/                             QML UI
```

### 四层划分

```text
UI 层          QML + ViewModel：界面、交互、绑定
                 ↓
应用核心层      src/core/：应用业务规则与跨模块协调，UI 与基础服务之间的桥接点
                 ↓
基础服务层      src/config log sqlite_repository daemon_service system_tray platform
                 ↑
应用装配层      src/app/：AppServices 创建全部对象并完成跨层信号连线
```

- `src/core/` 是"应用业务核心"，依赖基础服务层；不要误读为无依赖的底层核心。
- `src/app/` 是 composition root，从旁创建并装配对象，不属于普通业务层。
- 依赖方向单向向下：上层可依赖下层，下层不得反向依赖上层。

### 核心约束

- QML 不直接依赖 daemon、repository 或平台实现，一律经 ViewModel / 应用服务访问。
- 基础服务层不得 include `core/` 或 `core/viewmodels/` 的头文件。
- 对象创建与生命周期集中在 `AppServices`；QML singleton 注册集中在 `QmlSingletonRegistrar`。
- `main.cpp` 只做启动流程。

## 设计思路

### 启动与装配

```text
main.cpp → DatabaseConnection → QQmlApplicationEngine → AppServices
         → registerQmlSingletons(engine, services) → load Main.qml → exec()
```

- `AppServices`（`src/app/AppServices.h`）是运行期对象图拥有者，创建 repository、daemon client/api、应用服务、ViewModel、`VpnRuntimeService`、日志 sink、`FontHelper`、`AppState`、`SystemTrayManager` 等，并集中连线跨层信号（如 `wireRuntime()`）。
- `QmlSingletonRegistrar` 是唯一做 QML singleton 注册的地方；预创建对象注册给 QML 时保持 C++ 所有权（`QQmlEngine::setObjectOwnership(..., CppOwnership)`）。
- 启动细节见 `src/main.cpp`、`src/app/AppLaunchManager.*`。

### UI 层：QML + ViewModel

- ViewModel / Model 位于 `src/core/viewmodels/`，与应用服务一起编译进 `qtet_appcore`。
- ViewModel 负责暴露 QML 可绑定属性/信号/槽、转换数据、协调页面动作、调用应用服务；不应拼 daemon payload、写持久化细节、承载平台逻辑或自建静态 singleton。
- 运行状态展示模型 `NodeInfoModel` / `RuntimeLogModel` 属于应用核心层（`src/core/runtime/`），由 `VpnRuntimeService` 持有并填充。
- QML singleton `DangerousOperationViewModel` 是兼容注册名，实际注册对象是应用服务 `DangerousOperationService`。
- 新 QML 文件加入根 `CMakeLists.txt` 的 `qt_add_qml_module(... QML_FILES ...)`。

### 应用核心：src/core

```text
src/core/
├── config/        配置命令、导入导出（ConfigCommandService / ConfigImportExportService）
├── credential/    凭据服务与列表模型
├── favorite/      收藏节点导入导出、FavoriteNodeJsonCodec
├── logging/       RepositoryLogSink（日志落库）
├── runtime/       VpnRuntimeService / VpnController / StatusMonitor / 展示模型
├── settings/      SettingsStore / UpdateCheckService / DangerousOperationService
└── viewmodels/    QML facade 与 Model
```

- `VpnRuntimeService` 是应用级 VPN runtime 协调器，统一管理实例生命周期、心跳同步、外部实例发现与展示模型；QML 通过它访问 VPN 运行能力。
- `VpnController`（单实例状态机）与 `StatusMonitor`（daemon 状态异步解析）是 `VpnRuntimeService` 的私有协作者，不注册给 QML。
- 各服务职责与数据流详见对应目录的 `.h` 文件。

### 基础服务层

| 模块 | 职责 | 关键类型 |
| --- | --- | --- |
| `src/config/` | 配置值模型、TOML、校验、URL codec、daemon 载荷、`ConfigRunState` 枚举 | `NetworkConf`、`ConfigPayloadBuilder` |
| `src/log/` | 日志等级/实体、sink 接口、分发 | `LogHelper`、`LogDispatcher` |
| `src/sqlite_repository/` | SQLite 连接、幂等建表迁移、各仓库 | `DatabaseConnection`、`*Repository` |
| `src/daemon_service/` | daemon 本地 socket IPC、JSON-RPC、帧协议、高层 API | `DaemonClient`、`DaemonApi` |
| `src/system_tray/` | 托盘图标/菜单、窗口显隐、通知、消息分发 | `SystemTrayManager`、`TrayMessageSink` |
| `src/platform/` | 自启动、daemon 注册（UAC/pkexec）、字体 | `AutoStartHelper`、`DaemonRegisterHelper` |

- 日志统一经 `LogHelper::logInfo/logWarning/logError` 写入，`RepositoryLogSink` 落库。
- 上层应通过 `DaemonApi` 调用 daemon，不散落 method name 与 params。
- 自启动状态以系统实际状态为唯一权威源，`settings3.json` 不持久化 `autoStart`（`SettingsViewModel` 直连 `AutoStartHelper` 是有意的架构例外）。
- 测试可用内存 `QLocalServer` 模拟 daemon，不要求真实后端进程。

### 数据与运行时路径

```text
组织名/应用名      qteasytier / QtEasyTier
配置目录          ~/.config/qteasytier/QtEasyTier/
SQLite 数据库      <配置目录>/qteasytier.db
全局设置           <配置目录>/settings3.json
daemon socket     qtet-daemon.sock
公开服务器列表      :/publicservers.json（assets/publicservers.json）
```

## CMake target 组织

```text
appQtEasyTier
    ↓
qtet_appsupport
    ↓
qtet_appcore（应用核心 + ViewModel，依赖全部基础服务）
    ↓
qtet_sqlite_repository / qtet_daemon_service / qtet_platform / qtet_system_tray
    ↓
qtet_config / qtet_log
```

- 各模块 target 由对应源码目录的 `CMakeLists.txt` 定义；`add_qtet_library` 统一设置 include 根（`src`）。
- 新 C++ 文件加入所属模块 target，测试用 `add_core_test` 链接模块 target，不要重复编译生产 `.cpp`。
- daemon 构建/收集逻辑在 `cmake/QtEasyTierDaemon.cmake` 与 `cmake/scripts/*.cmake`，不要堆回根 `CMakeLists.txt`。

## 新增代码放置规则

| 要新增的内容 | 推荐目录 | CMake target |
| --- | --- | --- |
| 配置结构、TOML、校验、URL 编解码 | `src/config/` | `qtet_config` |
| 日志基础设施 | `src/log/` | `qtet_log` |
| SQLite repository | `src/sqlite_repository/` | `qtet_sqlite_repository` |
| daemon IPC / API | `src/daemon_service/` | `qtet_daemon_service` |
| 平台相关实现 | `src/platform/` | `qtet_platform` |
| 系统托盘 / 托盘消息 | `src/system_tray/` | `qtet_system_tray` |
| 应用核心业务 / 收藏编解码 / VPN 状态机 | `src/core/`（对应子目录） | `qtet_appcore` |
| QML ViewModel / Model | `src/core/viewmodels/` | `qtet_appcore` |
| 应用装配 / QML 注册 | `src/app/` | `qtet_appsupport` |
| QML 页面 / 组件 | `src/qml/` | `qt_add_qml_module` 的 `QML_FILES` |
| Qt resource | `assets/` | `assets/resources.qrc` |

放置代码前先自问：

- 这是 UI 表达，还是业务规则？
- 这是应用核心协调，还是底层基础设施？
- 这个类是否需要被 QML 直接使用？
- 这个类是否依赖具体平台、daemon 或 SQLite？

## 架构边界约定

- 不要把业务对象创建逻辑堆回 `main.cpp`。
- 不要给 ViewModel 或 `FontHelper` 重新添加静态 singleton。
- 不要把 QML singleton 注册散落到多个位置。
- 不要让 QML 直接依赖 `DaemonClient` / `DaemonApi` / `VpnController`。
- 不要把 `SystemTrayManager` 注册为 QML singleton。
- 不要重新引入裸 `QVariantList` 风格的 QML API，也不要让 QML 直接绑定 `VpnController`。
- 不要重新添加 `LogHelper::init(...)`。
- 基础服务层不得依赖 ViewModel、QML 或 `core/`。
- 新增跨模块业务操作优先放 `src/core/`，再由 ViewModel 做 QML-friendly 薄壳转发。

### 已知技术债务（新增代码不要模仿、不要扩大）

- `LogViewModel` → `LogRepository`、`FavoriteNodeViewModel` → `FavoriteNodeRepository`、`BackendStatusViewModel` → `DaemonClient`、`ImportNodesViewModel` → `FavoriteNodeJsonCodec` 的直连。
- `FontHelper` 直接注册为 QML singleton（纯 UI 工具类例外）。

## 测试与提交

- 修改后至少运行：

  ```bash
  cmake --build build -j
  ctest --test-dir build --output-on-failure
  ```

- 新增了 CMake target、QML 文件或资源时，建议从干净目录重新配置验证一遍。
- 提交前确认：改动属于哪个层与 target、是否需要更新 QML singleton 注册、是否需要补测试。
