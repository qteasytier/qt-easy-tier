add_qtet_library(qtet_config
    NetworkConf.cpp
    NetworkConfToml.cpp
    ConfigValidator.cpp
    ConfigUrlCodec.cpp
    ConfigUrlCodec.h
    ../application/config/ConfigPayloadBuilder.cpp
    ../application/config/ConfigPayloadBuilder.h
)
target_link_libraries(qtet_config PUBLIC Qt6::Core qtet_log)
