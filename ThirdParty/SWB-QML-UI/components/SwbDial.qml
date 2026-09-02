import QtQuick
import QtQuick.Controls.Basic

Dial {
    id: control

    property SwbStyle theme: SwbStyle {}

    implicitWidth: 84
    implicitHeight: 84
    hoverEnabled: enabled
    opacity: enabled ? 1.0 : 0.5

    // Secondary ring track with a primary arc from the start to the current value.
    background: Canvas {
        property real progress: control.position
        property color trackColor: control.theme.secondary
        property color fillColor: control.theme.primary
        onProgressChanged: requestPaint()
        onTrackColorChanged: requestPaint()
        onFillColorChanged: requestPaint()

        onPaint: {
            const ctx = getContext("2d")
            const cx = width / 2
            const cy = height / 2
            const r = Math.min(width, height) / 2 - 10
            const deg = Math.PI / 180
            const a0 = (control.startAngle - 90) * deg
            const a1 = (control.endAngle - 90) * deg
            const av = (control.angle - 90) * deg
            ctx.reset()
            ctx.lineWidth = control.theme.trackThickness
            ctx.lineCap = "round"
            // Track.
            ctx.strokeStyle = trackColor
            ctx.beginPath()
            ctx.arc(cx, cy, r, a0, a1, false)
            ctx.stroke()
            // Filled arc.
            if (control.position > 0) {
                ctx.strokeStyle = fillColor
                ctx.beginPath()
                ctx.arc(cx, cy, r, a0, av, false)
                ctx.stroke()
            }
        }
    }

    // Position the Slider-style handle around the circumference.
    handle: Rectangle {
        readonly property real r: Math.min(control.width, control.height) / 2 - 10
        readonly property real rad: (control.angle - 90) * Math.PI / 180
        x: control.width / 2 + r * Math.cos(rad) - width / 2
        y: control.height / 2 + r * Math.sin(rad) - height / 2
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
