pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Templates as T

Popup {
    id: control

    property SwbStyle theme: SwbStyle {}

    popupType: Popup.Item
    margins: 8
    padding: 10
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0.0
            to: 1.0
            duration: 100
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            property: "scale"
            from: 0.95
            to: 1.0
            duration: 100
            easing.type: Easing.OutCubic
        }
    }

    exit: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1.0
            to: 0.0
            duration: 100
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            property: "scale"
            from: 1.0
            to: 0.95
            duration: 100
            easing.type: Easing.OutCubic
        }
    }

    background: Rectangle {
        radius: control.theme.radius
        color: control.theme.popover
        border.color: control.theme.border
        border.width: 1
    }

    T.Overlay.modal: Rectangle {
        color: control.theme.withAlpha(control.theme.foreground, 0.08)
    }

    T.Overlay.modeless: Rectangle {
        color: "transparent"
    }
}
