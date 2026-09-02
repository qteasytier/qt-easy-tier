import QtQuick
import QtQuick.Controls.Basic

ToolBar {
    id: control

    property SwbStyle theme: SwbStyle {}

    // Docked toolbars use square corners; floating toolbars use rounded corners.
    property bool floating: false

    spacing: 4
    topPadding: 4 + SafeArea.margins.top
    leftPadding: 4 + SafeArea.margins.left
    rightPadding: 4 + SafeArea.margins.right
    bottomPadding: 4 + SafeArea.margins.bottom

    contentItem: Row {
        spacing: control.spacing
        layoutDirection: control.mirrored ? Qt.RightToLeft : Qt.LeftToRight
    }

    background: Rectangle {
        implicitHeight: control.theme.controlHeight + 8
        radius: control.floating ? control.theme.radius : 0
        color: control.theme.withAlpha(control.theme.secondary, 0.5)
        border.color: control.theme.border
        border.width: 1
    }
}
