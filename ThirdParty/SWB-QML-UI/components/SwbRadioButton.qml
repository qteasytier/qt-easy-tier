import QtQuick
import QtQuick.Controls.Basic

RadioButton {
    id: control

    property SwbStyle theme: SwbStyle {}

    property int boxSize: control.theme.iconSize

    spacing: 8
    padding: 0
    hoverEnabled: true
    opacity: enabled ? 1.0 : 0.5

    // With no text, use the circle width so the whole circle remains clickable.
    implicitWidth: text.length > 0 ? contentItem.implicitWidth : boxSize
    implicitHeight: text.length > 0 ? Math.max(boxSize, contentItem.implicitHeight) : boxSize

    indicator: Rectangle {
        y: (control.height - height) / 2
        implicitWidth: control.boxSize
        implicitHeight: control.boxSize
        radius: height / 2
        // Use a solid primary fill and matching border when checked.
        color: control.checked ? control.theme.primary : "transparent"
        border.color: control.checked ? control.theme.primary : control.theme.border
        border.width: 1
        Behavior on color { ColorAnimation { duration: control.theme.animationDuration } }
        Behavior on border.color { ColorAnimation { duration: control.theme.animationDuration } }

        // Checked center dot in primary-foreground, scaled with the outer circle.
        Rectangle {
            anchors.centerIn: parent
            width: control.boxSize / 2
            height: width
            radius: width / 2
            color: control.theme.primaryForeground
            opacity: control.checked ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: control.theme.animationDuration } }
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
        leftPadding: control.boxSize + control.spacing
        font.pixelSize: control.theme.fontSize
        font.weight: control.theme.fontWeight
        color: control.theme.foreground
        verticalAlignment: Text.AlignVCenter
    }
}
