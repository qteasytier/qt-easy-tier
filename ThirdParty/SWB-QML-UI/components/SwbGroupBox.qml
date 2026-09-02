import QtQuick
import QtQuick.Controls.Basic

GroupBox {
    id: control

    property SwbStyle theme: SwbStyle {}

    padding: 0
    spacing: 16
    topPadding: implicitLabelWidth > 0 ? implicitLabelHeight + spacing : 0
    font.pixelSize: 16
    font.weight: control.theme.fontWeight

    // Provide semantic title-and-content grouping without drawing an additional card border.
    label: Label {
        width: control.availableWidth
        text: control.title
        font: control.font
        color: control.theme.foreground
        opacity: control.enabled ? 1.0 : 0.5
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }

    background: null
}
