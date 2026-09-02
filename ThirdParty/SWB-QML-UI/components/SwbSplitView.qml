pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Templates as T

SplitView {
    id: control

    property SwbStyle theme: SwbStyle {}

    clip: true

    handle: Item {
        id: splitHandle

        readonly property bool active: T.SplitHandle.hovered || T.SplitHandle.pressed

        implicitWidth: control.orientation === Qt.Horizontal ? 5 : control.width
        implicitHeight: control.orientation === Qt.Horizontal ? control.height : 5

        // Place the divider at the start of the hit area and extend the transparent target behind it.
        Rectangle {
            width: control.orientation === Qt.Horizontal ? 1 : parent.width
            height: control.orientation === Qt.Horizontal ? parent.height : 1
            color: splitHandle.active ? control.theme.mutedForeground : control.theme.border

            Behavior on color { ColorAnimation { duration: control.theme.animationDuration } }
        }
    }
}
