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
                            text: qsTr("%1: %2").arg(modelData.command).arg(modelData.supported ? qsTr("works") : qsTr("not supported"))
                            iconName: modelData.supported ? "check" : "x"
                            iconColor: modelData.supported ? Theme.green : Theme.textFaint
                            Accessible.description: modelData.purpose
                            HoverHandler { id: chipHover }
                            C.ToolTip.visible: chipHover.hovered
                            C.ToolTip.delay: 400
                            C.ToolTip.text: modelData.purpose + (modelData.answer ? " (" + modelData.answer + ")" : "")
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
                Label { text: qsTr("Errors") }
                Value { text: adapterPanel.health.errors > 0 ? qsTr("%1 (\"?\", CAN ERROR, ...)").arg(adapterPanel.health.errors) : qsTr("None") }
                Label { text: qsTr("Combined requests") }
                Value { text: adapterPanel.health.multiPid ? qsTr("Yes, up to 6 values per request") : qsTr("No, one value per request") }
                Label { text: qsTr("Response counts") }
                Value {
                    text: adapterPanel.health.responseCounts === "working" ? qsTr("Working: answers return without waiting")
                        : adapterPanel.health.responseCounts === "notSupported" ? qsTr("Not supported by this adapter")
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
