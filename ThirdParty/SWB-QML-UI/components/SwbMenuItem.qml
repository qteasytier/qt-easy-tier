import QtQuick
import QtQuick.Controls.impl
import QtQuick.Controls.Basic

MenuItem {
    id: control

    property SwbStyle theme: SwbStyle {}

    property string variant: "default"  // default | destructive
    property string shortcutText: ""
    property bool inset: false

    readonly property real trailingWidth: control.checkable || control.subMenu
                                          ? control.theme.iconSize + control.spacing : 0
    property color activeBgColor: control.variant === "destructive"
                                            ? control.theme.destructiveBg : control.theme.accent
    property color textColor: control.variant === "destructive"
                                       ? control.theme.destructive
                                       : (control.highlighted ? control.theme.accentForeground
                                                              : control.theme.foreground)
    property color bgColor: control.highlighted || control.down
                                     ? control.activeBgColor
                                     : control.theme.withAlpha(control.activeBgColor, 0)

    leftPadding: control.mirrored ? 6 : (control.inset ? 28 : 6)
    rightPadding: control.mirrored ? (control.inset ? 28 : 6) : 6
    verticalPadding: 0
    spacing: 6
    icon.width: control.theme.iconSize
    icon.height: control.theme.iconSize
    hoverEnabled: enabled
    font.pixelSize: control.theme.fontSize
    font.weight: Font.Normal
    opacity: enabled ? 1.0 : 0.5

    contentItem: Item {
        implicitWidth: itemLabel.implicitWidth
                       + (shortcutLabel.visible ? shortcutLabel.implicitWidth + 16 : 0)
                       + control.trailingWidth
        implicitHeight: Math.max(itemLabel.implicitHeight, shortcutLabel.implicitHeight)
        clip: true

        LayoutMirroring.enabled: control.mirrored
        LayoutMirroring.childrenInherit: true

        SwbIconLabel {
            id: itemLabel
            anchors.left: parent.left
            anchors.right: shortcutLabel.visible ? shortcutLabel.left : parent.right
            anchors.rightMargin: shortcutLabel.visible ? 12 : control.trailingWidth
            anchors.verticalCenter: parent.verticalCenter
            spacing: control.spacing
            mirrored: control.mirrored
            display: control.display
            alignment: Qt.AlignLeft
            icon: control.icon
            text: control.text
            font: control.font
            color: control.textColor
        }

        Text {
            id: shortcutLabel
            anchors.right: parent.right
            anchors.rightMargin: control.trailingWidth
            anchors.verticalCenter: parent.verticalCenter
            visible: control.shortcutText.length > 0
            text: control.shortcutText
            color: control.highlighted ? control.textColor : control.theme.mutedForeground
            font.pixelSize: control.theme.fontSizeSm - 1
            font.letterSpacing: 0.5
        }
    }

    indicator: Canvas {
        x: control.mirrored ? control.leftPadding : control.width - control.rightPadding - width
        y: control.topPadding + (control.availableHeight - height) / 2
        width: control.theme.iconSize
        height: control.theme.iconSize
        visible: control.checkable && control.checked && !control.subMenu
        property color strokeColor: control.textColor
        onStrokeColorChanged: requestPaint()
        onVisibleChanged: if (visible) requestPaint()
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

    arrow: Canvas {
        x: control.mirrored ? control.leftPadding : control.width - control.rightPadding - width
        y: control.topPadding + (control.availableHeight - height) / 2
        width: control.theme.iconSize
        height: control.theme.iconSize
        visible: !!control.subMenu
        property color strokeColor: control.textColor
        property bool drawMirrored: control.mirrored
        onStrokeColorChanged: requestPaint()
        onDrawMirroredChanged: requestPaint()
        onVisibleChanged: if (visible) requestPaint()
        onPaint: {
            let ctx = getContext("2d")
            let s = width / 24
            ctx.reset()
            ctx.strokeStyle = strokeColor
            ctx.lineWidth = 2
            ctx.lineCap = "round"
            ctx.lineJoin = "round"
            ctx.beginPath()
            ctx.moveTo((drawMirrored ? 15 : 9) * s, 6 * s)
            ctx.lineTo((drawMirrored ? 9 : 15) * s, 12 * s)
            ctx.lineTo((drawMirrored ? 15 : 9) * s, 18 * s)
            ctx.stroke()
        }
    }

    background: Rectangle {
        implicitWidth: 144
        implicitHeight: 28
        radius: control.theme.radiusSm
        color: control.bgColor

        Behavior on color { ColorAnimation { duration: control.theme.animationDuration } }
    }
}
