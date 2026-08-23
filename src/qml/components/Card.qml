/* @brief 卡片容器组件：基于 Frame 提供圆角边框和统一内边距，用于包裹各类设置区块 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// 卡片容器：基于 Frame，提供圆角边框和统一内边距
// contentSpacing 属性名避免与 Qt 6.7+ Frame.spacing (FINAL) 冲突
/* @brief 卡片面板，内含 ColumnLayout 支持子元素垂直排列 */
Frame {
    id: root

    // 内容间距，由外部配置页面传入
    property int contentSpacing: 0

    // 边框颜色，可由外部覆盖（如危险操作卡片使用红色边框）
    property color borderColor: Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.15)

    padding: 16

    contentItem: ColumnLayout {
        spacing: root.contentSpacing
    }

    background: Rectangle {
        radius: 8
        color: palette.base
        border.color: root.borderColor
        border.width: 1
    }
}
