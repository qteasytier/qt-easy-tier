import QtQuick
import QtQuick.Controls.Basic

Frame {
    id: control

    property SwbStyle theme: SwbStyle {}

    padding: 16

    // Card-like content container with a distinct surface, subtle border, and shared radius.
    background: Rectangle {
        radius: control.theme.radius
        color: control.theme.popover
        border.color: control.theme.border
        border.width: 1
    }
}
