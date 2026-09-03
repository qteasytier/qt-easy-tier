/* @file FavoriteNodesPage.qml (DDE)
 * @brief DDE 版节点收藏页面：DTK 控件重写，管理常用节点的增删改查，支持节点名称/URI/公钥的编辑和清空操作
 *
 * 页面对话框（导入方式选择 / URL 导入 / 节点编辑 / 删除与清空确认）均为模态 DialogWindow。
 */
import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import QtEasyTier
import org.deepin.dtk

// 节点收藏页面：管理常用节点的增删改查
/* @brief 收藏页面根容器，包含节点列表和添加/清空按钮 */
Rectangle {
    id: root

    color: palette.window

    /* 当前正在编辑的节点数据库 ID，-1 表示新增模式 */
    property int editNodeId: -1
    /* 是否为编辑模式：true=编辑已有节点，false=新增节点 */
    property bool editMode: false

    // 页面加载时从数据库读取节点列表
    Component.onCompleted: FavoriteNodeViewModel.loadNodes()

    // 监听 ViewModel 错误信号，转发到全局错误提示
    Connections {
        target: FavoriteNodeViewModel
        function onErrorOccurred(message) { AppState.showError(message) }
    }

    // 主布局
    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 16
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 10

        // 页面标题
        Label {
            text: qsTr("节点收藏")
            font.pixelSize: 24
            font.bold: true
        }

        // 空状态提示：无节点时显示引导文字
        Label {
            visible: FavoriteNodeViewModel.count === 0
            text: qsTr("暂无收藏节点，点击下方按钮添加")
            color: palette.placeholderText
            font.pixelSize: 13
            Layout.fillWidth: true
            Layout.topMargin: 20
            horizontalAlignment: Text.AlignHCenter
        }

        // 节点列表
        ListView {
            id: nodeListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 0
            model: FavoriteNodeViewModel

            ScrollBar.vertical: ScrollBar {}

            // 单个节点行：悬停浅色反馈 + 行底分隔线
            delegate: Rectangle {
                id: nodeRow

                required property int index
                required property int nodeId
                required property string nodeName
                required property string nodeUri
                required property string nodePublicKey

                width: nodeListView.width
                height: 56
                color: rowHover.hovered
                     ? Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.04)
                     : "transparent"

                Behavior on color {
                    ColorAnimation { duration: 120 }
                }

                HoverHandler {
                    id: rowHover
                }

                // 行底分隔线（最后一行不显示）
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 1
                    color: Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.15)
                    visible: nodeRow.index < nodeListView.count - 1
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 4
                    spacing: 8

                    // 节点信息区：名称 + URI
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            // 有公钥时标注【安全】
                            text: nodeName + (nodePublicKey ? qsTr("【安全】") : "")
                            font.pixelSize: 14
                            font.bold: nodePublicKey !== ""
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Label {
                            text: nodeUri
                            font.pixelSize: 11
                            color: palette.placeholderText
                            font.family: "monospace"
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }
                    }

                    // 编辑按钮：弹出编辑对话框
                    IconToolButton {
                        iconSource: "qrc:/icons/edit.svg"
                        flat: true
                        onClicked: {
                            editNameField.text = nodeName
                            editUriField.text = nodeUri
                            editPublicKeyField.text = nodePublicKey
                            editNodeId = nodeId
                            editMode = true
                            nodeDialog.open()
                        }
                    }

                    // 删除按钮：弹出确认对话框
                    IconToolButton {
                        iconSource: "qrc:/icons/delete.svg"
                        flat: true
                        onClicked: {
                            deleteConfirmDialog.nodeName = nodeName
                            deleteConfirmDialog.nodeId = nodeId
                            deleteConfirmDialog.open()
                        }
                    }
                }
            }
        }

        // 底部操作按钮区
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            RowLayout {
                spacing: 6

                // 添加节点按钮
                RecommandButton {
                    Layout.fillWidth: true
                    text: qsTr("添加节点")
                    onClicked: {
                        // 清空编辑字段，进入新增模式
                        editNameField.text = ""
                        editUriField.text = ""
                        editPublicKeyField.text = ""
                        editNodeId = -1
                        editMode = false
                        nodeDialog.open()
                    }
                }

                // 批量导入节点：读取与 publicservers.json 相同格式的 JSON 文件
                Button {
                    Layout.fillWidth: true
                    text: qsTr("导入节点")
                    onClicked: importModeDialog.open()
                }

                // 批量导出节点：导出为与 publicservers.json 相同格式的 JSON 文件
                Button {
                    Layout.fillWidth: true
                    text: qsTr("导出节点")
                    enabled: FavoriteNodeViewModel.count > 0
                    onClicked: exportNodesFileDialog.open()
                }
            }

            // 清空列表按钮：危险操作（红色链接样式），仅在有节点时可用
            LinkButton {
                Layout.fillWidth: true
                text: qsTr("清空节点列表")
                textColor: appTheme.statusRed
                enabled: FavoriteNodeViewModel.count > 0
                onClicked: clearConfirmDialog.open()
            }
        }
    }

    // 批量导入方式选择对话框（两个并列入口按钮）
    DialogWindow {
        id: importModeDialog

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
            title: qsTr("导入节点")
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
                text: qsTr("从本地文件导入")
                onClicked: {
                    importModeDialog.close()
                    importNodesFileDialog.open()
                }
            }

            Button {
                Layout.fillWidth: true
                text: qsTr("从 URL 导入")
                onClicked: {
                    importModeDialog.close()
                    importUrlDialog.inputText = ""
                    importUrlDialog.open()
                }
            }

            Item { Layout.preferredHeight: 4 }
        }
    }

    // 批量导入 URL 输入对话框（输入模式基座）
    ConfirmDialog {
        id: importUrlDialog

        headerTitle: qsTr("从 URL 导入节点")
        inputMode: true
        confirmText: qsTr("导入")
        message: qsTr("节点 JSON 地址（http/https）")
        inputPlaceholder: qsTr("https://example.com/nodes.json")

        onAccepted: {
            var url = inputText.trim()
            if (url)
                FavoriteNodeViewModel.importNodesFromUrl(url)
        }
    }

    // 批量导入收藏节点文件对话框
    FileDialog {
        id: importNodesFileDialog
        title: qsTr("导入节点")
        nameFilters: [qsTr("JSON 文件 (*.json)"), qsTr("所有文件 (*)")]
        fileMode: FileDialog.OpenFile
        onAccepted: FavoriteNodeViewModel.importNodesFromFile(selectedFile.toString())
    }

    // 批量导出收藏节点文件对话框
    FileDialog {
        id: exportNodesFileDialog
        title: qsTr("导出节点")
        nameFilters: [qsTr("JSON 文件 (*.json)"), qsTr("所有文件 (*)")]
        fileMode: FileDialog.SaveFile
        currentFile: AppState.homeDirectory + "/favorite_nodes.json"
        onAccepted: {
            var url = selectedFile.toString()
            if (!url.endsWith(".json"))
                url += ".json"
            FavoriteNodeViewModel.exportNodesToFile(url)
        }
    }

    // 节点编辑/添加对话框（三字段表单，校验失败保持窗口打开）
    DialogWindow {
        id: nodeDialog

        modality: Qt.ApplicationModal

        title: ""
        width: 440

        function open() {
            hadShown = true
            visible = true
            requestActivate()
            editNameField.forceActiveFocus()
        }

        function close() {
            visible = false
        }

        property bool hadShown: false
        onVisibleChanged: {
            if (!visible && hadShown)
                hadShown = false
        }

        /* 点击确定：校验必填字段后保存（空必填不关窗） */
        function acceptNode() {
            var name = editNameField.text.trim()
            var uri = editUriField.text.trim()
            var publicKey = editPublicKeyField.text.trim()

            if (!name || !uri)
                return

            if (editMode && editNodeId >= 0) {
                FavoriteNodeViewModel.updateNode(editNodeId, name, uri, publicKey)
            } else {
                FavoriteNodeViewModel.addNode(name, uri, publicKey)
            }
            close()
        }

        header: DialogTitleBar {
            title: root.editMode ? qsTr("编辑节点") : qsTr("添加节点")
        }

        ColumnLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 24
            anchors.rightMargin: 24
            spacing: 8

            Label {
                Layout.topMargin: 12
                text: qsTr("节点名称（必填）")
            }

            TextField {
                id: editNameField
                Layout.fillWidth: true
                placeholderText: qsTr("请输入节点名称")
            }

            Label {
                text: qsTr("节点地址（必填）")
            }

            TextField {
                id: editUriField
                Layout.fillWidth: true
                placeholderText: qsTr("例如：tcp://example.com:27773")
            }

            Label {
                text: qsTr("节点公钥（选填）")
            }

            TextField {
                id: editPublicKeyField
                Layout.fillWidth: true
                placeholderText: qsTr("留空表示不使用公钥验证")
                onAccepted: nodeDialog.acceptNode()
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.bottomMargin: 12
                spacing: 10

                Item { Layout.fillWidth: true }

                Button {
                    text: qsTr("取消")
                    onClicked: nodeDialog.close()
                }

                RecommandButton {
                    text: root.editMode ? qsTr("保存") : qsTr("添加")
                    onClicked: nodeDialog.acceptNode()
                }
            }
        }
    }

    // 删除单个节点确认对话框
    ConfirmDialog {
        id: deleteConfirmDialog

        property string nodeName: ""
        property int nodeId: -1

        title: qsTr("确认删除")
        danger: true
        confirmText: qsTr("删除")
        message: nodeName !== ""
            ? qsTr("确定要删除节点 \"%1\" 吗？").arg(nodeName)
            : ""

        onAccepted: {
            if (deleteConfirmDialog.nodeId >= 0)
                FavoriteNodeViewModel.removeNode(deleteConfirmDialog.nodeId)
        }
    }

    // 清空全部节点确认对话框
    ConfirmDialog {
        id: clearConfirmDialog

        headerTitle: qsTr("确认清空")
        danger: true
        confirmText: qsTr("清空")
        message: qsTr("确定要清空所有节点吗？此操作不可恢复。")

        onAccepted: FavoriteNodeViewModel.clearAll()
    }
}
