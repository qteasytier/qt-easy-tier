import QtQuick
import QtQuick.Controls.Basic

Page {
    id: control

    property SwbStyle theme: SwbStyle {}

    // Supply only the themed page background and retain the native header and footer slots.
    background: Rectangle {
        color: control.theme.background
    }
}
