// SPDX-License-Identifier: GPL-3.0-or-later
//
// Vehicle info: what the car says about itself (service 09), decoded.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import Obdaryn

Item {
    id: page
    signal openPage(int p)
    readonly property bool ready: App.connectionState === App.Ready

    NotConnected {
        visible: !page.ready
        anchors.centerIn: parent
        width: Math.min(560, parent.width - 2 * Theme.s6)
        onConnectRequested: page.openPage(0)
    }

    C.ScrollView {
        id: scroll
        visible: page.ready
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: Math.min(Theme.readingMaxWidth, scroll.availableWidth - 2 * Theme.s6)
            x: (scroll.availableWidth - width) / 2
            spacing: Theme.s6

            PageHeader {
                Layout.fillWidth: true
                Layout.topMargin: Theme.s5
                title: qsTr("Vehicle info")
                subtitle: qsTr("What the car reports about itself, plus what Obdaryn can decode from the VIN.")
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                SectionTitle { text: qsTr("Identification"); Layout.bottomMargin: Theme.s2 }
                KeyValue { label: qsTr("VIN"); value: App.vin; mono: true; copyable: true
                           note: App.vin.length ? "" : qsTr("The car didn't report its VIN.") }
                KeyValue { label: qsTr("Manufacturer"); value: App.manufacturer
                           note: App.vin.length && !App.manufacturer.length ? qsTr("Not in Obdaryn's manufacturer list yet.") : "" }
                KeyValue { label: qsTr("Model year"); value: App.modelYear > 0 ? App.modelYear : ""
                           note: qsTr("Decoded from the VIN. European VINs don't always encode it, so treat it as a hint.") }
                KeyValue { label: qsTr("Built in"); value: App.vinRegion }
                // The check digit is mandatory only for North American and Chinese
                // VINs; elsewhere a mismatch is normal and must not read as a warning.
                KeyValue { label: qsTr("VIN check digit")
                           value: !App.vin.length ? ""
                                  : App.vinCheckDigitValid ? qsTr("Valid")
                                  : App.vinCheckDigitRequired ? qsTr("Not valid")
                                  : qsTr("Not used")
                           note: App.vin.length && !App.vinCheckDigitValid && !App.vinCheckDigitRequired
                                 ? qsTr("Normal for this car: only North American and Chinese VINs must have one.")
                                 : qsTr("Mandatory in North America and China only; European cars often don't use it.") }
                KeyValue { label: qsTr("OBD standard"); value: App.obdStandard }
                KeyValue { label: qsTr("Fuel"); value: App.fuelType }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                SectionTitle { text: qsTr("Modules"); detail: qsTr("answering generic OBD-II"); Layout.bottomMargin: Theme.s2 }
                Repeater {
                    model: App.modules
                    delegate: KeyValue {
                        required property var modelData
                        label: modelData.address
                        labelMono: true
                        value: modelData.name
                    }
                }
                Text {
                    visible: App.modules.length === 0
                    text: qsTr("No module reported its name.")
                    color: Theme.textMuted
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontBody
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                SectionTitle { text: qsTr("Software"); Layout.bottomMargin: Theme.s2 }
                KeyValue {
                    label: qsTr("Calibration IDs")
                    value: App.calibrationIds.join("\n")
                    mono: true
                    copyable: true
                    note: qsTr("Software versions of the emission-related modules. Useful when checking for updates or tuning.")
                }
                KeyValue {
                    label: qsTr("Calibration verification numbers")
                    value: App.cvns.join("  ")
                    mono: true
                    note: qsTr("Checksums of that software. A change without an official update can mean the ECU was modified.")
                }
            }

            // In-use performance tracking (service 09, PID 08 / 0B). Shown only
            // when the car reports it; monitors it doesn't have are left out.
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                visible: App.performance.available === true
                SectionTitle { text: qsTr("Monitor counters"); detail: App.performance.ecu ? qsTr("reported by %1").arg(App.performance.ecu) : ""; Layout.bottomMargin: Theme.s2 }
                KeyValue {
                    label: qsTr("Engine starts counted")
                    value: App.performance.available ? Number(App.performance.ignitionCycles).toLocaleString(Qt.locale(), "f", 0) : ""
                    note: qsTr("Clearing codes doesn't reset these counters: only a reprogrammed engine computer, or one that lost its memory, starts again from zero. Low numbers on a car with high mileage hint at a recent reset.")
                }
                KeyValue {
                    label: qsTr("Drives where monitors could run")
                    value: App.performance.available ? Number(App.performance.obdConditions).toLocaleString(Qt.locale(), "f", 0) : ""
                    note: qsTr("Below: how many of these drives each emission monitor finished.")
                }
                Repeater {
                    model: App.performance.monitors || []
                    delegate: KeyValue {
                        required property var modelData
                        label: modelData.name
                        value: (modelData.ratio >= 0
                                ? qsTr("%1 of %2 drives (%3 %)").arg(modelData.completed).arg(modelData.conditions)
                                      .arg(Math.round(modelData.ratio * 100))
                                : qsTr("%1 of %2 drives").arg(modelData.completed).arg(modelData.conditions))
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                SectionTitle { text: qsTr("Connection"); Layout.bottomMargin: Theme.s2 }
                KeyValue { label: qsTr("Adapter"); value: App.adapterVersion; note: App.adapterDescription }
                KeyValue { label: qsTr("Link"); value: App.demoMode ? qsTr("Demo car (simulated)") : App.transportDescription; mono: !App.demoMode }
                KeyValue { label: qsTr("Protocol"); value: App.protocolName }
                KeyValue { label: qsTr("Battery at the OBD socket")
                           value: App.batteryVoltage > 0 ? Number(App.batteryVoltage).toLocaleString(Qt.locale(), "f", 1) + " V" : "" }
            }
            Item { Layout.preferredHeight: Theme.s6 }
        }
    }
}
