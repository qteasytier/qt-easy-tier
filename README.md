# QtEasyTier

## 项目简介

QtEasyTier 是一个基于 Qt 框架开发的异地组网工具，用于创建和管理虚拟网络连接。
它提供了直观的图形界面，帮助用户轻松配置和管理虚拟网络，实现跨网络设备的安全通信。
程序后端为 EasyTier 核心，EasyTier 不同于传统工具，是一个去中心化的组网方案，无需依赖中心服务器，
每个节点平等对立，为用户提供更加安全，可靠，低成本的异地组网服务。

![QtEasyTier 界面展示](assets/qteasytier0.png)

## 项目特点

- 快速: 程序使用纯 Qt C++ 开发，无 Chromium 无 Webview，日常使用前端占用不超过50MB，运行高效快速。
- 美观: UI样式采用移植自 KDE 的 Breeze 样式，提供简洁美观现代化的用户界面。
- 简单: QtEasyTier 支持一键联机，小白也能轻松使用。
- 丰富：常规联机支持 EasyTier 大部分功能，按需定制你的虚拟网络。
- 安全: 后端为 EasyTier，一个简单、安全、去中心化的异地组网方案。

## 平台支持

- Windows 10/11
- 今后计划支持 Linux
- Mac 目前暂无支持计划（主要是作者没有Macbook），但欢迎大佬参与贡献

## 快速上手

### 编译安装

*可以在Release处直接下载预编译的二进制文件*

#### 环境要求
- Qt 6（推荐6.10.1）
- CMake 3.5 或更高版本
- llvm-mingw 编译器，支持 C++ 20+（Windows）

#### 编译步骤
1. 克隆项目仓库
```bash
git clone https://gitee.com/viagrahuang/qt-easy-tier.git
cd qt-easy-tier
```

2. 创建构建目录
```bash
mkdir build
cd build
```

3. 编译并安装项目
```bash
# 注意:构建QtEasyTier有两种模式
#   1. 正常模式, 配置保存在系统路径下
#   2. 便携模式, 配置保存在程序目录下, 构建命令加上 -DSAVE_CONF_IN_APP_DIR=true
#   ps: 为了不污染系统环境, 便携模式构建的程序禁用开机自启
cmake  ..
cmake --build . --config Release
cmake --install . --config Release
```

### 简单开始使用

#### 常规使用
- 双击 QtEasyTier.exe 启动应用程序
- 点击左下角加号创建网络配置
- 输入网络名称、用户名、密码等基本信息
- 配置高级选项（可选）
- 点击 "运行网络" 按钮即可启动网络连接

#### 一键联机
*QtEasyTier1.1.0新增功能*
- 启动程序后点击左下角一键联机按钮
- 房主勾选想添加服务器（可选）后点击运行按钮
- 将获得的房间号复制发给房客
- 房客选择相同服务器输入房间号并点击加入按钮即可加入网络

## 本程序依赖或使用的相关项目

### EasyTier

一个由 Rust 和 Tokio 驱动的简单、安全、去中心化的异地组网方案
- 官网：[https://easytier.cn/](https://easytier.cn/)

### Qt Framework

Qt 是一个跨平台的 C++ 应用程序开发框架，用于创建图形用户界面（GUI）和其他应用程序。
- 官网：[https://www.qt.io/](https://www.qt.io/)

### Breeze

Breeze 是 KDE Plasma 桌面环境的默认主题，本程序移植了其适用于 Qt 框架的样式库，为用户提供美观的界面。
- 官网：[https://kde.org/](https://kde.org/)

## 联系作者
- 项目地址：https://gitee.com/viagrahuang/qt-easy-tier
- 问题反馈：欢迎提交 Issue 和 PR
- 交流方式：作者混迹于EasyTier支持3群，欢迎加入交流：957189589

## 许可证

本项目采用 GNU General Public License v3.0 (GPLv3) 许可证。详情请见 LICENSE 文件。

## 赞助与支持

软件开发不易, 您的赞助与支持是对本项目持续开发和维护的重要动力，也是对作者持续努力的认可。
如果您认为本项目对您有帮助，请考虑赞助本项目。

赞助方式：<br>
微信支付、支付宝

<p>
<img src="assets/wechat.webp" width="220">
<img src="assets/alipay.webp" width="220">
</p>

[点击前往赞助详情页面](https://qtet.070219.xyz/other/donate/)
