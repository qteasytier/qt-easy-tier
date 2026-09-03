add_qtet_library(qtet_appsupport
    AppLaunchManager.cpp
    AppLaunchManager.h
    AppServices.cpp
    AppServices.h
    LanguageController.cpp
    LanguageController.h
    QmlSingletonRegistrar.cpp
    QmlSingletonRegistrar.h
)
target_link_libraries(qtet_appsupport PRIVATE Qt6::Core Qt6::Quick qtet_appcore)
target_link_libraries(qtet_appsupport PRIVATE qtet_system_tray)
