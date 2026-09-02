pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Templates as T

Dialog {
    id: control

    property SwbStyle theme: SwbStyle {}

    property bool closeButtonVisible: true

    popupType: Popup.Item
    margins: 16
    padding: 16
    spacing: 0
    modal: true
    dim: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

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

    background: Item {
        Rectangle {
            anchors.fill: parent
            radius: control.theme.radius
            color: control.theme.withAlpha(control.theme.foreground, 0.1)
        }

        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: Math.max(0, control.theme.radius - 1)
            color: control.theme.popover
        }
    }

    header: Item {
        implicitHeight: visible ? 44 : 0
        visible: control.title.length > 0 || control.closeButtonVisible

        SwbLabel {
            theme: control.theme
            anchors.left: parent.left
            anchors.right: closeButton.left
            anchors.leftMargin: 16
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            text: control.title
            font.pixelSize: 16
            elide: Text.ElideRight
        }

        SwbToolButton {
            id: closeButton

            theme: control.theme
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            text: "×"
            size: "sm"
            visible: control.closeButtonVisible
            Accessible.name: qsTr("Close")
            onClicked: control.reject()
        }
    }

    footer: SwbDialogButtonBox {
        theme: control.theme
        visible: count > 0
    }

    T.Overlay.modal: Rectangle {
        color: control.theme.withAlpha("#000000", 0.1)
    }

    T.Overlay.modeless: Rectangle {
        color: "transparent"
    }
}
