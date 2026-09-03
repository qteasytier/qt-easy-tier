/* @brief 密码字段渲染器：SwbTextField 密码模式 + 眼睛按钮切换明文，明文可见性为渲染器内部 UI 状态 */
import QtQuick
import QtQuick.Layouts
import QtEasyTier
import SwbControls

/* @brief 密码字段渲染器根布局 */
RowLayout {
    id: root

    /* 字段元数据（FormField 分发传入） */
    required property var field
    /* 是否明文显示（纯 UI 状态，不上传 ViewModel） */
    property bool revealText: false

    width: parent ? parent.width : 0

    SwbLabel {
        text: root.field.title
        // 标签列三向钉死同一宽度：保证各行输入框左缘对齐（英文标签较长，110 不够用）
        Layout.preferredWidth: 140
        Layout.minimumWidth: 140
        Layout.maximumWidth: 140
        elide: Text.ElideRight
    }

    Item {
        Layout.fillWidth: true
        implicitHeight: secretField.implicitHeight

        SwbTextField {
            id: secretField
            anchors.fill: parent
            rightPadding: secretToggle.implicitWidth + 8
            placeholderText: root.field.placeholder ?? ""
            text: ConfigEditorViewModel[root.field.key]
            onTextEdited: ConfigEditorViewModel.setFieldValue(root.field.key, text)
            echoMode: root.revealText ? TextInput.Normal : TextInput.Password
        }

        IconToolButton {
            id: secretToggle
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            iconSource: root.revealText ? "qrc:/icons/eye-slash.svg" : "qrc:/icons/eye.svg"
            onClicked: root.revealText = !root.revealText
        }
    }
}
