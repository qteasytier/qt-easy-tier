pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic

Tumbler {
    id: control

    property SwbStyle theme: SwbStyle {}

    implicitWidth: 64
    visibleItemCount: 5
    opacity: enabled ? 1.0 : 0.5

    readonly property real rowHeight: availableHeight / visibleItemCount
    property int scrollAnimationDuration: 100
    property int wheelAngleDeltaRemainder: 0
    property real lastWheelTimeMs: 0

    // Use a shorter movement duration than the internal view's slow 1000ms default.
    Component.onCompleted: applyMoveDuration()
    onWrapChanged: Qt.callLater(applyMoveDuration)   // Reapply after wrap rebuilds the internal view.
    onCountChanged: wheelAngleDeltaRemainder = 0
    onEnabledChanged: {
        if (!enabled)
            wheelAngleDeltaRemainder = 0
    }

    // Find the unexposed internal PathView/ListView under contentItem and override its move duration.
    function applyMoveDuration(duration) {
        if (!contentItem)
            return
        const d = duration === undefined ? scrollAnimationDuration : duration
        const views = contentItem.children
        for (let i = 0; i < views.length; ++i) {
            if (views[i]["highlightMoveDuration"] !== undefined) {
                views[i]["highlightMoveDuration"] = d
                return
            }
        }
    }

    // On Windows the delegate receives physical wheel events first, so update the index here
    // instead of relying on propagation to the internal PathView/ListView.
    function moveCurrentIndexByWheel(angleDeltaY, inverted) {
        if (count <= 1 || angleDeltaY === 0)
            return

        // A physical wheel step is usually 120; accumulate smaller high-resolution deltas.
        wheelAngleDeltaRemainder += inverted ? -angleDeltaY : angleDeltaY
        const steps = wheelAngleDeltaRemainder > 0
                ? Math.floor(wheelAngleDeltaRemainder / 120)
                : Math.ceil(wheelAngleDeltaRemainder / 120)
        if (steps === 0)
            return

        wheelAngleDeltaRemainder -= steps * 120

        // Snap instead of animating once the wheel outruns the move. A spaced notch animates
        // smoothly, but during a fast flick currentIndex advances faster than highlightMoveDuration
        // can follow; on a short wrapping tumbler the highlight then lags past half the ring and the
        // shortest-path move stalls or reverses. Snapping keeps the view locked to currentIndex.
        const nowMs = Date.now()
        applyMoveDuration(nowMs - lastWheelTimeMs < scrollAnimationDuration ? 0 : scrollAnimationDuration)
        lastWheelTimeMs = nowMs

        let nextIndex = currentIndex - steps
        if (wrap) {
            nextIndex = ((nextIndex % count) + count) % count
        } else if (nextIndex < 0 || nextIndex >= count) {
            nextIndex = Math.max(0, Math.min(count - 1, nextIndex))
        }

        currentIndex = nextIndex
    }

    // Center each label and fade items progressively toward the edges.
    delegate: Text {
        id: item
        text: modelData
        // Fill the whole row so the wheel/click area below covers it edge to edge; a text-sized
        // delegate leaves gaps beside the text where the wheel escapes to an enclosing Flickable.
        width: control.availableWidth
        height: control.rowHeight
        font.pixelSize: control.theme.fontSize
        font.weight: Math.abs(Tumbler.displacement) < 0.5 ? Font.DemiBold : Font.Normal
        color: control.theme.foreground
        opacity: 1.0 - Math.abs(Tumbler.displacement) / (control.visibleItemCount / 2)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

        required property var modelData
        required property int index

        // Tap to center an item smoothly. The internal view still handles drags and trackpad
        // scrolling, while only a click without movement changes the selection here.
        MouseArea {
            anchors.fill: parent
            onClicked: control.currentIndex = item.index
            scrollGestureEnabled: false

            // Handle the wheel above the internal view and stop propagation so it cannot process
            // the same event again after currentIndex changes.
            WheelHandler {
                target: null
                acceptedDevices: PointerDevice.Mouse
                orientation: Qt.Vertical
                blocking: true
                enabled: control.enabled && control.count > 1
                onWheel: (wheel) => control.moveCurrentIndexByWheel(wheel.angleDelta.y, wheel.inverted)
            }
        }
    }

    // Highlight the centered selection area beneath the text.
    background: Item {
        Rectangle {
            width: parent.width
            height: control.rowHeight
            y: (parent.height - height) / 2
            radius: control.theme.radiusSm
            color: control.theme.accent
        }
    }
}
