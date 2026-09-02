import QtQuick
import QtQuick.Controls.Basic

MenuSeparator {
    id: control

    property SwbStyle theme: SwbStyle {}

    horizontalPadding: 0
    verticalPadding: 4
    opacity: enabled ? 1.0 : 0.5

    contentItem: Rectangle {
        implicitWidth: 136
        implicitHeight: 1
        color: control.theme.border
    }
}
