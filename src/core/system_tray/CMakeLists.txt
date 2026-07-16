add_qtet_library(qtet_system_tray
    TrayMessageDispatcher.cpp
    TrayMessageDispatcher.h
    TrayMessageHelper.cpp
    TrayMessageHelper.h
    TrayMessageSink.h
    TrayMessageTypes.h
    SystemTrayManager.cpp
    SystemTrayManager.h
)
target_link_libraries(qtet_system_tray PUBLIC Qt6::Core Qt6::Network Qt6::Widgets qtet_service qtet_log)
