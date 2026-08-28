add_qtet_library(qtet_sqlite_repository
    DatabaseConnection.cpp
    NetworkConfigRepository.cpp
    FavoriteNodeRepository.cpp
    LogRepository.cpp
    FavoriteNode.h
)
target_link_libraries(qtet_sqlite_repository PUBLIC Qt6::Core Qt6::Sql qtet_config qtet_log)
