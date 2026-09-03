cmake_minimum_required(VERSION 3.16)

project(QtEasyTier VERSION 3.2.1 LANGUAGES CXX)
set(EASYTIER_FFI_VER 2.6.4)
set(QTET_DAEMON_VER 0.1.1)

# 向 C++ 侧暴露版本号
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/AppVersion.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/AppVersion.h
    @ONLY
)

set(QTET_OUTPUT_DIR "${CMAKE_BINARY_DIR}/Output")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${QTET_OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${QTET_OUTPUT_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${QTET_OUTPUT_DIR}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 构建后端可选参数
option(BUILD_WITH_DAEMON "Build and collect qtet-daemon runtime" ON)
# qtet-daemon 克隆来源：GITHUB / CNB，默认 GITHUB
set(CLONE_DAEMON_FROM "GITHUB" CACHE STRING "Clone qtet-daemon from GITHUB or CNB")
set_property(CACHE CLONE_DAEMON_FROM PROPERTY STRINGS GITHUB CNB)

# DDE 桌面适配开关：开启后构建基于 DDE（Deepin v25）的前端页面
option(BUILD_WITH_DDE "构建基于 DDE 的前端页面" OFF)
option(BUILD_WITH_DDE_TRAY_PLUGIN "构建 DDE 托盘网络状态插件" OFF)

include(cmake/QtEasyTierDaemon.cmake)

find_package(Qt6 6.8 REQUIRED COMPONENTS
    Core
    Sql
    Network
    Test
    Quick
    QuickControls2
    Widgets
    Concurrent
    Svg
    QuickDialogs2
    QuickEffects
    LinguistTools
)

if(COMMAND qt_policy)
    qt_policy(SET QTP0004 NEW)
endif()

# 指定（例如 -DCMAKE_PREFIX_PATH=/path/to/openssl-3.5.7），默认静态链接 libcrypto.a。
set(OPENSSL_USE_STATIC_LIBS TRUE CACHE BOOL "优先使用静态 OpenSSL 库")
find_package(OpenSSL REQUIRED)

qt_standard_project_setup(REQUIRES 6.8)

if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/importedcontent/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/importedcontent)
endif()

# SWB-QML-UI（shadcn 风格纯 QML 组件库，模块 URI 为 SwbControls），
# 本地修改记录见 ThirdParty/SWB-QML-UI/PATCHES.md。
if(NOT BUILD_WITH_DDE)
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory(ThirdParty/SWB-QML-UI)
endif()

function(add_qtet_library name)
    qt_add_library(${name} STATIC ${ARGN})
    # 静态库可能被链入共享模块（如 DDE 托盘插件 libqtetDdeTrayPlugin.so），
    # 需以位置无关代码（-fPIC）编译，否则 arm64 链接共享库时报 relocation 错误
    set_target_properties(${name} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_include_directories(${name} PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/generated ${CMAKE_SOURCE_DIR}/ThirdParty/toml++)
endfunction()

add_subdirectory(src/log)
add_subdirectory(src/config)
add_subdirectory(src/sqlite_repository)
add_subdirectory(src/daemon_service)
add_subdirectory(src/platform)
add_subdirectory(src/system_tray)
add_subdirectory(src/core)
add_subdirectory(src/app)
if(BUILD_WITH_DDE_TRAY_PLUGIN)
    add_subdirectory(src/dde_tray_plugin)
endif()

if(WIN32)
    set(QTET_WINDOWS_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/favicon/qtet.ico")
    set(QTET_WINDOWS_RC "${CMAKE_CURRENT_BINARY_DIR}/appQtEasyTier.rc")

    if(NOT EXISTS "${QTET_WINDOWS_ICON}")
        message(FATAL_ERROR "Windows icon file not found: ${QTET_WINDOWS_ICON}")
    endif()

    file(WRITE "${QTET_WINDOWS_RC}"
        "IDI_ICON1 ICON \"${QTET_WINDOWS_ICON}\"\n"
    )
    set_source_files_properties("${QTET_WINDOWS_RC}" PROPERTIES GENERATED TRUE)
endif()

# SwbControls 主题图标资源（月/太阳等）仅共享前端构建嵌入
set(QTET_APP_EXTRA_SOURCES "")
if(NOT BUILD_WITH_DDE)
    list(APPEND QTET_APP_EXTRA_SOURCES ThirdParty/SWB-QML-UI/assets/theme.qrc)
endif()

qt_add_executable(appQtEasyTier
    src/main.cpp
    assets/resources.qrc
    ${QTET_APP_EXTRA_SOURCES}
    ${QTET_WINDOWS_RC}
)

target_include_directories(appQtEasyTier PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/generated ${CMAKE_SOURCE_DIR}/ThirdParty/toml++)

set_target_properties(appQtEasyTier PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

qtet_configure_daemon()
qtet_attach_daemon_to_app(appQtEasyTier)

# SwbControls 静态库仅共享前端构建链接；DDE 前端不引用 SwbControls
set(QTET_APP_EXTRA_LIBS "")
if(NOT BUILD_WITH_DDE)
    list(APPEND QTET_APP_EXTRA_LIBS SwbControls SwbControlsplugin)
endif()

target_link_libraries(appQtEasyTier PRIVATE
    Qt6::Core
    Qt6::Sql
    Qt6::Network
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Svg
    Qt6::QuickDialogs2
    ${QTET_APP_EXTRA_LIBS}
    qtet_appsupport
)

if(BUILD_WITH_DDE)
    target_compile_definitions(appQtEasyTier PRIVATE QTET_WITH_DDE)
endif()

# ============================================
# QML 源:按目录组织(src/qml/*/CMakeLists.txt 各自收集,参考 SWB-QML-UI 的方式)
# 列表元素为相对 src/qml/ 的路径,兼作 QT_RESOURCE_ALIAS(qrc:/QtEasyTier/...)
#
# 前端整树条件选择(不再使用同名蒙版替换):
#   - BUILD_WITH_DDE=ON  编入 dde/ 完整 DTK 前端树(DdeMain + 自有 pages/
#     components/config_form,全部 org.deepin.dtk 控件实现)
#   - BUILD_WITH_DDE=OFF 编入 Main.qml + 共享 components/pages 树(SwbControls)
# 两棵树存在同名类型(NetworkPage/Card/Sidebar 等),同一模块内重名类型会
# 冲突,绝不混编,只能整树二选一。
# Theme.qml(纯 QtObject 状态色)与 PageContainer.qml(纯 QtQuick 页面容器)
# 不依赖任何控件库,由两种前端共用,无条件编入;PageContainer 按类型名引用
# 页面,DDE 构建下自动解析到 dde/pages/ 同名页面。
# ============================================
set(QTET_QML_FILES
    components/Theme.qml
    components/PageContainer.qml
)
set(QTET_QML_FILES_ABS "")
set(QTET_QML_SHARED_FILES "")
set(QTET_QML_DDE_FILES "")
add_subdirectory(src/qml/components)
add_subdirectory(src/qml/pages)
add_subdirectory(src/qml/dde)

if(BUILD_WITH_DDE)
    list(APPEND QTET_QML_FILES ${QTET_QML_DDE_FILES})
else()
    list(APPEND QTET_QML_FILES Main.qml ${QTET_QML_SHARED_FILES})
endif()

foreach(qml_rel IN LISTS QTET_QML_FILES)
    set_source_files_properties("${CMAKE_SOURCE_DIR}/src/qml/${qml_rel}"
        PROPERTIES QT_RESOURCE_ALIAS "${qml_rel}")
    list(APPEND QTET_QML_FILES_ABS "${CMAKE_SOURCE_DIR}/src/qml/${qml_rel}")
endforeach()

qt_add_qml_module(appQtEasyTier
    URI QtEasyTier
    VERSION 1.0
    RESOURCE_PREFIX "/"
    QML_FILES ${QTET_QML_FILES_ABS}
)

# ============================================
# 翻译：源语言为简体中文（qsTr/tr 直接写中文），
# 繁體/English 经 translations/*.ts 翻译并内嵌为 :/i18n/QtEasyTier_*.qm
# ============================================
qt_add_translations(appQtEasyTier
    TS_FILES translations/QtEasyTier_zh_TW.ts translations/QtEasyTier_en.ts
    RESOURCE_PREFIX "/i18n"
    QM_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/translations"
)

# lupdate 覆盖扫描：qt_add_translations 只扫描 app target 源（QML + main.cpp），
# qtet_appcore / qtet_system_tray 中的 tr() 不在其中，这里显式补扫。
# QML 侧同时覆盖共享树与 DDE 树（.ts 服务两种前端）；不用按构建模式的
# QTET_QML_FILES（单树扫描会把另一棵树的同源条目标记为 vanished）。
file(GLOB_RECURSE QTET_LUPDATE_QML "${CMAKE_SOURCE_DIR}/src/qml/*.qml")
add_custom_target(update-translations
    COMMAND Qt6::lupdate
            "${CMAKE_SOURCE_DIR}/src/main.cpp"
            "${CMAKE_SOURCE_DIR}/src/core/viewmodels/ConfigEditorViewModel.cpp"
            "${CMAKE_SOURCE_DIR}/src/core/viewmodels/runtime/BackendStatusViewModel.cpp"
            "${CMAKE_SOURCE_DIR}/src/core/settings"
            "${CMAKE_SOURCE_DIR}/src/app"
            "${CMAKE_SOURCE_DIR}/src/system_tray"
            ${QTET_LUPDATE_QML}
            -ts
            "${CMAKE_SOURCE_DIR}/translations/QtEasyTier_zh_TW.ts"
            "${CMAKE_SOURCE_DIR}/translations/QtEasyTier_en.ts"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "运行 lupdate 更新翻译源文件 (.ts)"
    VERBATIM
)

enable_testing()
add_subdirectory(tests)
