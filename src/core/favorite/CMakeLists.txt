add_qtet_library(qtet_favorite
    FavoriteNode.h
    FavoriteNodeJsonCodec.cpp
    FavoriteNodeJsonCodec.h
)
target_link_libraries(qtet_favorite PUBLIC Qt6::Core)
