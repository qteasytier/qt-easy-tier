/* @file ConfirmDialog.qml (DDE)
 * @brief DDE 通用模态确认/输入对话框基座：DTK DialogWindow 独立顶层窗
 *
 * 两种形态：纯确认（message + 取消/确定，danger 时确定用 WarningButton）、
 * 输入模式（inputMode，正文下单行输入框，确定时回写 inputText）。
 * 页面级确认/重命名/导入对话框统一从本基座实例化。
 */
import QtQuick
import QtQuick.Layouts
import org.deepin.dtk

DialogWindow {
    id: root

    modality: Qt.ApplicationModal

    /* Window.title 恒空（对齐 ErrorDialog）：标题只经 headerTitle 呈现于应用内标题栏，
     * 避免窗管未启用 NoTitlebar 时系统标题栏与应用内标题栏重复显示同一标题 */
    title: ""
    width: 420

    /* 标题栏文案（呈现于 DialogTitleBar；不写 Window.title） */
    property string headerTitle: ""

    /* 正文消息（支持换行，空串时不占位） */
    property string message: ""
    /* 危险操作：确定按钮改用 WarningButton */
    property bool danger: false
    /* 按钮文案 */
    property string confirmText: qsTr("确定")
    property string cancelText: qsTr("取消")
    /* 是否显示取消按钮（纯提示型对话框可关闭） */
    property bool showCancel: true

    /* 输入模式：正文下显示单行输入框；确定时先回写 inputText 再发 accepted */
    property bool inputMode: false
    property string inputText: ""
    property string inputPlaceholder: ""

    /* 确定/取消信号（命名沿用共享版 Dialog 的 onAccepted/onRejected 习惯） */
    signal accepted()
    signal rejected()
    /* 窗口关闭（含标题栏 X）时发出，兼容共享版 Dialog.onClosed */
    signal closed()

    /* 真正打开过标记：仅在 open→close 结束才发 closed */
    property bool hadShown: false

    function open() {
        hadShown = true
        visible = true
        requestActivate()
        if (inputMode) {
            inputField.text = root.inputText
            inputField.selectAll()
            inputField.forceActiveFocus()
        }
    }

    function close() {
        visible = false
    }

    onVisibleChanged: {
        if (!visible && hadShown) {
            hadShown = false
            closed()
        }
    }

    /* 确定按钮统一入口：输入模式下先落回输入值 */
    function _accept() {
        if (inputMode)
            root.inputText = inputField.text
        root.accepted()
        root.close()
    }

    header: DialogTitleBar {
        title: root.headerTitle
    }

    // DialogWindow 高度由内容自动决定，内容以 anchors 铺满并控制左右边距
    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 24
        anchors.rightMargin: 24
        spacing: 16

        Label {
            Layout.fillWidth: true
            Layout.topMargin: 12
            visible: root.message !== ""
            text: root.message
            wrapMode: Text.WordWrap
        }

        TextField {
            id: inputField
            Layout.fillWidth: true
            visible: root.inputMode
            placeholderText: root.inputPlaceholder
            onAccepted: root._accept()

            // Esc 关窗（DialogWindow 不内建 standardButtons 语义）
            Shortcut {
                sequence: "Escape"
                onActivated: root.close()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.bottomMargin: 12
            spacing: 10

            // 弹性空间，按钮组统一推至右下角
            Item { Layout.fillWidth: true }

            Button {
                visible: root.showCancel
                text: root.cancelText
                onClicked: {
                    root.rejected()
                    root.close()
                }
            }

            WarningButton {
                visible: root.danger
                text: root.confirmText
                onClicked: root._accept()
            }

            RecommandButton {
                visible: !root.danger
                text: root.confirmText
                onClicked: root._accept()
            }
        }
    }
}
