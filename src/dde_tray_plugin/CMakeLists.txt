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

# 安装目录：dde-dock 扫描/读取的标准绝对路径，安装时忽略 CMAKE_INSTALL_PREFIX；
# 需要更换安装路径时直接指定：-DDDE_TRAY_PLUGIN_INSTALL_DIR=/xxx -DDDE_DCC_ICON_INSTALL_DIR=/yyy
set(DDE_TRAY_PLUGIN_INSTALL_DIR "/usr/lib/dde-dock/plugins/system-trays" CACHE PATH
    "DDE 托盘插件安装目录（绝对路径）")
set(DDE_DCC_ICON_INSTALL_DIR "/usr/share/dde-dock/icons/dcc-setting" CACHE PATH
    "DDE 控制中心插件图标安装目录（绝对路径）")

# 接口头文件：系统 apt 包 dde-tray-loader-dev 安装的硬编码路径（/usr/include/dde-dock）
set(DDE_TRAY_INTERFACE_DIR "/usr/include/dde-dock")

foreach(interface_file IN ITEMS
    pluginsiteminterface.h
    pluginsiteminterface_v2.h
    pluginproxyinterface.h
    constants.h)
    if(NOT EXISTS "${DDE_TRAY_INTERFACE_DIR}/${interface_file}")
        message(FATAL_ERROR
            "DDE 托盘接口文件不存在：${DDE_TRAY_INTERFACE_DIR}/${interface_file}。\n"
            "请安装开发包：sudo apt install dde-tray-loader-dev")
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
# 插件构建产物输出到 Output/Plugins，与应用本体分离，便于打包脚本单独收集
set_target_properties(qtetDdeTrayPlugin PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${QTET_OUTPUT_DIR}/Plugins")
# 可选 DTK GUI（DciIcon 加载 .dci 控制中心图标）；未安装 dtk6gui 开发包时 icon() 回退 PNG
find_package(DTKGui QUIET)
if(DTKGui_FOUND)
    target_compile_definitions(qtetDdeTrayPlugin PRIVATE QTET_DDE_TRAY_HAS_DTK)
    target_link_libraries(qtetDdeTrayPlugin PRIVATE Dtk6::Gui)
endif()
install(TARGETS qtetDdeTrayPlugin
        LIBRARY DESTINATION "${DDE_TRAY_PLUGIN_INSTALL_DIR}")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../../assets/favicon/qtet.svg"
        DESTINATION "${DDE_DCC_ICON_INSTALL_DIR}"
        RENAME qteasytier-dde-tray-plugin.svg)
