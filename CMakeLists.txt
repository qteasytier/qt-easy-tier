cmake_minimum_required(VERSION 3.16)

project(QtEasyTier VERSION 3.0.2 LANGUAGES CXX)
set(EASYTIER_FFI_VER 2.6.4)

# 向 C++ 侧暴露版本号
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/application/AppVersion.h.in
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
option(CLONE_DAEMON_FROM_GITEE "Clone qtet-daemon from Gitee instead of GitHub" OFF)

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
)

if(COMMAND qt_policy)
    qt_policy(SET QTP0004 NEW)
endif()

qt_standard_project_setup(REQUIRES 6.8)

if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/importedcontent/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/importedcontent)
endif()

function(add_qtet_library name)
    qt_add_library(${name} STATIC ${ARGN})
    target_include_directories(${name} PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/generated ${CMAKE_SOURCE_DIR}/ThirdParty/toml++)
endfunction()

add_subdirectory(src/core/log)
add_subdirectory(src/core/config)
add_subdirectory(src/core/repository)
add_subdirectory(src/core/service)
add_subdirectory(src/core/util)
add_subdirectory(src/core/system_tray)
add_subdirectory(src/core/application)
add_subdirectory(src/core/vpn_manager)
add_subdirectory(src/core/viewmodel)
add_subdirectory(src/app)

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
    qtet_appsupport
)

set(QTET_QML_FILES
    src/qml/Main.qml
    src/qml/components/Sidebar.qml
    src/qml/components/SidebarItem.qml
    src/qml/components/PageContainer.qml
    src/qml/components/Card.qml
    src/qml/components/IconToolButton.qml
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

foreach(qml_file IN LISTS QTET_QML_FILES)
    file(RELATIVE_PATH qml_resource_alias "${CMAKE_CURRENT_SOURCE_DIR}/src/qml" "${CMAKE_CURRENT_SOURCE_DIR}/${qml_file}")
    set_source_files_properties(${qml_file} PROPERTIES QT_RESOURCE_ALIAS "${qml_resource_alias}")
endforeach()

qt_add_qml_module(appQtEasyTier
    URI QtEasyTier
    VERSION 1.0
    RESOURCE_PREFIX "/"
    QML_FILES ${QTET_QML_FILES}
)

enable_testing()
add_subdirectory(tests)
