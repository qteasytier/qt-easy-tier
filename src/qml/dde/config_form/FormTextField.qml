/* @file FormTextField.qml (DDE)
 * @brief DDE 版文本输入字段渲染器：左侧标签 + 右侧 DTK TextField，title 为空时输入框占满全宽 */
import QtQuick
import QtQuick.Layouts
import QtEasyTier
import org.deepin.dtk

/* @brief 文本字段渲染器根布局 */
RowLayout {
    id: root

    /* 字段元数据（FormField 分发传入） */
    required property var field

    width: parent ? parent.width : 0

    Label {
        text: root.field.title
        // 标签列三向钉死同一宽度：保证各行输入框左缘对齐（英文标签较长，110 不够用）
        Layout.preferredWidth: 140
        Layout.minimumWidth: 140
        Layout.maximumWidth: 140
        elide: Text.ElideRight
        visible: root.field.title !== ""
    }

    TextField {
        Layout.fillWidth: true
        placeholderText: root.field.placeholder ?? ""
        text: ConfigEditorViewModel[root.field.key]
        onTextEdited: ConfigEditorViewModel.setFieldValue(root.field.key, text)
    }
}
