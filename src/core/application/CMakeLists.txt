add_qtet_library(qtet_application
    config/ConfigCommandService.cpp
    config/ConfigCommandService.h
    config/ConfigImportExportService.cpp
    config/ConfigImportExportService.h
    config/ConfigOperationResult.h
    logging/RepositoryLogSink.cpp
    logging/RepositoryLogSink.h
    runtime/ConfigRunState.h
    settings/AutoStartService.cpp
    settings/AutoStartService.h
    settings/SettingsStore.cpp
    settings/SettingsStore.h
)
target_link_libraries(qtet_application PUBLIC Qt6::Core qtet_config qtet_repository qtet_service qtet_platform qtet_log)
