/* @brief 全局错误对话框：共享给普通模式与 DDE 模式复用，基于 SwbDialog 呈现 shadcn 风格 */
import QtQuick
import QtQuick.Controls
import SwbControls

/* @brief 错误对话框根控件，带标题栏与关闭按钮 */
SwbDialog {
    id: root

    property string text: ""

    title: qsTr("错误")
    parent: Overlay.overlay
    anchors.centerIn: parent
    standardButtons: Dialog.Ok
    width: Math.min(420, parent ? parent.width - 48 : 360)

    SwbLabel {
        text: root.text
        wrapMode: Text.WordWrap
        width: parent ? parent.width : 360
    }
}
