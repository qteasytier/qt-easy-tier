# QtEasyTier

QtEasyTier 是一个基于 Qt 6 / C++ 的 EasyTier 桌面客户端。它提供了直观的图形界面，帮助用户轻松配置和管理虚拟网络，实现跨网络设备的安全通信。

![应用截图](/assets/docs/readme.png)

本项目在以下三个平台同步开源，您可以在自己熟悉的平台 fork 本项目

|   平台   | 开源地址                                                                                             |
|:--------:|:-----------------------------------------------------------------------------------------------------|
| 腾讯 CNB | [https://cnb.cool/myqfeng/qteasytier/qt-easy-tier](https://cnb.cool/myqfeng/qteasytier/qt-easy-tier) |
|  Gitee   | [https://gitee.com/qteasytier/qt-easy-tier](https://gitee.com/qteasytier/qt-easy-tier)               |
|  GitHub  | [https://github.com/qteasytier/qt-easy-tier](https://github.com/qteasytier/qt-easy-tier)             |

**提交 PR 请前往 GitHub 或者腾讯 CNB，Issue 可在三个平台提交**

> QtEasyTier 3.0 对项目架构进行了重大重构，如需查看旧版本源码请前往[旧版仓库（已停用）](https://gitee.com/qteasytier/qteasytier_old)

## 功能特性

- 网络配置管理：创建、重命名、删除、导入和导出配置。
- 一键启动/停止：从配置列表直接控制指定网络实例。
- 运行状态查看：展示节点信息、运行日志和后端连接状态。
- 收藏节点管理：维护常用节点信息。
- 系统托盘：支持关闭到托盘、托盘唤起主窗口和自启动场景。

## 技术栈

- Qt 6.8+
- C++20
- QML / Qt Quick
- SQLite
- CMake 3.16+

## 目录结构

```text
src/
├── main.cpp                         应用入口
├── app/                             应用装配层（AppServices、QML singleton 注册）
├── core/                            应用核心层（UI 与基础服务之间的桥接）
│   └── viewmodels/                  暴露给 QML 的 ViewModel / Model
├── config/                          配置结构、TOML 序列化、校验、URL 编解码、daemon 载荷、运行状态枚举
├── sqlite_repository/               SQLite repository
├── daemon_service/                  daemon IPC、JSON-RPC 和 API 封装
├── system_tray/                     系统托盘与消息分发
├── log/                             日志基础设施
├── platform/                        平台相关实现（自启动、daemon 注册、字体）
└── qml/                             QML UI（UI 层）
```

更完整的开发说明见 [CONTRIBUTING.md](CONTRIBUTING.md)。

## 构建要求

请确保本机已安装：

- CMake 3.16 或更新版本
- 支持 C++20 的 C++ 编译器
- Qt 6.8 或更新版本，包含 `Core`、`Sql`、`Network`、`Test`、`Quick`、`Widgets`、`Concurrent`、`Svg`、`QuickDialogs2`
- OpenSSL 库
- `git`，用于默认构建 `qtet-daemon`

默认构建会从 GitHub 克隆并编译 `qtet-daemon`，因此需要可用网络。如果只想构建前端或进行离线验证，可以关闭后端构建。

Windows 当前仅适配 MinGW64(UCRT) 构建，不面向 MSVC / Visual Studio 生成器做兼容。

## 构建与运行

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/Output/appQtEasyTier
```

所有可执行文件和库会输出到 `build/Output/`。

### Windows 依赖安装

Windows 下推荐使用 MSYS2 工具安装必要依赖（UCRT）：

打开 MSYS2 UCRT 终端，执行以下命令
```bash
pacman -S  mingw-w64-ucrt-x86_64-gcc \
           mingw-w64-ucrt-x86_64-cmake \
           mingw-w64-ucrt-x86_64-ninja \
           mingw-w64-ucrt-x86_64-qt6-base \
           mingw-w64-ucrt-x86_64-qt6-declarative \
           mingw-w64-ucrt-x86_64-qt6-svg \
           mingw-w64-ucrt-x86_64-qt6-tools \
           mingw-w64-ucrt-x86_64-openssl \
           git
```

**您还需要将MSYS2 UCRT安装目录添加到环境变量 `PATH` 中**。

### 只构建前端

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DBUILD_WITH_DAEMON=OFF
cmake --build build -j
```

关闭 `BUILD_WITH_DAEMON` 后，CMake 不会构建和收集 `qtet-daemon`。这种模式适合离线开发、前端验证和单元测试。

### 指定后端克隆来源

默认从 GitHub 克隆后端源码，可通过 `CLONE_DAEMON_FROM` 选择来源（`GITEE` / `GITHUB` / `CNB`）：

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DCLONE_DAEMON_FROM=GITEE
cmake --build build -j
```

设置 `-DCLONE_DAEMON_FROM=GITEE` 后，CMake 会从 Gitee 克隆后端源码；设为 `CNB` 则从 cnb.cool 克隆。

## 测试

运行全部测试：

```bash
ctest --test-dir build --output-on-failure
```

运行单个测试：

```bash
ctest --test-dir build -R tst_network_conf --output-on-failure
./build/Output/tst_network_conf
```

## 配置文件

程序配置的数据存储在系统标准配置目录下。例如，Linux 下应用配置目录通常位于：

```text
~/.config/qteasytier/QtEasyTier/
```

程序采用SQLite存储网络配置，默认 SQLite 数据库为：

```text
~/.config/qteasytier/QtEasyTier/qteasytier.db
```

全局设置文件默认使用 `settings3.json`。

## 贡献

提交代码前建议至少运行：

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DBUILD_WITH_DAEMON=OFF
cmake --build build -j
ctest --test-dir build --output-on-failure
```

新增 C++ 源文件时，请加入对应模块目录的 `CMakeLists.txt`。新增 QML 文件时，请加入根 `CMakeLists.txt` 的 `QTET_QML_FILES` 列表。详细模块边界、对象生命周期和测试约定见 [CONTRIBUTING.md](CONTRIBUTING.md)。

## 赞助本项目
项目开发不易，如果您认为本项目对您有帮助，欢迎赞助项目开发，您的支持是我们继续开发的重要动力！

赞助方式：微信支付 & 支付宝

<p>
<img src="/assets/docs/wechat.png" width="220">
<img src="/assets/docs/alipay.png" width="220">
</p>

[点击前往赞助详情页面](https://qtet.cn/other/donate/)
