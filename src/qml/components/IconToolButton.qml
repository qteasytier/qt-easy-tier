/* @brief 统一的图标工具按钮：基于 SwbToolButton，固定为图标正方形按钮并保留原有 iconSource/iconTint API */
import QtQuick
import SwbControls

/* @brief 图标按钮根控件，透传 SwbToolButton 的悬停/焦点反馈 */
SwbToolButton {
    id: root

    // 图标资源地址（qrc SVG），保持与旧版 Image + MultiEffect 着色相同的 API 语义：
    // iconTint 为透明色时按 SVG 原色渲染，否则以前景色方式着色
    property url iconSource: ""
    property color iconTint: "transparent"
    property int iconSize: 18
    property int buttonSize: 32

    icon.source: root.iconSource
    icon.width: root.iconSize
    icon.height: root.iconSize
    icon.color: root.iconTint.a > 0 ? root.iconTint : "transparent"

    padding: 0
    implicitWidth: root.buttonSize
    implicitHeight: root.buttonSize
}
