# =====================================================================
# QtEasyTier CNB Linux 构建环境镜像
#
# 本镜像把系统依赖与 Qt 6.8.3 的安装固化到构建环境中，供 CNB
# docker.build 动态构建镜像机制缓存复用（平台按哈希自动缓存到制品库，
# 后续构建直接复用，无需重复安装依赖）。
#
# 架构差异通过 buildArgs 注入：
#   - amd64: QT_HOST=linux             QT_ARCH=linux_gcc_64    QT_DIR_NAME=gcc_64
#   - arm64: QT_HOST=linux_arm64       QT_ARCH=linux_gcc_arm64 QT_DIR_NAME=gcc_arm64
# =====================================================================

FROM ubuntu:24.04

# ---- 架构相关构建参数（由流水线 buildArgs 注入） ----
ARG QT_VERSION=6.8.3
ARG QT_HOST=linux
ARG QT_ARCH=linux_gcc_64
ARG QT_DIR_NAME=gcc_64

ENV DEBIAN_FRONTEND=noninteractive

# ---- 系统依赖（与旧 .st-setup-env 安装列表保持一致） ----
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
      python3 python3-pip \
    && rm -rf /var/lib/apt/lists/*

# ---- 下载并安装 Qt ${QT_VERSION}（${QT_ARCH}，宿主 ${QT_HOST}） ----
# Ubuntu 24.04 的 Python 3.12 受 PEP 668 限制，禁止 pip 全局安装；
# 本环境为构建专用，--break-system-packages 可安全绕过。
RUN pip3 install --no-cache-dir --break-system-packages aqtinstall \
    && aqt install-qt ${QT_HOST} desktop ${QT_VERSION} ${QT_ARCH} -O /opt/qt \
    && rm -rf /root/.cache

# ---- 固化 Qt 环境：前缀、PATH、动态库路径 ----
ENV Qt6_DIR=/opt/qt/${QT_VERSION}/${QT_DIR_NAME}
ENV PATH="${Qt6_DIR}/bin:$PATH"
ENV LD_LIBRARY_PATH="${Qt6_DIR}/lib"

# ---- 构建校验（确认 Qt 可用） ----
RUN qmake -query QT_VERSION \
    && qmake -query QT_INSTALL_PREFIX
