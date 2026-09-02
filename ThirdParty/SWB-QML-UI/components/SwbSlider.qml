import QtQuick
import QtQuick.Controls.Basic

Slider {
    id: control

    property SwbStyle theme: SwbStyle {}

    implicitWidth: 200
    opacity: enabled ? 1.0 : 0.5

    // Secondary track with a primary filled segment.
    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.availableWidth
        height: control.theme.trackThickness
        radius: height / 2
        color: control.theme.secondary

        // Fill to the handle center so the track cannot show through as a small gap.
        Rectangle {
            width: control.visualPosition * (parent.width - control.theme.handleSize) + control.theme.handleSize / 2
            height: parent.height
            radius: parent.radius
            color: control.theme.primary
        }
    }

    // Handle with a white fill and gray ring border.
    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: control.theme.handleSize
        implicitHeight: control.theme.handleSize
        radius: width / 2
        color: control.theme.handleColor
        border.color: control.theme.ring
        border.width: 1

        // Hover and keyboard-focus ring.
        Rectangle {
            anchors.fill: parent
            anchors.margins: -control.theme.focusRingWidth
            radius: parent.radius + control.theme.focusRingWidth
            color: "transparent"
            border.color: control.theme.focusRing
            border.width: control.theme.focusRingWidth
            visible: control.hovered || control.visualFocus
        }
    }
}
