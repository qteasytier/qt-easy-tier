add_qtet_library(qtet_appsupport
    AppLaunchManager.cpp
    AppLaunchManager.h
    AppServices.cpp
    AppServices.h
    QmlSingletonRegistrar.cpp
    QmlSingletonRegistrar.h
)
target_link_libraries(qtet_appsupport PUBLIC Qt6::Core Qt6::Quick qtet_viewmodel qtet_application qtet_repository qtet_service qtet_platform qtet_vpn qtet_log)
target_link_libraries(qtet_appsupport PUBLIC qtet_system_tray)
