# =====================================================================
# QtEasyTier CNB Deepin 构建环境镜像
#
# 本镜像在 deepin v25 (crimson) 基础上安装系统依赖、Qt 6.8 开发包、
# DTK6 开发包与 DDE 托盘插件开发包，供 CNB docker.build 动态构建
# 架构差异通过 buildArgs 注入：
#   - amd64: BASE_IMAGE=linuxdeepin/deepin:crimson
#   - arm64: BASE_IMAGE=linuxdeepin/deepin:crimson-arm64

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
      libpcre2-16-0 libmd4c0 libzstd1 libboost-all-dev \
      python3 \
      qml6-module-qtquick-dialogs \
      qt6-base-dev qt6-declarative-dev qt6-svg-dev libqt6sql6-sqlite \
      libdtk6core-dev libdtk6gui-dev libdtk6declarative-dev \
      qml6-module-qtquick-controls2-styles-chameleon \
      dde-tray-loader-dev \
    && rm -rf /var/lib/apt/lists/*
