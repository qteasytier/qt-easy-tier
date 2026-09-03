import QtQuick
import QtQuick.Controls.Basic

TextField {
    id: control

    property SwbStyle theme: SwbStyle {}

    implicitWidth: 200
    leftPadding: 10
    rightPadding: 10
    topPadding: 0
    bottomPadding: 0
    verticalAlignment: TextInput.AlignVCenter

    // Themed right-click editing menu (local patch: the T.ContextMenu attached
    // property is unavailable in the Qt 6.8.3 aqt build; plain Menu + TapHandler
    // keeps 6.8 support).
    SwbTextEditingContextMenu {
        id: editingMenu
        editor: control
        theme: control.theme
    }

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: editingMenu.popup()
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
