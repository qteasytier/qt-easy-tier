/* @brief 卡片容器组件：基于 SwbFrame 提供 shadcn 风格圆角面板、统一内边距与可选加粗标题 */
import QtQuick
import QtQuick.Layouts
import SwbControls

// 卡片容器：基于 SwbFrame（Frame），提供圆角边框、统一内边距与可选加粗标题
// contentSpacing 属性名避免与 Qt 6.7+ Frame.spacing (FINAL) 冲突
/* @brief 卡片面板，标题（可选）与内容按 ColumnLayout 垂直排列 */
SwbFrame {
    id: root

    /* 卡片标题：非空时在内容区顶部以粗体显示 */
    property string title: ""

    // 内容间距，由外部配置页面传入
    property int contentSpacing: 0

    // 边框颜色，可由外部覆盖（如危险操作卡片使用红色边框）
    property color borderColor: root.theme.border

    padding: 16

    contentItem: ColumnLayout {
        spacing: root.contentSpacing

        // 加粗标题（显式 Font.Bold 覆盖 SwbLabel 的 Medium weight 默认绑定）
        SwbLabel {
            text: root.title
            visible: root.title !== ""
            font.weight: Font.Bold
            Layout.bottomMargin: 2
        }
    }

    background: Rectangle {
        radius: root.theme.radius
        color: root.theme.popover
        border.color: root.borderColor
        border.width: 1
    }
}
