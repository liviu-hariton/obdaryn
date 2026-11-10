// SPDX-License-Identifier: GPL-3.0-or-later
//
// Overview: the answer to "what is my car telling me?" on one screen.
// Until a car is connected it shows the connection steps instead.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import Obdaryn

Item {
    id: page
    property bool bulbCheck: false
    signal openPage(int p)
    signal openCode(string code)

    readonly property bool ready: App.connectionState === App.Ready

    ConnectView { anchors.fill: parent; visible: !page.ready }

    C.ScrollView {
        id: scroll
        anchors.fill: parent
        visible: page.ready
        contentWidth: availableWidth

        ColumnLayout {
            id: content
            width: Math.min(Theme.contentMaxWidth, scroll.availableWidth - 2 * Theme.s6)
            x: (scroll.availableWidth - width) / 2
            spacing: Theme.s6

            Item { Layout.preferredHeight: Theme.s2 }

            // ---- Vehicle identity -----------------------------------------
            PageHeader {
                Layout.fillWidth: true
                title: App.manufacturer.length
                       ? (App.modelYear > 0 ? qsTr("%1 · %2").arg(App.manufacturer).arg(App.modelYear) : App.manufacturer)
                       : qsTr("Your car")
                subtitle: [App.protocolName,
                           App.modules.length === 1 ? qsTr("1 module answered") : qsTr("%1 modules answered").arg(App.modules.length),
                           App.lastScanTime.length ? qsTr("scanned at %1").arg(App.lastScanTime) : ""]
                          .filter(s => s.length).join("  ·  ")

                Text {
                    visible: App.scanning
                    text: App.scanStep + "…"
                    color: Theme.textMuted
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontSmall
                    anchors.verticalCenter: parent.verticalCenter
                }
                AppButton {
                    text: qsTr("Scan again")
                    iconName: "refresh-cw"
                    busy: App.scanning
                    enabled: !App.scanning
                    onClicked: App.runFullScan()
                }
            }

            // VIN on its own line: it is the one thing people copy.
            RowLayout {
                visible: App.vin.length > 0
                Layout.topMargin: -Theme.s5
                spacing: Theme.s2
                Text {
                    text: qsTr("VIN")
                    color: Theme.textFaint
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontSmall
                    font.weight: Font.DemiBold
                }
                Text {
                    text: App.vin
                    color: Theme.text
                    font.family: Theme.mono
                    font.pixelSize: Theme.fontBody
                    font.letterSpacing: 0.6
                }
                AppButton {
                    variant: "quiet"
                    iconName: "copy"
                    implicitHeight: 26
                    Accessible.name: qsTr("Copy VIN")
                    onClicked: App.copyToClipboard(App.vin)
                }
            }

            // ---- Status: MIL, readiness, battery --------------------------------
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: statusRow.implicitHeight + 2 * Theme.s5
                radius: Theme.radiusLarge
                color: Theme.surface
                border.width: 1
                border.color: Theme.hairline

                RowLayout {
                    id: statusRow
                    anchors.fill: parent
                    anchors.margins: Theme.s5
                    spacing: Theme.s5

                    component Status: RowLayout {
                        id: status
                        property bool bulb: false
                        property bool lead: false          // the cell that matters most right now
                        property color headingColor: Theme.text
                        property alias lamp: lampItem
                        property alias heading: headingText.text
                        property alias detail: detailText.text
                        Layout.fillWidth: true
                        // Pixel preferences (not weights) so the minimums can't starve the lead cell.
                        Layout.preferredWidth: lead ? 420 : 260
                        Layout.minimumWidth: lead ? 230 : 170
                        Layout.alignment: Qt.AlignTop
                        spacing: Theme.s4
                        Telltale { id: lampItem; size: status.lead ? 40 : 28; Layout.alignment: Qt.AlignTop; bulbCheck: status.bulb }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 3
                            Text {
                                id: headingText
                                Layout.fillWidth: true
                                color: status.headingColor
                                font.family: Theme.sans
                                font.pixelSize: status.lead ? Theme.fontSection + 2 : Theme.fontEmphasis
                                font.weight: Font.DemiBold
                                wrapMode: Text.WordWrap
                            }
                            Text {
                                id: detailText
                                Layout.fillWidth: true
                                color: Theme.textMuted
                                font.family: Theme.sans
                                font.pixelSize: Theme.fontBody
                                font.features: { "tnum": 1 }
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                    component Divider: Rectangle { Layout.fillHeight: true; implicitWidth: 1; color: Theme.hairline }

                    Status {
                        bulb: page.bulbCheck
                        lead: App.milOn
                        headingColor: App.milOn ? Theme.amberInk : Theme.text
                        lamp.iconName: "engine"
                        lamp.lit: App.milOn
                        lamp.litColor: Theme.amber
                        heading: App.milOn ? qsTr("Check-engine light on") : qsTr("Check-engine light off")
                        detail: {
                            const s = App.dtcs.storedCount, p = App.dtcs.pendingCount, m = App.dtcs.permanentCount
                            if (s + p + m === 0)
                                return qsTr("No trouble codes")
                            return [s ? qsTr("%n stored", "", s) : "", p ? qsTr("%n pending", "", p) : "",
                                    m ? qsTr("%n permanent", "", m) : ""].filter(x => x.length).join("\u00A0· ")  // dots never start a line
                        }
                    }
                    Divider {}
                    Status {
                        bulb: page.bulbCheck
                        lamp.iconName: App.likelyReadyForInspection ? "circle-check" : "list-checks"
                        lamp.lit: App.readinessValid
                        lamp.litColor: App.likelyReadyForInspection ? Theme.green : Theme.amber
                        heading: !App.readinessValid ? qsTr("Readiness unknown")
                               : App.likelyReadyForInspection ? qsTr("Likely ready for the ITP")
                               : qsTr("Not ready for the ITP")
                        detail: App.readinessValid
                                ? (App.incompleteMonitors === 0 ? qsTr("All monitors complete")
                                   : qsTr("%1 of %2 monitors not complete").arg(App.incompleteMonitors).arg(App.monitors.length))
                                : qsTr("The car didn't report its monitors.")
                    }
                    Divider {}
                    Status {
                        bulb: page.bulbCheck
                        lamp.iconName: "battery-medium"
                        lamp.lit: App.batteryVoltage > 0
                        lamp.litColor: App.batteryVoltage > 0 && App.batteryVoltage < 12.0 ? Theme.red : Theme.green
                        heading: App.batteryVoltage > 0
                                 ? qsTr("Battery %1\u00A0V").arg(Number(App.batteryVoltage).toLocaleString(Qt.locale(), "f", 1))
                                 : qsTr("Battery —")
                        detail: App.batteryVoltage <= 0 ? qsTr("Measured by the adapter at the OBD socket.")
                              : App.batteryVoltage >= 13.6 ? qsTr("Charging: the engine is running.")
                              : App.batteryVoltage >= 12.2 ? qsTr("Healthy with the engine off.")
                              : App.batteryVoltage >= 12.0 ? qsTr("Getting low. Start the engine soon.")
                              : qsTr("Low. Start the engine before continuing.")
                    }
                }
            }

            // ---- Trouble codes ----------------------------------------------------
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.s2

                SectionTitle {
                    Layout.fillWidth: true
                    text: qsTr("Trouble codes")
                    AppButton {
                        variant: "quiet"
                        text: qsTr("Details and clearing")
                        iconName: "chevron-right"
                        iconTrailing: true
                        onClicked: page.openPage(1)
                    }
                }

                EmptyState {
                    visible: App.dtcs.count === 0 && !App.scanning
                    Layout.topMargin: Theme.s2
                    iconName: "circle-check"
                    iconColor: Theme.green
                    title: qsTr("No trouble codes")
                    message: qsTr("No module reports a stored, pending or permanent fault.")
                }

                Repeater {
                    // One row per code; a code found in several memories shows several chips.
                    model: { App.dtcs.count; return App.dtcs.grouped() }
                    delegate: C.ItemDelegate {
                        id: row
                        required property var modelData
                        readonly property string code: modelData.code
                        readonly property string title: modelData.title
                        readonly property string ecu: modelData.ecu
                        readonly property string ecuName: modelData.ecuName
                        Layout.fillWidth: true
                        implicitHeight: Math.max(52, contentItem.implicitHeight + 20)
                        hoverEnabled: true
                        activeFocusOnTab: true
                        onClicked: page.openCode(code)
                        Accessible.name: code + " " + title + ", " + modelData.kinds.map(k => k.label).join(", ")
                        background: Rectangle {
                            color: row.hovered ? Theme.hover : "transparent"
                            radius: Theme.radius
                            border.width: row.visualFocus ? 2 : 0
                            border.color: Theme.blue
                            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hairline }
                        }
                        contentItem: RowLayout {
                            spacing: Theme.s4
                            Text {
                                Layout.preferredWidth: 72
                                text: row.code
                                color: Theme.text
                                font.family: Theme.mono
                                font.pixelSize: Theme.fontEmphasis
                                font.weight: Font.DemiBold
                            }
                            Text {
                                Layout.fillWidth: true
                                text: row.title
                                color: Theme.text
                                font.family: Theme.sans
                                font.pixelSize: Theme.fontBody
                                wrapMode: Text.WordWrap
                                maximumLineCount: 2
                                elide: Text.ElideRight
                            }
                            Repeater {
                                model: row.modelData.kinds
                                delegate: KindChip {
                                    required property var modelData
                                    kind: modelData.kind
                                    label: modelData.label
                                }
                            }
                            Text {
                                Layout.preferredWidth: 150
                                text: row.ecuName.length ? row.ecuName : row.ecu
                                color: Theme.textFaint
                                font.family: Theme.sans
                                font.pixelSize: Theme.fontSmall
                                elide: Text.ElideRight
                                horizontalAlignment: Text.AlignRight
                            }
                            Icon { name: "chevron-right"; color: Theme.textFaint; size: 16 }
                        }
                    }
                }
            }

            // ---- Live values (pinned) ------------------------------------------------
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.s3

                SectionTitle {
                    Layout.fillWidth: true
                    text: qsTr("Live values")
                    detail: App.liveData.samplesPerSecond > 0
                            ? qsTr("%1 readings/s").arg(Number(App.liveData.samplesPerSecond).toLocaleString(Qt.locale(), "f", 1))
                            : ""
                    AppButton {
                        variant: "quiet"
                        text: qsTr("All live data")
                        iconName: "chevron-right"
                        iconTrailing: true
                        onClicked: page.openPage(4)
                    }
                }

                EmptyState {
                    visible: App.liveData.count === 0
                    iconName: "activity"
                    title: App.scanning ? qsTr("Looking for live data…") : qsTr("No live data")
                    message: App.scanning ? "" : qsTr("The car didn't report any standard live values.")
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: content.width > 1000 ? 5 : 4
                    columnSpacing: Theme.s5
                    rowSpacing: Theme.s5

                    Repeater {
                        model: App.liveData
                        delegate: ColumnLayout {
                            id: tile
                            required property int index
                            required property string name
                            required property string unit
                            required property string valueText
                            required property var history
                            required property bool pinned
                            visible: pinned
                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            spacing: 4

                            Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.hairline; Layout.bottomMargin: Theme.s2 }
                            Text {
                                Layout.fillWidth: true
                                Layout.preferredHeight: Math.ceil(fontMetrics.height * 2)  // two lines, so tiles align
                                text: tile.name
                                color: Theme.textMuted
                                font.family: Theme.sans
                                font.pixelSize: Theme.fontSmall
                                wrapMode: Text.WordWrap
                                maximumLineCount: 2
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignBottom
                                FontMetrics { id: fontMetrics; font.family: Theme.sans; font.pixelSize: Theme.fontSmall }
                            }
                            RowLayout {
                                spacing: 5
                                Text {
                                    Layout.alignment: Qt.AlignBaseline
                                    text: tile.valueText
                                    color: Theme.text
                                    font.family: Theme.sans
                                    font.pixelSize: Theme.fontValue
                                    font.weight: Font.DemiBold
                                    font.letterSpacing: -0.6
                                    font.features: { "tnum": 1 }
                                }
                                Text {
                                    Layout.alignment: Qt.AlignBaseline
                                    text: tile.unit
                                    color: Theme.textMuted
                                    font.family: Theme.sans
                                    font.pixelSize: Theme.fontBody
                                }
                            }
                            Sparkline {
                                Layout.fillWidth: true
                                values: tile.history
                                color: Theme.series[tile.index % Theme.series.length]
                                sweep: page.bulbCheck
                            }
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: Theme.s6 }
        }
    }
}
