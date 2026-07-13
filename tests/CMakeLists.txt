find_package(Qt6 REQUIRED COMPONENTS Core Sql Network Test Widgets Quick)

function(add_core_test name)
    qt_add_executable(${name} ${ARGN})
    target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/ThirdParty/toml++)
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

add_core_test(tst_config_url_codec tst_config_url_codec.cpp)
target_link_libraries(tst_config_url_codec PRIVATE qtet_config)

add_core_test(tst_sqlite_repository tst_sqlite_repository.cpp)
target_link_libraries(tst_sqlite_repository PRIVATE qtet_repository)

add_core_test(tst_config_list_model tst_config_list_model.cpp)
target_link_libraries(tst_config_list_model PRIVATE qtet_viewmodel)

add_core_test(tst_favorite_node_repository tst_favorite_node_repository.cpp)
target_link_libraries(tst_favorite_node_repository PRIVATE qtet_repository)

add_core_test(tst_daemon_client tst_daemon_client.cpp)
target_link_libraries(tst_daemon_client PRIVATE qtet_viewmodel)

add_core_test(tst_autostart_helper tst_autostart_helper.cpp)
target_link_libraries(tst_autostart_helper PRIVATE qtet_platform)

add_core_test(tst_daemon_register_helper tst_daemon_register_helper.cpp)
target_link_libraries(tst_daemon_register_helper PRIVATE qtet_platform)

add_core_test(tst_settings_store tst_settings_store.cpp)
target_link_libraries(tst_settings_store PRIVATE qtet_application)

add_core_test(tst_autostart_service tst_autostart_service.cpp)
target_link_libraries(tst_autostart_service PRIVATE qtet_application)

add_core_test(tst_public_server_provider tst_public_server_provider.cpp)
target_link_libraries(tst_public_server_provider PRIVATE qtet_application)

add_core_test(tst_import_nodes_viewmodel tst_import_nodes_viewmodel.cpp)
target_link_libraries(tst_import_nodes_viewmodel PRIVATE qtet_viewmodel)

add_core_test(tst_app_services tst_app_services.cpp)
target_link_libraries(tst_app_services PRIVATE qtet_appsupport)

add_core_test(tst_log_repository tst_log_repository.cpp)
target_link_libraries(tst_log_repository PRIVATE qtet_repository)

add_core_test(tst_log_helper tst_log_helper.cpp)
target_link_libraries(tst_log_helper PRIVATE qtet_viewmodel)

add_core_test(tst_tray_message_dispatcher tst_tray_message_dispatcher.cpp)
target_link_libraries(tst_tray_message_dispatcher PRIVATE qtet_system_tray)

add_core_test(tst_system_tray_manager tst_system_tray_manager.cpp)
target_link_libraries(tst_system_tray_manager PRIVATE qtet_system_tray qtet_application)

add_core_test(tst_app_launch_options tst_app_launch_options.cpp)
target_link_libraries(tst_app_launch_options PRIVATE qtet_appsupport)
