add_qtet_library(qtet_platform
    AutoStartHelper.cpp
    AutoStartHelper.h
    DaemonRegisterHelper.cpp
    DaemonRegisterHelper.h
    FontHelper.cpp
    FontHelper.h
)
target_link_libraries(qtet_platform PUBLIC Qt6::Core Qt6::Network Qt6::Widgets qtet_log)

if(WIN32)
    target_link_libraries(qtet_platform PUBLIC shell32)
endif()
