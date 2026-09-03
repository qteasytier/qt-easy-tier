# 共享前端(SwbControls 风格)的配置表单渲染器(数据驱动配置表单)
# 与被渲染器专用的列表编辑器组件:仅非 DDE 构建编入。
# DDE 构建使用 dde/config_form/ 下的 DTK 版渲染器。
# 列表元素为相对 src/qml/ 的路径,由根 CMake 统一设置 QT_RESOURCE_ALIAS。

set(_config_form_qml
    components/config_form/FormField.qml
    components/config_form/FormSwitch.qml
    components/config_form/FormTextField.qml
    components/config_form/FormPassword.qml
    components/config_form/FormComboBox.qml
    components/config_form/FormSpinBox.qml
    components/config_form/FormFilePath.qml
    components/config_form/FormKeyActions.qml
    components/config_form/FormStringList.qml
    components/config_form/FormServerList.qml
    components/config_form/FormProxyNetworkList.qml
    components/config_form/EditableList.qml
    components/config_form/EditableListItem.qml
    components/config_form/ServerListItem.qml
    components/config_form/ProxyNetworkListItem.qml
    components/config_form/ImportNodesDialog.qml
)
list(APPEND QTET_QML_SHARED_FILES ${_config_form_qml})

set(QTET_QML_SHARED_FILES ${QTET_QML_SHARED_FILES} PARENT_SCOPE)
