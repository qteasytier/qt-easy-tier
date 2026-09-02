pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic

MonthGrid {
    id: control

    property SwbStyle theme: SwbStyle {}

    spacing: control.theme.calendarSpacing
    implicitWidth: 7 * control.theme.calendarCellSize + 6 * spacing
    implicitHeight: 6 * control.theme.calendarCellSize + 5 * spacing

    // Currently selected date; an invalid date leaves every cell unhighlighted.
    property date selectedDate
    onClicked: (date) => control.selectedDate = date

    delegate: Item {
        id: cell
        required property var model

        width: control.theme.calendarCellSize
        height: control.theme.calendarCellSize

        readonly property bool currentMonth: model.month === control.month
        readonly property bool today: model.today
        readonly property bool selected:
            !isNaN(control.selectedDate.getTime())
            && control.selectedDate.getFullYear() === model.year
            && control.selectedDate.getMonth() === model.month
            && control.selectedDate.getDate() === model.day

        // Cell background: primary when selected, accent on hover, secondary today, otherwise clear.
        Rectangle {
            anchors.fill: parent
            radius: control.theme.radius
            color: cell.selected ? control.theme.primary
                 : hover.hovered ? control.theme.accent
                 : cell.today ? control.theme.secondary
                 : control.theme.withAlpha(control.theme.accent, 0)
            Behavior on color { ColorAnimation { duration: control.theme.animationDuration } }

            HoverHandler { id: hover; enabled: cell.currentMonth }
        }

        Text {
            anchors.centerIn: parent
            text: cell.model.day
            font.pixelSize: control.theme.fontSize
            color: cell.selected ? control.theme.primaryForeground
                 : cell.currentMonth ? control.theme.foreground
                 : control.theme.mutedForeground
            opacity: cell.currentMonth ? 1.0 : 0.5     // Fade dates outside the current month.
        }
    }
}
