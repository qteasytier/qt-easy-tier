# 页面 QML:普通与 DDE 外壳共用的页面文件。
# 列表元素为相对 src/qml/ 的路径,由根 CMake 统一设置 QT_RESOURCE_ALIAS。

set(QTET_QML_FILES
    ${QTET_QML_FILES}
    pages/InstanceList.qml
    pages/NetworkPage.qml
    pages/NetworkOptions.qml
    pages/NetworkRuningStatus.qml
    pages/SettingsPage.qml
    pages/FavoriteNodesPage.qml
    pages/LogPage.qml
    PARENT_SCOPE
)
