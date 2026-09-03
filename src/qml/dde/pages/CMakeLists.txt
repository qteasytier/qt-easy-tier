# DDE(DTK)前端页面:与共享 pages/ 同名类型,仅在 BUILD_WITH_DDE=ON 时编入
# (两棵树同名类型绝不混编,由根 CMakeLists 整树二选一)。
# 列表元素为相对 src/qml/ 的路径,由根 CMake 统一设置 QT_RESOURCE_ALIAS。

set(QTET_QML_DDE_FILES
    ${QTET_QML_DDE_FILES}
    dde/pages/InstanceList.qml
    dde/pages/NetworkPage.qml
    dde/pages/NetworkOptions.qml
    dde/pages/NetworkRuningStatus.qml
    dde/pages/SettingsPage.qml
    dde/pages/FavoriteNodesPage.qml
    dde/pages/LogPage.qml
    PARENT_SCOPE
)
