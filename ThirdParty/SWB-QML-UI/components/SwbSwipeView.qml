import QtQuick
import QtQuick.Controls.Basic

SwipeView {
    id: control

    property SwbStyle theme: SwbStyle {}

    clip: true

    contentItem: ListView {
        id: view

        property bool relayoutPending: false

        function alignCurrentPage() {
            if (view.count === 0) {
                view.relayoutPending = false
                return
            }

            const expectedIndex = control.currentIndex
            view.forceLayout()
            view.positionViewAtIndex(expectedIndex, ListView.Beginning)
            control.setCurrentIndex(expectedIndex)
            view.relayoutPending = false

            if (control.currentIndex !== expectedIndex)
                control.setCurrentIndex(expectedIndex)
        }

        function scheduleAlignCurrentPage() {
            view.relayoutPending = true
            Qt.callLater(view.alignCurrentPage)
        }

        model: control.contentModel
        interactive: control.interactive
        currentIndex: control.currentIndex
        focus: control.focus
        spacing: control.spacing
        orientation: control.orientation
        snapMode: view.relayoutPending ? ListView.NoSnap : ListView.SnapOneItem
        boundsBehavior: Flickable.StopAtBounds
        highlightRangeMode: view.relayoutPending
                            ? ListView.NoHighlightRange
                            : ListView.StrictlyEnforceRange
        preferredHighlightBegin: 0
        preferredHighlightEnd: 0
        highlightMoveDuration: view.relayoutPending ? 0 : control.theme.animationDuration
        maximumFlickVelocity: 4 * (control.orientation === Qt.Horizontal ? width : height)

        onWidthChanged: {
            if (view.orientation === ListView.Horizontal)
                view.scheduleAlignCurrentPage()
        }

        onHeightChanged: {
            if (view.orientation === ListView.Vertical)
                view.scheduleAlignCurrentPage()
        }
    }
}
