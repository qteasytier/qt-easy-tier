# 配置表单渲染器(数据驱动配置表单)与被渲染器专用的列表编辑器组件。
# 列表元素为相对 src/qml/ 的路径,由根 CMake 统一设置 QT_RESOURCE_ALIAS。
# 注意:EditableListItem/ImportNodesDialog 是 DDE 蒙版组件,
# DDE 构建时由 dde/components/ 下同名组件替换(见根 CMakeLists 的分支)。

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
list(APPEND QTET_QML_FILES ${_config_form_qml})

# 蒙版组件共享版(仅非 DDE 构建):DDE 构建用 dde/components/ 同名版本替换
list(APPEND QTET_QML_MASK_SHARED
    components/config_form/EditableListItem.qml
    components/config_form/ImportNodesDialog.qml
)

set(QTET_QML_FILES ${QTET_QML_FILES} PARENT_SCOPE)
set(QTET_QML_MASK_SHARED ${QTET_QML_MASK_SHARED} PARENT_SCOPE)
