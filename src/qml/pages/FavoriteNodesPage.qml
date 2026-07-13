/* @brief 节点收藏页面：管理常用节点的增删改查，支持节点名称/URI/公钥的编辑和清空操作 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtEasyTier

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
            color: palette.highlight
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
            spacing: 4
            model: FavoriteNodeViewModel

            // 单个节点行
            delegate: Rectangle {
                required property int index
                required property int nodeId
                required property string nodeName
                required property string nodeUri
                required property string nodePublicKey

                width: nodeListView.width
                height: 56
                radius: 8
                // 偶数行透明，奇数行用极淡的文字色做分隔
                color: index % 2 === 0 ? "transparent"
                       : Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.04)

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
                    ToolButton {
                        icon.source: "qrc:/icons/edit.svg"
                        icon.color: "transparent"
                        icon.width: 16
                        icon.height: 16
                        flat: true
                        implicitWidth: 28
                        implicitHeight: 28
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
                    ToolButton {
                        icon.source: "qrc:/icons/delete.svg"
                        icon.color: "transparent"
                        icon.width: 16
                        icon.height: 16
                        flat: true
                        implicitWidth: 28
                        implicitHeight: 28
                        onClicked: {
                            deleteConfirmDialog.nodeName = nodeName
                            deleteConfirmDialog.nodeId = nodeId
                            deleteConfirmDialog.open()
                        }
                    }
                }
            }
        }

        // 底部操作按钮区（使用 0 边距，外层 ColumnLayout 已提供 16px）
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 0
            Layout.rightMargin: 0
            spacing: 0

            // 添加节点按钮
            Button {
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

            // 清空列表按钮：仅在有节点时可用
            Button {
                Layout.fillWidth: true
                text: qsTr("清空节点列表")
                flat: true
                enabled: FavoriteNodeViewModel.count > 0
                onClicked: clearConfirmDialog.open()
            }

        }
    }

    // 节点编辑/添加对话框
    Dialog {
        id: nodeDialog
        title: editMode ? qsTr("编辑节点") : qsTr("添加节点")
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        anchors.centerIn: parent
        width: Math.min(420, parent ? parent.width - 48 : 360)

        ColumnLayout {
            width: parent ? parent.width : 360
            spacing: 8

            Label {
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
            }
        }

        // 对话框打开后自动聚焦名称输入框
        onOpened: {
            editNameField.forceActiveFocus()
        }

        // 点击确定：校验必填字段后保存
        onAccepted: {
            var name = editNameField.text.trim()
            var uri = editUriField.text.trim()
            var publicKey = editPublicKeyField.text.trim()

            if (!name || !uri) return

            if (editMode && editNodeId >= 0) {
                FavoriteNodeViewModel.updateNode(editNodeId, name, uri, publicKey)
            } else {
                FavoriteNodeViewModel.addNode(name, uri, publicKey)
            }
        }
    }

    // 删除单个节点确认对话框
    Dialog {
        id: deleteConfirmDialog
        title: qsTr("确认删除")
        anchors.centerIn: parent
        width: Math.min(320, parent.width - 32)
        modal: true
        standardButtons: Dialog.Yes | Dialog.No

        property string nodeName: ""
        property int nodeId: -1

        Label {
            text: qsTr("确定要删除节点 \"%1\" 吗？").arg(deleteConfirmDialog.nodeName)
            wrapMode: Text.WordWrap
            width: parent ? parent.width : 280
        }

        onAccepted: {
            if (deleteConfirmDialog.nodeId >= 0)
                FavoriteNodeViewModel.removeNode(deleteConfirmDialog.nodeId)
        }
    }

    // 清空全部节点确认对话框
    Dialog {
        id: clearConfirmDialog
        title: qsTr("确认清空")
        anchors.centerIn: parent
        width: Math.min(320, parent.width - 32)
        modal: true
        standardButtons: Dialog.Yes | Dialog.No

        Label {
            text: qsTr("确定要清空所有节点吗？此操作不可恢复。")
            wrapMode: Text.WordWrap
            width: parent ? parent.width : 280
        }

        onAccepted: FavoriteNodeViewModel.clearAll()
    }
}
