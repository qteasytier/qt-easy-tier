/* @brief 数值字段渲染器：范围来自字段元数据 from/to，值经 setFieldValue 写回 */
import QtQuick
import QtQuick.Layouts
import QtEasyTier
import SwbControls

/* @brief 数值字段渲染器根布局 */
RowLayout {
    id: root

    /* 字段元数据（FormField 分发传入，from/to 为数值范围） */
    required property var field

    width: parent ? parent.width : 0

    SwbLabel {
        text: root.field.title
        Layout.preferredWidth: 110
    }

    SwbSpinBox {
        Layout.fillWidth: true
        from: root.field.from ?? 0
        to: root.field.to ?? 99
        value: ConfigEditorViewModel[root.field.key]
        onValueModified: ConfigEditorViewModel.setFieldValue(root.field.key, value)
    }
}
