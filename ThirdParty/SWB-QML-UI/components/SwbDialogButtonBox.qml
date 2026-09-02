pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic

DialogButtonBox {
    id: control

    property SwbStyle theme: SwbStyle {}

    spacing: 8
    padding: 16
    alignment: Qt.AlignRight
    contentWidth: contentList.contentWidth

    delegate: SwbButton {
        id: button

        theme: control.theme
        readonly property int role: button.DialogButtonBox.buttonRole
        variant: role === DialogButtonBox.DestructiveRole ? "destructive"
               : role === DialogButtonBox.AcceptRole || role === DialogButtonBox.YesRole ? "default"
               : "outline"
    }

    contentItem: ListView {
        id: contentList

        implicitWidth: contentWidth
        model: control.contentModel
        spacing: control.spacing
        orientation: ListView.Horizontal
        boundsBehavior: Flickable.StopAtBounds
        snapMode: ListView.SnapToItem
    }

    background: Rectangle {
        x: 1
        width: parent.width - 2
        height: parent.height - 1
        radius: Math.max(0, control.theme.radius - 1)
        topLeftRadius: 0
        topRightRadius: 0
        color: control.theme.withAlpha(control.theme.secondary, 0.5)

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 1
            color: control.theme.border
        }
    }
}
