add_qtet_library(qtet_platform
    AutoStartHelper.cpp
    DaemonRegisterHelper.cpp
    DaemonRegisterHelper.h
    FontHelper.cpp
)
target_link_libraries(qtet_platform PUBLIC Qt6::Core Qt6::Widgets qtet_log)

if(WIN32)
    target_link_libraries(qtet_platform PUBLIC shell32)
endif()
