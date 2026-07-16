add_qtet_library(qtet_log
    LogDispatcher.cpp
    LogDispatcher.h
    LogSink.h
    LogTypes.h
    ../util/LogHelper.cpp
    ../util/LogHelper.h
)
target_link_libraries(qtet_log PUBLIC Qt6::Core)
