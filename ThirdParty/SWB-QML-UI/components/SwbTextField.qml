import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Templates as T

TextField {
    id: control

    property SwbStyle theme: SwbStyle {}

    implicitWidth: 200
    leftPadding: 10
    rightPadding: 10
    topPadding: 0
    bottomPadding: 0
    verticalAlignment: TextInput.AlignVCenter

    // Themed right-click editing menu.
    T.ContextMenu.menu: SwbTextEditingContextMenu {
        editor: control
        theme: control.theme
    }

    font.pixelSize: control.theme.fontSize
    color: control.theme.foreground
    placeholderTextColor: control.theme.mutedForeground
    selectionColor: control.theme.primary
    selectedTextColor: control.theme.primaryForeground
    opacity: enabled ? 1.0 : 0.5

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: control.theme.controlHeight
        radius: control.theme.radius
        color: "transparent"
        border.color: control.activeFocus ? control.theme.ring : control.theme.border
        border.width: 1
        Behavior on border.color { ColorAnimation { duration: control.theme.animationDuration } }

        // Focus-visible ring.
        Rectangle {
            anchors.fill: parent
            anchors.margins: -control.theme.focusRingWidth
            radius: parent.radius + control.theme.focusRingWidth
            color: "transparent"
            border.color: control.theme.focusRing
            border.width: control.theme.focusRingWidth
            visible: control.activeFocus
        }
    }
}
