/* @brief 可编辑列表组件：通用列表 + 添加/删除功能，适合管理字符串列表（如端口、监听地址、CIDR 等） */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// 可编辑列表组件：通用列表 + 添加/删除功能
// 通过 model 属性绑定外部 ListModel，适合管理字符串列表（如端口、端点等）
/* @brief 可编辑列表根布局，包含 ListView、添加按钮和添加对话框 */
ColumnLayout {
    id: root

    /* 绑定的数据模型，外部通过 model alias 直接访问和操作 */
    property alias model: listModel
    /* 列表项中取值用的 key，默认 "value" */
    property string itemKey: "value"

    /* 是否在添加时检查重复项 */
    property bool checkDuplicates: false
    /* 检测到重复项时发出，参数为提示信息 */
    signal duplicateDetected(string msg)

    /* 列表数据发生增删变更时通知外部 */
    signal changed()

    /* 添加按钮和对话框的标题文本 */
    property string addDialogTitle: qsTr("添加项")
    /* 添加新项时对话框输入框的默认填充值 */
    property string defaultAddValue: ""

    spacing: 4

    ListModel { id: listModel }

    ListView {
        id: listView
        Layout.fillWidth: true
        // 根据项数动态计算高度，空列表时高度为 0
        Layout.preferredHeight: listModel.count === 0 ? 0 : listModel.count * 40 + 4
        spacing: 2
        model: listModel
        interactive: false

        delegate: EditableListItem {
            required property int index
            required property var model
            itemIndex: index
            itemText: model[root.itemKey]
            onRemoveRequested: function(idx) { listModel.remove(idx); root.changed() }
        }
    }

    Button {
        id: addButton
        Layout.fillWidth: true
        flat: true
        text: root.addDialogTitle
        contentItem: Label {
            text: addButton.text
            color: palette.highlight
            renderType: Text.NativeRendering
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        onClicked: {
            addDialog.fieldText = root.defaultAddValue
            addDialog.open()
        }
    }

    // 添加项对话框
    Dialog {
        id: addDialog
        title: root.addDialogTitle
        standardButtons: Dialog.Ok | Dialog.Cancel
        modal: true
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(420, parent ? parent.width - 48 : 360)

        // 暴露输入框文本便于外部预设
        property alias fieldText: editField.text

        TextField {
            id: editField
            width: parent.width
            placeholderText: root.defaultAddValue
        }

        // 打开时自动聚焦并全选
        onOpened: {
            editField.forceActiveFocus()
            editField.selectAll()
        }
        // 确认添加：去重检查 → 追加 → 通知变更
        onAccepted: {
            var value = editField.text.trim()
            if (!value) return
            if (root.checkDuplicates) {
                for (var i = 0; i < listModel.count; i++) {
                    if (listModel.get(i)[root.itemKey] === value) {
                        root.duplicateDetected(qsTr("已存在相同的项"))
                        return
                    }
                }
            }
            var item = {}
            item[root.itemKey] = value
            listModel.append(item)
            root.changed()
            editField.text = ""
        }
    }
}
