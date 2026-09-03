# DDE(DTK)前端的配置表单渲染器(数据驱动配置表单)与列表编辑器组件。
# 列表元素为相对 src/qml/ 的路径,由根 CMake 统一设置 QT_RESOURCE_ALIAS。

set(_dde_config_form_qml
    dde/config_form/FormField.qml
    dde/config_form/FormSwitch.qml
    dde/config_form/FormTextField.qml
    dde/config_form/FormPassword.qml
    dde/config_form/FormComboBox.qml
    dde/config_form/FormSpinBox.qml
    dde/config_form/FormFilePath.qml
    dde/config_form/FormKeyActions.qml
    dde/config_form/FormStringList.qml
    dde/config_form/FormServerList.qml
    dde/config_form/FormProxyNetworkList.qml
    dde/config_form/EditableList.qml
    dde/config_form/EditableListItem.qml
    dde/config_form/ServerListItem.qml
    dde/config_form/ProxyNetworkListItem.qml
    dde/config_form/ImportNodesDialog.qml
)
list(APPEND QTET_QML_DDE_FILES ${_dde_config_form_qml})

set(QTET_QML_DDE_FILES ${QTET_QML_DDE_FILES} PARENT_SCOPE)
