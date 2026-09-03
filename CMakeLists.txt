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
# 按上游 README 静态集成方式：BUILD_SHARED_LIBS=OFF 使 SwbControls 及其
# QML 插件成为静态库，全部编译产物链入可执行文件，部署不再携带 libSwbControls.so；
# 静态集成需同时链接自动生成的 SwbControlsplugin（QML 插件与类型注册）。
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(ThirdParty/SWB-QML-UI)

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

qt_add_executable(appQtEasyTier
    src/main.cpp
    assets/resources.qrc
    ThirdParty/SWB-QML-UI/assets/theme.qrc
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
    SwbControls
    SwbControlsplugin
    qtet_appsupport
)

if(BUILD_WITH_DDE)
    target_compile_definitions(appQtEasyTier PRIVATE QTET_WITH_DDE)
endif()

# ============================================
# QML 源:按目录组织(src/qml/*/CMakeLists.txt 各自收集,参考 SWB-QML-UI 的方式)
# 列表元素为相对 src/qml/ 的路径,兼作 QT_RESOURCE_ALIAS(qrc:/QtEasyTier/...)
# ============================================
set(QTET_QML_FILES Main.qml)
set(QTET_QML_FILES_ABS "")
set(QTET_QML_MASK_SHARED "")
set(QTET_QML_MASK_DDE "")
add_subdirectory(src/qml/components)
add_subdirectory(src/qml/pages)
add_subdirectory(src/qml/dde)

# Card/IconToolButton/EditableListItem/ErrorDialog/ImportNodesDialog/
# CredentialManageDialog 由 DDE 版同名组件按模式蒙版替换
# (GenerateCredentialDialog 仅存于 DDE 蒙版集:共享版新增凭证功能已并入
#  CredentialManageDialog,DDE 版仍为独立生成对话框)
if(BUILD_WITH_DDE)
    list(APPEND QTET_QML_FILES ${QTET_QML_MASK_DDE})
else()
    list(APPEND QTET_QML_FILES ${QTET_QML_MASK_SHARED})
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
# qtet_appcore / qtet_system_tray 中的 tr() 不在其中，这里显式补扫
add_custom_target(update-translations
    COMMAND Qt6::lupdate
            "${CMAKE_SOURCE_DIR}/src/main.cpp"
            "${CMAKE_SOURCE_DIR}/src/core/viewmodels/ConfigEditorViewModel.cpp"
            "${CMAKE_SOURCE_DIR}/src/core/viewmodels/runtime/BackendStatusViewModel.cpp"
            "${CMAKE_SOURCE_DIR}/src/core/settings"
            "${CMAKE_SOURCE_DIR}/src/app"
            "${CMAKE_SOURCE_DIR}/src/system_tray"
            ${QTET_QML_FILES_ABS}
            -ts
            "${CMAKE_SOURCE_DIR}/translations/QtEasyTier_zh_TW.ts"
            "${CMAKE_SOURCE_DIR}/translations/QtEasyTier_en.ts"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "运行 lupdate 更新翻译源文件 (.ts)"
    VERBATIM
)

enable_testing()
add_subdirectory(tests)
