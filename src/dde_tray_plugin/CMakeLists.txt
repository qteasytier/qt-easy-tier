add_qtet_library(qtet_dde_tray_status
    TrayStatusService.cpp
    TrayStatusService.h
    TrayStatusTypes.h
)
target_link_libraries(qtet_dde_tray_status PUBLIC
    Qt6::Core
    Qt6::Network
    Qt6::Sql
    qtet_daemon_service
    qtet_sqlite_repository
    qtet_config
    qtet_log
)

set(DDE_TRAY_LOADER_SOURCE_DIR "" CACHE PATH
    "dde-tray-loader 源码目录（含 interfaces/）；留空则使用系统安装的 dde-tray-loader-dev 头文件")
# Deepin 25 loader 扫描的绝对路径，安装时忽略 CMAKE_INSTALL_PREFIX；
# 临时验证可覆盖：-DDDE_TRAY_PLUGIN_INSTALL_DIR=/tmp/xxx
set(DDE_TRAY_PLUGIN_INSTALL_DIR "/usr/lib/dde-dock/plugins/system-trays" CACHE PATH
    "DDE 托盘插件安装目录（绝对路径）")

# 接口头文件来源：优先源码树模式（DDE_TRAY_LOADER_SOURCE_DIR/interfaces），
# 其次使用系统 apt 包 dde-tray-loader-dev 安装的头文件（/usr/include/dde-dock）。
if(DDE_TRAY_LOADER_SOURCE_DIR)
    set(DDE_TRAY_INTERFACE_DIR "${DDE_TRAY_LOADER_SOURCE_DIR}/interfaces")
else()
    set(DDE_TRAY_INTERFACE_DIR "/usr/include/dde-dock")
endif()

foreach(interface_file IN ITEMS
    pluginsiteminterface.h
    pluginsiteminterface_v2.h
    pluginproxyinterface.h
    constants.h)
    if(NOT EXISTS "${DDE_TRAY_INTERFACE_DIR}/${interface_file}")
        message(FATAL_ERROR
            "DDE 托盘接口文件不存在：${DDE_TRAY_INTERFACE_DIR}/${interface_file}。\n"
            "请安装开发包：sudo apt install dde-tray-loader-dev\n"
            "或提供源码路径：-DDDE_TRAY_LOADER_SOURCE_DIR=/path/to/dde-tray-loader")
    endif()
endforeach()

qt_add_plugin(qtetDdeTrayPlugin
        QtEasyTierTrayPlugin.cpp
        QtEasyTierTrayPlugin.h
        TrayStatusWidget.cpp
        TrayStatusWidget.h
        tray-plugin.json
        resources.qrc
    )
target_include_directories(qtetDdeTrayPlugin PRIVATE
        "${DDE_TRAY_INTERFACE_DIR}"
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
target_link_libraries(qtetDdeTrayPlugin PRIVATE
        Qt6::Core Qt6::Network Qt6::Sql Qt6::Widgets
        qtet_dde_tray_status
    )
install(TARGETS qtetDdeTrayPlugin
        LIBRARY DESTINATION "${DDE_TRAY_PLUGIN_INSTALL_DIR}")
install(FILES tray-plugin.json
        DESTINATION "${DDE_TRAY_PLUGIN_INSTALL_DIR}")
