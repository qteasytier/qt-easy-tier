import QtQuick
import QtQuick.Controls.Basic

TextArea {
    id: control

    property SwbStyle theme: SwbStyle {}

    implicitWidth: 240
    leftPadding: 10
    rightPadding: 10
    topPadding: 8
    bottomPadding: 8
    wrapMode: TextArea.Wrap          // Wrap multiline content automatically.

    // Themed right-click editing menu (local patch: the T.ContextMenu attached
    // property is unavailable in the Qt 6.8.3 aqt build; plain Menu + TapHandler
    // keeps 6.8 support).
    // 本地补丁（QtEasyTier）：菜单延迟到首次右键时才创建。
    // 每个文本控件常驻一份 SwbTextEditingContextMenu（Menu + 7 个 SwbMenuItem，
    // 每项还带 3 个 Canvas 图标），在表单类页面里累计代价很高；懒创建后
    // 只有真正用过右键菜单的控件才承担这份开销。
    Loader {
        id: editingMenuLoader
        active: false

        sourceComponent: SwbTextEditingContextMenu {
            editor: control
            theme: control.theme
        }
    }

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: {
            editingMenuLoader.active = true
            if (editingMenuLoader.item)
                editingMenuLoader.item.popup()
        }
    }

    font.pixelSize: control.theme.fontSize
    color: control.theme.foreground
    placeholderTextColor: control.theme.mutedForeground
    selectionColor: control.theme.primary
    selectedTextColor: control.theme.primaryForeground
    opacity: enabled ? 1.0 : 0.5

    background: Rectangle {
        implicitWidth: 240
        implicitHeight: 64           // Minimum height; TextArea grows with its content.
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
