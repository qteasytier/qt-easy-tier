import QtQuick
import QtQuick.Controls.Basic

ToolButton {
    id: control

    property SwbStyle theme: SwbStyle {}

    // Control properties.
    property string variant: "default"  // default | outline
    property string size: "default"     // sm | default | lg
    property int alignment: Qt.AlignCenter  // Qt.Alignment flags placing the icon and text in the button.

    // Resolve the fill from the variant and checked state; checked and hovered buttons use accent.
    property color bgColor: {
        if (control.checked)
            return control.hovered ? control.theme.accentHover : control.theme.accent
        switch (variant) {
        case "outline": return control.hovered ? control.theme.accent : control.theme.background
        // Use transparent accent instead of black transparent so transitions only change alpha.
        default:        return control.hovered ? control.theme.accent : control.theme.withAlpha(control.theme.accent, 0)
        }
    }
    property color textColor: control.checked ? control.theme.accentForeground : control.theme.foreground

    // Support sm, default, and lg heights; sm uses a smaller radius.
    property int controlHeight: size === "sm" ? control.theme.controlHeightSm
                                       : size === "lg" ? control.theme.controlHeightLg
                                       : control.theme.controlHeight
    property int radius: size === "sm" ? control.theme.radiusSm : control.theme.radius

    padding: 6
    spacing: 6
    icon.width: control.theme.iconSize
    icon.height: control.theme.iconSize

    hoverEnabled: enabled  // Disabled controls ignore hover to avoid a fill beneath reduced opacity.
    font.pixelSize: size === "sm" ? control.theme.fontSizeSm : control.theme.fontSize
    font.weight: control.theme.fontWeight
    opacity: enabled ? 1.0 : 0.5

    // Icon-only buttons collapse to a square while retaining a full click target.
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
