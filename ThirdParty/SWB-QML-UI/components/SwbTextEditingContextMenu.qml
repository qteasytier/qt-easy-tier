pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.impl
import QtQuick.Controls.Basic

// Context menu for text editing controls: undo/redo, clipboard, and selection
// entries that enable themselves from the attached editor's state.
SwbMenu {
    id: control

    required property Item editor

    // Rows lead with a hand-drawn glyph; the theme icon names declared by the
    // shared actions only select which glyph gets painted.
    delegate: SwbMenuItem {
        id: item
        theme: control.theme
        inset: true
        display: MenuItem.TextOnly

        Canvas {
            id: glyph
            x: 6
            anchors.verticalCenter: parent.verticalCenter
            width: control.theme.iconSize
            height: control.theme.iconSize
            property string iconName: item.action?.icon.name ?? ""
            property color strokeColor: item.textColor
            onStrokeColorChanged: requestPaint()

            function roundRect(ctx, x, y, w, h, r) {
                ctx.moveTo(x + r, y)
                ctx.arcTo(x + w, y, x + w, y + h, r)
                ctx.arcTo(x + w, y + h, x, y + h, r)
                ctx.arcTo(x, y + h, x, y, r)
                ctx.arcTo(x, y, x + w, y, r)
                ctx.closePath()
            }

            onPaint: {
                const ctx = getContext("2d")
                const s = width / 24
                ctx.reset()
                ctx.strokeStyle = glyph.strokeColor
                ctx.fillStyle = glyph.strokeColor
                ctx.lineWidth = 1.7
                ctx.lineCap = "round"
                ctx.lineJoin = "round"
                ctx.beginPath()
                switch (glyph.iconName) {
                case "edit-undo":
                    // Arrow bending back to the left.
                    ctx.moveTo(9 * s, 13.5 * s)
                    ctx.lineTo(4.5 * s, 9 * s)
                    ctx.lineTo(9 * s, 4.5 * s)
                    ctx.moveTo(4.5 * s, 9 * s)
                    ctx.lineTo(13.5 * s, 9 * s)
                    ctx.arc(13.5 * s, 14.25 * s, 5.25 * s, -Math.PI / 2, Math.PI / 2, false)
                    ctx.lineTo(10 * s, 19.5 * s)
                    ctx.stroke()
                    break
                case "edit-redo":
                    // Arrow bending forward to the right.
                    ctx.moveTo(15 * s, 13.5 * s)
                    ctx.lineTo(19.5 * s, 9 * s)
                    ctx.lineTo(15 * s, 4.5 * s)
                    ctx.moveTo(19.5 * s, 9 * s)
                    ctx.lineTo(10.5 * s, 9 * s)
                    ctx.arc(10.5 * s, 14.25 * s, 5.25 * s, -Math.PI / 2, Math.PI / 2, true)
                    ctx.lineTo(14 * s, 19.5 * s)
                    ctx.stroke()
                    break
                case "edit-cut":
                    // Scissors: two handle rings and three blade strokes.
                    ctx.moveTo(20 * s, 4 * s)
                    ctx.lineTo(8.4 * s, 15.6 * s)
                    ctx.moveTo(14.6 * s, 14.6 * s)
                    ctx.lineTo(20 * s, 20 * s)
                    ctx.moveTo(8.4 * s, 8.4 * s)
                    ctx.lineTo(12 * s, 12 * s)
                    ctx.stroke()
                    ctx.beginPath()
                    ctx.arc(6 * s, 6 * s, 2.6 * s, 0, Math.PI * 2)
                    ctx.stroke()
                    ctx.beginPath()
                    ctx.arc(6 * s, 18 * s, 2.6 * s, 0, Math.PI * 2)
                    ctx.stroke()
                    break
                case "edit-copy":
                    // Front sheet over the top-left corner of the sheet behind.
                    roundRect(ctx, 9 * s, 9 * s, 11 * s, 11 * s, 2 * s)
                    ctx.moveTo(5 * s, 15 * s)
                    ctx.lineTo(5 * s, 7 * s)
                    ctx.arcTo(5 * s, 5 * s, 7 * s, 5 * s, 2 * s)
                    ctx.lineTo(15 * s, 5 * s)
                    ctx.stroke()
                    break
                case "edit-paste":
                    // Clipboard: board outline open at the top for the clip.
                    ctx.moveTo(9 * s, 4 * s)
                    ctx.lineTo(7 * s, 4 * s)
                    ctx.arcTo(5 * s, 4 * s, 5 * s, 6 * s, 2 * s)
                    ctx.lineTo(5 * s, 19 * s)
                    ctx.arcTo(5 * s, 21 * s, 7 * s, 21 * s, 2 * s)
                    ctx.lineTo(17 * s, 21 * s)
                    ctx.arcTo(19 * s, 21 * s, 19 * s, 19 * s, 2 * s)
                    ctx.lineTo(19 * s, 6 * s)
                    ctx.arcTo(19 * s, 4 * s, 17 * s, 4 * s, 2 * s)
                    ctx.lineTo(15 * s, 4 * s)
                    ctx.stroke()
                    ctx.beginPath()
                    roundRect(ctx, 9 * s, 2 * s, 6 * s, 4 * s, 1.5 * s)
                    ctx.stroke()
                    break
                case "edit-delete":
                    // Trash can: lid line, handle, body, and two inner lines.
                    ctx.moveTo(3.5 * s, 6 * s)
                    ctx.lineTo(20.5 * s, 6 * s)
                    ctx.moveTo(9 * s, 6 * s)
                    ctx.lineTo(9 * s, 3.5 * s)
                    ctx.lineTo(15 * s, 3.5 * s)
                    ctx.lineTo(15 * s, 6 * s)
                    ctx.moveTo(18.5 * s, 6 * s)
                    ctx.lineTo(18.5 * s, 19 * s)
                    ctx.arcTo(18.5 * s, 21 * s, 16.5 * s, 21 * s, 2 * s)
                    ctx.lineTo(7.5 * s, 21 * s)
                    ctx.arcTo(5.5 * s, 21 * s, 5.5 * s, 19 * s, 2 * s)
                    ctx.lineTo(5.5 * s, 6 * s)
                    ctx.moveTo(10 * s, 10.5 * s)
                    ctx.lineTo(10 * s, 16.5 * s)
                    ctx.moveTo(14 * s, 10.5 * s)
                    ctx.lineTo(14 * s, 16.5 * s)
                    ctx.stroke()
                    break
                case "edit-select-all":
                    // Dashed selection frame around a centered letter.
                    ctx.setLineDash([2.5 * s, 2.5 * s])
                    roundRect(ctx, 4 * s, 4 * s, 16 * s, 16 * s, 3 * s)
                    ctx.stroke()
                    ctx.setLineDash([])
                    ctx.font = "bold " + (10 * s) + "px sans-serif"
                    ctx.textAlign = "center"
                    ctx.textBaseline = "middle"
                    ctx.fillText("A", 12 * s, 12.5 * s)
                    break
                }
            }
        }
    }

    UndoAction { editor: control.editor }
    RedoAction { editor: control.editor }

    SwbMenuSeparator { theme: control.theme }

    CutAction { editor: control.editor }
    CopyAction { editor: control.editor }
    PasteAction { editor: control.editor }
    DeleteAction { editor: control.editor }

    SwbMenuSeparator { theme: control.theme }

    SelectAllAction { editor: control.editor }
}
