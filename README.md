# QtEasyTier

QtEasyTier 是一个基于 Qt 6 / C++ / QML 的 EasyTier 桌面客户端，目标是提供更易用的远程组网配置、运行状态查看和日常管理体验。

项目使用 C++ 承担业务逻辑、持久化、daemon 通信和平台能力封装，QML 负责界面展示与交互绑定。应用前端通过 `qtet-daemon` 完成实际网络运行能力。

> QtEasyTier 3.0 对项目架构进行了根本架构上的重构，与之前版本不再兼容，如需查看之前版本的源码请前往[旧版仓库（已停用）](https://gitee.com/qteasytier/qteasytier_old)

![应用截图](https://qtet.cn/qtet.png)

## 功能特性

- 网络配置管理：创建、重命名、删除、导入和导出配置。
- 一键启动/停止：从配置列表直接控制指定网络实例。
- 运行状态查看：展示节点信息、运行日志和后端连接状态。
- 公共服务器列表：内置公共服务器资源，便于快速填写配置。
- 收藏节点管理：维护常用节点信息。
- 系统托盘：支持关闭到托盘、托盘唤起主窗口和自启动场景。
- 单实例启动：重复启动时会唤起已有主窗口。

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
├── app/                             应用装配、启动参数、QML singleton 注册
├── core/
│   ├── config/                      配置结构、TOML 序列化、校验、URL 编解码
│   ├── repository/                  SQLite repository
│   ├── service/                     daemon IPC、JSON-RPC 和 API 封装
│   ├── application/                 应用业务服务
│   ├── viewmodel/                   暴露给 QML 的 ViewModel / Model
│   ├── vpn_manager/                 VPN 生命周期和运行状态维护
│   ├── system_tray/                 系统托盘与消息分发
│   ├── log/                         日志基础设施
│   └── util/                        工具类与平台相关实现
└── qml/                             QML UI
```

更完整的开发说明见 [CONTRIBUTING.md](CONTRIBUTING.md)。

## 构建要求

请确保本机已安装：

- CMake 3.16 或更新版本
- 支持 C++20 的 C++ 编译器
- Qt 6.8 或更新版本，包含 `Core`、`Sql`、`Network`、`Test`、`Quick`、`Widgets`、`Concurrent`、`Svg`、`QuickDialogs2`
- `git` 和 `bash`，用于默认构建 `qtet-daemon`

默认构建会从 GitHub 克隆并编译 `qtet-daemon`，因此需要可用网络。如果只想构建前端或进行离线验证，可以关闭后端构建。

## 构建与运行

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/Output/appQtEasyTier
```

所有可执行文件和库会输出到 `build/Output/`。

### 只构建前端

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DBUILD_WITH_DAEMON=OFF
cmake --build build -j
```

关闭 `BUILD_WITH_DAEMON` 后，CMake 不会构建和收集 `qtet-daemon`。这种模式适合离线开发、前端验证和单元测试。

### 从 Gitee 克隆后端

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DCLONE_DAEMON_FROM_GITEE=ON
cmake --build build -j
```

开启 `CLONE_DAEMON_FROM_GITEE` 后，CMake 会给 `scripts/build_daemon.sh` 传入 `--gitee`，从 Gitee 克隆后端源码。

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

## 运行时数据

Linux 下应用配置目录通常位于：

```text
~/.config/qteasytier/QtEasyTier/
```

默认 SQLite 数据库：

```text
~/.config/qteasytier/QtEasyTier/qteasytier.db
```

全局设置文件由 `SettingsStore` 管理，默认使用 `settings3.json`。

## 打包资源

Linux 打包相关文件位于：

```text
assets/package/linux/
```

其中包含 `.desktop` 文件、systemd service、Debian control 文件和打包脚本。

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
<img src="https://qtet.cn/donate/wechat.png" width="220">
<img src="https://qtet.cn/donate/alipay.png" width="220">
</p>

[点击前往赞助详情页面](https://qtet.cn/other/donate/)
