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

默认从 GitHub 克隆后端源码；如需改用其他来源，可通过 `CLONE_DAEMON_FROM` 指定（`GITEE` / `GITHUB` / `CNB`）：

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DCLONE_DAEMON_FROM=GITEE
```

Windows 当前仅适配 MinGW64 构建，不面向 MSVC / Visual Studio 生成器做兼容。Windows 下 `qtet-daemon` 尚未适配，CMake 会跳过后端构建和收集，当前只构建前端：

```powershell
cmake -B build-win -S . -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DBUILD_WITH_DAEMON=OFF
cmake --build build-win
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
├── app_service/                     应用业务服务层
├── core/                            基础服务层
│   ├── config/                      配置数据结构、TOML 序列化、校验、URL 编解码
│   ├── sqlite_repository/           SQLite 持久化
│   ├── daemon_service/              daemon IPC 与 daemon API 封装
│   ├── system_tray/                 系统托盘与托盘消息
│   └── log/                         日志基础设施
├── platform/                        平台相关实现（自启动、daemon 注册、字体）
├── viewmodels/                      暴露给 QML 的 ViewModel / Model（与 app_service 同属 qtet_application target）
└── qml/                             QML UI
```

推荐从以下四层理解项目：

```text
UI 层
    QML + ViewModel
    负责界面、交互、绑定，为 QML 提供稳定的 C++ facade 和 Qt Model

应用服务层
    src/app_service/
    承接应用业务规则和跨模块协调，是 UI 层访问基础服务层的桥接点；
    其中 VpnRuntimeService 是应用级 VPN runtime 协调器，统一管理实例
    生命周期、心跳同步与运行状态展示模型

基础服务层
    src/core/config/、sqlite_repository/、daemon_service/、system_tray/、platform/、log/
    提供 SQLite、daemon IPC、平台能力、日志、配置序列化等基础设施

应用装配层
    src/app/
    AppServices 作为 composition root 创建全部对象并完成跨层信号连线
```

核心约束：

- QML 不直接依赖底层 daemon、repository 或平台实现。
- UI 层（QML + ViewModel）与基础服务层之间通过应用服务层桥接：
  应用服务从基础服务层获取信息并暴露给 UI 层，ViewModel 的构造依赖中不应
  出现 repository / DaemonClient / 平台工具等基础服务类型。
- 基础服务层不得 include `app_service` 或 `viewmodels` 的头文件。
- 跨层信号连线统一放在 `AppServices::wireRuntime()` 等装配函数中。
- 允许的例外：纯数据模型与工具类（如 `NetworkConf`、`ConfigRunState`、
  `FontHelper`）不强制过桥；日志/收藏/后端状态等薄壳 ViewModel 的历史直连
  属于已知技术债务，见"架构边界约定"。
- `main.cpp` 只做启动流程，不承载业务对象装配细节。

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
- `VpnRuntimeService`
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
Repository / Service / Application Service / ViewModel / VpnRuntimeService / SystemTrayManager
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
- `NetworkRuningStatus.qml` 使用 `VpnRuntimeService.nodeInfoModel` 和 `VpnRuntimeService.runtimeLogModel`。
- `ImportNodesDialog.qml` 使用 `ImportNodesViewModel`。

QML 通过 QmlSingletonRegistrar 注册的 singleton 访问能力。`VpnRuntimeService`
（应用服务层）是 QML 访问 VPN 运行能力的唯一入口；单实例状态机
`VpnController` 不注册为 QML singleton。

新增 QML 文件时，需要加入根 `CMakeLists.txt` 中 `qt_add_qml_module(... QML_FILES ...)`。

## ViewModel 层

ViewModel 位于：

```text
src/viewmodels/
```

ViewModel 和 `src/app_service/` 中的应用业务服务一起编译进同一个构建目标
`qtet_application`（二者不再拆分为独立物理 target）。

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
```

说明：

- `NodeInfoModel` / `RuntimeLogModel` 属于运行状态展示模型，位于应用服务层
  （`src/app_service/runtime/`），由 `VpnRuntimeService` 持有并填充，不属于 ViewModel 层。
- QML singleton `DangerousOperationViewModel` 是兼容注册名称，实际注册的是应用服务
  `DangerousOperationService`，不再对应独立的 C++ ViewModel 类型（见下文危险操作章节）。

### AppState

`AppState` 保存应用级 UI 状态，由 `AppServices` 创建并注册给 QML。

### SettingsViewModel

`SettingsViewModel` 向 QML 暴露设置项，并协调本地设置、自动回连、版本更新检查与自启动状态。

它直接持有自动回连 / 更新检查的异步状态，并通过注入的协作者完成操作：

```text
DaemonApi（自动回连 RPC）
UpdateCheckService（版本更新检查）
SettingsStore（settings3.json）
AutoStartHelper（系统自启动）
```

其中 `DaemonApi`、`UpdateCheckService`、`SettingsStore`、`AutoStartHelper` 均为非所有权依赖，
由 `AppServices` 创建并注入；`SettingsViewModel` 只保存指针/值，不构造底层对象。

开机自启动以系统实际状态为唯一权威源：`SettingsViewModel` 直接调用平台层
`AutoStartHelper` 读写 Windows 注册表 / XDG Autostart 条目，
`settings3.json` 不持久化 `autoStart` 字段。这是有意的架构例外。

典型关系：

```text
QML
    ↓
SettingsViewModel
    ↓
DaemonApi / UpdateCheckService / SettingsStore / AutoStartHelper（仅自启动）
```

### ConfigListModel

`ConfigListModel` 暴露配置列表给 QML，并作为配置列表相关操作的入口。

配置读写、导入导出分别由以下应用服务承担：

```text
ConfigCommandService
ConfigImportExportService
```

`ConfigListModel` / `ConfigEditorViewModel` 只依赖应用服务层，不直接持有 repository 或 DaemonApi。

典型关系：

```text
QML
    ↓
ConfigListModel
    ↓
ConfigCommandService / ConfigImportExportService
    ↓
NetworkConfigRepository / DaemonApi / ConfigPayloadBuilder
```

### ConfigEditorViewModel

`ConfigEditorViewModel` 管理当前正在编辑的网络配置，提供 QML 表单绑定、字段修改、校验和保存入口。

它和 `NetworkPageViewModel` 共用同一个由 `AppServices` 预创建的实例，避免页面层和编辑器层出现两套不一致的编辑状态。

配置的加载与保存通过 `ConfigCommandService` 完成（`load` / `loadAll` / `save`），不直接访问 repository。

### FavoriteNodeViewModel

`FavoriteNodeViewModel` 专注于用户收藏节点：

- 收藏节点列表模型
- 收藏节点增删改查
- 收藏节点批量导入导出

公开服务器节点使用同一套收藏节点 JSON 格式，由 `FavoriteNodeJsonCodec` 解析。

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
FavoriteNodeViewModel / FavoriteNodeJsonCodec
```

当前实现中公共节点 JSON 解析由 `FavoriteNodeJsonCodec` 提供，字段为 `uri`、`display_name` 和 `publicKey`。

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

所有 VPN 启停与状态查询通过应用服务层 `VpnRuntimeService` 完成，不使用反射调用。

典型关系：

```text
NetworkPage.qml
    ↓
NetworkPageViewModel
    ↓
ConfigListModel / ConfigEditorViewModel / VpnRuntimeService
```

### DangerousOperationService

`DangerousOperationService` 是设置页「危险操作」卡片的 QML-facing 应用服务，
提供后端安装/卸载与清空全部数据的完整编排，并直接以 QML 属性、方法、信号暴露。

QML singleton 名称为 `DangerousOperationViewModel`，这只是为兼容既有 QML 保留的
注册名称，实际注册对象是 `DangerousOperationService`，不存在独立的 C++ ViewModel 类型。

典型关系：

```text
SettingsPage.qml（QML 名称 DangerousOperationViewModel）
    ↓
DangerousOperationService
    ↓
VpnRuntimeService / 各 Repository / DaemonRegisterHelper
```

### NodeInfoModel 与 RuntimeLogModel

`NodeInfoModel` 和 `RuntimeLogModel` 分别将 VPN 运行时节点信息和运行日志暴露为 `QAbstractListModel`。

它们位于应用服务层 `src/app_service/runtime/`，由 `VpnRuntimeService` 持有并填充：
`VpnRuntimeService` 将 StatusMonitor 解析的节点/日志数据写入对应 `VpnController` 缓存，
再填充当前查看实例的展示模型，基础服务层不接触任何 UI 类型。

QML 应使用：

```qml
VpnRuntimeService.nodeInfoModel
VpnRuntimeService.runtimeLogModel
```

不要重新引入裸 `QVariantList` 风格的 QML API，也不要让 QML 直接绑定 `VpnController`。

## Application Service 层

应用服务位于：

```text
src/app_service/
```

这一层承接应用业务规则和跨模块协调，是 UI 层（QML + ViewModel）与基础服务层之间的桥接点。

目录结构：

```text
src/app_service/
├── config/        配置读写、导入导出、命令服务
├── dangerous/     危险操作编排（后端安装/卸载、清空数据）
├── favorite/      收藏节点导入导出
├── logging/       日志落库 sink
├── runtime/       VPN 运行服务与运行状态展示模型
└── settings/      设置持久化、自启动、后端设置桥接
```

### config

路径：

```text
src/app_service/config/
```

主要类型：

```text
ConfigCommandService
ConfigImportExportService
ConfigOperationResult
```

职责划分：

- `ConfigCommandService`：配置的读写（`load` / `loadAll` / `save`）与增删改命令。
- `ConfigImportExportService`：配置的文件/URL 导入导出与 daemon 校验。
- `ConfigOperationResult`：统一表达配置操作结果。

daemon 载荷构建（`ConfigPayloadBuilder`）位于基础服务层 `src/core/config/`。

### runtime

路径：

```text
src/app_service/runtime/
```

主要类型：

```text
VpnRuntimeService
NodeInfoModel
RuntimeLogModel
```

职责划分：

- `VpnRuntimeService`：应用级 VPN runtime 协调器，统一管理实例生命周期、心跳同步、
  外部实例发现、stopAll 收敛与当前查看实例；持有并填充节点信息/运行日志展示模型，
  转发启停、状态查询、日志导出等操作。QML 通过它访问 VPN 运行能力。
- `NodeInfoModel` / `RuntimeLogModel`：运行状态展示模型，数据由 `VpnRuntimeService` 注入。

### settings

路径：

```text
src/app_service/settings/
```

主要类型：

```text
SettingsStore
UpdateCheckService
```

职责划分：

- `SettingsStore`：读写全局设置文件 `settings3.json`（不含自启动字段）。
- `UpdateCheckService`：执行版本更新检查（HTTP、版本比较与更新对话框）。

自动回连与更新检查的异步状态由 `SettingsViewModel`（`src/viewmodels`）直接持有和协调：
它经注入的 `DaemonApi` 发起自动回连 RPC，并监听 `UpdateCheckService` 终态信号收敛忙状态。

开机自启动不在此目录：`SettingsViewModel` 直接调用平台层 `AutoStartHelper`，
以系统实际状态为唯一权威源，`settings3.json` 不持久化该字段。

全局设置不走 SQLite，默认位于：

```text
~/.config/qteasytier/QtEasyTier/settings3.json
```

### dangerous

路径：

```text
src/app_service/dangerous/
```

主要类型：

```text
DangerousOperationService
```

`DangerousOperationService` 编排后端安装/卸载（`DaemonRegisterHelper`）与清空全部数据
（`VpnRuntimeService.stopAll` → 各仓库清库 → 设置文件重置 → 关闭系统自启动）流程，
直接以 QML 注册名 `DangerousOperationViewModel` 暴露给设置页。

### favorite

路径：

```text
src/app_service/favorite/
```

主要类型：

```text
FavoriteNodeImportExportService
FavoriteNodeJsonCodec
```

收藏节点值类型 `FavoriteNode` 作为 SQLite repository 的记录类型位于：

```text
src/core/sqlite_repository/FavoriteNode.h
```

`FavoriteNodeJsonCodec` 负责读取和写出收藏节点 JSON 格式。内置公开服务器列表默认读取 Qt resource：

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
src/app_service/logging/
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
- daemon 载荷构建（`ConfigPayloadBuilder`）：将 `NetworkConf` 序列化为 `cfg_str` JSON payload，
  供 `VpnController` 启动实例与 `ConfigImportExportService` 导入校验使用。
- 公共运行状态枚举（`ConfigRunState`）：统一配置运行状态表达，
  供 runtime、system_tray、viewmodel 等各层共用。

这一层是配置领域基础模块，不应依赖 UI、ViewModel 或 VPN runtime。

## sqlite_repository 持久化层

路径：

```text
src/core/sqlite_repository/
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
~/.config/qteasytier/QtEasyTier/qteasytier.db
```

repository 层不应依赖 QML，也不应包含页面逻辑。

## daemon_service 层

路径：

```text
src/core/daemon_service/
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
ConfigCommandService / VpnRuntimeService
    ↓
DaemonApi 或 DaemonClient
    ↓
qtet-daemon.sock
    ↓
qtet-daemon
```

UI 层通过应用服务层访问 daemon 能力（如 `SettingsViewModel` 经注入的 `DaemonApi` 封装自动回连、`VpnRuntimeService` 封装运行控制），
不直接接触 `DaemonClient` / `DaemonApi`。已知债务：`BackendStatusViewModel` 仍直接依赖 `DaemonClient`（见架构边界约定）。

测试中通常通过内存 `QLocalServer` 模拟 daemon，不需要真实 daemon 后台进程。

## runtime 应用服务层

`VpnController`、`StatusMonitor` 与 `VpnRuntimeService` 一起位于：

```text
src/app_service/runtime/
```

职责：

- 单实例生命周期状态机（`VpnController`）。
- 启动和停止流程协调（单实例）。
- daemon 状态数据异步解析（`StatusMonitor`）。
- 多实例协调、心跳同步、外部实例发现与展示模型填充（`VpnRuntimeService`）。

主要对象：

```text
VpnController
StatusMonitor
VpnRuntimeService
```

`VpnController` 和 `StatusMonitor` 不再是独立物理 target，作为 `qtet_application`
内部协作者与应用 runtime 服务编译在一起。

### VpnController

`VpnController` 是单实例状态机与运行时数据缓存，不接触任何 UI 类型：
它保留内部 `nodeInfos()` / `logEntries()` 缓存，供 `VpnRuntimeService` 查询或导出日志使用，但不应直接暴露给 QML。

### StatusMonitor

`StatusMonitor` 将 daemon `collect_network_infos` 返回的 JSON 在后台线程解码解析，
通过 `instanceInfoParsed` 信号通知 `VpnRuntimeService` 写入对应 controller 缓存。

典型关系：

```text
VpnController 内部运行时缓存
    ↓
VpnRuntimeService（填充 NodeInfoModel / RuntimeLogModel）
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

## platform 层

路径：

```text
src/platform/
```

`qtet_platform` target 的源码目录，封装平台相关能力，目前包含：

```text
AutoStartHelper
DaemonRegisterHelper
FontHelper
```

- `AutoStartHelper`：开机自启动（Windows 注册表 / Linux XDG Autostart）。
- `DaemonRegisterHelper`：daemon 系统服务注册/启动（UAC / pkexec 提权），被 `AppServices` 在 daemon 断连时调用。
- `FontHelper`：由 `AppServices` 显式构造并注册给 QML，不应恢复为静态 singleton。

职责是封装平台相关能力。自启动状态以系统实际状态为唯一权威源：`SettingsViewModel`
直接调用 `AutoStartHelper` 读取/写入，这是明确且有限的例外；除自启动外，其余平台能力
仍应通过 application service 暴露，QML 不应直接依赖具体平台实现。

`LogHelper`（日志工具入口）位于 `src/core/log/`，见上文「log 基础模块」一节。

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
qtet_sqlite_repository
qtet_daemon_service
qtet_platform
qtet_system_tray
qtet_application
qtet_appsupport
```

总体依赖方向：

```text
appQtEasyTier
    ↓
qtet_appsupport
    ↓
qtet_application（应用业务服务 + ViewModel，依赖全部基础服务）
    ↓
qtet_sqlite_repository / qtet_daemon_service / qtet_platform / qtet_system_tray
    ↓
qtet_config / qtet_log
```

要点：

- 基础服务层只保留 `qtet_config` / `qtet_log` / `qtet_sqlite_repository` /
  `qtet_daemon_service` / `qtet_system_tray` / `qtet_platform`；其余能力（收藏、
  VPN 状态机、应用业务、ViewModel）全部收敛进 `qtet_application`。
- `qtet_application` 同时包含 `src/app_service/` 的业务服务与 `src/viewmodels/` 的
  ViewModel / Model，并编译 `FavoriteNodeJsonCodec`、`VpnController`、`StatusMonitor`；
  它依赖 `qtet_sqlite_repository`、`qtet_daemon_service` 及各基础服务。
- `FavoriteNode` 值类型属于 `qtet_sqlite_repository`（SQLite 记录），
  `qtet_application` 不得反向依赖 repository。
- 历史薄壳 ViewModel 对 `qtet_sqlite_repository` / `qtet_daemon_service` /
  `qtet_platform` 的直连已成为 `qtet_application` 内部实现依赖，
  不应再作为消费者可见的公共依赖扩散。

`qtet_system_tray` 由 `qtet_appsupport` 直接链接，依赖 `qtet_daemon_service`、`qtet_config` 和 `qtet_log`。

原则：

- 上层可以依赖下层。
- 下层不要反向依赖上层。
- 基础服务不得 include `app_service` 或 `viewmodels` 头文件。
- `appQtEasyTier` 链接 `qtet_appsupport`，不要重新聚合生产 `.cpp`。
- 新 C++ 源文件应加入所属模块 target；`src/app_service` 与 `src/viewmodels` 的新文件都加入 `qtet_application`。
- 默认 `BUILD_WITH_DAEMON=ON` 会构建并收集 `qtet-daemon`；传入 `-DBUILD_WITH_DAEMON=OFF` 时跳过后端构建 target 和 post-build 收集步骤；`-DCLONE_DAEMON_FROM=GITEE`（或 `CNB`）时从对应来源克隆后端源码，默认 `GITHUB`。daemon 构建与收集逻辑位于 `cmake/QtEasyTierDaemon.cmake`、`cmake/scripts/BuildDaemon.cmake` 和 `cmake/scripts/CollectDaemon.cmake`，不要把这类流程重新堆回根 `CMakeLists.txt`。
- Windows 当前只构建前端：即使 `BUILD_WITH_DAEMON=ON`，CMake 也会跳过 `qtet-daemon` 构建和收集。Windows 开发仅按 MinGW64 工具链适配，不为 MSVC 添加专用配置或兼容代码。

各 target 大致职责：

| Target | 职责 |
| --- | --- |
| `qtet_config` | 配置数据结构、TOML 序列化、校验、URL 编解码、daemon 载荷构建、运行状态枚举 |
| `qtet_log` | 日志基础类型、日志分发、日志工具入口 |
| `qtet_sqlite_repository` | SQLite repository 和数据库连接、`FavoriteNode` 记录类型 |
| `qtet_daemon_service` | daemon IPC 和 daemon API |
| `qtet_platform` | 平台相关实现（源码在 `src/platform/`） |
| `qtet_system_tray` | 系统托盘、托盘消息、真实通知输出 |
| `qtet_application` | 应用业务服务层 + 暴露给 QML 的 ViewModel / Model + 收藏编解码 + VPN 状态机（配置/设置/收藏/日志/危险操作/VPN 运行桥） |
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
tst_settings_viewmodel
tst_favorite_node_json_codec
tst_favorite_node_viewmodel
tst_import_nodes_viewmodel
tst_app_services
tst_vpn_runtime_service
tst_dangerous_operation_service
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
src/viewmodels/ConfigEditorViewModel.*
src/core/config/ConfigPayloadBuilder.*
src/core/sqlite_repository/
tests/
```

如果字段需要进入 TOML、SQLite 或 daemon payload，应分别补齐序列化、持久化和 payload 构建测试。

### 新增 daemon API 调用

优先修改：

```text
src/core/daemon_service/DaemonApi.*
```

调用方应通过 `DaemonApi` 使用，不要在 ViewModel 或 QML-facing 类中散落 method name 和 JSON params。

### 新增应用业务操作

优先放在：

```text
src/app_service/
```

如果该操作需要暴露给 QML，再由对应 ViewModel 包一层 QML-friendly API。

### 新增 QML 可绑定数据列表

优先使用 `QAbstractListModel` 风格的 model，放在：

```text
src/viewmodels/
```

避免直接暴露复杂 `QVariantList` 作为长期 API。

### 新增页面级协调逻辑

优先放在页面级 ViewModel，例如：

```text
src/viewmodels/runtime/NetworkPageViewModel.*
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
| 日志基础设施 | `src/core/log/` | `qtet_log` |
| SQLite repository | `src/core/sqlite_repository/` | `qtet_sqlite_repository` |
| daemon IPC / API | `src/core/daemon_service/` | `qtet_daemon_service` |
| 平台相关实现 | `src/platform/` | `qtet_platform` |
| 应用业务服务 | `src/app_service/` | `qtet_application` |
| 收藏节点编解码 | `src/app_service/favorite/` | `qtet_application` |
| VPN 状态机 / 运行状态 | `src/app_service/runtime/` | `qtet_application` |
| 系统托盘 / 托盘消息 | `src/core/system_tray/` | `qtet_system_tray` |
| QML ViewModel / Model | `src/viewmodels/` | `qtet_application` |
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
- 不要让 QML 直接依赖 `DaemonClient` / `DaemonApi` / `VpnController`。
- 不要把 `SystemTrayManager` 注册为 QML singleton；托盘行为由 `AppServices` 和 `main.cpp` 协作完成。
- 不要重新引入裸 `QVariantList` 风格的 QML API 或让 QML 直接绑定 `VpnController`。
- 不要重新添加 `LogHelper::init(...)`。
- 不要在测试 target 中重复编译大量生产 `.cpp`，应链接模块 target。
- 不要让基础服务层（`config` / `log` / `sqlite_repository` / `daemon_service` / `system_tray` / `platform`）
  依赖 ViewModel、QML 或 `app_service`。
- 不要绕过 `DaemonApi` 在上层散落 daemon method name。
- 新增跨模块业务操作时，优先放入 `src/app_service/`，再由 ViewModel 做 QML 友好的薄壳转发。

### 已知技术债务（暂不处理）

以下直连属于历史遗留，新增代码不要模仿，也不要顺手扩大：

- `LogViewModel` 直接依赖 `LogRepository`。
- `FavoriteNodeViewModel` 直接依赖 `FavoriteNodeRepository`。
- `BackendStatusViewModel` 直接依赖 `DaemonClient`。
- `ImportNodesViewModel` 直接使用 `FavoriteNodeJsonCodec`。
- `FontHelper`（`src/platform/`）直接注册为 QML singleton（纯 UI 工具类例外）。

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
~/.config/qteasytier/QtEasyTier/qteasytier.db
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
- 保持 UI 层与基础服务层之间经应用服务层桥接，基础服务层不接触任何 UI 类型。
- 保持底层基础设施模块可测试、可复用。
- 保持 CMake target 与源码目录职责一致。
- 保持测试通过模块 target 链接生产代码。
- 保持系统托盘行为由 `SystemTrayManager` 集中管理，不直接散落在 QML 或 ViewModel 中。

如果一项改动会破坏这些目标，应优先重新审视设计，而不是为了短期方便绕过架构边界。
