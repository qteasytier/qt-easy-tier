import QtQuick

QtObject {
    property bool darkMode: SwbTheme.darkMode
    readonly property SwbPalette palette: darkMode ? SwbTheme.darkPalette : SwbTheme.lightPalette

    // Pick a value by this style's own light/dark mode, so a per-instance
    // override keeps switching with the color scheme:
    //   theme.primary: theme.adaptive("#7c3aed", "#a78bfa")
    function adaptive(light, dark) { return darkMode ? dark : light }

    property color primary:             palette.primary
    property color primaryForeground:   palette.primaryForeground
    property color secondary:           palette.secondary
    property color secondaryForeground: palette.secondaryForeground
    property color accent:              palette.accent
    property color accentForeground:    palette.accentForeground
    property color background:          palette.background
    property color popover:             palette.popover
    property color foreground:          palette.foreground
    property color mutedForeground:     palette.mutedForeground
    property color border:              palette.border
    property color destructive:         palette.destructive
    property color destructiveForeground: palette.destructiveForeground
    property color ring:                palette.ring

    // Derived tokens follow the palette when it customized them away from the
    // default formulas (mirrored from SwbPalette); otherwise they re-derive from
    // the local base value, so overriding e.g. `primary` on one instance cascades
    // to its hover color without touching any global state.
    property color primaryHover: Qt.colorEqual(palette.primaryHover, palette.withAlpha(palette.primary, 0.8))
                                 ? withAlpha(primary, 0.8) : palette.primaryHover
    property color secondaryHover: Qt.colorEqual(palette.secondaryHover, Qt.darker(palette.secondary, 1.05))
                                   ? Qt.darker(secondary, 1.05) : palette.secondaryHover
    property color accentHover: Qt.colorEqual(palette.accentHover, Qt.darker(palette.accent, 1.08))
                                ? Qt.darker(accent, 1.08) : palette.accentHover
    property color destructiveBg: Qt.colorEqual(palette.destructiveBg, palette.withAlpha(palette.destructive, 0.1))
                                  ? withAlpha(destructive, 0.1) : palette.destructiveBg
    property color destructiveBgHover: Qt.colorEqual(palette.destructiveBgHover, palette.withAlpha(palette.destructive, 0.2))
                                       ? withAlpha(destructive, 0.2) : palette.destructiveBgHover
    property color focusRing: Qt.colorEqual(palette.focusRing, palette.withAlpha(palette.ring, 0.5))
                              ? withAlpha(ring, 0.5) : palette.focusRing
    property int focusRingWidth:       SwbTheme.focusRingWidth

    property int radius:        SwbTheme.radius
    property int radiusSm:      SwbTheme.radiusSm === SwbTheme.radius - 2
                                ? radius - 2 : SwbTheme.radiusSm
    property int fontSize:      SwbTheme.fontSize
    property int fontSizeSm:    SwbTheme.fontSizeSm === SwbTheme.fontSize - 1
                                ? fontSize - 1 : SwbTheme.fontSizeSm
    property int fontWeight:    SwbTheme.fontWeight
    property int controlHeight: SwbTheme.controlHeight
    property int controlHeightSm: SwbTheme.controlHeightSm === SwbTheme.controlHeight - 4
                                  ? controlHeight - 4 : SwbTheme.controlHeightSm
    property int controlHeightLg: SwbTheme.controlHeightLg === SwbTheme.controlHeight + 4
                                  ? controlHeight + 4 : SwbTheme.controlHeightLg
    property int iconSize:      SwbTheme.iconSize
    property int handleSize:    SwbTheme.handleSize
    property color handleColor: SwbTheme.handleColor
    property int trackThickness: SwbTheme.trackThickness
    property int calendarCellSize: SwbTheme.calendarCellSize
    property int calendarSpacing:  SwbTheme.calendarSpacing
    property int animationDuration: SwbTheme.animationDuration

    function withAlpha(c, a) {
        return Qt.rgba(c.r, c.g, c.b, a)
    }
}
