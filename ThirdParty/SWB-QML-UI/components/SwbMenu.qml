pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Templates as T

Menu {
    id: control

    property SwbStyle theme: SwbStyle {}

    popupType: Popup.Item
    margins: 8
    overlap: 4
    padding: 4

    delegate: SwbMenuItem { theme: control.theme }

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

    contentItem: ListView {
        implicitHeight: contentHeight
        model: control.contentModel
        currentIndex: control.currentIndex
        interactive: contentHeight + control.topPadding + control.bottomPadding > control.height
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        ScrollIndicator.vertical: SwbScrollIndicator { theme: control.theme }
    }

    background: Rectangle {
        implicitWidth: 144
        implicitHeight: 28
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
