import QtQuick
import QtQuick.Controls.impl
import QtQuick.Controls.Basic

RoundButton {
    id: control

    property SwbStyle theme: SwbStyle {}

    // Control properties.
    property string variant: "default"  // default | destructive | outline | secondary | ghost | link
    property string size: "default"     // sm | default | lg
    property int alignment: Qt.AlignCenter  // Qt.Alignment flags placing the icon and text in the button.

    // Resolve the fill from the variant; destructive uses subtle red and hover darkens the default.
    property color bgColor: {
        switch (variant) {
        case "destructive": return hovered ? control.theme.destructiveBgHover : control.theme.destructiveBg
        case "secondary":   return hovered ? control.theme.secondaryHover : control.theme.secondary
        case "outline":     return hovered ? control.theme.accent : control.theme.background
        case "ghost":       return hovered ? control.theme.accent : control.theme.withAlpha(control.theme.accent, 0)
        case "link":        return "transparent"
        default:            return hovered ? control.theme.primaryHover : control.theme.primary
        }
    }
    property color textColor: {
        switch (variant) {
        case "destructive": return control.theme.destructive
        case "secondary":   return control.theme.secondaryForeground
        case "outline":
        case "ghost":       return control.theme.foreground
        case "link":        return control.theme.primary
        default:            return control.theme.primaryForeground
        }
    }

    // Support sm, default, and lg heights; a half-height radius yields circles or pills.
    property int controlHeight: size === "sm" ? control.theme.controlHeightSm
                                       : size === "lg" ? control.theme.controlHeightLg
                                       : control.theme.controlHeight
    radius: controlHeight / 2
    horizontalPadding: 10
    verticalPadding: 0
    // Icons follow the size token and tint with the variant text color.
    icon.width: control.theme.iconSize
    icon.height: control.theme.iconSize
    icon.color: control.textColor

    hoverEnabled: enabled  // Disabled controls do not react to hover.
    font.pixelSize: size === "sm" ? control.theme.fontSizeSm : control.theme.fontSize
    font.weight: control.theme.fontWeight
    font.underline: control.variant === "link" && control.hovered
    opacity: enabled ? 1.0 : 0.5

    // Collapse to a square for short content so the button renders as a circle.
    implicitWidth: Math.max(controlHeight, implicitContentWidth + leftPadding + rightPadding)

    contentItem: SwbIconLabel {
        alignment: control.alignment
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        icon: control.icon
        text: control.text
        font: control.font
        color: control.textColor
        Behavior on color { ColorAnimation { duration: control.theme.animationDuration } }
    }

    background: Rectangle {
        implicitWidth: control.controlHeight
        implicitHeight: control.controlHeight
        radius: control.radius
        color: control.bgColor
        // Outline reveals the border color; other variants keep a transparent placeholder.
        border.color: control.variant === "outline" ? control.theme.border : "transparent"
        border.width: 1
        Behavior on color { ColorAnimation { duration: control.theme.animationDuration } }

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
}
