// SPDX-License-Identifier: GPL-3.0-or-later
//
// Live data: every standard value the car supports, with its recent history.
// Pinned values also appear on the Overview and refresh on every cycle.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import Obdaryn

Item {
    id: page
    property bool bulbCheck: false
    signal openPage(int p)
    readonly property bool ready: App.connectionState === App.Ready
    property bool showRaw: false

    NotConnected {
        visible: !page.ready
        anchors.centerIn: parent
        width: Math.min(560, parent.width - 2 * Theme.s6)
        onConnectRequested: page.openPage(0)
    }

    ColumnLayout {
        visible: page.ready
        anchors.fill: parent
        anchors.margins: Theme.s6
        anchors.topMargin: Theme.s5
        spacing: Theme.s4

        PageHeader {
            Layout.fillWidth: true
            title: qsTr("Live data")
            subtitle: App.liveData.count > 0
                      ? qsTr("%1 values · %2 readings per second · pinned values refresh first")
                            .arg(App.liveData.count)
                            .arg(Number(App.liveData.samplesPerSecond).toLocaleString(Qt.locale(), "f", 1))
                      : ""
            AppButton {
                variant: "quiet"
                iconName: "copy"
                text: qsTr("Copy as CSV")
                onClicked: App.copyToClipboard(App.liveData.toCsv())
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.s4
            C.TextField {
                id: search
                Layout.preferredWidth: 320
                implicitHeight: 34
                leftPadding: 34
                placeholderText: qsTr("Filter, e.g. temperature")
                placeholderTextColor: Theme.textFaint
                color: Theme.text
                selectionColor: Theme.blueFill
                selectedTextColor: "#FFFFFF"
                font.family: Theme.sans
                font.pixelSize: Theme.fontBody
                Accessible.name: qsTr("Filter live values")
                background: Rectangle {
                    radius: Theme.radius
                    color: Theme.field
                    border.width: search.activeFocus ? 2 : 1
                    border.color: search.activeFocus ? Theme.blue : Theme.fieldBorder
                    Icon { name: "search"; size: 16; color: Theme.textFaint; x: 11; anchors.verticalCenter: parent.verticalCenter }
                }
            }
            Item { Layout.fillWidth: true }
            ToggleSwitch {
                text: qsTr("Show raw bytes")
                checked: page.showRaw
                onToggled: page.showRaw = checked
            }
        }

        // Column headings.
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.s2
            spacing: Theme.s4
            component Heading: Text {
                color: Theme.textFaint
                font.family: Theme.sans
                font.pixelSize: Theme.fontSmall
                font.weight: Font.DemiBold
            }
            Item { Layout.preferredWidth: 28 }
            Heading { Layout.fillWidth: true; text: qsTr("Value") }
            Heading { Layout.preferredWidth: 220; text: qsTr("Last 2 minutes") }
            Heading { Layout.preferredWidth: 150; horizontalAlignment: Text.AlignRight; text: qsTr("Reading") }
            Heading { visible: page.showRaw; Layout.preferredWidth: 130; text: qsTr("Raw") }
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: App.liveData
            boundsBehavior: Flickable.StopAtBounds
            C.ScrollBar.vertical: C.ScrollBar {}

            delegate: Item {
                id: row
                required property int index
                required property int pid
                required property string pidHex
                required property string name
                required property string unit
                required property string valueText
                required property string raw
                required property var history
                required property bool pinned
                readonly property bool matches: search.text.length === 0
                    || name.toLowerCase().indexOf(search.text.toLowerCase()) >= 0
                    || pidHex.toLowerCase() === search.text.toLowerCase()

                width: ListView.view.width
                height: matches ? 52 : 0
                visible: matches

                Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hairline }

                RowLayout {
                    anchors.fill: parent
                    spacing: Theme.s4

                    AppButton {
                        Layout.preferredWidth: 28
                        implicitHeight: 28
                        variant: "quiet"
                        iconName: "pin"
                        opacity: row.pinned ? 1 : 0.35
                        Accessible.name: row.pinned ? qsTr("Unpin %1").arg(row.name) : qsTr("Pin %1 to the overview").arg(row.name)
                        onClicked: App.liveData.setPinned(row.pid, !row.pinned)
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        Text {
                            Layout.fillWidth: true
                            text: row.name
                            color: Theme.text
                            font.family: Theme.sans
                            font.pixelSize: Theme.fontBody
                            elide: Text.ElideRight
                        }
                        Text {
                            text: qsTr("PID %1").arg(row.pidHex)
                            color: Theme.textFaint
                            font.family: Theme.mono
                            font.pixelSize: Theme.fontCaption
                        }
                    }
                    Sparkline {
                        Layout.preferredWidth: 220
                        values: row.history
                        color: Theme.series[row.index % Theme.series.length]
                        sweep: page.bulbCheck
                    }
                    RowLayout {
                        Layout.preferredWidth: 150
                        Layout.fillWidth: false   // the spacer inside must not make this cell grow
                        spacing: 5
                        Item { Layout.fillWidth: true }
                        Text {
                            Layout.alignment: Qt.AlignBaseline
                            text: row.valueText
                            color: Theme.text
                            font.family: Theme.sans
                            font.pixelSize: Theme.fontSection
                            font.weight: Font.DemiBold
                            font.features: { "tnum": 1 }
                        }
                        Text {
                            Layout.alignment: Qt.AlignBaseline
                            Layout.preferredWidth: 44
                            text: row.unit
                            color: Theme.textMuted
                            font.family: Theme.sans
                            font.pixelSize: Theme.fontSmall
                        }
                    }
                    Text {
                        visible: page.showRaw
                        Layout.preferredWidth: 130
                        text: row.raw
                        color: Theme.textMuted
                        font.family: Theme.mono
                        font.pixelSize: Theme.fontSmall
                    }
                }
            }
        }
    }
}
