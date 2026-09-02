import QtQuick
import QtQuick.Controls.Basic

ScrollView {
    id: control

    property SwbStyle theme: SwbStyle {}

    padding: 0
    clip: true
    focusPolicy: Qt.StrongFocus

    // Keep the viewport transparent and show a translucent ring only for keyboard focus.
    background: Rectangle {
        color: "transparent"
        border.color: control.visualFocus ? control.theme.focusRing : "transparent"
        border.width: control.visualFocus ? control.theme.focusRingWidth : 0
        Behavior on border.color { ColorAnimation { duration: control.theme.animationDuration } }
    }

    // Reuse SwbScrollBar for both orientations.
    ScrollBar.vertical: SwbScrollBar {
        theme: control.theme
        parent: control
        x: control.mirrored ? 0 : control.width - width
        y: control.topPadding
        height: control.availableHeight
        active: control.ScrollBar.horizontal.active
    }

    ScrollBar.horizontal: SwbScrollBar {
        theme: control.theme
        parent: control
        x: control.leftPadding
        y: control.height - height
        width: control.availableWidth
        active: control.ScrollBar.vertical.active
    }
}
