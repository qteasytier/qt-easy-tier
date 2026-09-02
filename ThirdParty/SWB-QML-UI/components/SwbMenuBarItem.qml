import QtQuick
import QtQuick.Controls.impl
import QtQuick.Controls.Basic

MenuBarItem {
    id: control

    property SwbStyle theme: SwbStyle {}

    property color bgColor: control.down || control.highlighted || control.hovered
                                      ? control.theme.secondary
                                      : control.theme.withAlpha(control.theme.secondary, 0)

    property int alignment: Qt.AlignCenter  // Qt.Alignment flags placing the icon and text in the item.

    horizontalPadding: 6
    verticalPadding: 0
    spacing: 6
    icon.width: control.theme.iconSize
    icon.height: control.theme.iconSize
    hoverEnabled: enabled
    font.pixelSize: control.theme.fontSize
    font.weight: control.theme.fontWeight
    opacity: enabled ? 1.0 : 0.5

    contentItem: SwbIconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        alignment: control.alignment
        icon: control.icon
        text: control.text
        font: control.font
        color: control.theme.foreground
    }

    background: Rectangle {
        implicitHeight: control.theme.controlHeight - 6
        radius: control.theme.radiusSm
        color: control.bgColor

        Behavior on color { ColorAnimation { duration: control.theme.animationDuration } }

        Rectangle {
            anchors.fill: parent
            anchors.margins: -control.theme.focusRingWidth
            radius: parent.radius + control.theme.focusRingWidth
            color: "transparent"
            border.color: control.theme.focusRing
            border.width: control.theme.focusRingWidth
            visible: control.visualFocus
        }
    }
}
