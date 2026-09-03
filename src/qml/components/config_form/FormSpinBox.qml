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
        // 标签列三向钉死同一宽度：保证各行输入框左缘对齐（英文标签较长，110 不够用）
        Layout.preferredWidth: 140
        Layout.minimumWidth: 140
        Layout.maximumWidth: 140
        elide: Text.ElideRight
    }

    SwbSpinBox {
        Layout.fillWidth: true
        from: root.field.from ?? 0
        to: root.field.to ?? 99
        value: ConfigEditorViewModel[root.field.key]
        onValueModified: ConfigEditorViewModel.setFieldValue(root.field.key, value)
    }
}
