import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Templates as T

SpinBox {
    id: control

    property SwbStyle theme: SwbStyle {}

    // Width of each step button, arranged side by side like a FluentWinUI3 NumberBox.
    property int stepWidth: 28

    editable: true
    implicitWidth: 140
    padding: 0
    leftPadding: 12
    rightPadding: stepWidth * 2 + 4
    font.pixelSize: control.theme.fontSize
    opacity: enabled ? 1.0 : 0.5

    validator: IntValidator {
        locale: control.locale.name
        bottom: Math.min(control.from, control.to)
        top: Math.max(control.from, control.to)
    }

    // Left-aligned editable value.
    contentItem: TextInput {
        z: 2
        text: control.displayText
        clip: width < implicitWidth
        font: control.font
        color: control.theme.foreground
        selectionColor: control.theme.primary
        selectedTextColor: control.theme.primaryForeground
        horizontalAlignment: Qt.AlignLeft
        verticalAlignment: Qt.AlignVCenter
        readOnly: !control.editable
        validator: control.validator
        inputMethodHints: control.inputMethodHints

        // Themed right-click editing menu.
        T.ContextMenu.menu: SwbTextEditingContextMenu {
            editor: parent
            theme: control.theme
        }
    }

    // Input-style frame and focus-visible ring.
    background: Rectangle {
        implicitWidth: 140
        implicitHeight: control.theme.controlHeight
        radius: control.theme.radius
        color: "transparent"
        border.color: control.activeFocus ? control.theme.ring : control.theme.border
        border.width: 1
        Behavior on border.color { ColorAnimation { duration: control.theme.animationDuration } }

        Rectangle {
            anchors.fill: parent
            anchors.margins: -control.theme.focusRingWidth
            radius: parent.radius + control.theme.focusRingWidth
            color: "transparent"
            border.color: control.theme.focusRing
            border.width: control.theme.focusRingWidth
            visible: control.activeFocus
        }
    }

    // Increment button on the far right, following the frame's right corners.
    up.indicator: Item {
        x: control.width - width
        implicitWidth: control.stepWidth
        height: control.height

        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            topRightRadius: control.theme.radius - 1
            bottomRightRadius: control.theme.radius - 1
            color: control.up.pressed ? control.theme.accentHover
                 : control.up.hovered ? control.theme.accent : "transparent"
        }
        Canvas {                                  // lucide chevron-up
            anchors.centerIn: parent
            width: control.theme.iconSize
            height: control.theme.iconSize
            property color strokeColor: control.theme.mutedForeground
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
                ctx.moveTo(6 * s, 15 * s)
                ctx.lineTo(12 * s, 9 * s)
                ctx.lineTo(18 * s, 15 * s)
                ctx.stroke()
            }
        }
    }

    // Decrement button to the left of the increment button.
    down.indicator: Item {
        x: control.up.indicator.x - width
        implicitWidth: control.stepWidth
        height: control.height

        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            color: control.down.pressed ? control.theme.accentHover
                 : control.down.hovered ? control.theme.accent : "transparent"
        }
        Canvas {                                  // lucide chevron-down
            anchors.centerIn: parent
            width: control.theme.iconSize
            height: control.theme.iconSize
            property color strokeColor: control.theme.mutedForeground
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
                ctx.moveTo(6 * s, 9 * s)
                ctx.lineTo(12 * s, 15 * s)
                ctx.lineTo(18 * s, 9 * s)
                ctx.stroke()
            }
        }
    }
}
