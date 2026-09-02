pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic

HorizontalHeaderView {
    id: control

    property SwbStyle theme: SwbStyle {}

    implicitWidth: syncView ? syncView.width : 0
    // Use a 1px fallback while the synchronized view has no delegates and contentHeight is zero.
    implicitHeight: Math.max(1, contentHeight)

    delegate: HorizontalHeaderViewDelegate {
        id: headerCell

        padding: 8

        // Fixed-height, left-aligned column header with a subtle selected-column highlight.
        background: Rectangle {
            implicitHeight: 40
            color: headerCell.selected ? control.theme.secondary : control.theme.background

            // Bottom divider aligned with the table row separators.
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: control.theme.border
            }
        }

        contentItem: Text {
            text: headerCell.model[headerCell.headerView.textRole]
            font.pixelSize: 12
            font.weight: control.theme.fontWeight
            color: control.theme.foreground
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }
}
