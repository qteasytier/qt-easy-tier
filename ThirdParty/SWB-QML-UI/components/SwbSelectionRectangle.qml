pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic

SelectionRectangle {
    id: control

    property SwbStyle theme: SwbStyle {}

    topLeftHandle: Handle {}
    bottomRightHandle: Handle {}

    // Selection-corner handles match slider handles and show an accent ring while dragging.
    component Handle: Rectangle {
        id: handleRoot

        width: control.theme.handleSize
        height: width
        radius: width / 2
        color: control.theme.handleColor
        border.color: control.theme.ring
        border.width: 1
        visible: handleRoot.SelectionRectangle.control.active

        Rectangle {
            anchors.fill: parent
            anchors.margins: -control.theme.focusRingWidth
            radius: parent.radius + control.theme.focusRingWidth
            color: "transparent"
            border.color: control.theme.focusRing
            border.width: control.theme.focusRingWidth
            visible: handleRoot.SelectionRectangle.dragging
        }
    }
}
