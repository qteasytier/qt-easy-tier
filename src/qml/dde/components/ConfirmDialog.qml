/* @file ConfirmDialog.qml (DDE)
 * @brief DDE 通用模态确认/输入对话框基座：DTK DialogWindow 独立顶层窗
 *
 * 两种形态：
 *  - 纯确认（默认）：message 正文 + 取消/确定按钮（danger 时确定用 WarningButton）；
 *  - 输入模式（inputMode）：正文下附带单行输入框，回车即确定，确定时回写 inputText。
 * 页面级对话框（删除确认、重命名、URL 导入等）统一从本基座实例化，
 * 保持 DDE 前端“模态 DialogWindow + DialogTitleBar + 右下角按钮组”的统一风格。
 */
import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import org.deepin.dtk

DialogWindow {
    id: root

    modality: Qt.ApplicationModal

    width: 420

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

    /* 确定/取消信号（命名沿用 QQC.Dialog 的 onAccepted/onRejected 习惯） */
    signal accepted()
    signal rejected()
    /* 窗口关闭（含标题栏 X）时发出，兼容共享版 QQC.Dialog.onClosed */
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
        title: root.title
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
            QQC.Shortcut {
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
