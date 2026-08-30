# DDE 托盘插件专属测试。仅在 BUILD_WITH_DDE_TRAY_PLUGIN 启用时构建；
# add_core_test 定义于 tests/CMakeLists.txt，子目录可直接调用。
add_core_test(tst_tray_status_service tst_tray_status_service.cpp)
target_link_libraries(tst_tray_status_service PRIVATE qtet_dde_tray_status)
