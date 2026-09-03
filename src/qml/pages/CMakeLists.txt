# 共享前端页面:仅非 DDE 构建编入(DDE 构建使用 dde/pages/ 下的 DTK 版页面)。
# 列表元素为相对 src/qml/ 的路径,由根 CMake 统一设置 QT_RESOURCE_ALIAS。

set(QTET_QML_SHARED_FILES
    ${QTET_QML_SHARED_FILES}
    pages/InstanceList.qml
    pages/NetworkPage.qml
    pages/NetworkOptions.qml
    pages/NetworkRuningStatus.qml
    pages/SettingsPage.qml
    pages/FavoriteNodesPage.qml
    pages/LogPage.qml
    PARENT_SCOPE
)
