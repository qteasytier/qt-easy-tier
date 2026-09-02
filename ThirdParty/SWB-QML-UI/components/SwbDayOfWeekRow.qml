pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic

DayOfWeekRow {
    id: control

    property SwbStyle theme: SwbStyle {}

    spacing: control.theme.calendarSpacing
    padding: 0
    implicitWidth: 7 * control.theme.calendarCellSize + 6 * spacing
    implicitHeight: control.theme.calendarCellSize

    // Muted weekday abbreviations aligned with the month-grid columns.
    delegate: Text {
        required property string shortName
        text: shortName
        width: control.theme.calendarCellSize
        height: control.theme.calendarCellSize
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: 12
        font.weight: Font.Normal
        color: control.theme.mutedForeground
    }
}
