/* @file ServerListItem.qml (DDE)
 * @brief DDE 版服务器地址列表组件：管理 uri + publicKey 列表，支持添加、编辑、删除和去重检测，DTK 控件版
 *
 * 行样式沿用共享版（悬停浅色反馈 + 行底分隔线），添加/编辑对话框为模态 DialogWindow。
 */
import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import QtEasyTier
import org.deepin.dtk

// 服务器地址列表组件：管理 uri + publicKey 列表
// 支持添加、编辑、删除、去重检测
/* @brief 服务器列表根布局，包含 ListView、添加按钮和添加/编辑对话框 */
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

    QQC.ListView {
        id: listView
        Layout.fillWidth: true
        // 动态计算高度（行高 38，分隔线风格无行距）
        Layout.preferredHeight: listModel.count === 0 ? 0 : listModel.count * 38
        spacing: 0
        model: listModel
        clip: true
        interactive: false

        delegate: Rectangle {
            id: serverRow

            required property int index
            required property var model

            width: ListView.view.width
            height: 38
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
                visible: serverRow.index < listView.count - 1
            }

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

                // IconToolButton 位于 dde/components/（跨子目录），经 QtEasyTier 显式导入解析
                IconToolButton {
                    iconSource: "qrc:/icons/edit.svg"
                    flat: true
                    onClicked: addDialog.openForEdit(serverRow.index)
                }

                IconToolButton {
                    iconSource: "qrc:/icons/delete.svg"
                    flat: true
                    onClicked: { listModel.remove(index); root.changed() }
                }
            }
        }
    }

    Button {
        id: addButton
        Layout.fillWidth: true
        text: qsTr("添加服务器地址")
        onClicked: addDialog.openForAdd()
    }

    // 添加/编辑服务器地址对话框（editingIndex 区分模式：-1 添加，>=0 编辑）
    // 校验失败（空 uri/重复）时保持窗口打开
    DialogWindow {
        id: addDialog

        modality: Qt.ApplicationModal

        title: ""
        width: 440

        /* 当前对话框模式：-1 添加，>=0 为正在编辑的列表项索引 */
        property int editingIndex: -1

        function open() {
            dialogTitleBar.title = editingIndex >= 0 ? qsTr("编辑服务器地址") : qsTr("添加服务器地址")
            hadShown = true
            visible = true
            requestActivate()
            uriField.forceActiveFocus()
            uriField.selectAll()
        }

        function close() {
            visible = false
        }

        property bool hadShown: false
        onVisibleChanged: {
            if (!visible && hadShown)
                hadShown = false
        }

        /* 确认提交：非空校验 → uri 去重检查（编辑时排除自身）→ 追加/原位更新 → 通知变更 */
        function acceptServer() {
            var uri = uriField.text.trim()
            if (!uri)
                return
            for (var i = 0; i < listModel.count; i++) {
                if (i !== editingIndex && listModel.get(i).uri === uri) {
                    root.duplicateDetected(qsTr("服务器地址已存在"))
                    return
                }
            }
            if (editingIndex >= 0) {
                listModel.set(editingIndex, { uri: uri, publicKey: publicKeyField.text.trim() })
                editingIndex = -1
            } else {
                listModel.append({ uri: uri, publicKey: publicKeyField.text.trim() })
                uriField.text = ""
                publicKeyField.text = ""
            }
            root.changed()
            close()
        }

        /* 以添加模式打开：清空输入 */
        function openForAdd() {
            editingIndex = -1
            uriField.text = ""
            publicKeyField.text = ""
            open()
        }

        /* 以编辑模式打开：预填当前项内容 */
        function openForEdit(idx) {
            editingIndex = idx
            var item = listModel.get(idx)
            uriField.text = item.uri
            publicKeyField.text = item.publicKey || ""
            open()
        }

        header: DialogTitleBar {
            id: dialogTitleBar
            title: qsTr("添加服务器地址")
        }

        ColumnLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 24
            anchors.rightMargin: 24
            spacing: 8

            Label {
                Layout.topMargin: 12
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
                onAccepted: addDialog.acceptServer()
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.bottomMargin: 12
                spacing: 10

                Item { Layout.fillWidth: true }

                Button {
                    text: qsTr("取消")
                    onClicked: addDialog.close()
                }

                RecommandButton {
                    text: addDialog.editingIndex >= 0 ? qsTr("保存") : qsTr("添加")
                    onClicked: addDialog.acceptServer()
                }
            }
        }
    }
}
