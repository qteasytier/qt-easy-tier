# QtEasyTier 分层收敛重构总结

本文件记录对 QtEasyTier 架构所做的一次系统性分层收敛。核心目标是消除“基础服务层与应用服务层之间无收益的机械桥接”，同时保留有独立职责的真实边界（daemon IPC、配置编解码、SQLite 持久化、单实例状态机）。

所有改动分 5 个阶段完成，共 9 个提交，每个阶段均独立构建并通过 CTest 全量验证。

## 背景问题

原架构把大量本属于应用层的职责放进了 `src/core/`，又为了遵守名义分层在其上叠加一层转发，形成重复状态、方法透传和信号中继：

```text
QML → NetworkPageViewModel → VpnRuntimeService → VpnManager → VpnController → DaemonApi → DaemonClient
QML → SettingsViewModel   → SettingsBackendService → DaemonApi / UpdateCheckService
QML → DangerousOperationViewModel → DangerousOperationService
```

其中多个中间层只是“同名方法 + 同名信号”的一对一转发，没有带来业务语义或复用边界。

## 阶段一：合并 VPN 运行时协调服务

提交：`a350d77 refactor: 合并 VPN 运行时协调服务`

- 删除 `VpnManager`（多实例协调、心跳同步、外部实例发现、`stopAll` 收敛全部迁入）。
- `VpnRuntimeService` 升级为应用级 runtime 协调器，直接注入 `DaemonClient` / `DaemonApi` / `NetworkConfigRepository` / `StatusMonitor`。
- 保留 `VpnController`（单实例状态机）与 `StatusMonitor`（异步解析）作为内部协作者。
- 启用链缩短一层，删除机械信号 `instanceInfoUpdated`。
- 测试 `tst_vpn_manager` → `tst_vpn_runtime_service`。

## 阶段二：删除纯薄壳对象

提交：

- `99bdafa refactor: 直接暴露危险操作服务`
- `112c5da refactor: 直接使用系统自启动状态`
- `7a6e131 docs: 同步薄壳收敛后的架构说明`

### 2A 危险操作

- 删除 `DangerousOperationViewModel`。
- `DangerousOperationService` 增加 QML 元数据（`Q_PROPERTY` / `Q_INVOKABLE`），直接向 QML 暴露。
- QML 单例注册名保持 `DangerousOperationViewModel`，`SettingsPage.qml` 零改动。
- 新增元对象回归测试锁定 QML 接口。

### 2B 自启动

- 删除 `AutoStartService`。
- 系统注册表 / XDG Autostart 成为自启动状态的唯一权威源；`settings3.json` 不再持久化 `autoStart` 字段（`SettingsStore` 同步移除该字段，旧字段读取时自动忽略）。
- `SettingsViewModel` 直接调用 `AutoStartHelper`（有意的架构例外），新增 `refreshAutoStart()`。
- “清空全部数据”现在会同时关闭系统自启动项。
- 测试 `tst_autostart_service` → `tst_settings_viewmodel`。

## 阶段三：合并设置后端服务状态

提交：`512337c refactor: 合并设置后端服务状态`

- 删除 `SettingsBackendService`。
- `SettingsViewModel` 直接注入 `DaemonApi` / `UpdateCheckService`，持有自动回连状态、busy 状态与更新检查 busy 状态。
- 完整保留异步行为：daemon 返回实际值为准、失败回滚、查询失败仅记日志、更新检查终态信号收敛 busy。
- `tst_settings_viewmodel` 新增 fake-daemon 自动回连测试（查询/设置/失败/未连接）。

## 阶段四：配置协调链与状态一致性修复

提交：

- `2271a23 fix: 修复配置重命名与删除状态一致性`
- `a6cf42c refactor: 暴露完整配置运行状态`

### 重命名覆盖修复

- `ConfigListModel` 新增 `configRenamed` 信号。
- `ConfigEditorViewModel` 新增 `syncDisplayName()`（仅同步显示名、不标 dirty、不触发自动保存）。
- 由 `AppServices::wireConfigCoordination()` 连接，杜绝列表重命名后被编辑器旧快照覆盖。

### 删除流程修复

- 页面不再把 `deleteConfig()` 返回的“请求已接受”当作删除完成。
- 以 `configDeleted` 为真实删除完成事件，`NetworkPageViewModel::handleConfigDeleted()` 负责清空选择。
- 新增 `ConfigEditorViewModel::discardAndClear()`（丢弃式清空，不刷写待保存修改），避免已删除配置“复活”。

### 完整运行状态

- `ConfigListModel` 新增 `runState` role 与 `instanceState()` 查询（外部实例固定 Running）。
- `NetworkPageViewModel` 新增 `currentInstanceRunState` / `currentInstanceBusy`（由完整状态派生）。
- QML：列表区分启动中/停止中/运行错误并禁用过渡期操作；编辑器在 busy 时整体禁用。

## 阶段五：合并应用与 ViewModel 构建目标

提交：

- `f2021d5 refactor: 合并应用与视图模型构建目标`
- `cf729c2 build: 收紧应用装配目标依赖`
- `0bc2329 docs: 同步阶段4/5架构说明`

- 删除物理 target `qtet_viewmodel`，ViewModel / Model 源码并入 `qtet_application`，随后物理移入 `src/app_service/viewmodels/`。
- 测试链接全部迁移为直接依赖被测模块。
- `qtet_appsupport` 依赖收紧为 `PRIVATE`（仅 `Qt6::Core` / `Qt6::Quick` / `qtet_application` / `qtet_system_tray`）。

## 最终物理结构

```text
qtet_config               配置值模型、TOML、校验、URL codec、密钥
qtet_log                  日志基础设施
qtet_sqlite_repository    SQLite 持久化（含 FavoriteNode 记录类型）
qtet_daemon_service       daemon IPC、帧协议、DaemonApi
qtet_platform             自启动、daemon 注册、字体
qtet_system_tray          系统托盘、托盘消息

qtet_appcore               src/core + viewmodels + 收藏编解码 + VPN 状态机
  ├── VpnRuntimeService / VpnController / StatusMonitor（VPN runtime 协调）
  ├── FavoriteNodeJsonCodec（收藏导入导出）
  ├── ConfigListModel / ConfigEditorViewModel
  ├── SettingsViewModel（设置状态唯一所有者）
  ├── DangerousOperationService（QML 注册名 DangerousOperationViewModel）
  └── 其他应用核心服务与 ViewModel

qtet_appsupport           AppServices、QmlSingletonRegistrar、AppLaunchManager

appQtEasyTier             仅链接 qtet_appsupport
```

目录结构随最新调整：`src/app_service` 更名为 `src/core`（应用核心），原基础模块
`config`/`log`/`sqlite_repository`/`daemon_service`/`system_tray` 提升到 `src/` 顶层；
应用核心 target 名从 `qtet_application` 更新为 `qtet_appcore`。

已删除的物理 target：`qtet_favorite`（值类型归 `qtet_sqlite_repository`、
codec 归 `qtet_appcore`）、`qtet_vpn`（并入 `qtet_appcore`）。

## 行为保持

重构只消除无收益层次，不改变用户可见行为：

- QML singleton 名称与 `SettingsPage.qml` / `NetworkPage.qml` 等调用接口保持不变。
- daemon RPC 方法、心跳间隔、`stopAll` 超时不变。
- 自动回连/更新检查的异步语义、错误处理、busy 收敛不变。
- 配置导入导出、危险操作、收藏、日志等既有行为不变。
- `ConfigRunState` 枚举及其判断函数保留。

## 验证结果

- 全量构建通过（`BUILD_WITH_DAEMON=OFF`，静态 OpenSSL prefix 已生效）。
- 26/26 CTest 通过。
- 被删除类型全仓库零残留：`VpnManager`、`DangerousOperationViewModel`（仅保留 QML 注册名）、`AutoStartService`、`SettingsBackendService`、`qtet_viewmodel`。
