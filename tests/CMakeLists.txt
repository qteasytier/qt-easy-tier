find_package(Qt6 REQUIRED COMPONENTS Core Sql Network Test Widgets Quick)

# 创建单个 CTest 测试可执行文件。用法：
#   add_core_test(tst_xxx tst_xxx.cpp)
#   target_link_libraries(tst_xxx PRIVATE qtet_xxx)   # 链接被测模块
function(add_core_test name)
    qt_add_executable(${name} ${ARGN})
    target_include_directories(${name} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}/generated
        ${CMAKE_SOURCE_DIR}/ThirdParty/toml++
    )
    target_link_libraries(${name} PRIVATE
        Qt6::Core
        Qt6::Sql
        Qt6::Network
        Qt6::Test
        Qt6::Widgets
        Qt6::Quick
    )
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_core_test(tst_network_conf tst_network_conf.cpp)
target_link_libraries(tst_network_conf PRIVATE qtet_config)

add_core_test(tst_x25519_key_helper tst_x25519_key_helper.cpp)
target_link_libraries(tst_x25519_key_helper PRIVATE qtet_config)

add_core_test(tst_config_url_codec tst_config_url_codec.cpp)
target_link_libraries(tst_config_url_codec PRIVATE qtet_config)

add_core_test(tst_sqlite_repository tst_sqlite_repository.cpp)
target_link_libraries(tst_sqlite_repository PRIVATE qtet_sqlite_repository)

add_core_test(tst_config_list_model tst_config_list_model.cpp)
target_link_libraries(tst_config_list_model PRIVATE qtet_appcore)

add_core_test(tst_config_editor_viewmodel tst_config_editor_viewmodel.cpp)
target_link_libraries(tst_config_editor_viewmodel PRIVATE qtet_appcore)

# 网络配置编辑器薄壳的 QML 冒烟测试：经构建树模块目录加载 pages/NetworkOptions.qml。
# 模块 qmldir 带 prefer :/QtEasyTier/，优先从 qrc 解析类型，故把同一份 QML
# （QTET_QML_FILES 元素为相对 src/qml 的路径，兼作 QT_RESOURCE_ALIAS）
# 与图标资源一并嵌入测试进程。
# 仅非 DDE 构建注册：DDE 构建无共享版 NetworkOptions 可加载（整树条件选择），
# 且 DTK 渲染器不宜在无 DTK 环境实例化。
if(NOT BUILD_WITH_DDE)
    add_core_test(tst_network_options_smoke tst_network_options_smoke.cpp ${CMAKE_SOURCE_DIR}/assets/resources.qrc)
    # 静态集成：按上游 README 同时链接 SwbControlsplugin（QML 插件与类型注册）
    target_link_libraries(tst_network_options_smoke PRIVATE qtet_appsupport SwbControls SwbControlsplugin)
    target_compile_definitions(tst_network_options_smoke PRIVATE QTET_TEST_QML_DIR="${CMAKE_BINARY_DIR}")
    set(QTET_SMOKE_QML_ABS "")
    foreach(qml_rel IN LISTS QTET_QML_FILES)
        list(APPEND QTET_SMOKE_QML_ABS "${CMAKE_SOURCE_DIR}/src/qml/${qml_rel}")
        set_source_files_properties("${CMAKE_SOURCE_DIR}/src/qml/${qml_rel}" PROPERTIES QT_RESOURCE_ALIAS "${qml_rel}")
    endforeach()
    qt_add_resources(tst_network_options_smoke "qtet_qml_smoke"
        PREFIX "/QtEasyTier"
        FILES ${QTET_SMOKE_QML_ABS}
    )
endif()

add_core_test(tst_favorite_node_repository tst_favorite_node_repository.cpp)
target_link_libraries(tst_favorite_node_repository PRIVATE qtet_sqlite_repository)

add_core_test(tst_daemon_client tst_daemon_client.cpp)
# 同时覆盖 DaemonClient（qtet_daemon_service）与 BackendStatusViewModel（qtet_appcore）的历史混合测试
target_link_libraries(tst_daemon_client PRIVATE qtet_appcore)

add_core_test(tst_credential_service tst_credential_service.cpp)
target_link_libraries(tst_credential_service PRIVATE qtet_appcore)

add_core_test(tst_credential_list_model tst_credential_list_model.cpp)
target_link_libraries(tst_credential_list_model PRIVATE qtet_appcore)

add_core_test(tst_autostart_helper tst_autostart_helper.cpp)
target_link_libraries(tst_autostart_helper PRIVATE qtet_platform)

add_core_test(tst_daemon_register_helper tst_daemon_register_helper.cpp)
target_link_libraries(tst_daemon_register_helper PRIVATE qtet_platform)

add_core_test(tst_settings_store tst_settings_store.cpp)
target_link_libraries(tst_settings_store PRIVATE qtet_appcore)

add_core_test(tst_settings_viewmodel tst_settings_viewmodel.cpp)
target_link_libraries(tst_settings_viewmodel PRIVATE qtet_appcore qtet_platform)

add_core_test(tst_favorite_node_json_codec tst_favorite_node_json_codec.cpp)
target_link_libraries(tst_favorite_node_json_codec PRIVATE qtet_appcore)

add_core_test(tst_favorite_node_import_export_service tst_favorite_node_import_export_service.cpp)
target_link_libraries(tst_favorite_node_import_export_service PRIVATE qtet_appcore)

add_core_test(tst_favorite_node_viewmodel tst_favorite_node_viewmodel.cpp)
target_link_libraries(tst_favorite_node_viewmodel PRIVATE qtet_appcore)

add_core_test(tst_import_nodes_viewmodel tst_import_nodes_viewmodel.cpp)
target_link_libraries(tst_import_nodes_viewmodel PRIVATE qtet_appcore)

add_core_test(tst_app_services tst_app_services.cpp)
target_link_libraries(tst_app_services PRIVATE qtet_appsupport)

add_core_test(tst_vpn_runtime_service tst_vpn_runtime_service.cpp)
target_link_libraries(tst_vpn_runtime_service PRIVATE qtet_appcore)

add_core_test(tst_dangerous_operation_service tst_dangerous_operation_service.cpp)
target_link_libraries(tst_dangerous_operation_service PRIVATE qtet_appcore)

add_core_test(tst_log_repository tst_log_repository.cpp)
target_link_libraries(tst_log_repository PRIVATE qtet_sqlite_repository)

add_core_test(tst_log_helper tst_log_helper.cpp)
target_link_libraries(tst_log_helper PRIVATE qtet_appcore qtet_sqlite_repository qtet_log)

add_core_test(tst_tray_message_dispatcher tst_tray_message_dispatcher.cpp)
target_link_libraries(tst_tray_message_dispatcher PRIVATE qtet_system_tray)

add_core_test(tst_system_tray_manager tst_system_tray_manager.cpp)
target_link_libraries(tst_system_tray_manager PRIVATE qtet_system_tray)

add_core_test(tst_app_launch_manager tst_app_launch_manager.cpp)
target_link_libraries(tst_app_launch_manager PRIVATE qtet_appsupport)

# DDE 托盘插件专属测试（tests/dde/）
if(BUILD_WITH_DDE_TRAY_PLUGIN)
    add_subdirectory(dde)
endif()
