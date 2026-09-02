import QtQuick
import QtQuick.Controls.Basic

ToolSeparator {
    id: control

    property SwbStyle theme: SwbStyle {}

    horizontalPadding: vertical ? 6 : 2
    verticalPadding: vertical ? 4 : 6
    opacity: enabled ? 1.0 : 0.5

    contentItem: Rectangle {
        implicitWidth: control.vertical ? 1 : 30
        implicitHeight: control.vertical ? 24 : 1
        color: control.theme.border
    }
}
