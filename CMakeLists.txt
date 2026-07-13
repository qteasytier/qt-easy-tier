cmake_minimum_required(VERSION 3.16)

project(QtEasyTier VERSION 0.1 LANGUAGES CXX)

set(QTET_OUTPUT_DIR "${CMAKE_BINARY_DIR}/Output")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${QTET_OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${QTET_OUTPUT_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${QTET_OUTPUT_DIR}")
set(QT_QML_OUTPUT_DIRECTORY "${QTET_OUTPUT_DIR}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)


find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Sql
    Network
    Test
    Quick
    Widgets
    Concurrent
    Svg
    QuickDialogs2
)

qt_policy(SET QTP0004 NEW)

qt_standard_project_setup(REQUIRES 6.5)

if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/importedcontent/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/importedcontent)
endif()

# 核心层源文件列表 — 经 qt_add_executable 编译后随主程序链接
# 新文件添加时同步加入此列表
set(CORE_SOURCES
    src/core/config/NetworkConf.cpp
    src/core/config/NetworkConfToml.cpp
    src/core/config/ConfigValidator.cpp
    src/core/repository/DatabaseConnection.cpp
    src/core/repository/NetworkConfigRepository.cpp
    src/core/repository/FavoriteNodeRepository.cpp
    src/core/repository/LogRepository.cpp
    src/core/application/config/ConfigPayloadBuilder.cpp
    src/core/application/config/ConfigPayloadBuilder.h
    src/core/application/config/ConfigCommandService.cpp
    src/core/application/config/ConfigCommandService.h
    src/core/application/config/ConfigImportExportService.cpp
    src/core/application/config/ConfigImportExportService.h
    src/core/application/config/ConfigOperationResult.h
    src/core/application/nodes/PublicServerProvider.cpp
    src/core/application/nodes/PublicServerProvider.h
    src/core/application/settings/AutoStartService.cpp
    src/core/application/settings/AutoStartService.h
    src/core/application/settings/SettingsStore.cpp
    src/core/application/settings/SettingsStore.h
    src/core/application/runtime/ConfigRunState.h
    src/core/application/logging/RepositoryLogSink.cpp
    src/core/application/logging/RepositoryLogSink.h
    src/core/log/LogDispatcher.cpp
    src/core/log/LogDispatcher.h
    src/core/log/LogSink.h
    src/core/log/LogTypes.h
    src/core/service/IpcMessage.cpp
    src/core/service/FrameProtocol.cpp
    src/core/service/DaemonClient.cpp
    src/core/service/DaemonApi.cpp
    src/core/service/DaemonApi.h
    src/core/util/AutoStartHelper.cpp
    src/core/util/DaemonRegisterHelper.cpp
    src/core/util/DaemonRegisterHelper.h
    src/core/util/FontHelper.cpp
    src/core/util/LogHelper.cpp
    src/core/viewmodel/AppState.cpp
    src/core/viewmodel/ConfigListModel.cpp
    src/core/viewmodel/ConfigEditorViewModel.cpp
    src/core/viewmodel/SettingsViewModel.cpp
    src/core/viewmodel/FavoriteNodeViewModel.cpp
    src/core/viewmodel/LogViewModel.cpp
    src/core/viewmodel/nodes/ImportNodesViewModel.cpp
    src/core/viewmodel/nodes/ImportNodesViewModel.h
    src/core/viewmodel/runtime/BackendStatusViewModel.cpp
    src/core/viewmodel/runtime/BackendStatusViewModel.h
    src/core/viewmodel/runtime/NodeInfoModel.cpp
    src/core/viewmodel/runtime/NodeInfoModel.h
    src/core/viewmodel/runtime/RuntimeLogModel.cpp
    src/core/viewmodel/runtime/RuntimeLogModel.h
    src/core/viewmodel/runtime/NetworkPageViewModel.cpp
    src/core/viewmodel/runtime/NetworkPageViewModel.h
    src/core/vpn_manager/VpnController.cpp
    src/core/vpn_manager/VpnManager.cpp
    src/core/vpn_manager/StatusMonitor.cpp
    src/app/AppServices.cpp
    src/app/AppServices.h
    src/app/QmlSingletonRegistrar.cpp
    src/app/QmlSingletonRegistrar.h
)

function(add_qtet_library name)
    qt_add_library(${name} STATIC ${ARGN})
    target_include_directories(${name} PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/ThirdParty/toml++)
endfunction()

add_qtet_library(qtet_log
    src/core/log/LogDispatcher.cpp
    src/core/log/LogDispatcher.h
    src/core/log/LogSink.h
    src/core/log/LogTypes.h
    src/core/util/LogHelper.cpp
    src/core/util/LogHelper.h
)
target_link_libraries(qtet_log PUBLIC Qt6::Core)

add_qtet_library(qtet_config
    src/core/config/NetworkConf.cpp
    src/core/config/NetworkConfToml.cpp
    src/core/config/ConfigValidator.cpp
    src/core/application/config/ConfigPayloadBuilder.cpp
    src/core/application/config/ConfigPayloadBuilder.h
)
target_link_libraries(qtet_config PUBLIC Qt6::Core qtet_log)

add_qtet_library(qtet_repository
    src/core/repository/DatabaseConnection.cpp
    src/core/repository/NetworkConfigRepository.cpp
    src/core/repository/FavoriteNodeRepository.cpp
    src/core/repository/LogRepository.cpp
)
target_link_libraries(qtet_repository PUBLIC Qt6::Core Qt6::Sql qtet_config qtet_log)

add_qtet_library(qtet_service
    src/core/service/IpcMessage.cpp
    src/core/service/FrameProtocol.cpp
    src/core/service/DaemonClient.cpp
    src/core/service/DaemonApi.cpp
    src/core/service/DaemonApi.h
)
target_link_libraries(qtet_service PUBLIC Qt6::Core Qt6::Network qtet_log)

add_qtet_library(qtet_platform
    src/core/util/AutoStartHelper.cpp
    src/core/util/DaemonRegisterHelper.cpp
    src/core/util/DaemonRegisterHelper.h
    src/core/util/FontHelper.cpp
)
target_link_libraries(qtet_platform PUBLIC Qt6::Core Qt6::Widgets qtet_log)

add_qtet_library(qtet_system_tray
    src/core/system_tray/TrayMessageDispatcher.cpp
    src/core/system_tray/TrayMessageDispatcher.h
    src/core/system_tray/TrayMessageHelper.cpp
    src/core/system_tray/TrayMessageHelper.h
    src/core/system_tray/TrayMessageSink.h
    src/core/system_tray/TrayMessageTypes.h
    src/core/system_tray/SystemTrayManager.cpp
    src/core/system_tray/SystemTrayManager.h
)
target_link_libraries(qtet_system_tray PUBLIC Qt6::Core Qt6::Network Qt6::Widgets qtet_service qtet_log)

add_qtet_library(qtet_application
    src/core/application/config/ConfigCommandService.cpp
    src/core/application/config/ConfigCommandService.h
    src/core/application/config/ConfigImportExportService.cpp
    src/core/application/config/ConfigImportExportService.h
    src/core/application/config/ConfigOperationResult.h
    src/core/application/logging/RepositoryLogSink.cpp
    src/core/application/logging/RepositoryLogSink.h
    src/core/application/nodes/PublicServerProvider.cpp
    src/core/application/nodes/PublicServerProvider.h
    src/core/application/runtime/ConfigRunState.h
    src/core/application/settings/AutoStartService.cpp
    src/core/application/settings/AutoStartService.h
    src/core/application/settings/SettingsStore.cpp
    src/core/application/settings/SettingsStore.h
)
target_link_libraries(qtet_application PUBLIC Qt6::Core qtet_config qtet_repository qtet_service qtet_platform qtet_log)

add_qtet_library(qtet_vpn
    src/core/vpn_manager/VpnController.cpp
    src/core/vpn_manager/VpnManager.cpp
    src/core/vpn_manager/StatusMonitor.cpp
)
target_link_libraries(qtet_vpn PUBLIC Qt6::Core Qt6::Concurrent qtet_application qtet_config qtet_repository qtet_service qtet_log)

add_qtet_library(qtet_viewmodel
    src/core/viewmodel/AppState.cpp
    src/core/viewmodel/ConfigListModel.cpp
    src/core/viewmodel/ConfigEditorViewModel.cpp
    src/core/viewmodel/SettingsViewModel.cpp
    src/core/viewmodel/FavoriteNodeViewModel.cpp
    src/core/viewmodel/LogViewModel.cpp
    src/core/viewmodel/nodes/ImportNodesViewModel.cpp
    src/core/viewmodel/nodes/ImportNodesViewModel.h
    src/core/viewmodel/runtime/BackendStatusViewModel.cpp
    src/core/viewmodel/runtime/BackendStatusViewModel.h
    src/core/viewmodel/runtime/NodeInfoModel.cpp
    src/core/viewmodel/runtime/NodeInfoModel.h
    src/core/viewmodel/runtime/RuntimeLogModel.cpp
    src/core/viewmodel/runtime/RuntimeLogModel.h
    src/core/viewmodel/runtime/NetworkPageViewModel.cpp
    src/core/viewmodel/runtime/NetworkPageViewModel.h
)
target_link_libraries(qtet_viewmodel PUBLIC Qt6::Core Qt6::Sql qtet_application qtet_config qtet_repository qtet_service qtet_platform qtet_vpn qtet_log)

add_qtet_library(qtet_appsupport
    src/app/AppLaunchOptions.cpp
    src/app/AppLaunchOptions.h
    src/app/AppServices.cpp
    src/app/AppServices.h
    src/app/QmlSingletonRegistrar.cpp
    src/app/QmlSingletonRegistrar.h
)
target_link_libraries(qtet_appsupport PUBLIC Qt6::Core Qt6::Quick qtet_viewmodel qtet_application qtet_repository qtet_service qtet_platform qtet_vpn qtet_log)
target_link_libraries(qtet_appsupport PUBLIC qtet_system_tray)

qt_add_executable(appQtEasyTier
    src/main.cpp
    assets/resources.qrc
)

target_include_directories(appQtEasyTier PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/ThirdParty/toml++)

set_target_properties(appQtEasyTier PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

find_program(BASH_EXECUTABLE bash REQUIRED)
find_program(GIT_EXECUTABLE git)

set(QTET_DAEMON_SRC_DIR "${CMAKE_BINARY_DIR}/qtet-daemon")
set(QTET_DAEMON_OUTPUT_DIR "${QTET_DAEMON_SRC_DIR}/build/Output")

if(WIN32)
    set(QTET_DAEMON_BIN_NAME "qtet-daemon.exe")
    set(QTET_DAEMON_RENAMED_BIN "qtet-daemon.exe")
else()
    set(QTET_DAEMON_BIN_NAME "qtet-daemon")
    set(QTET_DAEMON_RENAMED_BIN "qtet-daemon")
endif()

if(GIT_EXECUTABLE)
    add_custom_target(qtet_daemon ALL
        COMMAND "${CMAKE_COMMAND}" -E env
                "CLONE_DIR=qtet-daemon"
                "BUILD_TYPE=$<IF:$<CONFIG:Debug>,Debug,Release>"
                "${BASH_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/build_daemon.sh"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "Building qtet-daemon runtime"
        VERBATIM
    )

    add_dependencies(appQtEasyTier qtet_daemon)

    add_custom_command(TARGET appQtEasyTier POST_BUILD
        COMMAND "${BASH_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/collect-daemon.sh"
                "${QTET_DAEMON_OUTPUT_DIR}"
                "${QTET_OUTPUT_DIR}"
                "${QTET_DAEMON_BIN_NAME}"
                "${QTET_DAEMON_RENAMED_BIN}"
        COMMENT "Collecting qtet-daemon runtime into output directory"
        VERBATIM
    )
else()
    message(WARNING "git not found, skipping qtet-daemon build. Please place qtet-daemon runtime in the output directory manually.")
endif()

target_link_libraries(appQtEasyTier PRIVATE
    Qt6::Core
    Qt6::Sql
    Qt6::Network
    Qt6::Quick
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Svg
    Qt6::QuickDialogs2
    qtet_appsupport
)

qt_add_qml_module(appQtEasyTier
    URI QtEasyTier
    VERSION 1.0
    RESOURCE_PREFIX "/"
    QML_FILES
        src/qml/Main.qml
        src/qml/components/Sidebar.qml
        src/qml/components/SidebarItem.qml
        src/qml/components/PageContainer.qml
        src/qml/components/Card.qml
        src/qml/components/EditableList.qml
        src/qml/components/EditableListItem.qml
        src/qml/components/QtETabWidget.qml
        src/qml/components/ServerListItem.qml
        src/qml/components/ProxyNetworkListItem.qml
        src/qml/components/ImportNodesDialog.qml
        src/qml/components/Theme.qml
        src/qml/pages/LeftPanel.qml
        src/qml/pages/NetworkPage.qml
        src/qml/pages/NetworkOptions.qml
        src/qml/pages/NetworkRuningStatus.qml
        src/qml/pages/SettingsPage.qml
        src/qml/pages/FavoriteNodesPage.qml
        src/qml/pages/LogPage.qml
)

enable_testing()
add_subdirectory(tests)
