/* @brief 服务器地址列表组件：管理 uri + publicKey 列表，支持添加、删除和去重检测 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// 服务器地址列表组件：管理 uri + publicKey 列表
// 支持添加、删除、去重检测
/* @brief 服务器列表根布局，包含 ListView、添加按钮和添加对话框 */
ColumnLayout {
    id: root

    /* 绑定的数据模型，每项含 uri 和 publicKey 字段 */
    property alias model: listModel

    /* 列表发生增删变更时通知外部写回 ViewModel */
    signal changed()
    /* 添加时检测到重复 uri 时发出 */
    signal duplicateDetected(string msg)

    spacing: 4

    ListModel { id: listModel }

    ListView {
        id: listView
        Layout.fillWidth: true
        // 动态计算高度
        Layout.preferredHeight: listModel.count === 0 ? 0 : listModel.count * 40 + 4
        spacing: 2
        model: listModel
        interactive: false

        delegate: Rectangle {
            required property int index
            required property var model

            width: ListView.view.width
            height: 38
            // 斑马条纹背景
            color: index % 2 === 0 ? "transparent"
                                   : Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.04)
            radius: 4

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 4
                spacing: 4

                Label {
                    text: model.uri + (model.publicKey ? qsTr("【安全】") : "")
                    font.bold: model.publicKey !== ""
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }

                ToolButton {
                    icon.source: "qrc:/icons/delete.svg"
                    icon.color: "transparent"
                    icon.width: 14
                    icon.height: 14
                    padding: 0
                    flat: true
                    implicitWidth: 28
                    implicitHeight: 28
                    onClicked: { listModel.remove(index); root.changed() }
                }
            }
        }
    }

    Button {
        id: addButton
        Layout.fillWidth: true
        flat: true
        text: qsTr("添加服务器地址")
        contentItem: Label {
            text: addButton.text
            color: palette.highlight
            renderType: Text.NativeRendering
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        onClicked: {
            addDialog.uriText = ""
            addDialog.publicKeyText = ""
            addDialog.open()
        }
    }

    // 添加服务器地址对话框
    Dialog {
        id: addDialog
        title: qsTr("添加服务器地址")
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(420, parent ? parent.width - 48 : 360)

        // 暴露输入框文本便于预设／清空
        property alias uriText: uriField.text
        property alias publicKeyText: publicKeyField.text

        ColumnLayout {
            width: parent ? parent.width : 360
            spacing: 8

            Label {
                text: qsTr("服务器地址（必填）")
            }

            TextField {
                id: uriField
                Layout.fillWidth: true
                placeholderText: "tcp://example.qtet.cn:11010"
            }

            Label {
                text: qsTr("服务器公钥（选填）")
            }

            TextField {
                id: publicKeyField
                Layout.fillWidth: true
                placeholderText: qsTr("留空表示不使用公钥验证")
            }
        }

        // 打开时自动聚焦地址输入框
        onOpened: {
            uriField.forceActiveFocus()
            uriField.selectAll()
        }

        // 确认添加：去重检查 → 追加 → 通知变更
        onAccepted: {
            var uri = uriField.text.trim()
            if (!uri) return
            for (var i = 0; i < listModel.count; i++) {
                if (listModel.get(i).uri === uri) {
                    root.duplicateDetected(qsTr("服务器地址已存在"))
                    return
                }
            }
            listModel.append({ uri: uri, publicKey: publicKeyField.text.trim() })
            root.changed()
            uriField.text = ""
            publicKeyField.text = ""
        }
    }
}
