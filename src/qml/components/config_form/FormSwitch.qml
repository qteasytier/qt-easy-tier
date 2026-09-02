/* @brief 开关字段渲染器：左侧文字标签占满剩余宽度、右侧纯开关，上下留呼吸边距缓解行间密集 */
import QtQuick
import QtQuick.Layouts
import QtEasyTier
import SwbControls

/* @brief 开关渲染器根容器：内含 SwbLabel + 无文字 SwbSwitch 组合 */
Item {
    id: root

    /* 字段元数据（FormField 分发传入） */
    required property var field

    /* 开关行上下呼吸边距（渲染器位于 Loader 内，Layout 附加属性无效，以增高自身实现） */
    readonly property int verticalBreathing: 2

    width: parent ? parent.width : 0
    implicitHeight: switchRow.implicitHeight + root.verticalBreathing * 2

    RowLayout {
        id: switchRow
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter

        SwbLabel {
            Layout.fillWidth: true
            text: root.field.title
        }

        SwbSwitch {
            checked: ConfigEditorViewModel[root.field.key]
            onToggled: ConfigEditorViewModel.setFieldValue(root.field.key, checked)
        }
    }
}
