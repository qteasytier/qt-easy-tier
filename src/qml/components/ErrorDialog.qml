/* @brief 全局错误对话框：共享给普通模式与 DDE 模式复用 */
import QtQuick
import QtQuick.Controls

Dialog {
    id: root

    property string text: ""

    title: qsTr("错误")
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    standardButtons: Dialog.Ok
    width: Math.min(420, parent ? parent.width - 48 : 360)

    Label {
        text: root.text
        wrapMode: Text.WordWrap
        width: parent ? parent.width : 360
    }
}
