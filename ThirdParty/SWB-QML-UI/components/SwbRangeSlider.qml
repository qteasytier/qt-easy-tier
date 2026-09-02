import QtQuick
import QtQuick.Controls.Basic

RangeSlider {
    id: control

    property SwbStyle theme: SwbStyle {}

    implicitWidth: 200
    opacity: enabled ? 1.0 : 0.5

    // Secondary track with a primary segment between the two handles.
    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.availableWidth
        height: control.theme.trackThickness
        radius: height / 2
        color: control.theme.secondary

        // Align the fill with both handle centers so the track cannot show through as a gap.
        Rectangle {
            x: control.first.position * (parent.width - control.theme.handleSize) + control.theme.handleSize / 2
            width: (control.second.position - control.first.position) * (parent.width - control.theme.handleSize)
            height: parent.height
            radius: parent.radius
            color: control.theme.primary
        }
    }

    // Start handle with a white fill and gray ring border.
    first.handle: Rectangle {
        x: control.leftPadding + control.first.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: control.theme.handleSize
        implicitHeight: control.theme.handleSize
        radius: width / 2
        color: control.theme.handleColor
        border.color: control.theme.ring
        border.width: 1

        // Hover and pressed ring.
        Rectangle {
            anchors.fill: parent
            anchors.margins: -control.theme.focusRingWidth
            radius: parent.radius + control.theme.focusRingWidth
            color: "transparent"
            border.color: control.theme.focusRing
            border.width: control.theme.focusRingWidth
            visible: control.first.pressed || control.first.hovered
        }
    }

    // End handle.
    second.handle: Rectangle {
        x: control.leftPadding + control.second.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: control.theme.handleSize
        implicitHeight: control.theme.handleSize
        radius: width / 2
        color: control.theme.handleColor
        border.color: control.theme.ring
        border.width: 1

        Rectangle {
            anchors.fill: parent
            anchors.margins: -control.theme.focusRingWidth
            radius: parent.radius + control.theme.focusRingWidth
            color: "transparent"
            border.color: control.theme.focusRing
            border.width: control.theme.focusRingWidth
            visible: control.second.pressed || control.second.hovered
        }
    }
}
