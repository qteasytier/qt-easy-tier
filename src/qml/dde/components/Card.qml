/* @file Card.qml (DDE)
 * @brief DDE 版卡片容器：用 DTK Frame 包裹，背景/边框/圆角与共享版一致
 */
import QtQuick
import QtQuick.Layouts
import org.deepin.dtk 1.0 as D
import org.deepin.dtk.style 1.0 as DStyle

D.Frame {
    id: root

    // 内容间距；避免与 Qt 6.7+ Frame.spacing (FINAL) 冲突
    property int contentSpacing: 0
    // 边框颜色，可由外部覆盖（如危险操作卡片）
    property color borderColor: Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.15)

    radius: DStyle.Style.control.radius
    padding: 16

    contentItem: ColumnLayout {
        spacing: root.contentSpacing
    }

    background: Rectangle {
        radius: root.radius
        color: palette.base
        border.color: root.borderColor
        border.width: 1
    }
}
