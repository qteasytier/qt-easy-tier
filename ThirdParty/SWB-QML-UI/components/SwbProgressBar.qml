import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects

ProgressBar {
    id: control

    property SwbStyle theme: SwbStyle {}
    property bool animationPaused: false

    implicitWidth: 200
    opacity: enabled ? 1.0 : 0.5

    // Fully rounded, 6px-high secondary track.
    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 6
        radius: height / 2
        color: control.theme.secondary
    }

    // Primary completed segment whose width follows value smoothly.
    contentItem: Item {
        id: progressContent

        Rectangle {
            visible: !control.indeterminate
            width: control.visualPosition * parent.width
            height: parent.height
            radius: height / 2
            color: control.theme.primary
            Behavior on width { NumberAnimation { duration: control.theme.animationDuration; easing.type: Easing.OutCubic } }
        }

        // The effect renders this hidden source through the rounded track mask.
        Rectangle {
            id: indeterminateSource

            anchors.fill: parent
            radius: height / 2
            color: "transparent"
            clip: true
            visible: false

            Rectangle {
                id: sweep

                property real animationProgress: 0
                readonly property real preferredWidth: parent.width * 0.4
                readonly property real unclippedX: -preferredWidth + animationProgress * (parent.width + preferredWidth)

                x: Math.max(0, unclippedX)
                width: Math.max(0, Math.min(parent.width, unclippedX + preferredWidth) - x)
                height: parent.height
                radius: height / 2
                color: control.theme.primary

                NumberAnimation on animationProgress {
                    running: control.indeterminate && control.visible && !control.animationPaused
                    from: 0
                    to: 1
                    duration: 1200
                    easing.type: Easing.Linear
                    loops: Animation.Infinite
                }
            }
        }

        Rectangle {
            id: progressMask

            anchors.fill: parent
            radius: height / 2
            color: "white"
            visible: false
            layer.enabled: true
        }

        MultiEffect {
            anchors.fill: parent
            visible: control.indeterminate
            source: indeterminateSource
            maskEnabled: true
            maskSource: progressMask
        }
    }
}
