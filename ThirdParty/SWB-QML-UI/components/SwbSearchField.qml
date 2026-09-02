import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Templates as T

TextField {
    id: control

    property SwbStyle theme: SwbStyle {}

    implicitWidth: 200
    leftPadding: 34   // Reserve space for the search icon.
    rightPadding: 34  // Reserve space for the clear button.
    topPadding: 0
    bottomPadding: 0
    verticalAlignment: TextInput.AlignVCenter

    // Themed right-click editing menu.
    T.ContextMenu.menu: SwbTextEditingContextMenu {
        editor: control
        theme: control.theme
    }

    font.pixelSize: control.theme.fontSize
    color: control.theme.foreground
    placeholderText: qsTr("Search")
    placeholderTextColor: control.theme.mutedForeground
    selectionColor: control.theme.primary
    selectedTextColor: control.theme.primaryForeground
    opacity: enabled ? 1.0 : 0.5

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: control.theme.controlHeight
        radius: control.theme.radius
        color: "transparent"
        border.color: control.activeFocus ? control.theme.ring : control.theme.border
        border.width: 1
        Behavior on border.color { ColorAnimation { duration: control.theme.animationDuration } }

        // Focus-visible ring.
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

    // Decorative search icon on the left.
    Canvas {
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.verticalCenter: parent.verticalCenter
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
            ctx.arc(11 * s, 11 * s, 7 * s, 0, 2 * Math.PI)
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(21 * s, 21 * s)
            ctx.lineTo(16 * s, 16 * s)
            ctx.stroke()
        }
    }

    // Show the clear button for non-empty text and retain focus after clearing.
    Item {
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        width: 24
        height: 24
        visible: control.text.length > 0

        HoverHandler { id: clearHover }

        // Fade from transparent accent to avoid a black flash during hover transitions.
        Rectangle {
            anchors.fill: parent
            radius: control.theme.radiusSm
            color: clearHover.hovered ? control.theme.accent : control.theme.withAlpha(control.theme.accent, 0)
            Behavior on color { ColorAnimation { duration: control.theme.animationDuration } }
        }

        // Clear icon.
        Canvas {
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
                ctx.beginPath()
                ctx.moveTo(18 * s, 6 * s)
                ctx.lineTo(6 * s, 18 * s)
                ctx.moveTo(6 * s, 6 * s)
                ctx.lineTo(18 * s, 18 * s)
                ctx.stroke()
            }
        }

        TapHandler {
            onTapped: {
                control.clear()
                control.forceActiveFocus()
            }
        }
    }
}
