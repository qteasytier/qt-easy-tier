/* @brief 节点收藏页面：管理常用节点的增删改查，支持节点名称/URI/公钥的编辑和清空操作 */
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
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

    /* nodeDialog 懒加载打开前的暂存注入值 */
    property int pendingNodeId: -1
    property bool pendingEditMode: false
    property string pendingNodeName: ""
    property string pendingNodeUri: ""
    property string pendingNodePubkey: ""
    /* deleteConfirmDialog 懒加载打开前的暂存注入值 */
    property string pendingDeleteName: ""
    property int pendingDeleteId: -1

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
                    IconToolButton {
                        iconSource: "qrc:/icons/edit.svg"
                        flat: true
                        onClicked: {
                            root.pendingNodeName = nodeName
                            root.pendingNodeUri = nodeUri
                            root.pendingNodePubkey = nodePublicKey
                            root.pendingNodeId = nodeId
                            root.pendingEditMode = true
                            nodeDialogLoader.active = true
                        }
                    }

                    // 删除按钮：弹出确认对话框
                    IconToolButton {
                        iconSource: "qrc:/icons/delete.svg"
                        flat: true
                        onClicked: {
                            root.pendingDeleteName = nodeName
                            root.pendingDeleteId = nodeId
                            deleteConfirmDialogLoader.active = true
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

            RowLayout {
                spacing: 6

                // 添加节点按钮
                Button {
                    Layout.fillWidth: true
                    text: qsTr("添加节点")
                    onClicked: {
                        // 清空注入值，进入新增模式
                        root.pendingNodeName = ""
                        root.pendingNodeUri = ""
                        root.pendingNodePubkey = ""
                        root.pendingNodeId = -1
                        root.pendingEditMode = false
                        nodeDialogLoader.active = true
                    }
                }

                // 批量导入节点：读取与 publicservers.json 相同格式的 JSON 文件
                Button {
                    Layout.fillWidth: true
                    text: qsTr("导入节点")
                    onClicked: importModeDialogLoader.active = true
                }

                // 批量导出节点：导出为与 publicservers.json 相同格式的 JSON 文件
                Button {
                    Layout.fillWidth: true
                    text: qsTr("导出节点")
                    enabled: FavoriteNodeViewModel.count > 0
                    onClicked: exportNodesFileDialog.open()
                }
            }



            // 清空列表按钮：仅在有节点时可用
            Button {
                Layout.fillWidth: true
                text: qsTr("清空节点列表")
                flat: true
                enabled: FavoriteNodeViewModel.count > 0
                onClicked: clearConfirmDialogLoader.active = true
            }

        }
    }

    // 批量导入方式选择对话框（懒加载：打开时创建，关闭即卸载）
    Component {
        id: importModeDialogComponent
        Dialog {
            id: importModeDialog
            title: qsTr("导入节点")
            modal: true
            anchors.centerIn: parent
            width: Math.min(360, parent ? parent.width - 48 : 320)

            ColumnLayout {
                width: parent ? parent.width : 320
                spacing: 8

                Button {
                    Layout.fillWidth: true
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
                        importUrlDialogLoader.active = true
                    }
                }
            }
            onClosed: importModeDialogLoader.active = false
        }
    }

    Loader {
        id: importModeDialogLoader
        active: false
        onLoaded: item.open()
    }

    // 批量导入 URL 输入对话框（懒加载：打开时创建，关闭即卸载）
    Component {
        id: importUrlDialogComponent
        Dialog {
            id: importUrlDialog
            title: qsTr("从 URL 导入节点")
            standardButtons: Dialog.Ok | Dialog.Cancel
            modal: true
            anchors.centerIn: parent
            width: Math.min(420, parent ? parent.width - 48 : 360)

            ColumnLayout {
                width: parent ? parent.width : 360
                spacing: 8

                Label {
                    text: qsTr("节点 JSON 地址（http/https）")
                }

                TextField {
                    id: importUrlField
                    Layout.fillWidth: true
                    placeholderText: qsTr("https://example.com/nodes.json")
                    inputMethodHints: Qt.ImhUrlCharactersOnly
                }
            }

            onOpened: importUrlField.forceActiveFocus()
            onAccepted: {
                var url = importUrlField.text.trim()
                if (url)
                    FavoriteNodeViewModel.importNodesFromUrl(url)
            }
            onClosed: importUrlDialogLoader.active = false
        }
    }

    Loader {
        id: importUrlDialogLoader
        active: false
        onLoaded: item.open()
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

    // 节点编辑/添加对话框（懒加载，数据由外部暂存注入）
    Component {
        id: nodeDialogComponent
        Dialog {
            id: nodeDialog
            title: nodeDialog.editing ? qsTr("编辑节点") : qsTr("添加节点")
            property int nodeIdValue: -1
            property bool editing: false
            property string nameText: ""
            property string uriText: ""
            property string publicKeyText: ""
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
                    text: nodeDialog.nameText
                    Layout.fillWidth: true
                    placeholderText: qsTr("请输入节点名称")
                }

                Label {
                    text: qsTr("节点地址（必填）")
                }

                TextField {
                    id: editUriField
                    text: nodeDialog.uriText
                    Layout.fillWidth: true
                    placeholderText: qsTr("例如：tcp://example.com:27773")
                }

                Label {
                    text: qsTr("节点公钥（选填）")
                }

                TextField {
                    id: editPublicKeyField
                    text: nodeDialog.publicKeyText
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

                if (nodeDialog.editing && nodeDialog.nodeIdValue >= 0) {
                    FavoriteNodeViewModel.updateNode(nodeDialog.nodeIdValue, name, uri, publicKey)
                } else {
                    FavoriteNodeViewModel.addNode(name, uri, publicKey)
                }
            }
            onClosed: nodeDialogLoader.active = false
        }
    }

    Loader {
        id: nodeDialogLoader
        active: false
        onLoaded: {
            item.nodeIdValue = root.pendingNodeId
            item.editing = root.pendingEditMode
            item.nameText = root.pendingNodeName
            item.uriText = root.pendingNodeUri
            item.publicKeyText = root.pendingNodePubkey
            item.open()
        }
    }

    // 删除单个节点确认对话框（懒加载，数据由外部暂存注入）
    Component {
        id: deleteConfirmDialogComponent
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
            onClosed: deleteConfirmDialogLoader.active = false
        }
    }

    Loader {
        id: deleteConfirmDialogLoader
        active: false
        onLoaded: {
            item.nodeName = root.pendingDeleteName
            item.nodeId = root.pendingDeleteId
            item.open()
        }
    }

    // 清空全部节点确认对话框（懒加载：打开时创建，关闭即卸载）
    Component {
        id: clearConfirmDialogComponent
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
            onClosed: clearConfirmDialogLoader.active = false
        }
    }

    Loader {
        id: clearConfirmDialogLoader
        active: false
        onLoaded: item.open()
    }
}
