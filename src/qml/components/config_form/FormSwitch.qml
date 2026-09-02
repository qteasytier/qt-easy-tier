/* @brief 开关字段渲染器：整行 SwbSwitch，值经 setFieldValue 写回 ViewModel */
import QtQuick
import QtQuick.Layouts
import QtEasyTier
import SwbControls

/* @brief 开关渲染器根布局 */
ColumnLayout {
    id: root

    /* 字段元数据（FormField 分发传入） */
    required property var field

    width: parent ? parent.width : 0

    SwbSwitch {
        Layout.fillWidth: true
        text: root.field.title
        checked: ConfigEditorViewModel[root.field.key]
        onToggled: ConfigEditorViewModel.setFieldValue(root.field.key, checked)
    }
}
