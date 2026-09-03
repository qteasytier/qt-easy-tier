/* @file FormPassword.qml (DDE)
 * @brief DDE 版密码字段渲染器：密码模式 TextField + 眼睛按钮切换明文
 */
import QtQuick
import QtQuick.Layouts
import QtEasyTier
import org.deepin.dtk

/* @brief 密码字段渲染器根布局 */
RowLayout {
    id: root

    /* 字段元数据（FormField 分发传入） */
    required property var field
    /* 是否明文显示（纯 UI 状态，不上传 ViewModel） */
    property bool revealText: false

    width: parent ? parent.width : 0

    Label {
        text: root.field.title
        // 标签列钉死固定宽度，保证各行输入框左缘对齐
        Layout.preferredWidth: 140
        Layout.minimumWidth: 140
        Layout.maximumWidth: 140
        elide: Text.ElideRight
    }

    Item {
        Layout.fillWidth: true
        implicitHeight: secretField.implicitHeight

        TextField {
            id: secretField
            anchors.fill: parent
            rightPadding: secretToggle.implicitWidth + 8
            placeholderText: root.field.placeholder ?? ""
            text: ConfigEditorViewModel[root.field.key]
            onTextEdited: ConfigEditorViewModel.setFieldValue(root.field.key, text)
            echoMode: root.revealText ? TextInput.Normal : TextInput.Password
        }

        // IconToolButton 位于 dde/components/（跨子目录），经 QtEasyTier 显式导入解析
        IconToolButton {
            id: secretToggle
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            iconSource: root.revealText ? "qrc:/icons/eye-slash.svg" : "qrc:/icons/eye.svg"
            onClicked: root.revealText = !root.revealText
        }
    }
}
