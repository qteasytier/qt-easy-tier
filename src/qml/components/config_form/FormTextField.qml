/* @brief 文本输入字段渲染器：左侧标签 + 右侧 SwbTextField，title 为空时输入框占满全宽 */
import QtQuick
import QtQuick.Layouts
import QtEasyTier
import SwbControls

/* @brief 文本字段渲染器根布局 */
RowLayout {
    id: root

    /* 字段元数据（FormField 分发传入） */
    required property var field

    width: parent ? parent.width : 0

    SwbLabel {
        text: root.field.title
        Layout.preferredWidth: 110
        visible: root.field.title !== ""
    }

    SwbTextField {
        Layout.fillWidth: true
        placeholderText: root.field.placeholder ?? ""
        text: ConfigEditorViewModel[root.field.key]
        onTextEdited: ConfigEditorViewModel.setFieldValue(root.field.key, text)
    }
}
