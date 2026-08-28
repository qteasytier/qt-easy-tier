add_qtet_library(qtet_repository
    DatabaseConnection.cpp
    NetworkConfigRepository.cpp
    FavoriteNodeRepository.cpp
    LogRepository.cpp
    FavoriteNode.h
)
target_link_libraries(qtet_repository PUBLIC Qt6::Core Qt6::Sql qtet_config qtet_log)
