# QtEasyTier 项目上下文

## 项目概述

QtEasyTier 是一个基于 Qt C++ 框架开发的异地组网工具，提供直观的图形界面帮助用户配置和管理虚拟网络。其后端核心为 EasyTier —— 一个去中心化的组网方案，无需依赖中心服务器，每个节点平等独立，为用户提供安全、可靠、低成本的异地组网服务。

### 项目特点
- **快速**: 纯 Qt C++ 开发，无 Chromium/Webview，前端占用不超过 50MB
- **美观**: UI 样式移植自 KDE 的 Breeze 样式
- **简单**: 支持一键联机功能
- **丰富**: 支持 EasyTier 大部分功能，按需定制虚拟网络
- **安全**: 基于 EasyTier 去中心化组网方案

### 技术栈
- **语言**: C++ 20
- **框架**: Qt 6（推荐 6.10.1），支持 Qt5 兼容
- **构建系统**: CMake 3.16+
- **编译器**: llvm-mingw（Windows）
- **UI 样式**: KDE Breeze Style（移植版）

### 当前版本
- 版本号: 1.3.1
- 发布日期: 2026-02-24
- 平台支持: Windows 10/11（Linux 计划支持中）

---

## 目录结构

```
QtEasyTier/
├── Qt_HEAD/           # 头文件目录
│   ├── mainwindow.h   # 主窗口类
│   ├── netpage.h      # 网络配置页面
│   ├── oneclick.h     # 一键联机功能
│   ├── publicserver.h # 公共服务器列表对话框
│   ├── setting.h      # 设置对话框
│   └── donate.h       # 赞助对话框
├── Qt_SRC/            # 源文件目录
│   ├── mainwindow.cpp
│   ├── netpage.cpp
│   ├── oneclick.cpp
│   ├── publicserver.cpp
│   ├── setting.cpp
│   └── donate.cpp
├── Qt_UI/             # Qt Designer UI 文件
│   ├── mainwindow.ui
│   ├── netpage.ui
│   ├── oneclick.ui
│   ├── publicserver.ui
│   ├── setting.ui
│   └── donate.ui
├── Qt_QRC/            # 资源文件（图标等）
│   ├── icons.qrc      # 资源文件定义
│   ├── icon.ico       # 应用程序图标
│   ├── alipay.png     # 支付宝收款码
│   ├── wechat.png     # 微信收款码
│   └── *.svg          # SVG 图标资源
├── Sources/           # 核心源码
│   ├── main.cpp       # 程序入口、单实例检测、Breeze样式加载
│   ├── generateconf.h # 配置生成工具头文件
│   ├── generateconf.cpp # 配置生成、Base32编解码、联机码生成、端口检测
│   ├── easytierworker.h # EasyTier进程管理工作类头文件
│   ├── easytierworker.cpp # EasyTier进程管理工作类实现
│   ├── webdashboardworker.h # Web控制台进程管理工作类头文件
│   └── webdashboardworker.cpp # Web控制台进程管理工作类实现
├── ThirdParty/        # 第三方依赖
│   ├── Breeze/        # Breeze 主题资源
│   │   ├── data/      # 配色方案、本地化
│   │   │   ├── color-schemes/  # 颜色方案
│   │   │   ├── kstyle/         # 样式主题
│   │   │   └── locale/         # 多语言翻译
│   │   └── styles/    # 样式库
│   └── EasyTier/      # EasyTier 核心程序
│       ├── win/       # Windows 版本
│       └── linux/     # Linux 版本
├── CDIRCalculator/    # CIDR 计算器子项目
├── assets/            # 项目资源（图片、manifest等）
│   ├── app.manifest   # Windows 应用程序清单
│   ├── publicserver.json # 公共服务器列表
│   ├── LICENSE.rtf    # 许可证文件
│   └── *.png/*.webp   # 图片资源
├── cmake-build-debug/ # Debug 构建目录
└── cmake-build-release/ # Release 构建目录
```

---

## 核心类说明

### MainWindow
主窗口类，负责：
- 网络配置管理（加载/保存配置）
- 系统托盘管理
- 界面切换（NetPage、OneClick、设置）
- 单实例运行检测
- Web 控制台启动与管理
- 赞助窗口弹出逻辑

**关键成员：**
- `m_netpages`: 网络配置页面列表
- `m_oneClick`: 一键联机页面
- `m_isHideOnTray`: 隐藏到托盘标志
- `trayIcon`: 系统托盘图标
- `m_webWorker`: Web 控制台进程管理对象
- `m_configPath`: 配置保存路径（根据编译模式决定）

### NetPage
网络配置页面，核心功能包括：
- **基础设置**: 用户名、网络名称、密码、IP 地址、DHCP
- **低延迟优先**: `--latency-first` 参数
- **私有模式**: 控制网络可见性
- **服务器管理**: 添加/删除服务器节点、公共服务器列表
- **高级设置**: KCP 代理、QUIC、TUN 模式、P2P、IPv6、Magic DNS 等
- **运行管理**: 启动/停止 EasyTier 进程、日志显示、节点信息展示
- **配置导入导出**: 支持配置文件的导入导出
- **日志查看**: 打开日志文件功能

**关键成员：**
- `m_workerThread`: Worker 工作线程
- `m_worker`: EasyTierWorker 工作对象
- `realRpcPort`: 实际 RPC 端口号
- `m_peerTable`: 节点信息表格
- `m_logTextEdit`: 日志显示控件
- `m_processDialog`: 启动过程对话框

**信号：**
- `networkStarted()`: 网络启动信号
- `networkFinished()`: 网络停止信号

### OneClick
一键联机功能页面，提供：
- **房主模式**: 创建房间、生成联机码、显示房主 IP
- **房客模式**: 输入联机码加入网络
- **身份状态管理**: `UserRole` 枚举（None/Host/Guest）
- **联机人数显示**: 实时显示当前网络中的节点数

**关键成员：**
- `m_workerThread`: Worker 工作线程
- `m_worker`: EasyTierWorker 工作对象
- `m_currentRole`: 当前角色状态
- `m_currentNetworkId/m_currentPassword`: 当前房间信息
- `m_processDialog`: 启动过程对话框

### EasyTierWorker
EasyTier 进程管理工作类，设计用于在独立线程中运行，负责：
- **进程生命周期管理**: 启动、停止 EasyTier 核心进程
- **日志处理**: 实时读取进程输出并保存到日志文件
- **节点信息获取**: 通过 CLI 定时获取节点信息
- **状态管理**: 进程状态枚举（Stopped/Starting/Running/Stopping）

**关键成员：**
- `m_easytierProcess`: EasyTier 核心进程
- `m_cliProcess`: CLI 进程（获取节点信息）
- `m_cliTimer`: CLI 信息更新定时器
- `m_processState`: 当前进程状态
- `m_rpcPort`: RPC 端口号
- `m_logFile/m_logStream`: 日志文件相关

**信号：**
- `processStarted(bool, QString)`: 进程启动完成
- `processStopped(bool)`: 进程停止完成
- `logOutput(QString, bool)`: 日志输出
- `peerInfoUpdated(QJsonArray)`: 节点信息更新
- `processCrashed(int)`: 进程异常终止

**常量配置：**
- `EASYTIER_START_TIMEOUT_MS = 30000`: 启动超时时间
- `CLI_UPDATE_INTERVAL_MS = 2000`: CLI 更新间隔
- `PROCESS_START_WAIT_MS = 3000`: 进程启动等待
- `PROCESS_KILL_WAIT_MS = 5000`: 进程终止等待
- `MAX_LOG_LINES = 1000`: 最大日志显示行数

### WebDashboardWorker
Web 控制台进程管理工作类，负责：
- **进程生命周期管理**: 启动、停止 easytier-web-embed 进程
- **状态管理**: 进程状态枚举（Stopped/Starting/Running/Stopping）
- **超时检测**: 启动超时自动终止

**关键成员：**
- `m_process`: Web 控制台进程
- `m_processState`: 当前进程状态

**信号：**
- `processStarted(bool, QString)`: 进程启动完成
- `processStopped(bool)`: 进程停止完成
- `stateChanged(ProcessState)`: 进程状态变化
- `processCrashed(int, QProcess::ExitStatus)`: 进程异常终止

**常量配置：**
- `START_TIMEOUT_MS = 10000`: 启动超时时间
- `KILL_WAIT_MS = 5000`: 进程终止等待

### PublicServer
公共服务器列表对话框：
- 从 `publicserver.json` 加载服务器列表
- 支持多选服务器地址
- 返回选中的服务器列表

### Settings
设置对话框，包含：
- 版本检测（核心/CLI/Web 版本）
- 开机自启设置
- 自动回连设置
- 隐藏到系统托盘设置
- 自动检查更新设置
- 日志管理（保存天数、清空日志）
- Web 控制台配置
- 检查更新功能
- 启动次数计数与赞助弹窗控制

**关键成员：**
- `VersionDetectionWorker`: 版本检测工作类（在线程中运行）
- `m_autoRun`: 自动回连标志
- `m_autoStart`: 开机自启标志
- `m_isHideOnTray`: 隐藏到托盘标志
- `m_autoUpdate`: 自动检查更新标志
- `m_logRetentionDays`: 日志保存天数
- `m_launchCount`: 启动次数计数
- `m_shouldShowDonate`: 是否应弹出赞助窗口标志
- `m_webConfig`: Web 控制台配置
- `m_versionThread`: 版本检测线程
- `m_networkManager`: 网络请求管理器

**关键方法：**
- `detectSoftwareVersion()`: 检测软件版本
- `isAutoRun()`: 获取自动回连状态
- `isHideOnTray()`: 是否隐藏到系统托盘
- `isAutoUpdate()`: 是否自动检查更新
- `shouldShowDonate()`: 是否应该弹出赞助窗口
- `getWebConsoleConfig()`: 获取 Web 控制台配置
- `getLogRetentionDays()`: 获取日志保存天数
- `cleanupOldLogs(int)`: 清理过期日志文件
- `clearAllLogs()`: 清空所有日志文件

### Donate
赞助对话框，提供：
- 赞助说明文字展示
- 微信/支付宝收款码展示
- 赞助详情页面链接

---

## 构建与运行

### 环境要求
- Qt 6（推荐 6.10.1），支持 Qt5 向后兼容
- CMake 3.16+
- llvm-mingw 编译器（Windows）
- C++ 20 标准

### 构建命令

#### 正常模式（配置保存在系统路径）
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
cmake --install . --config Release
```

#### 便携模式（配置保存在程序目录）
```bash
mkdir build
cd build
cmake .. -DSAVE_CONF_IN_APP_DIR=true
cmake --build . --config Release
cmake --install . --config Release
```

> 注意：便携模式构建的程序禁用开机自启功能，避免污染系统环境。

### 构建输出
- 可执行文件输出到 `build/bin/`
- 安装目录为 `Install/`

### 运行参数
- `--auto-start`: 开机自启模式启动（不显示主窗口）

### 构建依赖复制
CMake 构建过程会自动复制以下内容到输出目录：
- ThirdParty/Breeze 目录（主题资源）
- ThirdParty/EasyTier/win 目录（重命名为 etcore）
- assets/publicserver.json（公共服务器列表）
- CIDRCalculator.exe（CIDR 计算器）
- 自动调用 windeployqt 部署 Qt 运行时

---

## 编码规范

### 文件组织
- 头文件放在 `Qt_HEAD/` 目录
- 源文件放在 `Qt_SRC/` 目录
- UI 文件放在 `Qt_UI/` 目录
- 资源文件放在 `Qt_QRC/` 目录
- 核心工具类放在 `Sources/` 目录
- 第三方依赖放在 `ThirdParty/` 目录
- 项目资源放在 `assets/` 目录

### 命名约定
- 类名: 大驼峰命名（如 `NetPage`, `OneClick`）
- 成员变量: `m_` 前缀 + 小驼峰（如 `m_easytierProcess`, `m_netpages`）
- 私有方法: 下划线前缀（如 `_changeWidget`）
- 信号/槽: 小驼峰命名

### Qt 信号槽
使用 Qt 5+ 的函数指针连接方式：
```cpp
connect(button, &QPushButton::clicked, this, &ClassName::onButtonClicked);
```

### 线程安全
- 工作类（如 `EasyTierWorker`, `WebDashboardWorker`）继承 `QObject`，可在独立线程中运行
- 信号槽连接使用 `Qt::QueuedConnection` 确保线程安全
- 遵循 Qt 线程绑定工作对象的规范

### 注释规范
使用 Doxygen 风格注释：
```cpp
/// @brief 简要描述
/// @param name 参数说明
/// @return 返回值说明
```

### 类型转换注意事项
Qt 6 中 `QList::size()` 等方法返回 `qsizetype`（即 `long long`），在赋值给 `int` 变量时需使用 `static_cast<int>()` 显式转换，避免 Clang-Tidy 窄化转换警告：
```cpp
for (int i = 0; i < static_cast<int>(list.size()); ++i) {
    // ...
}
```

---

## 关键实现细节

### 单实例运行
使用 `QLocalServer`/`QLocalSocket` 实现单实例检测，服务名称为 `QtEasyTierbyViahuang`。

### 配置保存路径
通过编译宏 `SAVE_CONF_IN_APP_DIR` 控制：
- `true`: 配置保存在程序目录的 `config/` 子目录（便携模式）
- `false`: 配置保存在系统标准配置路径（`QStandardPaths::AppConfigLocation`）

### EasyTier 进程管理
- 使用 `EasyTierWorker` 类在独立线程中管理进程
- 通过信号槽机制与主界面通信
- 日志输出重定向到界面和文件
- RPC 端口用于获取节点信息（随机分配或用户指定）
- 使用 `easytier-cli` 异步获取节点信息
- 启动过程显示进度对话框

### Web 控制台管理
- 使用 `WebDashboardWorker` 类管理 easytier-web-embed 进程
- 支持配置下发端口、前端端口、协议等参数
- 支持本地 API 或远程 API 模式
- 启动后自动打开浏览器访问控制台
- 默认账户：admin / admin

### 端口检测
- Windows 平台使用 IP Helper API (`GetTcpTable2`) 检测端口占用
- 非Windows平台使用 `QTcpServer` 方式检测
- RPC 端口在 10000-50000 范围内随机分配

### 联机码生成
- 使用 Base32 编码生成房间号和密码
- 房间号格式: `QtET-OneClick-XXXXX`（带前缀）
- 联机码格式: `XXXX-XXXX-XXXX-XXXX`（16位，含分隔符）
- 相关函数：`generateRoomCredentials()`、`encodeConnectionCode()`、`decodeConnectionCode()`

### 版本检测
- 使用独立线程 `VersionDetectionWorker` 执行版本检测
- 通过 `QProcess` 运行 `easytier-core -V`、`easytier-cli -V` 和 `easytier-web-embed -V` 获取版本
- 支持检测 Core、CLI、Web 三种组件版本
- 使用信号槽机制返回检测结果

### 软件更新检查
- 通过 Gitee API 获取最新版本号
- 对比当前版本与最新版本
- 支持自动检查更新和手动检查更新

### 赞助弹窗逻辑
- 通过 `Settings` 类的 `m_launchCount` 记录启动次数
- 当启动次数达到 50 次时，设置 `m_shouldShowDonate` 为 true
- 主窗口在加载配置后检查该标志，若为 true 则弹出赞助窗口

### 日志管理
- 日志文件保存在程序目录的 `log/` 子目录
- 文件命名格式：`yyyyMMdd_HHmmss_Base32网络名.log`
- 支持设置日志保存天数（0/3/7/14/30天）
- 启动和退出时自动清理过期日志

### Windows 资源嵌入
- 自动生成 RC 文件嵌入 manifest 和图标
- manifest 文件位于 `assets/app.manifest`
- 图标文件位于 `Qt_QRC/icon.ico`
- 程序请求管理员权限运行

---

## 依赖说明

### Qt 组件
- `Qt::Widgets` - GUI 组件
- `Qt::Network` - 网络功能（用于检查更新）

### Windows 系统库
- `iphlpapi` - IP Helper API（端口检测）
- `ws2_32` - Windows Socket 2

### 外部依赖
- **EasyTier 核心**: 放置在 `ThirdParty/EasyTier/win/` 目录，包含 `easytier-core.exe`、`easytier-cli.exe` 和 `easytier-web-embed.exe`
- **Breeze 样式**: 从 KDE 移植的样式库，放置在 `ThirdParty/Breeze/` 目录

---

## 常见任务

### 添加新的网络配置选项
1. 在 `netpage.h` 中添加成员变量和 getter 方法
2. 在 `netpage.cpp` 的 `initAdvancedSettings()` 或相关初始化函数中创建 UI 控件
3. 在 `getNetworkConfig()` 和 `setNetworkConfig()` 中添加配置保存/加载逻辑
4. 在 `generateconf.cpp` 的 `generateConfCommand()` 中添加命令行参数生成

### 修改 UI 布局
直接编辑 `Qt_UI/` 目录下的 `.ui` 文件（Qt Designer 格式）。

### 添加新的界面翻译
在 `ThirdParty/Breeze/data/locale/` 目录下添加或修改 `.qm` 翻译文件。

### 调试构建问题
- 检查 CMake 配置输出
- 确认 Qt 路径正确设置
- 验证 `ThirdParty/EasyTier/win/` 目录存在且包含必要文件
- 检查 `assets/app.manifest` 文件是否存在

### 更新公共服务器列表
编辑 `assets/publicserver.json` 文件，格式为 JSON 数组，每项包含 `url` 和 `contributor` 字段。

### 添加新的设置项
1. 在 `setting.h` 中添加成员变量
2. 在 `loadSettings()` 和 `saveSettings()` 中添加读写逻辑
3. 在 UI 文件或初始化函数中添加对应的控件

---

## Git 提交规范

- 提交信息使用中文描述
- 保持原子提交，每个 commit 只做一件事
- 不要提交 `build*/`、`Install/`、`*.user` 等生成文件
- `ThirdParty/EasyTier/` 目录已添加到 `.gitignore`（仅保留目录结构）

---

## 联系方式

- 项目地址: https://gitee.com/viagrahuang/qt-easy-tier
- 问题反馈: 欢迎提交 Issue 和 PR
- 交流群: EasyTier支持3群 (957189589)
- 许可证: GPLv3.0
