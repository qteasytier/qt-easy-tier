import QtQuick
import QtQuick.Controls.Basic

TabBar {
    id: control

    property SwbStyle theme: SwbStyle {}

    property string variant: "default"  // default | line

    padding: variant === "default" ? 3 : 0
    spacing: variant === "line" ? 4 : 0

    contentItem: ListView {
        id: tabList

        model: control.contentModel
        currentIndex: control.currentIndex
        spacing: control.spacing
        orientation: ListView.Horizontal
        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.AutoFlickIfNeeded
        snapMode: ListView.SnapToItem
        highlightMoveDuration: 0
        highlightRangeMode: ListView.ApplyRange
        preferredHighlightBegin: 40
        preferredHighlightEnd: width - 40
    }

    background: Rectangle {
        implicitHeight: control.theme.controlHeight
        radius: control.variant === "default" ? control.theme.radius : 0
        color: control.variant === "default"
               ? control.theme.secondary : control.theme.withAlpha(control.theme.secondary, 0)

        Behavior on color { ColorAnimation { duration: control.theme.animationDuration } }

        // The default variant keeps one selection block and moves it continuously between tabs.
        Rectangle {
            x: control.leftPadding + (tabList.currentItem
                                      ? tabList.currentItem.x - tabList.contentX : 0)
            y: control.topPadding
            width: tabList.currentItem ? tabList.currentItem.width : 0
            height: control.availableHeight
            radius: control.theme.radiusSm
            color: control.theme.background
            border.color: control.theme.border
            border.width: 1
            visible: control.variant === "default" && tabList.currentItem

            Behavior on x {
                NumberAnimation {
                    duration: control.theme.animationDuration
                    easing.type: Easing.OutCubic
                }
            }
            Behavior on width {
                NumberAnimation {
                    duration: control.theme.animationDuration
                    easing.type: Easing.OutCubic
                }
            }
        }
    }
}
