/* @brief 统一的图标工具按钮：显式收紧内边距，并直接绘制图标内容，避免 WinUI 默认按钮内容区留白 */
import QtQuick
import QtQuick.Controls
import QtQuick.Effects

ToolButton {
    id: root

    property url iconSource: ""
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

    contentItem: Item {
        Image {
            anchors.centerIn: parent
            width: root.iconSize
            height: root.iconSize
            source: root.iconSource
            fillMode: Image.PreserveAspectFit
            smooth: true
            sourceSize: Qt.size(root.iconSize, root.iconSize)
            layer.enabled: root.iconTint.a > 0
            layer.effect: MultiEffect {
                colorization: 1.0
                colorizationColor: root.iconTint
            }
        }
    }
}
