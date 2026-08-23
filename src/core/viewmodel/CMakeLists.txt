add_qtet_library(qtet_viewmodel
    AppState.cpp
    ConfigListModel.cpp
    ConfigEditorViewModel.cpp
    DangerousOperationViewModel.cpp
    SettingsViewModel.cpp
    FavoriteNodeViewModel.cpp
    LogViewModel.cpp
    nodes/ImportNodesViewModel.cpp
    nodes/ImportNodesViewModel.h
    runtime/BackendStatusViewModel.cpp
    runtime/BackendStatusViewModel.h
    runtime/NetworkPageViewModel.cpp
    runtime/NetworkPageViewModel.h
)
target_link_libraries(qtet_viewmodel PUBLIC Qt6::Core Qt6::Sql qtet_application qtet_config qtet_repository qtet_service qtet_favorite qtet_log)
