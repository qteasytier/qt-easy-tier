/* @file FormFilePath.qml (DDE)
 * @brief DDE 版文件路径字段渲染器：顶部标签 + DTK 路径输入框 + 选择按钮，内嵌 JSON 文件选择对话框 */
import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import QtEasyTier
import org.deepin.dtk

/* @brief 文件路径字段渲染器根布局 */
ColumnLayout {
    id: root

    /* 字段元数据（FormField 分发传入） */
    required property var field

    width: parent ? parent.width : 0

    // 标签独立成行（沿用原“临时密钥文件(.json)”的布局形态）
    Label {
        text: root.field.title
        visible: root.field.title !== ""
        Layout.topMargin: 4
    }

    RowLayout {
        Layout.fillWidth: true

        TextField {
            id: pathField
            Layout.fillWidth: true
            placeholderText: root.field.placeholder ?? ""
            text: ConfigEditorViewModel[root.field.key]
            onTextEdited: ConfigEditorViewModel.setFieldValue(root.field.key, text)
        }

        // IconToolButton 位于 dde/components/（跨子目录），经 QtEasyTier 显式导入解析
        IconToolButton {
            iconSource: "qrc:/icons/edit.svg"
            onClicked: fileDialog.open()
        }
    }

    // 文件选择对话框（FileMode 选择的必然是文件）
    FileDialog {
        id: fileDialog
        title: qsTr("选择临时密钥文件")
        nameFilters: [qsTr("JSON 文件 (*.json)"), qsTr("所有文件 (*)")]
        fileMode: FileDialog.OpenFile
        onAccepted: {
            var p = ConfigEditorViewModel.toLocalFilePath(selectedFile.toString())
            if (p !== "")
                ConfigEditorViewModel.setFieldValue(root.field.key, p)
        }
    }
}
