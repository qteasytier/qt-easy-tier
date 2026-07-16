# QtEasyTier 开发者指南

本文档面向参与 QtEasyTier 开发的维护者和贡献者，重点说明当前源码架构、模块边界、对象生命周期、QML 暴露方式、CMake target 组织方式，以及新增代码时应遵守的放置规则。

QtEasyTier 是一个 Qt 6.8+ / C++20 / QML 桌面应用。C++ 后端承担主要业务逻辑，QML 主要负责界面展示与绑定。QML 模块 URI 为 `QtEasyTier`，应用入口位于 `src/main.cpp`。

## 快速构建与测试

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

默认构建会同时构建并收集 `qtet-daemon`。只构建前端或离线验证 CMake 时，可以关闭后端构建：

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DBUILD_WITH_DAEMON=OFF
```

默认从 GitHub 克隆后端源码；如需改用 Gitee，可开启：

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DCLONE_DAEMON_FROM_GITEE=ON
```

运行应用：

```bash
./build/Output/appQtEasyTier
```

运行单个测试示例：

```bash
ctest --test-dir build -R tst_network_conf --output-on-failure
./build/Output/tst_network_conf
```

## 总体架构

当前架构按职责分层，大体结构如下：

```text
src/
├── main.cpp                         应用入口
├── app/                             应用装配层
├── core/
│   ├── config/                      配置数据结构、TOML 序列化、校验、URL 编解码
│   ├── repository/                  SQLite 持久化
│   ├── service/                     daemon IPC 与 daemon API 封装
│   ├── application/                 应用业务服务层
│   ├── viewmodel/                   暴露给 QML 的 ViewModel / Model
│   ├── vpn_manager/                 VPN 生命周期状态机
│   ├── system_tray/                 系统托盘与托盘消息
│   ├── log/                         日志基础设施
│   └── util/                        工具类与平台相关实现
└── qml/                             QML UI
```

推荐从以下四层理解项目：

```text
QML UI
    负责界面、交互、绑定

ViewModel
    为 QML 提供稳定的 C++ facade 和 Qt Model

Application Service
    承接应用业务规则和跨模块协调

Infrastructure
    提供 SQLite、daemon IPC、平台能力、日志、配置序列化等基础设施
```

核心约束：

- QML 不直接依赖底层 daemon、repository 或平台实现。
- ViewModel 面向 QML 提供接口，不应堆积过多底层细节。
- 应用业务逻辑优先放在 `src/core/application/`。
- `main.cpp` 只做启动流程，不承载业务对象装配细节。
- 底层模块不反向依赖 UI 或 ViewModel。

## 启动流程

应用入口是：

```text
src/main.cpp
```

启动流程如下：

```text
src/main.cpp
    ↓
创建 QApplication
    ↓
设置 organizationName / applicationName
    ↓
打开 DatabaseConnection
    ↓
创建 QQmlApplicationEngine
    ↓
创建 AppServices
    ↓
registerQmlSingletons(engine, services)
    ↓
engine.load(QUrl("qrc:/QtEasyTier/Main.qml"))
    ↓
SystemTrayManager 绑定主窗口
    ↓
进入 QApplication::exec()
```

`src/main.cpp` 只负责应用启动壳，不负责创建所有业务对象，也不直接集中书写 QML singleton 注册逻辑。

## 应用装配层

应用装配层位于：

```text
src/app/
├── AppLaunchManager.h
├── AppLaunchManager.cpp
├── AppServices.h
├── AppServices.cpp
├── QmlSingletonRegistrar.h
└── QmlSingletonRegistrar.cpp
```

### AppLaunchManager

`AppLaunchManager` 只放与应用入口相关的轻量启动逻辑，例如判断当前是否来自开机自启动入口、管理前端单实例 socket，避免在 `main.cpp` 中堆叠不可测试的判断。

### AppServices

`AppServices` 是应用运行期对象图的拥有者，也就是 composition root。它负责创建、连接和持有以下对象：

- repository
- daemon client / daemon API
- application service
- ViewModel
- `VpnManager`
- 日志 sink
- `FontHelper`
- `AppState`
- `SystemTrayManager`

它集中管理对象生命周期，避免 ViewModel 自己维护进程级静态 singleton。

典型关系：

```text
main.cpp
    ↓
AppServices
    ↓
Repository / Service / Application Service / ViewModel / VpnManager / SystemTrayManager
```

新增应用级对象时，优先判断它是否应该由 `AppServices` 创建和持有。一般来说，需要贯穿整个应用生命周期、需要暴露给 QML，或需要在多个服务之间共享的对象，适合放入 `AppServices`。

### QmlSingletonRegistrar

`QmlSingletonRegistrar` 专门负责把 `AppServices` 中预创建的 C++ 对象注册成 QML singleton。

典型关系：

```text
AppServices 中的对象
    ↓
QmlSingletonRegistrar
    ↓
qmlRegisterSingletonType
    ↓
QML singleton
```

所有预创建对象注册给 QML 时应保持 C++ 所有权：

```cpp
QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);
```

这样 QML 只使用对象，不拥有对象，避免 QML 引擎析构时误删 C++ 持有的对象。

## QML 层

QML 位于：

```text
src/qml/
├── Main.qml
├── components/
└── pages/
```

QML 模块 URI 固定为：

```text
QtEasyTier
```

入口由 C++ 加载：

```cpp
engine.load(QUrl(QStringLiteral("qrc:/QtEasyTier/Main.qml")));
```

QML 应通过 ViewModel 和 Model 访问后端能力，不应直接依赖底层 daemon client、repository 或平台实现。

当前重要绑定关系：

- `Main.qml` 使用 `BackendStatusViewModel` 观察后端连接状态。
- `NetworkPage.qml` 使用 `NetworkPageViewModel` 协调页面动作。
- `NetworkRuningStatus.qml` 使用 `VpnManager.nodeInfoModel` 和 `VpnManager.runtimeLogModel`。
- `ImportNodesDialog.qml` 使用 `ImportNodesViewModel`。

新增 QML 文件时，需要加入根 `CMakeLists.txt` 中 `qt_add_qml_module(... QML_FILES ...)`。

## ViewModel 层

ViewModel 位于：

```text
src/core/viewmodel/
```

它们是 QML 和 C++ 后端之间的主要 facade。ViewModel 应负责：

- 暴露 QML 可绑定属性、信号和槽。
- 将底层数据转换为 QML 友好的形式。
- 协调少量页面级行为。
- 调用 application service 完成业务操作。

ViewModel 不应负责：

- 直接拼 daemon JSON-RPC payload。
- 直接实现复杂持久化细节。
- 直接承载平台相关逻辑。
- 自己维护静态 singleton 生命周期。

当前主要 ViewModel / Model：

```text
AppState
SettingsViewModel
ConfigListModel
ConfigEditorViewModel
FavoriteNodeViewModel
LogViewModel
BackendStatusViewModel
NetworkPageViewModel
ImportNodesViewModel
NodeInfoModel
RuntimeLogModel
```

### AppState

`AppState` 保存应用级 UI 状态，由 `AppServices` 创建并注册给 QML。

### SettingsViewModel

`SettingsViewModel` 向 QML 暴露设置项，并协调设置修改、自启动状态等能力。

设置持久化和自启动细节不直接放在 `SettingsViewModel` 中，而是通过：

```text
src/core/application/settings/SettingsStore.*
src/core/application/settings/AutoStartService.*
```

典型关系：

```text
QML
    ↓
SettingsViewModel
    ↓
SettingsStore / AutoStartService
```

### ConfigListModel

`ConfigListModel` 暴露配置列表给 QML，并作为配置列表相关操作的入口。

配置命令、导入导出和 daemon payload 构建分别由以下服务承担：

```text
ConfigCommandService
ConfigImportExportService
ConfigPayloadBuilder
DaemonApi
```

典型关系：

```text
QML
    ↓
ConfigListModel
    ↓
ConfigCommandService / ConfigImportExportService
    ↓
Repository / DaemonApi / ConfigPayloadBuilder
```

### ConfigEditorViewModel

`ConfigEditorViewModel` 管理当前正在编辑的网络配置，提供 QML 表单绑定、字段修改、校验和保存入口。

它和 `NetworkPageViewModel` 共用同一个由 `AppServices` 预创建的实例，避免页面层和编辑器层出现两套不一致的编辑状态。

### FavoriteNodeViewModel

`FavoriteNodeViewModel` 专注于用户收藏节点：

- 收藏节点列表模型
- 收藏节点增删改查

公开服务器节点不属于它的职责，公开服务器由 `PublicServerProvider` 提供。

### ImportNodesViewModel

`ImportNodesViewModel` 为导入节点弹窗提供统一列表，它会合并：

- 用户收藏节点
- 内置公开服务器节点

典型关系：

```text
ImportNodesDialog.qml
    ↓
ImportNodesViewModel
    ↓
FavoriteNodeViewModel / PublicServerProvider
```

### BackendStatusViewModel

`BackendStatusViewModel` 暴露 daemon 后端连接状态给 QML，避免 QML 直接依赖 `DaemonClient`。

典型关系：

```text
QML
    ↓
BackendStatusViewModel
    ↓
DaemonClient
```

### NetworkPageViewModel

`NetworkPageViewModel` 是网络页面的页面级 facade，用于协调启动、停止、编辑、导入、刷新等页面动作。

典型关系：

```text
NetworkPage.qml
    ↓
NetworkPageViewModel
    ↓
ConfigListModel / ConfigEditorViewModel / VpnManager
```

### NodeInfoModel 与 RuntimeLogModel

`NodeInfoModel` 和 `RuntimeLogModel` 分别将 VPN 运行时节点信息和运行日志暴露为 `QAbstractListModel`。

QML 应使用：

```qml
VpnManager.nodeInfoModel
VpnManager.runtimeLogModel
```

不要重新引入裸 `QVariantList` 风格的 QML API，例如旧的 `VpnManager.nodeInfos` 或 `VpnManager.logEntries`。

## Application Service 层

应用服务位于：

```text
src/core/application/
```

这一层承接应用业务规则和跨模块协调，位于 ViewModel 与底层基础设施之间。

目录结构：

```text
src/core/application/
├── config/
├── logging/
├── nodes/
├── runtime/
└── settings/
```

### config

路径：

```text
src/core/application/config/
```

主要类型：

```text
ConfigPayloadBuilder
ConfigCommandService
ConfigImportExportService
ConfigOperationResult
```

职责划分：

- `ConfigPayloadBuilder`：将 `NetworkConf` 转换为 daemon 需要的 `cfg_str` payload。
- `ConfigCommandService`：处理启动、停止、删除运行实例等配置命令。
- `ConfigImportExportService`：处理配置导入导出。
- `ConfigOperationResult`：统一表达配置操作结果。

### runtime

路径：

```text
src/core/application/runtime/
```

`ConfigRunState` 是公共运行状态类型，用于统一配置运行状态表达。相关对象应优先使用它表达运行状态，而不是各自定义重复状态枚举。

### settings

路径：

```text
src/core/application/settings/
```

主要类型：

```text
SettingsStore
AutoStartService
```

职责划分：

- `SettingsStore`：读写全局设置文件 `settings3.json`。
- `AutoStartService`：封装自启动启用、禁用、查询。

全局设置不走 SQLite，默认位于：

```text
~/.config/qteasytier/QtEasyTier/settings3.json
```

### nodes

路径：

```text
src/core/application/nodes/
```

主要类型：

```text
PublicServerProvider
```

`PublicServerProvider` 负责读取内置公开服务器列表。默认读取 Qt resource：

```text
:/publicservers.json
```

该资源由以下文件加入 qrc：

```text
assets/resources.qrc
```

### logging

路径：

```text
src/core/application/logging/
```

主要类型：

```text
RepositoryLogSink
```

`RepositoryLogSink` 将 `LogDispatcher` 分发出来的日志写入 `LogRepository`。

日志链路：

```text
LogHelper
    ↓
LogDispatcher
    ↓
RepositoryLogSink
    ↓
LogRepository
    ↓
SQLite
```

## config 基础模块

路径：

```text
src/core/config/
```

职责：

- `NetworkConf` 数据结构。
- TOML 序列化和反序列化。
- 配置校验。
- URL 编解码（`ConfigUrlCodec`）。

这一层是配置领域基础模块，不应依赖 UI、ViewModel 或 VPN manager。

## repository 持久化层

路径：

```text
src/core/repository/
```

职责：

- SQLite 连接。
- 配置持久化。
- 收藏节点持久化。
- 日志持久化。
- 幂等建表和迁移。

`DatabaseConnection::open()` 负责打开数据库并执行幂等迁移建表。

默认数据库位置：

```text
~/.config/qteasytier/QtEasyTier/qteasy-tier-configs.db
```

repository 层不应依赖 QML，也不应包含页面逻辑。

## service 层

路径：

```text
src/core/service/
```

职责：

- `qtet-daemon` IPC。
- JSON-RPC。
- 自定义帧协议。
- daemon 高层 API 封装。

主要对象：

```text
DaemonClient
DaemonApi
```

`DaemonClient` 负责底层连接、请求发送和响应接收。

`DaemonApi` 负责把具体 daemon 方法封装为明确的 C++ 方法，避免 method name 和 params magic string 散落到 ViewModel 或 application service 中。

典型关系：

```text
ConfigCommandService / VpnManager / BackendStatusViewModel
    ↓
DaemonApi 或 DaemonClient
    ↓
qtet-daemon.sock
    ↓
qtet-daemon
```

测试中通常通过内存 `QLocalServer` 模拟 daemon，不需要真实 daemon 后台进程。

## vpn_manager 层

路径：

```text
src/core/vpn_manager/
```

职责：

- VPN 生命周期状态机。
- 启动和停止流程协调。
- 运行状态刷新。
- 节点信息刷新。
- 运行日志刷新。

主要对象：

```text
VpnManager
VpnController
StatusMonitor
```

### VpnManager

`VpnManager` 是暴露给 QML 的 VPN 运行控制入口之一。

它对 QML 暴露结构化 model：

```text
nodeInfoModel
runtimeLogModel
```

不要重新添加旧的 QML 兼容属性：

```text
nodeInfos
logEntries
```

### VpnController

`VpnController` 偏内部状态机和运行时数据缓存。它可以保留内部 `nodeInfos()` / `logEntries()` 缓存，供 `VpnManager` 刷新 `NodeInfoModel` / `RuntimeLogModel` 或导出日志使用，但不应直接暴露给 QML。

典型关系：

```text
VpnController 内部运行时缓存
    ↓
VpnManager
    ↓
NodeInfoModel / RuntimeLogModel
    ↓
QML
```

## system_tray 层

路径：

```text
src/core/system_tray/
```

职责：

- 系统托盘图标、右键菜单和窗口显隐行为。
- 真实系统通知输出。
- 统一的托盘消息类型与分发接口。

主要对象：

```text
SystemTrayManager
TrayMessageDispatcher
TrayMessageHelper
TrayMessageSink
```

`SystemTrayManager` 在 `main.cpp` 中绑定主窗口，并控制关闭到托盘行为。它由 `AppServices` 创建和持有，但不注册为 QML singleton。上层代码通过 `TrayMessageSink` 接口输出托盘消息，避免直接依赖 `SystemTrayManager`。

## log 基础模块

路径：

```text
src/core/log/
```

主要类型：

```text
LogTypes
LogSink
LogDispatcher
```

职责：

- 定义日志等级和日志实体。
- 定义日志 sink 接口。
- 分发日志到多个 sink。

`LogDispatcher::instance()` 当前保留。它是日志基础设施的全局 dispatcher，不属于 ViewModel singleton。

业务代码应通过 `LogHelper` 写日志：

```cpp
LogHelper::logInfo(...);
LogHelper::logWarning(...);
LogHelper::logError(...);
```

`LogHelper::init(...)` 已删除，不应重新引入。

## util 工具模块

路径：

```text
src/core/util/
```

当前主要包含：

```text
LogHelper
FontHelper
AutoStartHelper
DaemonRegisterHelper
```

`FontHelper` 由 `AppServices` 显式构造并注册给 QML，不应恢复为静态 singleton。

`DaemonRegisterHelper` 负责 daemon 系统服务注册/启动相关平台操作，被 `AppServices` 在 daemon 断连时调用。

## platform 层

虽然源码目录 `src/core/platform/` 当前不存在，但 `qtet_platform` target 仍作为一个概念层存在，实际源码放在 `src/core/util/` 下，目前包含：

```text
AutoStartHelper
DaemonRegisterHelper
FontHelper
```

职责是封装平台相关能力。上层通常应通过 application service 使用这些能力，例如通过 `AutoStartService` 使用自启动能力，而不是让 QML 或 ViewModel 直接依赖具体平台实现。

## 资源文件

资源文件入口：

```text
assets/resources.qrc
```

公开服务器列表：

```text
assets/publicservers.json
```

运行时读取路径：

```text
:/publicservers.json
```

新增资源时，需要加入 `assets/resources.qrc`。

## CMake target 组织

根 `CMakeLists.txt` 负责全局配置、应用 target、QML 模块和后端构建开关；以下模块 target 分别由对应源码目录的 `CMakeLists.txt` 定义：

```text
qtet_config
qtet_log
qtet_repository
qtet_service
qtet_platform
qtet_system_tray
qtet_application
qtet_vpn
qtet_viewmodel
qtet_appsupport
```

总体依赖方向：

```text
appQtEasyTier
    ↓
qtet_appsupport
    ↓
qtet_viewmodel
    ↓
qtet_application / qtet_vpn
    ↓
qtet_repository / qtet_service / qtet_platform
    ↓
qtet_config / qtet_log
```

`qtet_system_tray` 由 `qtet_appsupport` 直接链接，与 `qtet_viewmodel` 处于同一层级，依赖 `qtet_service` 和 `qtet_log`。

原则：

- 上层可以依赖下层。
- 下层不要反向依赖上层。
- `appQtEasyTier` 链接 `qtet_appsupport`，不要重新聚合生产 `.cpp`。
- 新 C++ 源文件应加入所属模块 target，而不是随意加入应用 target。
- 默认 `BUILD_WITH_DAEMON=ON` 会构建并收集 `qtet-daemon`；传入 `-DBUILD_WITH_DAEMON=OFF` 时跳过后端构建 target 和 post-build 收集步骤；传入 `-DCLONE_DAEMON_FROM_GITEE=ON` 时会给 `scripts/build_daemon.sh` 追加 `--gitee`，从 Gitee 克隆后端源码。

各 target 大致职责：

| Target | 职责 |
| --- | --- |
| `qtet_config` | 配置数据结构、TOML 序列化、校验、URL 编解码 |
| `qtet_log` | 日志基础类型、日志分发、日志工具入口 |
| `qtet_repository` | SQLite repository 和数据库连接 |
| `qtet_service` | daemon IPC 和 daemon API |
| `qtet_platform` | 平台相关实现（源码在 `src/core/util/`） |
| `qtet_system_tray` | 系统托盘、托盘消息、真实通知输出 |
| `qtet_application` | 应用业务服务层 |
| `qtet_vpn` | VPN 生命周期和运行状态管理 |
| `qtet_viewmodel` | 暴露给 QML 的 ViewModel / Model |
| `qtet_appsupport` | AppServices 和 QML singleton 注册 |

## 测试组织

测试位于：

```text
tests/
```

测试目标应链接对应模块 target，不要在测试中重复列生产 `.cpp`。

当前主要测试目标包括：

```text
tst_network_conf
tst_config_url_codec
tst_sqlite_repository
tst_config_list_model
tst_favorite_node_repository
tst_daemon_client
tst_daemon_register_helper
tst_autostart_helper
tst_settings_store
tst_autostart_service
tst_public_server_provider
tst_import_nodes_viewmodel
tst_app_services
tst_log_repository
tst_log_helper
tst_tray_message_dispatcher
tst_system_tray_manager
tst_app_launch_manager
```

新增生产代码时，应优先为对应模块补充测试。涉及 daemon 的测试可以使用内存 `QLocalServer` 模拟，不要求真实 daemon 后台。

## 常见开发场景

### 新增配置字段

通常需要检查或修改：

```text
src/core/config/
src/core/viewmodel/ConfigEditorViewModel.*
src/core/application/config/ConfigPayloadBuilder.*
src/core/repository/
tests/
```

如果字段需要进入 TOML、SQLite 或 daemon payload，应分别补齐序列化、持久化和 payload 构建测试。

### 新增 daemon API 调用

优先修改：

```text
src/core/service/DaemonApi.*
```

调用方应通过 `DaemonApi` 使用，不要在 ViewModel 或 QML-facing 类中散落 method name 和 JSON params。

### 新增应用业务操作

优先放在：

```text
src/core/application/
```

如果该操作需要暴露给 QML，再由对应 ViewModel 包一层 QML-friendly API。

### 新增 QML 可绑定数据列表

优先使用 `QAbstractListModel` 风格的 model，放在：

```text
src/core/viewmodel/
```

避免直接暴露复杂 `QVariantList` 作为长期 API。

### 新增页面级协调逻辑

优先放在页面级 ViewModel，例如：

```text
src/core/viewmodel/runtime/NetworkPageViewModel.*
```

不要把复杂协调逻辑堆在 QML 文件中。

### 新增资源

加入：

```text
assets/resources.qrc
```

运行时通过 `:/...` 访问。

### 新增 QML 文件

文件放入：

```text
src/qml/components/
src/qml/pages/
```

并加入根 `CMakeLists.txt` 的 `qt_add_qml_module(... QML_FILES ...)`。

## 新增代码放置规则

按职责选择目录和 target：

| 要新增的内容 | 推荐目录 | CMake target |
| --- | --- | --- |
| 配置结构、TOML、校验、URL 编解码 | `src/core/config/` | `qtet_config` |
| 日志基础设施 | `src/core/log/` 或 `src/core/util/LogHelper.*` | `qtet_log` |
| SQLite repository | `src/core/repository/` | `qtet_repository` |
| daemon IPC / API | `src/core/service/` | `qtet_service` |
| 平台相关实现 | `src/core/util/` | `qtet_platform` |
| 应用业务服务 | `src/core/application/` | `qtet_application` |
| VPN 状态机 / 运行状态 | `src/core/vpn_manager/` | `qtet_vpn` |
| 系统托盘 / 托盘消息 | `src/core/system_tray/` | `qtet_system_tray` |
| QML ViewModel / Model | `src/core/viewmodel/` | `qtet_viewmodel` |
| 应用装配 / QML 注册 | `src/app/` | `qtet_appsupport` |
| QML 页面 / 组件 | `src/qml/` | `qt_add_qml_module` 的 `QML_FILES` |
| Qt resource | `assets/` | `assets/resources.qrc` |

如果不确定代码该放哪里，优先问以下问题：

- 这是 UI 表达，还是业务规则？
- 这是应用业务协调，还是底层基础设施？
- 这个类是否需要被 QML 直接使用？
- 这个类是否需要贯穿整个应用生命周期？
- 这个类是否依赖具体平台、daemon 或 SQLite？

## 架构边界约定

请遵守以下约定：

- 不要把业务对象创建逻辑重新堆回 `main.cpp`。
- 不要给 ViewModel 或 `FontHelper` 重新添加静态 singleton。
- 不要把 QML singleton 注册散落到多个位置。
- 不要让 QML 直接依赖 `DaemonClient`。
- 不要把 `SystemTrayManager` 注册为 QML singleton；托盘行为由 `AppServices` 和 `main.cpp` 协作完成。
- 不要重新添加 `VpnManager.nodeInfos` / `VpnManager.logEntries` 这类旧兼容属性。
- 不要重新添加 `LogHelper::init(...)`。
- 不要在测试 target 中重复编译大量生产 `.cpp`，应链接模块 target。
- 不要让底层模块依赖 ViewModel 或 QML。
- 不要绕过 `DaemonApi` 在上层散落 daemon method name。

## 数据与运行时路径

应用固定设置：

```text
organizationName = qteasytier
applicationName = QtEasyTier
```

Linux 下 `AppConfigLocation` 通常为：

```text
~/.config/qteasytier/QtEasyTier/
```

SQLite 数据库默认路径：

```text
~/.config/qteasytier/QtEasyTier/qteasy-tier-configs.db
```

全局设置文件：

```text
~/.config/qteasytier/QtEasyTier/settings3.json
```

daemon socket：

```text
qtet-daemon.sock
```

公开服务器资源：

```text
:/publicservers.json
```

## 开发前检查清单

修改代码前，建议先确认：

- 这项改动属于哪个层次和哪个 CMake target。
- 是否已有对应 application service 可以复用。
- 是否需要新增或更新 ViewModel API。
- 是否影响 QML singleton 注册。
- 是否需要更新 `assets/resources.qrc` 或 `qt_add_qml_module(... QML_FILES ...)`。
- 是否需要为 daemon API 补充 `DaemonApi` 方法。
- 是否需要补充 repository migration 或测试。
- 是否影响系统托盘行为（新增/修改 tray 消息、窗口关闭逻辑等）。

提交前，至少运行：

```bash
cmake --build build -j
ctest --test-dir build --output-on-failure
```

如果新增了 CMake target、QML 文件或资源文件，建议从干净目录重新配置验证：

```bash
cmake -B build-check -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build-check -j
ctest --test-dir build-check --output-on-failure
```

## 维护目标

当前架构的维护目标是：

- 保持 `main.cpp` 简洁。
- 保持对象生命周期集中在 `AppServices`。
- 保持 QML 注册集中在 `QmlSingletonRegistrar`。
- 保持 QML 通过 ViewModel / Model 访问后端。
- 保持业务逻辑沉淀在 application service。
- 保持底层基础设施模块可测试、可复用。
- 保持 CMake target 与源码目录职责一致。
- 保持测试通过模块 target 链接生产代码。
- 保持系统托盘行为由 `SystemTrayManager` 集中管理，不直接散落在 QML 或 ViewModel 中。

如果一项改动会破坏这些目标，应优先重新审视设计，而不是为了短期方便绕过架构边界。
