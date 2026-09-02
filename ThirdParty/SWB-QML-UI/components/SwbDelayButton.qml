import QtQuick
import QtQuick.Controls.impl
import QtQuick.Controls.Basic

DelayButton {
    id: control

    property SwbStyle theme: SwbStyle {}

    // Control properties.
    property string variant: "default"  // default | destructive
    property string size: "default"     // sm | default | lg

    // Base fill before confirmation: subtle red for destructive and light gray for default.
    property color baseColor: {
        switch (variant) {
        case "destructive": return hovered ? control.theme.destructiveBgHover : control.theme.destructiveBg
        default:            return hovered ? control.theme.secondaryHover : control.theme.secondary
        }
    }
    // Hold-progress fill and the text colors on each side of the progress edge.
    property color fillColor:     variant === "destructive" ? control.theme.destructive : control.theme.primary
    property color baseTextColor: variant === "destructive" ? control.theme.destructive : control.theme.secondaryForeground
    property color fillTextColor: variant === "destructive" ? control.theme.destructiveForeground : control.theme.primaryForeground

    // Support sm, default, and lg heights; sm uses a smaller radius.
    property int controlHeight: size === "sm" ? control.theme.controlHeightSm
                                       : size === "lg" ? control.theme.controlHeightLg
                                       : control.theme.controlHeight
    property int radius: size === "sm" ? control.theme.radiusSm : control.theme.radius

    delay: 1500                         // Required hold duration in milliseconds.
    horizontalPadding: 14
    verticalPadding: 0

    hoverEnabled: enabled
    font.pixelSize: size === "sm" ? control.theme.fontSizeSm : control.theme.fontSize
    font.weight: control.theme.fontWeight
    opacity: enabled ? 1.0 : 0.5

    // Fill over the remaining hold duration and retract quickly when released.
    transition: Transition {
        NumberAnimation {
            duration: control.delay * (control.pressed ? 1.0 - control.progress : 0.3 * control.progress)
        }
    }

    // Clip two text layers so the filled portion uses the contrasting foreground.
    contentItem: ItemGroup {
        ClippedText {
            clip: control.progress > 0
            clipX: -control.leftPadding + control.progress * control.width
            clipWidth: (1.0 - control.progress) * control.width
            visible: control.progress < 1
            text: control.text
            font: control.font
            color: control.baseTextColor
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        ClippedText {
            clip: control.progress > 0
            clipX: -control.leftPadding
            clipWidth: control.progress * control.width
            visible: control.progress > 0
            text: control.text
            font: control.font
            color: control.fillTextColor
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    background: Rectangle {
        implicitWidth: 100
        implicitHeight: control.controlHeight
        radius: control.radius
        color: control.baseColor
        Behavior on color { ColorAnimation { duration: control.theme.animationDuration } }

        // Fill hold progress from left to right.
        Rectangle {
            width: control.progress * parent.width
            height: parent.height
            radius: parent.radius
            color: control.fillColor
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
}
