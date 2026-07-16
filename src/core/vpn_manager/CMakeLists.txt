add_qtet_library(qtet_vpn
    VpnController.cpp
    VpnManager.cpp
    StatusMonitor.cpp
)
target_link_libraries(qtet_vpn PUBLIC Qt6::Core Qt6::Concurrent qtet_application qtet_config qtet_repository qtet_service qtet_log)
