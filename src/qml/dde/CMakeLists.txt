# DDE(DTK)前端:仅 BUILD_WITH_DDE=ON 时编入的完整独立 UI 树。
# 含主窗口壳(DdeMain)、关于对话框与通用组件,全部使用 org.deepin.dtk
# 控件实现,不依赖 SwbControls;pages/ 与 config_form/ 子目录为 DTK 版
# 页面与表单渲染器,与共享树同名类型绝不混编(由根 CMakeLists 整树二选一)。
# 列表元素为相对 src/qml/ 的路径,由根 CMake 统一设置 QT_RESOURCE_ALIAS。

set(QTET_QML_DDE_FILES
    dde/DdeMain.qml
    dde/AppAboutDialog.qml
    dde/components/Sidebar.qml
    dde/components/Card.qml
    dde/components/IconToolButton.qml
    dde/components/ErrorDialog.qml
    dde/components/ConfirmDialog.qml
    dde/components/TabHeader.qml
    dde/components/CredentialManageDialog.qml
)

# DTK 版页面与配置表单渲染器
add_subdirectory(pages)
add_subdirectory(config_form)

set(QTET_QML_DDE_FILES ${QTET_QML_DDE_FILES} PARENT_SCOPE)
