import QtQuick
import QtQuick.Controls.Basic

Pane {
    id: control

    property SwbStyle theme: SwbStyle {}

    padding: 12

    // Use a muted background for a flat panel, distinct from the bordered Frame.
    background: Rectangle {
        radius: control.theme.radius
        color: control.theme.withAlpha(control.theme.secondary, 0.5)
    }
}
