import QtQuick
import QtQuick.Controls.Basic

ToolTip {
    id: control

    property SwbStyle theme: SwbStyle {}

    // Center above the trigger; 9 combines a 4px gap with 5px of visible arrow.
    x: parent ? (parent.width - implicitWidth) / 2 : 0
    y: -implicitHeight - 9

    popupType: Popup.Item
    margins: 6
    leftPadding: 12
    rightPadding: 12
    topPadding: 6
    bottomPadding: 6

    // Limit long text to a maximum width and wrap it automatically.
    width: Math.min(implicitWidth, 320)

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 100; easing.type: Easing.OutCubic }
        NumberAnimation { property: "scale"; from: 0.95; to: 1.0; duration: 100; easing.type: Easing.OutCubic }
    }

    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 100; easing.type: Easing.OutCubic }
        NumberAnimation { property: "scale"; from: 1.0; to: 0.95; duration: 100; easing.type: Easing.OutCubic }
    }

    // Invert foreground and background for high contrast in both color modes.
    contentItem: Text {
        text: control.text
        font.pixelSize: 12
        color: control.theme.background
        wrapMode: Text.Wrap
    }

    background: Rectangle {
        radius: control.theme.radiusSm
        color: control.theme.foreground

        // A 45-degree square forms the arrow, shifted upward so most of it stays inside the bubble.
        Rectangle {
            width: 10
            height: 10
            rotation: 45
            color: parent.color
            anchors.horizontalCenter: parent.horizontalCenter
            y: parent.height - height / 2 - 2
        }
    }
}
