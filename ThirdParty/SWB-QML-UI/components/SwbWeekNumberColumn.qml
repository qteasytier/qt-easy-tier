pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic

WeekNumberColumn {
    id: control

    property SwbStyle theme: SwbStyle {}

    spacing: control.theme.calendarSpacing
    padding: 0
    implicitWidth: control.theme.calendarCellSize
    implicitHeight: 6 * control.theme.calendarCellSize + 5 * spacing

    // Muted week numbers aligned with the month-grid row height.
    delegate: Text {
        required property int weekNumber
        text: weekNumber
        width: control.theme.calendarCellSize
        height: control.theme.calendarCellSize
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: 12
        font.weight: Font.Normal
        color: control.theme.mutedForeground
    }
}
