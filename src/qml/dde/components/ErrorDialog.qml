/* @file ErrorDialog.qml
 * @brief DDE 模式错误弹窗：使用 DTK DialogWindow 作为独立顶层窗口
 */
import QtQuick
import QtQuick.Layouts
import org.deepin.dtk

DialogWindow {
    id: root

    property string text: ""

    title: ""
    width: 420

    function open() {
        visible = true
        requestActivate()
    }

    function close() {
        visible = false
    }

    header: DialogTitleBar {
        id: dialogTitleBar
        title: qsTr("错误")
    }

    // DialogWindow 高度由内容自动决定，此处按子项自然尺寸排布即可
    ColumnLayout {
        width: root.width
        spacing: 16

        Label {
            Layout.fillWidth: true
            Layout.topMargin: 12
            Layout.leftMargin: 24
            Layout.rightMargin: 24
            text: root.text
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        ButtonBox {
            Layout.alignment: Qt.AlignRight
            Layout.rightMargin: 24
            Layout.bottomMargin: 12

            Button {
                focus: true
                text: qsTr("确定")
                onClicked: root.close()
            }
        }
    }
}
