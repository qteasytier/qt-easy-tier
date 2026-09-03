# 共享前端(SwbControls 风格)的通用组件与配置表单渲染器:仅非 DDE 构建编入。
# DDE 构建使用 dde/ 下的完整 DTK 前端树(见 dde/CMakeLists.txt),
# 不编译任何 import SwbControls 的文件,两棵树同名类型绝不混编。
# Theme.qml / PageContainer.qml 由根 CMakeLists 无条件编入(两种前端共用)。
# 列表元素为相对 src/qml/ 的路径,由根 CMake 统一设置 QT_RESOURCE_ALIAS。

set(_components_shared_qml
    components/Sidebar.qml
    components/Card.qml
    components/IconToolButton.qml
    components/ErrorDialog.qml
    components/CredentialManageDialog.qml
)
list(APPEND QTET_QML_SHARED_FILES ${_components_shared_qml})

# 配置表单渲染器(数据驱动配置表单)与列表编辑器组件
add_subdirectory(config_form)

set(QTET_QML_SHARED_FILES ${QTET_QML_SHARED_FILES} PARENT_SCOPE)
