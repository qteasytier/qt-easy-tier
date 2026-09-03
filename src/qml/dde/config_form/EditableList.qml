/* @file EditableList.qml (DDE)
 * @brief DDE 版可编辑列表组件：通用列表 + 添加/编辑/删除功能，适合管理字符串列表（如端口、监听地址、CIDR 等），DTK 控件版
 *
 * 添加/编辑对话框为模态 DialogWindow（独立顶层窗）：校验失败（空值/重复）时保持窗口打开。
 */
import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import QtEasyTier
import org.deepin.dtk

// 可编辑列表组件：通用列表 + 添加/编辑/删除功能
// 通过 model 属性绑定外部 ListModel，适合管理字符串列表（如端口、端点等）
/* @brief 可编辑列表根布局，包含 ListView、添加按钮和添加/编辑对话框 */
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

    /* 编辑对话框标题：由添加标题前缀“添加”推导为“编辑”，无此前缀时回退通用标题 */
    function editDialogTitle() {
        var prefix = qsTr("添加")
        return root.addDialogTitle.indexOf(prefix) === 0
            ? qsTr("编辑") + root.addDialogTitle.substring(prefix.length)
            : qsTr("编辑项")
    }

    spacing: 4

    ListModel { id: listModel }

    QQC.ListView {
        id: listView
        Layout.fillWidth: true
        // 根据项数动态计算高度，空列表时高度为 0（行高 38，分隔线风格无行距）
        Layout.preferredHeight: listModel.count === 0 ? 0 : listModel.count * 38
        spacing: 0
        model: listModel
        clip: true
        interactive: false

        delegate: EditableListItem {
            id: listItem
            required property int index
            required property var model
            itemIndex: index
            itemText: model[root.itemKey]
            onRemoveRequested: function(idx) { listModel.remove(idx); root.changed() }
            onEditRequested: function(idx) { addDialog.openForEdit(idx) }
        }
    }

    Button {
        id: addButton
        Layout.fillWidth: true
        text: root.addDialogTitle
        onClicked: addDialog.openForAdd()
    }

    // 添加/编辑项对话框（editingIndex 区分模式：-1 添加，>=0 编辑）
    // 校验失败（空值/重复）时不关窗，由用户修正后重试
    DialogWindow {
        id: addDialog

        modality: Qt.ApplicationModal

        title: ""
        width: 420

        /* 当前对话框模式：-1 添加，>=0 为正在编辑的列表项索引 */
        property int editingIndex: -1

        /* 标题随模式切换（DialogWindow.title 由 DialogTitleBar 呈现） */
        function open() {
            dialogTitleBar.title = editingIndex >= 0 ? root.editDialogTitle() : root.addDialogTitle
            hadShown = true
            visible = true
            requestActivate()
            editField.forceActiveFocus()
            editField.selectAll()
        }

        function close() {
            visible = false
        }

        /* 真正打开过标记：仅在 open→close 结束才发 closed（兼容页面级清理逻辑） */
        property bool hadShown: false
        onVisibleChanged: {
            if (!visible && hadShown)
                hadShown = false
        }

        /* 确认提交：非空校验 → 去重检查（编辑时排除自身）→ 追加/原位更新 → 通知变更 */
        function acceptItem() {
            var value = editField.text.trim()
            if (!value)
                return
            if (root.checkDuplicates) {
                for (var i = 0; i < listModel.count; i++) {
                    if (i !== editingIndex && listModel.get(i)[root.itemKey] === value) {
                        root.duplicateDetected(qsTr("已存在相同的项"))
                        return
                    }
                }
            }
            if (editingIndex >= 0) {
                listModel.setProperty(editingIndex, root.itemKey, value)
                editingIndex = -1
            } else {
                var item = {}
                item[root.itemKey] = value
                listModel.append(item)
                editField.text = ""
            }
            root.changed()
            close()
        }

        /* 以添加模式打开：预填默认值 */
        function openForAdd() {
            editingIndex = -1
            editField.text = root.defaultAddValue
            open()
        }

        /* 以编辑模式打开：预填当前项内容 */
        function openForEdit(idx) {
            editingIndex = idx
            editField.text = listModel.get(idx)[root.itemKey]
            open()
        }

        header: DialogTitleBar {
            id: dialogTitleBar
            title: root.addDialogTitle
        }

        ColumnLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 24
            anchors.rightMargin: 24
            spacing: 16

            TextField {
                id: editField
                Layout.fillWidth: true
                Layout.topMargin: 12
                placeholderText: root.defaultAddValue
                onAccepted: addDialog.acceptItem()
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.bottomMargin: 12
                spacing: 10

                Item { Layout.fillWidth: true }

                Button {
                    text: qsTr("取消")
                    onClicked: addDialog.close()
                }

                RecommandButton {
                    text: addDialog.editingIndex >= 0 ? qsTr("保存") : qsTr("添加")
                    onClicked: addDialog.acceptItem()
                }
            }
        }
    }
}
