pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic

StackView {
    id: control

    property SwbStyle theme: SwbStyle {}

    property int orientation: Qt.Horizontal

    readonly property real slideDistance: orientation === Qt.Horizontal ? width : height
    readonly property int slideDirection: orientation === Qt.Horizontal && mirrored ? -1 : 1

    clip: true

    component SlideAnimation: PropertyAnimation {
        property: control.orientation === Qt.Horizontal ? "x" : "y"
        duration: control.theme.animationDuration
        easing.type: Easing.OutCubic
    }

    popEnter: Transition {
        SlideAnimation { from: -control.slideDirection * control.slideDistance; to: 0 }
    }

    popExit: Transition {
        SlideAnimation { from: 0; to: control.slideDirection * control.slideDistance }
    }

    pushEnter: Transition {
        SlideAnimation { from: control.slideDirection * control.slideDistance; to: 0 }
    }

    pushExit: Transition {
        SlideAnimation { from: 0; to: -control.slideDirection * control.slideDistance }
    }

    replaceEnter: Transition {
        SlideAnimation { from: control.slideDirection * control.slideDistance; to: 0 }
    }

    replaceExit: Transition {
        SlideAnimation { from: 0; to: -control.slideDirection * control.slideDistance }
    }
}
