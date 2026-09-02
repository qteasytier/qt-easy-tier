pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic

MenuBar {
    id: control

    property SwbStyle theme: SwbStyle {}

    spacing: 2
    topPadding: 3 + SafeArea.margins.top
    leftPadding: 3 + SafeArea.margins.left
    rightPadding: 3 + SafeArea.margins.right
    bottomPadding: 3 + SafeArea.margins.bottom

    delegate: SwbMenuBarItem { theme: control.theme }

    contentItem: Row {
        spacing: control.spacing
        layoutDirection: control.mirrored ? Qt.RightToLeft : Qt.LeftToRight

        Repeater {
            model: control.contentModel
        }
    }

    background: Rectangle {
        implicitHeight: control.theme.controlHeight
        radius: control.theme.radius
        color: control.theme.background
        border.color: control.theme.border
        border.width: 1
    }
}
