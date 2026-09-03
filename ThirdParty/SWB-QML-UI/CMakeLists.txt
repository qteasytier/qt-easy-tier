cmake_minimum_required(VERSION 3.21)
project(SWB-QML-UI VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# Qt installation prefix; override it with -DQT_PATH=...
# [QtEasyTier 补丁] 默认值改为空且仅在路径存在时追加，避免向宿主构建泄漏无效的 Windows 路径
set(QT_PATH "" CACHE PATH "Qt 安装前缀")
if(QT_PATH AND EXISTS "${QT_PATH}")
    list(APPEND CMAKE_PREFIX_PATH "${QT_PATH}")
endif()

# Default to Release when no build type is specified.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# Build shared libraries by default to match a shared Qt installation.
option(BUILD_SHARED_LIBS "构建动态库（关则静态库）" ON)
# Build the example by default only when this is the top-level project.
option(SWB_BUILD_EXAMPLES "编译示例程序" ${PROJECT_IS_TOP_LEVEL})

# [QtEasyTier 补丁] 移除 QuickEffects 组件：SwbProgressBar 已改为不依赖
# QtQuick.Effects（见源码补丁），且 aqt 默认归档与 deepin crimson 源的 Qt 6.8
# 均不含 Qt6QuickEffects 开发配置。
find_package(Qt6 REQUIRED COMPONENTS Quick)
# [QtEasyTier 补丁] 上游为 REQUIRES 6.10；本仓库与 CI 基于 Qt 6.8。
# 库内实际使用的版本敏感 API（Popup.popupType、T.ContextMenu、QtQuick.VectorImage）
# 均自 Qt 6.8 起可用（VectorImage 在 6.8 为 tech preview），故降级为 6.8。
qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory(components)
if(SWB_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
