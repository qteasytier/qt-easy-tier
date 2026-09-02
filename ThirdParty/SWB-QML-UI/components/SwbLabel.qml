import QtQuick
import QtQuick.Controls.Basic

Label {
    id: control

    property SwbStyle theme: SwbStyle {}

    font.pixelSize: control.theme.fontSize
    font.weight: control.theme.fontWeight
    color: control.theme.foreground
    linkColor: control.theme.primary
    opacity: enabled ? 1.0 : 0.5
}
