import QtQuick
import QtQuick.Controls.Basic

BusyIndicator {
    id: control

    property SwbStyle theme: SwbStyle {}

    // Size presets: sm=16, default=24, and lg=32.
    property string size: "default"
    property int diameter: size === "sm" ? 16 : (size === "lg" ? 32 : 24)

    // Arc color can be overridden to match its container, such as a button foreground.
    property color color: control.theme.mutedForeground
    property bool animationPaused: false

    implicitWidth: diameter
    implicitHeight: diameter
    padding: 0

    // Continuously rotate a round-capped arc covering roughly three quarters of a circle.
    contentItem: Item {
        implicitWidth: control.diameter
        implicitHeight: control.diameter
        opacity: control.running ? 1.0 : 0.0
        Behavior on opacity { OpacityAnimator { duration: control.theme.animationDuration } }

        Canvas {
            id: arc
            anchors.fill: parent
            readonly property real lineWidth: Math.max(1.5, width / 12)
            property color strokeColor: control.color
            onStrokeColorChanged: requestPaint()

            onPaint: {
                const ctx = getContext("2d")
                const cx = width / 2
                const cy = height / 2
                const r = width / 2 - lineWidth
                ctx.reset()
                ctx.lineWidth = lineWidth
                ctx.lineCap = "round"
                ctx.strokeStyle = strokeColor
                ctx.beginPath()
                // Start at the top and draw 270 degrees clockwise, leaving a quarter gap.
                ctx.arc(cx, cy, r, -Math.PI / 2, Math.PI, false)
                ctx.stroke()
            }

            // Drive rotation on the GUI thread with a wall-clock timer so the speed is
            // identical on every machine. A RotationAnimator advances on the render thread
            // once per vsync, so its speed drifts whenever the real frame rate differs from
            // the display's reported refresh rate (high-refresh or mismatched monitors).
            NumberAnimation on rotation {
                running: control.running && control.visible && !control.animationPaused
                from: 0
                to: 360
                duration: 900
                easing.type: Easing.Linear
                loops: Animation.Infinite
            }
        }
    }
}
