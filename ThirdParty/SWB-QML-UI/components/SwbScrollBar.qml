import QtQuick
import QtQuick.Controls.Basic

ScrollBar {
    id: control

    property SwbStyle theme: SwbStyle {}

    // Thin track with a 1px inset and an 8px thumb.
    padding: 1
    minimumSize: 0.1

    // Always-visible rounded thumb that darkens on hover or drag.
    contentItem: Rectangle {
        implicitWidth: 8
        implicitHeight: 8
        // Use half the thickness as the radius for rounded ends in either orientation.
        radius: Math.min(width, height) / 2
        color: (control.hovered || control.pressed) ? control.theme.mutedForeground : control.theme.border
        visible: control.size < 1.0

        Behavior on color { ColorAnimation { duration: control.theme.animationDuration } }
    }
}
