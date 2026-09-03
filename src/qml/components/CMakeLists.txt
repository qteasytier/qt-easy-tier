# 通用外壳组件:随所有构建编译(不参与 DDE 蒙版替换的部分)。
# 列表元素为相对 src/qml/ 的路径,由根 CMake 统一设置 QT_RESOURCE_ALIAS。

set(_components_qml
    components/Sidebar.qml
    components/PageContainer.qml
    components/Theme.qml
)
list(APPEND QTET_QML_FILES ${_components_qml})

# DDE 蒙版组件的共享版(仅非 DDE 构建):DDE 构建用 dde/components/ 同名版本替换
# 注意:GenerateCredentialDialog 仅存在于 DDE 蒙版集(共享版已删除,新增凭证
# 功能并入共享版 CredentialManageDialog;DDE 版保留独立生成对话框的旧交互)
set(_components_mask_shared
    components/Card.qml
    components/IconToolButton.qml
    components/ErrorDialog.qml
    components/CredentialManageDialog.qml
)
list(APPEND QTET_QML_MASK_SHARED ${_components_mask_shared})

# 配置表单渲染器与列表编辑器
add_subdirectory(config_form)

set(QTET_QML_FILES ${QTET_QML_FILES} PARENT_SCOPE)
set(QTET_QML_MASK_SHARED ${QTET_QML_MASK_SHARED} PARENT_SCOPE)
