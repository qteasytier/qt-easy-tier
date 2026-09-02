pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic

VerticalHeaderView {
    id: control

    property SwbStyle theme: SwbStyle {}

    // Use a 1px fallback while the synchronized view has no delegates and contentWidth is zero.
    implicitWidth: Math.max(1, contentWidth)
    implicitHeight: syncView ? syncView.height : 0

    delegate: VerticalHeaderViewDelegate {
        id: headerCell

        padding: 8

        // Minimum-width centered row header with a subtle selected-row highlight.
        background: Rectangle {
            implicitWidth: 28
            color: headerCell.selected ? control.theme.secondary : control.theme.background

            // Right divider separating the header from the table body.
            Rectangle {
                anchors.right: parent.right
                width: 1
                height: parent.height
                color: control.theme.border
            }
        }

        contentItem: Text {
            text: headerCell.model[headerCell.headerView.textRole]
            font.pixelSize: 12
            font.weight: control.theme.fontWeight
            color: control.theme.foreground
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
