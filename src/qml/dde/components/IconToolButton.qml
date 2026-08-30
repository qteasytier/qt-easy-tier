/* @file IconToolButton.qml (DDE)
 * @brief DDE 版统一图标工具按钮：DTK ToolButton 原生绘制，图标色随 DDE 主题
 */
import QtQuick
import org.deepin.dtk 1.0 as D

D.ToolButton {
    id: root

    property url iconSource: ""
    // 保留以兼容调用方；DDE 下图标色随主题
    property color iconTint: "transparent"
    property int iconSize: 18
    property int buttonSize: 32

    padding: 0
    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0
    implicitWidth: buttonSize
    implicitHeight: buttonSize

    // 纯图标按钮，避免 ToolButton 默认 TextUnderIcon 叠加空文本
    display: D.IconLabel.IconOnly
    icon.source: root.iconSource
    icon.width: root.iconSize
    icon.height: root.iconSize
}
