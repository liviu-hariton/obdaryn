// SPDX-License-Identifier: GPL-3.0-or-later
//
// Charts: several values over time, one strip per value on a shared time axis
// (different units never share a y-axis). Live from the car, or from a
// recording, optionally compared with a second recording.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Shapes
import Obdaryn

Item {
    id: page
    signal openPage(int p)
    /// Set by Main: the screen is showing (live charts poll their values only then).
    property bool current: false
    readonly property var charts: App.charts
    readonly property bool live: charts.source === "live"
    readonly property bool ready: App.connectionState === App.Ready
    readonly property bool comparing: !live && charts.recordingB.length > 0
    readonly property int labelWidth: 52     // y-axis labels left of each plot
    readonly property int plotHeight: 84

    Binding { target: page.charts; property: "active"; value: page.current }

    FileDialog {
        id: openDialog
        property int slot: 0
        title: qsTr("Open a recording")
        nameFilters: [qsTr("Obdaryn recordings (*.obdrec)"), qsTr("All files (*)")]
        onAccepted: page.charts.loadRecording(slot, selectedFile)
    }

    C.ScrollView {
        id: scroll
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            id: content
            width: Math.min(Theme.contentMaxWidth, scroll.availableWidth - 2 * Theme.s6)
            x: (scroll.availableWidth - width) / 2
            spacing: Theme.s4

            PageHeader {
                Layout.fillWidth: true
                Layout.topMargin: Theme.s5
                title: qsTr("Charts")
                subtitle: page.live ? qsTr("Live values, one chart each on the same time axis. Hover to read every value at one moment.")
                                    : qsTr("A whole recording, one chart per value. Add a second recording to compare two drives.")
                Segmented {
                    model: [{ value: "live", label: qsTr("Live") }, { value: "recording", label: qsTr("Recordings") }]
                    value: page.charts.source
                    onPicked: v => page.charts.source = v
                }
                AppButton {
                    variant: "quiet"
                    iconName: "copy"
                    text: qsTr("Copy as CSV")
                    enabled: page.charts.strips.length > 0
                    onClicked: App.copyToClipboard(page.charts.toCsv())
                }
            }

            // ---- Controls: one row above the charts ------------------------------
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.s3
                visible: page.live
                Text {
                    text: qsTr("Last")
                    color: Theme.textMuted
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontBody
                }
                Segmented {
                    model: [{ value: "60", label: qsTr("1 min") }, { value: "120", label: qsTr("2 min") },
                            { value: "300", label: qsTr("5 min") }, { value: "600", label: qsTr("10 min") }]
                    value: String(page.charts.windowSeconds)
                    onPicked: v => page.charts.windowSeconds = parseInt(v)
                }
                Item { Layout.fillWidth: true }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.s3
                visible: !page.live
                // Recordings to chart: from the recordings folder, or any file.
                readonly property var choices: App.recordings.map(r => ({ label: r.title + " · " + r.when, value: r.path }))
                Dropdown {
                    id: pickA
                    Layout.fillWidth: true
                    Layout.minimumWidth: 140
                    Layout.maximumWidth: 300
                    model: [{ label: qsTr("Choose a recording…"), value: "" }].concat(parent.choices)
                    value: ""
                    displayText: page.charts.recordingA.length ? page.charts.recordingA : qsTr("Choose a recording…")
                    Accessible.name: qsTr("Recording to chart")
                    onPicked: v => { if (v.length) page.charts.loadRecording(0, v) }
                }
                Dropdown {
                    id: pickB
                    Layout.fillWidth: true
                    Layout.minimumWidth: 140
                    Layout.maximumWidth: 300
                    enabled: page.charts.recordingA.length > 0
                    model: [{ label: qsTr("Compare with…"), value: "" }].concat(parent.choices)
                    value: ""
                    displayText: page.charts.recordingB.length ? page.charts.recordingB : qsTr("Compare with…")
                    Accessible.name: qsTr("Second recording to compare")
                    onPicked: v => page.charts.loadRecording(1, v)
                }
                AppButton {
                    variant: "quiet"
                    visible: page.comparing
                    iconName: "x"
                    text: qsTr("Stop comparing")
                    onClicked: page.charts.loadRecording(1, "")
                }
                AppButton {
                    variant: "quiet"
                    iconName: "folder-open"
                    text: qsTr("Open a file…")
                    onClicked: { openDialog.slot = page.charts.recordingA.length ? 1 : 0; openDialog.open() }
                }
                Item { Layout.fillWidth: true }
            }

            // ---- Presets and the values to chart ------------------------------------
            Flow {
                Layout.fillWidth: true
                spacing: 6
                visible: page.charts.available.length > 0
                Text {
                    height: 26
                    verticalAlignment: Text.AlignVCenter
                    rightPadding: Theme.s2
                    text: qsTr("Presets")
                    color: Theme.textMuted
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontSmall
                }
                Repeater {
                    model: page.charts.presets.filter(p => p.available)
                    delegate: Row {
                        id: preset
                        required property var modelData
                        spacing: 0
                        AppButton {
                            variant: "secondary"
                            implicitHeight: 26
                            text: preset.modelData.name
                            onClicked: page.charts.applyPreset(preset.modelData.name)
                        }
                        AppButton {
                            visible: preset.modelData.saved
                            variant: "quiet"
                            implicitHeight: 26
                            iconName: "x"
                            Accessible.name: qsTr("Delete the preset %1").arg(preset.modelData.name)
                            onClicked: page.charts.deletePreset(preset.modelData.name)
                        }
                    }
                }
                AppButton {
                    visible: !saveRow.visible
                    variant: "quiet"
                    implicitHeight: 26
                    text: qsTr("Save as preset…")
                    enabled: page.charts.strips.length > 0
                    onClicked: { saveRow.visible = true; presetName.text = ""; presetName.forceActiveFocus() }
                }
                Row {
                    id: saveRow
                    visible: false
                    spacing: 6
                    TextInput {
                        id: presetName
                        width: 200
                        placeholder: qsTr("Preset name")
                        onAccepted: savePreset.clicked()
                    }
                    AppButton {
                        id: savePreset
                        anchors.bottom: presetName.bottom
                        implicitHeight: 36
                        variant: "primary"
                        text: qsTr("Save")
                        enabled: presetName.text.trim().length > 0
                        onClicked: { page.charts.savePreset(presetName.text); saveRow.visible = false }
                    }
                    AppButton {
                        anchors.bottom: presetName.bottom
                        implicitHeight: 36
                        variant: "quiet"
                        text: qsTr("Cancel")
                        onClicked: saveRow.visible = false
                    }
                }
            }

            // The charted values as removable chips, and a list to add more:
            // a few dozen values would bury the charts as toggles.
            Flow {
                Layout.fillWidth: true
                spacing: 6
                visible: page.charts.available.length > 0
                Text {
                    height: 36
                    verticalAlignment: Text.AlignVCenter
                    rightPadding: Theme.s2
                    text: qsTr("Values")
                    color: Theme.textMuted
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontSmall
                }
                Repeater {
                    model: page.charts.available.filter(v => v.selected)
                    delegate: AppButton {
                        required property var modelData
                        variant: "secondary"
                        implicitHeight: 36
                        iconName: "x"
                        iconTrailing: true
                        text: modelData.name
                        Accessible.name: qsTr("Remove %1 from the charts").arg(modelData.name)
                        onClicked: page.charts.toggleSignal(modelData.pid)
                    }
                }
                Dropdown {
                    id: addValue
                    width: 240
                    readonly property var choices: page.charts.available.filter(v => !v.selected)
                    readonly property bool full: page.charts.strips.length >= page.charts.maxSignals
                    enabled: !full && choices.length > 0
                    model: [{ label: full ? qsTr("Up to %1 values").arg(page.charts.maxSignals) : qsTr("Add a value…"), value: "" }]
                           .concat(choices.map(v => ({ label: v.name, value: String(v.pid) })))
                    value: ""
                    Accessible.name: qsTr("Add a value to the charts")
                    onPicked: v => { if (v.length) page.charts.toggleSignal(parseInt(v)); currentIndex = 0 }
                }
            }

            // ---- Empty states ------------------------------------------------------
            EmptyState {
                visible: page.live && !page.ready
                Layout.topMargin: Theme.s5
                iconName: "chart-line"
                title: qsTr("Connect to a car for live charts")
                message: qsTr("Or chart a recording: it works without the car.")
                AppButton { text: qsTr("Chart a recording"); onClicked: page.charts.source = "recording" }
            }
            EmptyState {
                visible: !page.live && page.charts.recordingA.length === 0
                Layout.topMargin: Theme.s5
                iconName: "chart-line"
                title: qsTr("Choose a recording")
                message: App.recordings.length > 0 ? qsTr("Pick one above. Recordings are made with Record session in the sidebar while connected.")
                                                   : qsTr("No recordings yet. While connected, press Record session in the sidebar, then chart the recording here.")
            }
            EmptyState {
                visible: page.charts.available.length > 0 && page.charts.strips.length === 0 && (page.ready || !page.live)
                Layout.topMargin: Theme.s5
                iconName: "chart-line"
                title: qsTr("Pick the values to chart")
                message: qsTr("Choose a preset or up to %1 values above.").arg(page.charts.maxSignals)
            }

            // ---- Legend (two series only: comparing recordings) -------------------
            Row {
                visible: page.comparing
                spacing: Theme.s5
                Repeater {
                    model: [{ key: "A", name: page.charts.recordingA, dashed: false }, { key: "B", name: page.charts.recordingB, dashed: true }]
                    delegate: Row {
                        required property var modelData
                        spacing: 8
                        // A short line key, the way the series is drawn.
                        Shape {
                            width: 20; height: 10
                            anchors.verticalCenter: parent.verticalCenter
                            ShapePath {
                                strokeColor: modelData.dashed ? Theme.chartB : Theme.chartA
                                strokeWidth: 2
                                strokeStyle: modelData.dashed ? ShapePath.DashLine : ShapePath.SolidLine
                                dashPattern: [2, 1.5]
                                capStyle: ShapePath.RoundCap
                                startX: 1; startY: 5
                                PathLine { x: 19; y: 5 }
                            }
                        }
                        Text {
                            text: modelData.key + ": " + modelData.name
                            textFormat: Text.PlainText
                            color: Theme.text
                            font.family: Theme.sans
                            font.pixelSize: Theme.fontSmall
                        }
                    }
                }
            }

            // ---- The strips ------------------------------------------------------------
            Item {
                id: stripsArea
                Layout.fillWidth: true
                Layout.bottomMargin: Theme.s6
                visible: page.charts.strips.length > 0 && (page.ready || !page.live)
                implicitHeight: stripColumn.implicitHeight + axis.height + 6
                readonly property real plotWidth: width - page.labelWidth
                onPlotWidthChanged: page.charts.setPlotSize(plotWidth, page.plotHeight)
                Component.onCompleted: page.charts.setPlotSize(plotWidth, page.plotHeight)

                Column {
                    id: stripColumn
                    width: parent.width
                    spacing: Theme.s4
                    Repeater {
                        model: page.charts.strips
                        delegate: Column {
                            id: strip
                            required property var modelData
                            width: stripColumn.width
                            spacing: 12 // room for the top axis label

                            // Name, current value and the range shown. Text in text
                            // colours; the line key carries the series colour.
                            RowLayout {
                                width: parent.width
                                spacing: Theme.s2
                                Text {
                                    text: strip.modelData.name
                                    color: Theme.text
                                    font.family: Theme.sans
                                    font.pixelSize: Theme.fontBody
                                    font.weight: Font.DemiBold
                                    elide: Text.ElideRight
                                    Layout.maximumWidth: parent.width * 0.5
                                }
                                Text {
                                    visible: strip.modelData.latest.length > 0
                                    text: strip.modelData.latest
                                    color: Theme.text
                                    font.family: Theme.sans
                                    font.pixelSize: Theme.fontBody
                                    font.features: { "tnum": 1 }
                                }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: strip.modelData.empty ? qsTr("no readings in this window") : strip.modelData.rangeText
                                    color: Theme.textFaint
                                    font.family: Theme.sans
                                    font.pixelSize: Theme.fontSmall
                                    font.features: { "tnum": 1 }
                                }
                            }

                            Item {
                                width: parent.width
                                height: page.plotHeight
                                // y-axis labels: the top and bottom of the visible range.
                                Text {
                                    width: page.labelWidth - 8
                                    horizontalAlignment: Text.AlignRight
                                    y: -height / 2
                                    text: strip.modelData.topLabel
                                    color: Theme.textFaint
                                    font.family: Theme.sans
                                    font.pixelSize: Theme.fontCaption
                                    font.features: { "tnum": 1 }
                                }
                                Text {
                                    width: page.labelWidth - 8
                                    horizontalAlignment: Text.AlignRight
                                    y: page.plotHeight - height / 2
                                    text: strip.modelData.bottomLabel
                                    color: Theme.textFaint
                                    font.family: Theme.sans
                                    font.pixelSize: Theme.fontCaption
                                    font.features: { "tnum": 1 }
                                }
                                Item {
                                    id: plot
                                    x: page.labelWidth
                                    width: parent.width - page.labelWidth
                                    height: page.plotHeight
                                    clip: true
                                    // Recessive grid: top, middle and baseline hairlines.
                                    Repeater {
                                        model: [0, 0.5, 1]
                                        delegate: Rectangle {
                                            required property real modelData
                                            y: Math.min(plot.height - 1, modelData * plot.height)
                                            width: plot.width; height: 1
                                            color: Theme.hairline
                                        }
                                    }
                                    Shape {
                                        anchors.fill: parent
                                        preferredRendererType: Shape.CurveRenderer
                                        ShapePath {
                                            strokeColor: Theme.chartB
                                            strokeWidth: 2
                                            strokeStyle: ShapePath.DashLine
                                            dashPattern: [2, 1.5]
                                            fillColor: "transparent"
                                            capStyle: ShapePath.RoundCap
                                            joinStyle: ShapePath.RoundJoin
                                            PathPolyline { path: strip.modelData.lineB }
                                        }
                                        ShapePath {
                                            strokeColor: Theme.chartA
                                            strokeWidth: 2
                                            fillColor: "transparent"
                                            capStyle: ShapePath.RoundCap
                                            joinStyle: ShapePath.RoundJoin
                                            PathPolyline { path: strip.modelData.lineA }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Shared time axis.
                Item {
                    id: axis
                    anchors.top: stripColumn.bottom
                    anchors.topMargin: 6
                    x: page.labelWidth
                    width: stripsArea.plotWidth
                    height: 16
                    Repeater {
                        model: page.charts.timeTicks
                        delegate: Text {
                            required property var modelData
                            x: Math.max(0, Math.min(axis.width - width, modelData.position * axis.width - width / 2))
                            text: modelData.text
                            color: Theme.textFaint
                            font.family: Theme.sans
                            font.pixelSize: Theme.fontCaption
                            font.features: { "tnum": 1 }
                        }
                    }
                }

                // ---- Crosshair: one vertical line through every strip, one readout ----
                // Mouse: follows the pointer. Keyboard: Tab to the charts, then
                // ← → move it (Shift: bigger steps), Home / End jump, Esc hides it.
                Item {
                    id: hover
                    objectName: "chartsCursor"
                    x: page.labelWidth
                    width: stripsArea.plotWidth
                    height: stripColumn.height
                    activeFocusOnTab: true
                    Accessible.role: Accessible.Chart
                    Accessible.name: qsTr("Charts. Use the left and right arrow keys to read the values at a moment.")
                    property real fraction: 1
                    property bool keyboard: false
                    readonly property bool showing: mouse.containsMouse || (activeFocus && keyboard)
                    readonly property real cursorX: mouse.containsMouse ? mouse.mouseX : fraction * width
                    readonly property real cursorY: mouse.containsMouse ? mouse.mouseY : height / 2
                    property var reading: ({ time: "", rows: [] })
                    function update() { reading = page.charts.readout(fraction) }
                    function step(delta) { keyboard = true; fraction = Math.max(0, Math.min(1, fraction + delta)); update() }
                    Keys.onLeftPressed: e => step(e.modifiers & Qt.ShiftModifier ? -0.1 : -0.01)
                    Keys.onRightPressed: e => step(e.modifiers & Qt.ShiftModifier ? 0.1 : 0.01)
                    Keys.onPressed: e => {
                        if (e.key === Qt.Key_Home) { step(-1); e.accepted = true }
                        else if (e.key === Qt.Key_End) { step(1); e.accepted = true }
                        else if (e.key === Qt.Key_Escape) { keyboard = false; e.accepted = true }
                    }
                    onActiveFocusChanged: if (activeFocus) { keyboard = true; update() }
                    Connections { target: page.charts; function onStripsChanged() { if (hover.showing) hover.update() } }

                    MouseArea {
                        id: mouse
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.NoButton
                        onPositionChanged: { hover.fraction = Math.max(0, Math.min(1, mouseX / width)); hover.update() }
                        onContainsMouseChanged: if (containsMouse) hover.update()
                    }
                }
                // Keyboard focus ring around the whole chart area, clear of the titles.
                Rectangle {
                    x: -8
                    y: -8
                    width: stripsArea.width + 16
                    height: stripColumn.height + axis.height + 22
                    radius: Theme.radiusLarge
                    color: "transparent"
                    border.width: 2
                    border.color: Theme.blue
                    visible: hover.activeFocus
                }
                Rectangle {
                    visible: hover.showing
                    x: page.labelWidth + hover.cursorX
                    width: 1
                    height: stripColumn.height
                    color: Theme.textMuted
                }
                Rectangle {
                    id: readoutBox
                    visible: hover.showing
                    readonly property real anchorX: page.labelWidth + hover.cursorX
                    x: anchorX + 14 + width > stripsArea.width ? anchorX - 14 - width : anchorX + 14
                    y: Math.max(0, Math.min(stripColumn.height - height, hover.cursorY - height / 2))
                    width: readoutColumn.implicitWidth + 2 * Theme.s3
                    height: readoutColumn.implicitHeight + 2 * Theme.s2
                    radius: Theme.radius
                    color: Theme.surface
                    border.width: 1
                    border.color: Theme.hairline
                    Column {
                        id: readoutColumn
                        x: Theme.s3
                        y: Theme.s2
                        spacing: 3
                        Text {
                            text: hover.reading.time
                            color: Theme.textMuted
                            font.family: Theme.sans
                            font.pixelSize: Theme.fontSmall
                            font.features: { "tnum": 1 }
                        }
                        Repeater {
                            model: hover.reading.rows
                            delegate: Row {
                                required property var modelData
                                spacing: 8
                                // Values lead, names follow.
                                Text {
                                    text: page.comparing ? "A " + modelData.a + "   B " + (modelData.b || "—") : modelData.a
                                    color: Theme.text
                                    font.family: Theme.sans
                                    font.pixelSize: Theme.fontSmall
                                    font.weight: Font.DemiBold
                                    font.features: { "tnum": 1 }
                                }
                                Text {
                                    text: modelData.name
                                    color: Theme.textMuted
                                    font.family: Theme.sans
                                    font.pixelSize: Theme.fontSmall
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
