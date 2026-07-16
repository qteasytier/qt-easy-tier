add_qtet_library(qtet_service
    IpcMessage.cpp
    FrameProtocol.cpp
    DaemonClient.cpp
    DaemonApi.cpp
    DaemonApi.h
)
target_link_libraries(qtet_service PUBLIC Qt6::Core Qt6::Network qtet_log)
