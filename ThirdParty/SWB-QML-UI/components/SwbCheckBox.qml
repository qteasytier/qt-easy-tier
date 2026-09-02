import QtQuick
import QtQuick.Controls.Basic

CheckBox {
    id: control

    property SwbStyle theme: SwbStyle {}

    property int boxSize: control.theme.iconSize

    spacing: 8
    padding: 0
    hoverEnabled: true
    opacity: enabled ? 1.0 : 0.5

    // With no text, use the box width so the whole box remains clickable.
    implicitWidth: text.length > 0 ? contentItem.implicitWidth : boxSize
    implicitHeight: text.length > 0 ? Math.max(boxSize, contentItem.implicitHeight) : boxSize

    indicator: Rectangle {
        y: (control.height - height) / 2
        implicitWidth: control.boxSize
        implicitHeight: control.boxSize
        radius: 4
        color: control.checked ? control.theme.primary : "transparent"
        border.color: control.checked ? control.theme.primary : control.theme.border
        border.width: 1
        Behavior on color { ColorAnimation { duration: control.theme.animationDuration } }
        Behavior on border.color { ColorAnimation { duration: control.theme.animationDuration } }

        // Lucide check path, recolored by the theme and faded in when checked.
        Canvas {
            id: tick
            anchors.fill: parent
            opacity: control.checked ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: control.theme.animationDuration } }

            property color strokeColor: control.theme.primaryForeground
            onStrokeColorChanged: requestPaint()

            onPaint: {
                let ctx = getContext("2d")
                let s = width / 24
                ctx.reset()
                ctx.strokeStyle = strokeColor
                ctx.lineWidth = 2
                ctx.lineCap = "round"
                ctx.lineJoin = "round"
                ctx.beginPath()
                ctx.moveTo(20 * s, 6 * s)
                ctx.lineTo(9 * s, 17 * s)
                ctx.lineTo(4 * s, 12 * s)
                ctx.stroke()
            }
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
