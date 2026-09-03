/* @file NetworkPage.qml (DDE)
 * @brief DDE 版网络页面：DTK 控件重写，配置管理与网络运行的入口页面，根据后端连接状态切换显示连接提示或完整功能面板
 *
 * 根据后端连接状态显示不同视图：
 *   未连接 → 连接提示
 *   已连接 → 左侧配置列表 + 右侧配置编辑或运行状态
 * 页面对话框（导入方式选择 / URL 导入）为模态 DialogWindow。
 */
import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import QtQuick.Dialogs
import QtEasyTier
import org.deepin.dtk

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
        id: connectedContent

        visible: BackendStatusViewModel.connected
        anchors.fill: parent

        function contentComponent() {
            if (NetworkPageViewModel.currentInstanceName === "")
                return emptyContentComponent
            if (NetworkPageViewModel.showEditor)
                return networkOptionsComponent
            if (NetworkPageViewModel.showRuntimeStatus)
                return networkRuntimeStatusComponent
            return emptyContentComponent
        }

        Component {
            id: emptyContentComponent

            Item {
                anchors.fill: parent

                Label {
                    anchors.centerIn: parent
                    text: qsTr("请先选中或新建配置项")
                    color: palette.placeholderText
                }
            }
        }

        Component {
            id: networkOptionsComponent

            NetworkOptions {
                anchors.fill: parent
            }
        }

        Component {
            id: networkRuntimeStatusComponent

            NetworkRuningStatus {
                anchors.fill: parent
            }
        }

        RowLayout {
            anchors.fill: parent
            spacing: 0

            // 左侧面板：配置列表（dde/pages/ 同目录，隐式解析）
            InstanceList {
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
                onImportRequested: importChoiceDialog.open()
            }

            // 分隔线
            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.15)
            }

            // 右侧内容区：根据状态切换显示
            Loader {
                id: contentLoader
                Layout.fillWidth: true
                Layout.fillHeight: true
                sourceComponent: connectedContent.contentComponent()
            }
        }

        // 导入方式选择对话框（模态独立窗：两个并列入口按钮）
        DialogWindow {
            id: importChoiceDialog

            modality: Qt.ApplicationModal

            title: ""
            width: 380

            function open() {
                visible = true
                requestActivate()
            }

            function close() {
                visible = false
            }

            header: DialogTitleBar {
                title: qsTr("导入配置")
            }

            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 24
                anchors.rightMargin: 24
                spacing: 12

                RecommandButton {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    text: qsTr("从文件导入")
                    onClicked: {
                        importChoiceDialog.close()
                        importFileDialog.open()
                    }
                }

                Button {
                    Layout.fillWidth: true
                    text: qsTr("从 URL 导入")
                    onClicked: {
                        importChoiceDialog.close()
                        importUrlDialog.open()
                    }
                }

                Item { Layout.preferredHeight: 4 }
            }
        }

        // 导入 URL 对话框（多行粘贴 qtet:// 配置 URL）
        DialogWindow {
            id: importUrlDialog

            modality: Qt.ApplicationModal

            title: ""
            width: 520

            function open() {
                hadShown = true
                visible = true
                requestActivate()
            }

            function close() {
                visible = false
            }

            property bool hadShown: false
            onVisibleChanged: {
                if (!visible && hadShown)
                    hadShown = false
            }

            /* 确认导入：空输入不关窗 */
            function acceptImport() {
                var url = importUrlField.text.trim()
                if (url === "")
                    return
                NetworkPageViewModel.importConfigUrl(url)
                importUrlField.text = ""
                close()
            }

            header: DialogTitleBar {
                title: qsTr("从 URL 导入")
            }

            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 24
                anchors.rightMargin: 24
                spacing: 16

                Label {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    text: qsTr("粘贴 qtet:// 开头的配置 URL：")
                }

                // 多行 URL 输入：QQC.TextArea + DTK 风格自绘背景（org.deepin.dtk 无 TextArea）
                QQC.TextArea {
                    id: importUrlField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 80
                    wrapMode: TextEdit.WrapAnywhere
                    placeholderText: "qtet://..."

                    background: Rectangle {
                        radius: 8
                        color: palette.base
                        border.width: 1
                        border.color: Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.15)
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.bottomMargin: 12
                    spacing: 10

                    Item { Layout.fillWidth: true }

                    Button {
                        text: qsTr("取消")
                        onClicked: importUrlDialog.close()
                    }

                    RecommandButton {
                        text: qsTr("导入")
                        onClicked: importUrlDialog.acceptImport()
                    }
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
                NetworkPageViewModel.importConfigFile(selectedFile.toString())
            }
        }

        // 导入失败时的错误提示
        Connections {
            target: ConfigListModel
            function onImportFailed(message) { AppState.showError(message) }
        }
    }
}
