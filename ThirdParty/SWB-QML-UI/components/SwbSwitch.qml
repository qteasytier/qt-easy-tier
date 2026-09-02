import QtQuick
import QtQuick.Controls.Basic

Switch {
    id: control

    property SwbStyle theme: SwbStyle {}

    property string size: "default"  // sm | default

    // Two size presets for the track and thumb.
    property real trackWidth:  size === "sm" ? 24 : 32
    property real trackHeight: size === "sm" ? 14 : 18
    property real thumbSize:   size === "sm" ? 12 : 16
    property real inset: (trackHeight - thumbSize) / 2.0  // Thumb inset from the track.

    spacing: 8
    padding: 0
    hoverEnabled: true
    opacity: enabled ? 1.0 : 0.5

    // With no text, use the track width so the whole track remains clickable.
    implicitWidth: text.length > 0 ? contentItem.implicitWidth : trackWidth
    implicitHeight: text.length > 0 ? Math.max(trackHeight, contentItem.implicitHeight) : trackHeight

    indicator: Rectangle {
        y: (control.height - height) / 2
        implicitWidth: control.trackWidth
        implicitHeight: control.trackHeight
        radius: height / 2
        color: control.checked ? control.theme.primary : control.theme.border
        Behavior on color { ColorAnimation { duration: control.theme.animationDuration } }

        // Thumb.
        Rectangle {
            width: control.thumbSize
            height: control.thumbSize
            radius: height / 2
            y: (parent.height - height) / 2
            x: control.inset + control.position * (parent.width - width - 2 * control.inset)
            color: control.checked ? control.theme.primaryForeground
                                   : (control.theme.darkMode ? control.theme.foreground : control.theme.background)
            Behavior on x { NumberAnimation { duration: control.theme.animationDuration; easing.type: Easing.OutCubic } }
            Behavior on color { ColorAnimation { duration: control.theme.animationDuration } }
        }

        // Focus ring.
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

    contentItem: Text {
        text: control.text
        visible: control.text.length > 0
        leftPadding: control.trackWidth + control.spacing
        font.pixelSize: control.theme.fontSize
        font.weight: control.theme.fontWeight
        color: control.theme.foreground
        verticalAlignment: Text.AlignVCenter
    }
}
