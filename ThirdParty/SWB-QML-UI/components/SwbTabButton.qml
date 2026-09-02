import QtQuick
import QtQuick.Controls.impl
import QtQuick.Controls.Basic

TabButton {
    id: control

    property SwbStyle theme: SwbStyle {}

    // The attached property exposes only the parent base type, so read custom variants at runtime.
    // qmllint disable missing-property
    property string variant: {
        const tabBar = control.TabBar.tabBar
        return tabBar && tabBar["variant"] ? tabBar["variant"] : "default"
    }
    // qmllint enable missing-property

    property color textColor: checked || hovered
                                               ? control.theme.foreground : control.theme.mutedForeground

    property int alignment: Qt.AlignCenter  // Qt.Alignment flags placing the icon and text in the tab.

    horizontalPadding: 6
    verticalPadding: 0
    spacing: 6
    icon.width: control.theme.iconSize
    icon.height: control.theme.iconSize
    hoverEnabled: enabled
    font.pixelSize: control.theme.fontSize
    font.weight: control.theme.fontWeight
    opacity: enabled ? 1.0 : 0.5

    contentItem: SwbIconLabel {
        alignment: control.alignment
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        icon: control.icon
        // IconLabel treats a single ampersand as a mnemonic marker; escape it for literal labels.
        text: control.text.replace(/&/g, "&&")
        font: control.font
        color: control.textColor

        Behavior on color { ColorAnimation { duration: control.theme.animationDuration } }
    }

    background: Rectangle {
        implicitHeight: control.variant === "default"
                        ? control.theme.controlHeight - 6 : control.theme.controlHeight
        radius: control.variant === "default" ? control.theme.radiusSm : 0
        color: "transparent"
        border.color: "transparent"
        border.width: 1

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 2
            radius: height / 2
            color: control.theme.foreground
            opacity: control.variant === "line" && control.checked ? 1.0 : 0.0

            Behavior on opacity { NumberAnimation { duration: control.theme.animationDuration } }
        }

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
