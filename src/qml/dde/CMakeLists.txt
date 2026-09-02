# DDE(DTK)外壳与蒙版替换组件:仅 BUILD_WITH_DDE=ON 时编入(由根 CMakeLists 决定)。
# 蒙版组件与共享版同名(IconToolButton/Card/EditableListItem/ErrorDialog/
# ImportNodesDialog/GenerateCredentialDialog/CredentialManageDialog),
# 以类型名在 QtEasyTier 模块内实现按构建替换。
# 列表元素为相对 src/qml/ 的路径,由根 CMake 统一设置 QT_RESOURCE_ALIAS。

set(QTET_QML_MASK_DDE
    dde/DdeMain.qml
    dde/AppAboutDialog.qml
    dde/components/IconToolButton.qml
    dde/components/Card.qml
    dde/components/EditableListItem.qml
    dde/components/ErrorDialog.qml
    dde/components/ImportNodesDialog.qml
    dde/components/GenerateCredentialDialog.qml
    dde/components/CredentialManageDialog.qml
    PARENT_SCOPE
)
