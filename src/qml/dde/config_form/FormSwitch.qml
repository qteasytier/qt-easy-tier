/* @file FormSwitch.qml (DDE)
 * @brief DDE 版开关字段渲染器：左侧 DTK Label 占满剩余宽度、右侧 DTK Switch，上下留呼吸边距缓解行间密集 */
import QtQuick
import QtQuick.Layouts
import QtEasyTier
import org.deepin.dtk

/* @brief 开关渲染器根容器：内含 Label + 无文字 Switch 组合 */
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

        Label {
            Layout.fillWidth: true
            text: root.field.title
        }

        Switch {
            checked: ConfigEditorViewModel[root.field.key]
            onToggled: ConfigEditorViewModel.setFieldValue(root.field.key, checked)
        }
    }
}
