// SPDX-License-Identifier: GPL-3.0-or-later
//
// Design tokens: colours, type, spacing. Every screen reads its values here.
//
// State colours follow the ISO 2575 telltale convention drivers already know
// from their instrument cluster: red = stop / fault, amber = caution / pending,
// green = complete / connected, blue = information and selection.
pragma Singleton

import QtQuick
import QtCore

QtObject {
    id: theme

    // ---- Appearance --------------------------------------------------------
    // "system" follows macOS light/dark; "light" / "dark" force a theme.
    property Settings settings: Settings {
        category: "ui"
        property string appearance: "system"
        property bool reduceMotion: false
    }
    readonly property bool dark: settings.appearance === "dark"
        || (settings.appearance === "system" && Qt.styleHints.colorScheme === Qt.ColorScheme.Dark)
    readonly property bool reduceMotion: settings.reduceMotion

    // ---- Colour --------------------------------------------------------------
    readonly property color window:      dark ? "#0F1114" : "#F7F8FA"
    readonly property color sidebar:     dark ? "#14171B" : "#EEF0F3"
    readonly property color surface:     dark ? "#171A1F" : "#FFFFFF"
    readonly property color hover:       dark ? "#1D2127" : "#E8EBEF"
    readonly property color selected:    dark ? "#232831" : "#E1E6EE"
    readonly property color hairline:    dark ? "#262A31" : "#DCE0E6"
    readonly property color field:       dark ? "#12151A" : "#FFFFFF"
    readonly property color fieldBorder: dark ? "#323843" : "#C9CFD8"

    readonly property color text:        dark ? "#E8EAED" : "#15181C"
    readonly property color textMuted:   dark ? "#A3A9B2" : "#545C67"
    readonly property color textFaint:   dark ? "#7E858F" : "#646C77"   // 4.65:1 on the sidebar, 5.0 on the window

    // Telltale colours (ISO 2575), for lamps, dots and icons.
    readonly property color red:         dark ? "#F0564A" : "#C8372D"
    readonly property color amber:       dark ? "#F2A93B" : "#B26700"
    readonly property color green:       dark ? "#3CBF7A" : "#1E8A50"
    readonly property color blue:        dark ? "#5B97F7" : "#2F6FE0"
    // Text in a state colour must reach 4.5:1 on the window: use these "ink" variants.
    readonly property color amberInk:    dark ? "#F2A93B" : "#9A5800"
    readonly property color greenInk:    dark ? "#3CBF7A" : "#17703F"
    readonly property color redInk:      dark ? "#F0564A" : "#B3302A"
    readonly property color blueInk:     dark ? "#5B97F7" : "#2860C4"   // links and blue text (5.5:1 on light)
    readonly property color blueFill:    dark ? "#2F6FE0" : "#2F6FE0"   // primary button ground (white text 4.7:1)
    readonly property color blueFillHover: "#2A68D8"                   // darker on hover, so white text gains (5.2:1)
    readonly property color redFill:     dark ? "#C8372D" : "#C8372D"
    readonly property color onFill:      "#FFFFFF"                      // text and marks on a filled ground
    readonly property color unlit:       dark ? "#3A3F47" : "#B9C0CA"   // a telltale that is off
    readonly property color trackOff:    dark ? "#636A75" : "#848C97"   // a switch that is off: 3:1 against the window
    readonly property color amberFill:   "#9A5800"                      // pending-count badge ground (white text 5.6:1)

    // Charts comparing two recordings: A and B. Validated with the dataviz
    // palette checks (lightness band, colour-blind and normal-vision separation,
    // contrast) on each theme's surface; B is also drawn dashed and named in a
    // legend, so the pair never relies on colour alone.
    readonly property color chartA: dark ? "#4C88EE" : "#2F6FE0"
    readonly property color chartB: dark ? "#2EA866" : "#1E8A50"

    // Series colours for live traces: calm, distinct, never red/amber (those mean state).
    readonly property var series: dark
        ? ["#5B97F7", "#3CBF7A", "#C792EA", "#4FC1C9", "#A3A9B2", "#9AA4FF"]   // 5: not the text colour
        : ["#2F6FE0", "#1E8A50", "#8A4FC4", "#1C8C95", "#3B424B", "#5560D8"]

    // ---- Type ------------------------------------------------------------------
    readonly property string sans: "Inter"
    readonly property string mono: "JetBrains Mono"

    readonly property int fontCaption: 11
    readonly property int fontSmall: 12
    readonly property int fontBody: 14
    readonly property int fontEmphasis: 16
    readonly property int fontSection: 18
    readonly property int fontTitle: 26
    readonly property int fontValue: 30
    readonly property int fontDisplay: 40

    // ---- Space & shape -----------------------------------------------------------
    readonly property int s1: 4
    readonly property int s2: 8
    readonly property int s3: 12
    readonly property int s4: 16
    readonly property int s5: 24
    readonly property int s6: 32
    readonly property int s7: 48
    readonly property int radius: 6
    readonly property int radiusLarge: 10
    readonly property int sidebarWidth: 232
    readonly property int contentMaxWidth: 1180
    readonly property int readingMaxWidth: 920   // single-column pages (readiness, vehicle, settings)

    // ---- Motion ------------------------------------------------------------------
    // All exponential ease-out; Reduce motion zeroes them (and skips the bulb check and sweep).
    readonly property int fast: reduceMotion ? 0 : 120         // hover, toggles, small state changes
    readonly property int normal: reduceMotion ? 0 : 220       // telltale cross-fade, jumps, expanding rows
    readonly property int bulbCheck: reduceMotion ? 0 : 1200   // every telltale lit when the car first answers
    readonly property int sweep: reduceMotion ? 0 : 1100       // sparklines drawing in during the bulb check

    /// "#RRGGBB" without the hash, for image://icon/<name>/<RRGGBB>.
    function hex(c) {
        const s = c.toString()
        return s.length === 9 ? s.substring(3) : s.substring(1)   // "#AARRGGBB" or "#RRGGBB"
    }
}
