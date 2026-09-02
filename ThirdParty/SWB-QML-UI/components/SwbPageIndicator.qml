import QtQuick
import QtQuick.Controls.Basic

PageIndicator {
    id: control

    property SwbStyle theme: SwbStyle {}

    padding: 4
    spacing: 6
    // Navigation dots are interactive by default; disable them explicitly for display-only use.
    interactive: true
    opacity: enabled ? 1.0 : 0.5

    // Use primary for the current page, border for others, and stronger contrast while pressed.
    delegate: Rectangle {
        required property int index

        implicitWidth: 8
        implicitHeight: 8
        radius: width / 2

        // qmllint disable unqualified
        color: index === control.currentIndex ? control.theme.primary
                                               : pressed ? control.theme.mutedForeground : control.theme.border

        Behavior on color { ColorAnimation { duration: control.theme.animationDuration } }
        // qmllint enable unqualified
    }

    contentItem: Row {
        spacing: control.spacing

        Repeater {
            model: control.count
            delegate: control.delegate
        }
    }
}
