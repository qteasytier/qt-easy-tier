import QtQuick
import QtQuick.Controls.Basic

ScrollIndicator {
    id: control

    property SwbStyle theme: SwbStyle {}

    padding: 2

    // Non-interactive position hint with a rounded border-colored thumb that fades after scrolling.
    contentItem: Rectangle {
        implicitWidth: 4
        implicitHeight: 4
        radius: Math.min(width, height) / 2
        color: control.theme.border
        visible: control.size < 1.0
        opacity: 0.0

        states: State {
            name: "active"
            when: control.active && control.enabled
            PropertyChanges { control.contentItem.opacity: 1.0 }
        }

        transitions: [
            Transition {
                to: "active"
                NumberAnimation {
                    target: control.contentItem
                    property: "opacity"
                    duration: control.theme.animationDuration
                }
            },
            Transition {
                from: "active"
                SequentialAnimation {
                    PauseAnimation { duration: 450 }
                    NumberAnimation {
                        target: control.contentItem
                        property: "opacity"
                        duration: control.theme.animationDuration
                        to: 0.0
                    }
                }
            }
        ]
    }
}
