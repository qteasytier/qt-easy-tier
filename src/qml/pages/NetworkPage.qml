/* @brief 网络页面：配置管理与网络运行的入口页面，根据后端连接状态切换显示连接提示或完整功能面板 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtEasyTier

// 网络页面：配置管理与网络运行的入口页面
// 根据后端连接状态显示不同视图：
//   未连接 → 连接提示
//   已连接 → 左侧配置列表 + 右侧配置编辑或运行状态
/* @brief 网络页面根容器，管理左侧面板和右侧内容区的布局 */
Rectangle {
    id: root

    color: palette.window

    // 页面初始化时查询运行状态
    Component.onCompleted: NetworkPageViewModel.refreshRunning()

    // 未连接状态：显示连接提示
    Item {
        visible: !BackendStatusViewModel.connected
        anchors.fill: parent

        Label {
            anchors.centerIn: parent
            text: BackendStatusViewModel.connecting
                ? qsTr("正在连接后端...")
                : qsTr("后端尚未连接")
            font.pixelSize: 24
            color: palette.placeholderText
        }
    }

    // 已连接状态：显示完整功能面板
    Item {
        visible: BackendStatusViewModel.connected
        anchors.fill: parent

        RowLayout {
            anchors.fill: parent
            spacing: 0

            // 左侧面板：配置列表
            LeftPanel {
                id: leftPanel
                Layout.preferredWidth: 220
                Layout.fillHeight: true
                onConfigSelected: function(instanceName) { NetworkPageViewModel.selectConfig(instanceName) }
                onCreateRequested: NetworkPageViewModel.createConfig()
                onDeleteRequested: function(instanceName) { NetworkPageViewModel.deleteConfig(instanceName) }
                onRenameRequested: function(instanceName, newDisplayName) {
                    NetworkPageViewModel.renameConfig(instanceName, newDisplayName)
                }
                onStartRequested: function(instanceName) { NetworkPageViewModel.startConfig(instanceName) }
                onStopRequested: function(instanceName) { NetworkPageViewModel.stopConfig(instanceName) }
                onImportRequested: importFileDialog.open()
            }

            // 分隔线
            Rectangle {
                width: 1
                Layout.fillHeight: true
                color: Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.12)
            }

            // 右侧内容区：根据状态切换显示
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // 未选中配置：提示选择或新建
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: NetworkPageViewModel.currentInstanceName === ""

                    Label {
                        anchors.centerIn: parent
                        text: qsTr("请先选中或新建配置项")
                        color: palette.placeholderText
                    }
                }

                // 已选中配置 + 未运行：显示配置编辑界面
                NetworkOptions {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: NetworkPageViewModel.currentInstanceName !== "" && NetworkPageViewModel.showEditor
                }

                // 已选中配置 + 运行中：显示运行状态界面
                NetworkRuningStatus {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: NetworkPageViewModel.showRuntimeStatus
                }
            }
        }

        // 导入配置文件对话框
        FileDialog {
            id: importFileDialog
            title: qsTr("导入配置")
            nameFilters: [qsTr("TOML 文件 (*.toml)"), qsTr("所有文件 (*)")]
            fileMode: FileDialog.OpenFile
            onAccepted: {
                NetworkPageViewModel.importConfig(selectedFile.toString())
            }
        }

        // 导入失败时的错误提示
        Connections {
            target: ConfigListModel
            function onImportFailed(message) { AppState.showError(message) }
        }
    }
}
