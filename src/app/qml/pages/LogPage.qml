// SPDX-License-Identifier: GPL-3.0-or-later
//
// Adapter log: every command Obdaryn sent and every answer, for troubleshooting.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import Obdaryn

Item {
    id: page

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.s6
        anchors.topMargin: Theme.s5
        spacing: Theme.s4

        PageHeader {
            Layout.fillWidth: true
            title: qsTr("Adapter log")
            subtitle: qsTr("Every command sent to the adapter (→), every answer (←) and every blocked or failed command (!). Copy it into a bug report if something looks wrong.")
            AppButton { variant: "quiet"; iconName: "copy"; text: qsTr("Copy log"); onClicked: App.copyToClipboard(App.trafficLog.toText()) }
            AppButton { variant: "quiet"; iconName: "x"; text: qsTr("Clear log"); onClicked: App.trafficLog.clear() }
        }

        // ---- The adapter and how the connection is doing -------------------------
        RowLayout {
            id: adapterPanel
            Layout.fillWidth: true
            visible: App.connectionState === App.Ready && !App.replayMode && !App.demoMode || App.adapterCheck.done === true
            spacing: Theme.s6
            readonly property var check: App.adapterCheck
            readonly property var health: App.adapterHealth
            function num(v, d) { return Number(v).toLocaleString(Qt.locale(), "f", d) }

            // What it is and what it supports.
            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 3
                Layout.alignment: Qt.AlignTop
                spacing: Theme.s2
                Text {
                    Layout.fillWidth: true
                    text: [adapterPanel.check.identification || App.adapterVersion, adapterPanel.check.description || App.adapterDescription]
                          .filter(t => t && t.length).join(" · ")
                    textFormat: Text.PlainText // the adapter's own words
                    color: Theme.text
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontBody
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true
                    text: adapterPanel.check.done ? adapterPanel.check.summary : qsTr("Checking what the adapter supports…")
                    color: Theme.textMuted
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontSmall
                    wrapMode: Text.WordWrap
                    lineHeight: 1.25
                }
                Flow {
                    Layout.fillWidth: true
                    visible: adapterPanel.check.done === true
                    spacing: 6
                    Repeater {
                        model: adapterPanel.check.items || []
                        delegate: Chip {
                            required property var modelData
                            // Plain name first, the command itself after it in mono.
                            text: modelData.name
                            code: modelData.command
                            iconName: modelData.supported ? "check" : "x"
                            iconColor: modelData.supported ? Theme.green : Theme.textFaint
                            Accessible.role: Accessible.StaticText
                            Accessible.name: qsTr("%1: %2").arg(modelData.name).arg(modelData.supported ? qsTr("works") : qsTr("not supported"))
                            Accessible.description: modelData.purpose
                            HoverHandler { id: chipHover }
                            C.ToolTip.visible: chipHover.hovered
                            C.ToolTip.delay: 400
                            C.ToolTip.text: (modelData.supported ? qsTr("Works.") : qsTr("Not supported by this adapter.")) + " " + modelData.purpose
                                            + (modelData.answer ? " (" + modelData.answer + ")" : "")
                        }
                    }
                }
            }

            // Live numbers.
            GridLayout {
                Layout.preferredWidth: 2
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                columns: 2
                columnSpacing: Theme.s4
                rowSpacing: 4
                component Label: Text {
                    Layout.alignment: Qt.AlignTop | Qt.AlignLeft // level with a value's first line
                    color: Theme.textMuted
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontSmall
                }
                component Value: Text {
                    Layout.fillWidth: true
                    color: Theme.text
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontSmall
                    font.features: { "tnum": 1 }
                    wrapMode: Text.WordWrap
                }
                Label { text: qsTr("Answer time") }
                Value {
                    text: adapterPanel.health.commands > 0
                          ? qsTr("%1 ms on average, 95 % within %2 ms").arg(adapterPanel.num(adapterPanel.health.averageMs, 0)).arg(adapterPanel.num(adapterPanel.health.p95Ms, 0))
                          : "—"
                }
                Label { text: qsTr("No answer") }
                Value {
                    text: qsTr("%1 of %2 commands").arg(adapterPanel.health.timeouts).arg(adapterPanel.health.commands)
                    color: adapterPanel.health.timeouts > 0 ? Theme.amberInk : Theme.text
                }
                Label { text: qsTr("Error answers") }
                Value { text: adapterPanel.health.errors > 0 ? qsTr("%n command(s) answered with an error; the log below shows which", "", adapterPanel.health.errors) : qsTr("None") }
                Label { text: qsTr("Values per request") }
                Value { text: adapterPanel.health.multiPid ? qsTr("Up to 6 at once") : qsTr("One at a time") }
                Label { text: qsTr("Quick answers") }
                Value {
                    // The ELM327 "response count" digit: the adapter returns as soon as
                    // the expected answers are in, instead of waiting for silence.
                    text: adapterPanel.health.responseCounts === "working" ? qsTr("On: the adapter stops waiting once the car has answered")
                        : adapterPanel.health.responseCounts === "notSupported" ? qsTr("Not supported: each answer waits for the car to go quiet")
                        : adapterPanel.health.responseCounts === "off" ? qsTr("Off")
                        : qsTr("Not used yet")
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Theme.radiusLarge
            color: Theme.surface
            border.width: 1
            border.color: Theme.hairline

            EmptyState {
                visible: App.trafficLog.count === 0
                anchors.centerIn: parent
                width: Math.min(420, parent.width - 48)
                iconName: "terminal"
                title: qsTr("Nothing yet")
                message: qsTr("Traffic appears here as soon as Obdaryn connects to an adapter.")
            }

            ListView {
                id: log
                anchors.fill: parent
                anchors.margins: Theme.s3
                clip: true
                model: App.trafficLog
                boundsBehavior: Flickable.StopAtBounds
                C.ScrollBar.vertical: C.ScrollBar {}
                // Follow new lines unless the user scrolled up to read.
                property bool follow: true
                onMovementEnded: follow = atYEnd
                onCountChanged: if (follow) Qt.callLater(positionViewAtEnd)

                delegate: RowLayout {
                    id: line
                    required property string time
                    required property string direction
                    required property string text
                    width: ListView.view.width
                    spacing: Theme.s3
                    Text {
                        text: line.time
                        color: Theme.textFaint
                        font.family: Theme.mono
                        font.pixelSize: Theme.fontSmall
                        Layout.alignment: Qt.AlignTop
                    }
                    Text {
                        Layout.preferredWidth: 14
                        Layout.alignment: Qt.AlignTop
                        text: line.direction === ">" ? "→" : line.direction === "<" ? "←" : "!"
                        color: line.direction === ">" ? Theme.blue : line.direction === "<" ? Theme.textMuted : Theme.amberInk
                        font.family: Theme.mono
                        font.pixelSize: Theme.fontSmall
                        font.weight: Font.Bold
                    }
                    Text {
                        Layout.fillWidth: true
                        text: line.text
                        color: line.direction === "!" ? Theme.amberInk : Theme.text
                        font.family: Theme.mono
                        font.pixelSize: Theme.fontSmall
                        wrapMode: Text.WrapAnywhere
                    }
                }
            }
        }
    }
}
