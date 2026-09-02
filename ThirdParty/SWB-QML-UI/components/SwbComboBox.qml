pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Templates as T

ComboBox {
    id: control

    property SwbStyle theme: SwbStyle {}

    property string placeholderText: ""

    // Optional: show a delete button on every non-selected item. Clicking it emits
    // removeRequested with the source index; the control never mutates the data —
    // the ViewModel deletes and notifies the model via begin/endRemoveRows.
    property bool removable: false
    signal removeRequested(int index)

    // Optional: press and drag an item to reorder it. On release moveRequested is
    // emitted with the source and final indices; the ViewModel moves the data via
    // begin/endMoveRows. Disabled while filtering so the source index stays reliable.
    property bool reorderable: false
    signal moveRequested(int fromIndex, int toIndex)

    // Filter only after the user types; initially show every item when opening.
    property bool searching: false
    readonly property string query: searching ? editText.toLowerCase() : ""
    // Number of matches for the current query, used by the empty-state message.
    readonly property int matchCount: {
        if (query.length === 0)
            return count
        var n = 0
        for (var i = 0; i < count; i++)
            if (String(textAt(i)).toLowerCase().indexOf(query) >= 0)
                n++
        return n
    }

    editable: true
    implicitWidth: 220
    leftPadding: 10
    rightPadding: 34
    topPadding: 0
    bottomPadding: 0
    font.pixelSize: control.theme.fontSize
    opacity: enabled ? 1.0 : 0.5

    // Enter selects the first matching item.
    onAccepted: {
        for (var i = 0; i < count; i++) {
            if (query.length === 0 || String(textAt(i)).toLowerCase().indexOf(query) >= 0) {
                currentIndex = i
                popup.close()
                break
            }
        }
    }

    // Editable trigger background matching TextField, with a focus highlight.
    background: Rectangle {
        implicitWidth: 220
        implicitHeight: control.theme.controlHeight
        radius: control.theme.radius
        color: "transparent"
        border.color: control.activeFocus ? control.theme.ring : control.theme.border
        border.width: 1
        Behavior on border.color { ColorAnimation { duration: control.theme.animationDuration } }

        // Focus ring shown while editing.
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

    // Editable trigger content that filters as the user types.
    contentItem: TextField {
        padding: 0
        text: control.editable ? control.editText : control.displayText
        placeholderText: control.placeholderText
        placeholderTextColor: control.theme.mutedForeground
        font: control.font
        color: control.theme.foreground
        selectionColor: control.theme.primary
        selectedTextColor: control.theme.primaryForeground
        verticalAlignment: TextInput.AlignVCenter
        enabled: control.editable
        background: null

        onTextEdited: {
            control.searching = true
            if (!control.popup.visible)
                control.popup.open()
        }

        // Themed right-click editing menu.
        T.ContextMenu.menu: SwbTextEditingContextMenu {
            editor: parent
            theme: control.theme
        }
        // Open after the click finishes so cursor placement is preserved and the popup overlay
        // cannot mistake the end of the same click for an outside press.
        TapHandler {
            acceptedButtons: Qt.LeftButton
            onTapped: if (!control.popup.visible) Qt.callLater(control.popup.open)
        }
    }

    // Right-side arrow trigger highlighted with accent on hover or while open.
    indicator: Item {
        x: control.width - width - 8
        y: control.topPadding + (control.availableHeight - height) / 2
        width: 24
        height: 24

        // Passive hover detection preserves the existing click behavior.
        HoverHandler { id: arrowHover }

        // Fill with accent only while hovered and closed. Start from zero-alpha accent rather
        // than transparent black to prevent a black flash during the transition.
        Rectangle {
            anchors.fill: parent
            radius: control.theme.radiusSm
            color: arrowHover.hovered && !control.popup.visible ? control.theme.accent : control.theme.withAlpha(control.theme.accent, 0)
            Behavior on color { ColorAnimation { duration: control.theme.animationDuration } }
        }

        // Dropdown arrow icon.
        Canvas {
            anchors.centerIn: parent
            width: control.theme.iconSize
            height: control.theme.iconSize
            property color strokeColor: control.theme.mutedForeground
            onStrokeColorChanged: requestPaint()
            onPaint: {
                let ctx = getContext("2d")
                let s = width / 24
                ctx.reset()
                ctx.strokeStyle = strokeColor
                ctx.lineWidth = 2
                ctx.lineCap = "round"
                ctx.lineJoin = "round"
                ctx.beginPath()
                ctx.moveTo(6 * s, 9 * s)
                ctx.lineTo(12 * s, 15 * s)
                ctx.lineTo(18 * s, 9 * s)
                ctx.stroke()
            }
        }
    }

    // Shared row height; the drag gap and drop-target math reuse this value.
    readonly property int _itemHeight: 32

    // Reorder state, both source indices: the dragged row and its current drop
    // target. -1 means no drag is in progress.
    property int _dragSourceIndex: -1
    property int _dragTargetIndex: -1
    // True only for the frame that settles a drop: row offsets snap to 0 without
    // animation while forceLayout() applies the moved model in the same frame, so
    // nothing flashes back to its old slot before jumping to the new one.
    property bool _settling: false

    // Adjust currentIndex after a removal so the intended item stays selected.
    // Runs only once the data actually shrank, so ignoring the signal leaves the
    // selection untouched instead of desyncing it.
    function _applyRemove(index) {
        if (index < 0 || index >= count)
            return
        var ci = currentIndex
        var before = count
        removeRequested(index)
        if (count >= before)          // nobody removed the row: leave the selection alone
            return
        var newIndex = ci
        if (index === ci)             // removed the selected row: select the row after it
            newIndex = index >= count ? count - 1 : index
        else if (index < ci)          // a row before it went away: shift down by one
            newIndex = ci - 1
        // index > ci: unaffected
        if (newIndex !== currentIndex) {
            var keyword = editText    // setCurrentIndex may reset editText; keep the search term
            currentIndex = newIndex
            if (searching)
                editText = keyword
        }
    }

    // Adjust currentIndex after a move so the same item stays selected — the shift
    // equals removing at `from` then inserting at `to`. Runs only once the data
    // actually moved (the moved text now sits at `to`), so ignoring the signal
    // leaves the selection untouched instead of desyncing it.
    function _applyMove(from, to) {
        if (from < 0 || to < 0 || from === to)   // unchanged position: emit nothing
            return
        var moved = textAt(from)
        var ci = currentIndex
        moveRequested(from, to)
        if (textAt(to) !== moved)     // nobody moved the data: leave the selection alone
            return
        var newIndex = ci
        if (ci === from) {
            newIndex = to
        } else {
            if (from < ci) newIndex -= 1
            if (to <= newIndex) newIndex += 1
        }
        if (newIndex !== currentIndex)
            currentIndex = newIndex
    }

    // Collapse list items that do not match the current query. Not clipped, so the
    // dragged row and the rows making room for it can slide across their slots.
    delegate: ItemDelegate {
        id: item
        width: ListView.view.width
        height: matched ? control._itemHeight : 0
        visible: matched
        padding: 0
        hoverEnabled: control.hoverEnabled
        z: dragging ? 2 : 1

        required property int index

        readonly property string modelText: control.textAt(index)
        readonly property bool matched: control.query.length === 0
                                        || modelText.toLowerCase().indexOf(control.query) >= 0
        readonly property bool current: control.currentIndex === index
        readonly property bool dragging: control._dragSourceIndex === index

        highlighted: control.highlightedIndex === index

        // Drag distance of the dragged row. The centroid is measured against the
        // delegate root, which never moves, so it stays stable while rowVisual slides.
        readonly property real dragDy: dragging && dragHandler.active
            ? dragHandler.centroid.position.y - dragHandler.centroid.pressPosition.y : 0

        // Vertical shift of the row: the dragged row follows the finger, the others
        // step aside to open a gap at the drop position.
        readonly property real rowOffset: {
            if (control._dragSourceIndex < 0)
                return 0
            if (dragging)
                return dragDy
            var s = control._dragSourceIndex
            var t = control._dragTargetIndex
            if (t < 0)
                return 0
            if (s < t && index > s && index <= t)
                return -control._itemHeight
            if (s > t && index >= t && index < s)
                return control._itemHeight
            return 0
        }

        // Update the drop target from the dragged row's current center.
        onDragDyChanged: {
            if (dragging && dragHandler.active)
                control._dragTargetIndex = Math.max(0, Math.min(control.count - 1,
                    index + Math.round(dragDy / control._itemHeight)))
        }

        background: null

        // A whole-row press starts a reorder once past the drag threshold; a press
        // that does not move still falls through to the delegate click for selection.
        DragHandler {
            id: dragHandler
            target: null
            enabled: control.reorderable && !control.searching
            xAxis.enabled: false
            yAxis.enabled: true
            onActiveChanged: {
                if (active) {
                    control._dragSourceIndex = item.index
                    control._dragTargetIndex = item.index
                } else if (control._dragSourceIndex === item.index) {
                    var from = control._dragSourceIndex
                    var to = control._dragTargetIndex
                    // Settle atomically: freeze the offsets (no animation) and snap
                    // them to 0, move the data, then force an immediate layout so the
                    // new real positions and the zeroed offsets land in the same frame
                    // — no flash back to the pre-drag layout before the move.
                    control._settling = true
                    control._dragSourceIndex = -1
                    control._dragTargetIndex = -1
                    control._applyMove(from, to)
                    item.ListView.view.forceLayout()
                    Qt.callLater(function() { control._settling = false })
                }
            }
        }

        contentItem: Item {
            // Row visual; slides vertically for drag-follow and the gap animation.
            Item {
                id: rowVisual
                width: item.width
                height: control._itemHeight
                y: item.rowOffset
                opacity: item.dragging ? 0.85 : 1.0

                // The dragged row tracks the finger with no easing; every other case
                // — making room, snapping back on release — animates.
                Behavior on y {
                    enabled: !control._settling && (!item.dragging || !dragHandler.active)
                    NumberAnimation { duration: control.theme.animationDuration; easing.type: Easing.OutCubic }
                }

                Rectangle {
                    anchors.fill: parent
                    radius: control.theme.radiusSm
                    color: (item.dragging || item.highlighted) ? control.theme.accent : "transparent"
                    border.color: control.theme.ring
                    border.width: item.dragging ? 1 : 0
                }

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 8
                    anchors.right: iconArea.left
                    anchors.rightMargin: 4
                    anchors.verticalCenter: parent.verticalCenter
                    text: item.modelText
                    font.pixelSize: control.theme.fontSize
                    color: item.highlighted ? control.theme.accentForeground : control.theme.foreground
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }

                Item {
                    id: iconArea
                    anchors.right: parent.right
                    anchors.rightMargin: 6
                    anchors.verticalCenter: parent.verticalCenter
                    height: 24
                    // When removable, reserve check slot + gap + delete button so every
                    // row's × lines up regardless of selection; otherwise just the check
                    // slot, matching the plain combo box.
                    width: control.removable ? (control.theme.iconSize + 2 + 24) : 24

                    // Delete button: shown on every item when removable and pinned to the
                    // right so all × marks align (the selected item included). A contained
                    // tap removes the item without selecting it, closing the popup, or
                    // changing the search text.
                    Item {
                        id: removeButton
                        visible: control.removable
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        width: 24
                        height: 24

                        HoverHandler { id: removeHover }

                        // Fade from transparent to avoid a flash during hover transitions.
                        Rectangle {
                            anchors.fill: parent
                            radius: control.theme.radiusSm
                            color: removeHover.hovered ? control.theme.destructiveBg
                                   : control.theme.withAlpha(control.theme.destructive, 0)
                            Behavior on color { ColorAnimation { duration: control.theme.animationDuration } }
                        }

                        // Delete (×) icon.
                        Canvas {
                            anchors.centerIn: parent
                            width: control.theme.iconSize
                            height: control.theme.iconSize
                            property color strokeColor: removeHover.hovered ? control.theme.destructive
                                                                            : control.theme.mutedForeground
                            onStrokeColorChanged: requestPaint()
                            onPaint: {
                                let ctx = getContext("2d")
                                let s = width / 24
                                ctx.reset()
                                ctx.strokeStyle = strokeColor
                                ctx.lineWidth = 2
                                ctx.lineCap = "round"
                                ctx.beginPath()
                                ctx.moveTo(18 * s, 6 * s)
                                ctx.lineTo(6 * s, 18 * s)
                                ctx.moveTo(6 * s, 6 * s)
                                ctx.lineTo(18 * s, 18 * s)
                                ctx.stroke()
                            }
                        }

                        // Press-to-release grab so a drag never starts on the button.
                        TapHandler {
                            gesturePolicy: TapHandler.ReleaseWithinBounds
                            onTapped: control._applyRemove(item.index)
                        }
                    }

                    // Check icon on the selected item: left of the × when removable,
                    // otherwise centered in the rightmost slot (plain combo box).
                    Canvas {
                        id: checkMark
                        visible: item.current
                        anchors.right: control.removable ? removeButton.left : parent.right
                        anchors.rightMargin: control.removable ? 2 : (24 - control.theme.iconSize) / 2
                        anchors.verticalCenter: parent.verticalCenter
                        width: control.theme.iconSize
                        height: control.theme.iconSize
                        property color strokeColor: control.theme.foreground
                        onStrokeColorChanged: requestPaint()
                        onVisibleChanged: if (visible) requestPaint()
                        onPaint: {
                            let ctx = getContext("2d")
                            let s = width / 24
                            ctx.reset()
                            ctx.strokeStyle = strokeColor
                            ctx.lineWidth = 2
                            ctx.lineCap = "round"
                            ctx.lineJoin = "round"
                            ctx.beginPath()
                            ctx.moveTo(20 * s, 6 * s)
                            ctx.lineTo(9 * s, 17 * s)
                            ctx.lineTo(4 * s, 12 * s)
                            ctx.stroke()
                        }
                    }
                }
            }
        }
    }

    // Popup panel with fade, zoom, and slide-in animation.
    popup: Popup {
        id: popup

        y: control.height + 6
        width: control.width
        implicitHeight: control.matchCount === 0 ? 40 : Math.min(contentItem.implicitHeight + 8, 300)
        padding: 4

        enter: Transition {
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: control.theme.animationDuration; easing.type: Easing.OutCubic }
            NumberAnimation { property: "scale"; from: 0.95; to: 1.0; duration: control.theme.animationDuration; easing.type: Easing.OutCubic }
        }
        exit: Transition {
            NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: control.theme.animationDuration; easing.type: Easing.OutCubic }
            NumberAnimation { property: "scale"; from: 1.0; to: 0.95; duration: control.theme.animationDuration; easing.type: Easing.OutCubic }
        }

        // Reset filtering and restore the selected value when the popup closes.
        onClosed: {
            control.searching = false
            control.editText = control.currentIndex < 0 ? "" : control.currentText
        }

        background: Rectangle {
            radius: control.theme.radius
            color: control.theme.popover
            border.color: control.theme.border
            border.width: 1
        }
        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            boundsBehavior: Flickable.StopAtBounds
            ScrollIndicator.vertical: ScrollIndicator {}

            // Show the empty state when no items match.
            Text {
                anchors.centerIn: parent
                text: qsTr("No items found.")
                color: control.theme.mutedForeground
                font.pixelSize: control.theme.fontSize
                visible: control.matchCount === 0
            }
        }
    }
}
