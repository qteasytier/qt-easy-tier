pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Templates as T

Drawer {
    id: control

    property SwbStyle theme: SwbStyle {}

    // Optional inner drag handle used only as a visual hint; the control handles dragging itself.
    property bool handleVisible: false

    // Left and right drawers are vertical; top and bottom drawers are horizontal.
    readonly property bool sideways: edge === Qt.LeftEdge || edge === Qt.RightEdge

    // Vertical drawers use a fixed width capped at three quarters of the window and fill its height.
    // Horizontal drawers fill the width and fit their content while leaving at least 96px visible.
    width: sideways ? Math.min(384, Math.round((parent?.width ?? 512) * 0.75))
                    : (parent?.width ?? 0)
    height: sideways ? (parent?.height ?? 0)
                     : Math.min(implicitHeight, (parent?.height ?? implicitHeight) - 96)

    popupType: Popup.Item
    padding: 16
    modal: true
    dim: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    // Reserve space on the handle side so content cannot overlap it.
    topPadding: padding + (handleVisible && edge === Qt.BottomEdge ? 12 : 0)
    bottomPadding: padding + (handleVisible && edge === Qt.TopEdge ? 12 : 0)
    leftPadding: padding + (handleVisible && edge === Qt.RightEdge ? 12 : 0)
    rightPadding: padding + (handleVisible && edge === Qt.LeftEdge ? 12 : 0)

    // Slide along the docked edge, driven by position.
    enter: Transition { NumberAnimation { duration: 450; easing.type: Easing.OutQuint } }
    exit: Transition { NumberAnimation { duration: 450; easing.type: Easing.OutQuint } }

    background: Rectangle {
        color: control.theme.popover

        // One-pixel divider facing the page content.
        Rectangle {
            width: control.sideways ? 1 : parent.width
            height: control.sideways ? parent.height : 1
            x: control.edge === Qt.LeftEdge ? parent.width - 1 : 0
            y: control.edge === Qt.TopEdge ? parent.height - 1 : 0
            color: control.theme.border
        }

        // Short rounded drag handle centered on the inner edge.
        Rectangle {
            visible: control.handleVisible
            width: control.sideways ? 4 : 48
            height: control.sideways ? 48 : 4
            radius: 2
            color: control.theme.border
            x: control.sideways ? (control.edge === Qt.LeftEdge ? parent.width - width - 4 : 4)
                                : (parent.width - width) / 2
            y: control.sideways ? (parent.height - height) / 2
                                : (control.edge === Qt.TopEdge ? parent.height - height - 4 : 4)
        }
    }

    T.Overlay.modal: Rectangle {
        color: control.theme.withAlpha("#000000", 0.1)
    }

    T.Overlay.modeless: Rectangle {
        color: "transparent"
    }
}
