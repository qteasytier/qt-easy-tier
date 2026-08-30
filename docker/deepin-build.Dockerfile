# =====================================================================
# QtEasyTier CNB Deepin 构建环境镜像
#
# 本镜像在 deepin v25 (crimson) 基础上安装系统依赖、Qt 6.8 开发包、
# DTK6 开发包与 DDE 托盘插件开发包，供 CNB docker.build 动态构建
# 镜像机制缓存复用（平台按哈希自动缓存到制品库）。
#
# 架构差异通过 buildArgs 注入：
#   - amd64: BASE_IMAGE=linuxdeepin/deepin:crimson
#   - arm64: BASE_IMAGE=linuxdeepin/deepin:crimson-arm64
#
# 与通用 Linux 构建环境（docker/linux-build.Dockerfile）不同：
#   - 不通过 aqt 下载 Qt，直接使用 crimson 软件源自带的 Qt 6.8
#     （deepin 25 系统 Qt 已为 6.8，满足项目 find_package(Qt6 6.8) 要求），
#     因此不设置 Qt6_DIR/PATH/LD_LIBRARY_PATH，find_package 走默认路径。
#   - 额外安装 DTK6 与 dde-tray-loader 开发包，用于构建
#     BUILD_WITH_DDE（DTK QML 前端）与 BUILD_WITH_DDE_TRAY_PLUGIN（托盘插件）。
# =====================================================================

ARG BASE_IMAGE=linuxdeepin/deepin:crimson
FROM ${BASE_IMAGE}

ENV DEBIAN_FRONTEND=noninteractive

# ---- 系统依赖（通用 Linux 镜像列表 + Qt6 开发包 + DTK6 + DDE 托盘插件开发包） ----
# libdtk6declarative-dev 依赖并带入 libdtk6core-dev/libdtk6gui-dev；
# qml6-module-qtquick-controls2-styles-chameleon 提供 org.deepin.dtk QML 模块
# （构建 QML 编译与运行测试所需）。
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
      build-essential cmake ninja-build git bash file dpkg-dev ca-certificates \
      libgl1-mesa-dev libxkbcommon-dev libxkbcommon-x11-dev \
      libfontconfig1-dev libdbus-1-dev libfreetype6-dev \
      libx11-dev libxcb1-dev libxext-dev libxfixes-dev libxi-dev \
      libxrender-dev libxcb-glx0-dev libxcb-keysyms1-dev \
      libxcb-image0-dev libxcb-shm0-dev libxcb-icccm4-dev \
      libxcb-sync-dev libxcb-xfixes0-dev libxcb-shape0-dev \
      libxcb-randr0-dev libxcb-render-util0-dev libxcb-util-dev \
      libxcb-cursor-dev libxcb-xinerama0-dev libxcb-xkb-dev \
      libxcb-xinput-dev libegl1-mesa-dev libgles2-mesa-dev \
      libglu1-mesa-dev libpng-dev libssl-dev \
      libglib2.0-0 libglib2.0-dev libicu-dev libdouble-conversion3 \
      libpcre2-16-0 libmd4c0 libzstd1 \
      python3 \
      qt6-base-dev qt6-declarative-dev qt6-svg-dev \
      libdtk6core-dev libdtk6gui-dev libdtk6declarative-dev \
      qml6-module-qtquick-controls2-styles-chameleon \
      dde-tray-loader-dev \
    && rm -rf /var/lib/apt/lists/*

# ---- 构建校验（确认 Qt 6.8 可用） ----
RUN qmake -query QT_VERSION \
    && qmake -query QT_INSTALL_PREFIX
