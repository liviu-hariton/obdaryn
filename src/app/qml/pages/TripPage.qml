// SPDX-License-Identifier: GPL-3.0-or-later
//
// Trip computer: distance, fuel and consumption for the current drive,
// how the fuel is measured, fill-up calibration and recent trips.
// Read at a glance by a passenger on a test drive, so the four main numbers
// are big and come first.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import Obdaryn

Item {
    id: page
    signal openPage(int p)
    readonly property bool ready: App.connectionState === App.Ready
    readonly property var trip: App.trip

    function num(v, decimals) { return Number(v).toLocaleString(Qt.locale(), "f", decimals) }
    /// Accepts "1.8" and "1,8" (Romanian keyboards type a comma).
    function parseNumber(text) { return parseFloat(String(text).replace(",", ".")) }

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
            id: content
            width: Math.min(Theme.contentMaxWidth, scroll.availableWidth - 2 * Theme.s6)
            x: (scroll.availableWidth - width) / 2
            spacing: Theme.s6

            PageHeader {
                Layout.fillWidth: true
                Layout.topMargin: Theme.s5
                title: qsTr("Trip")
                subtitle: page.trip.active ? qsTr("Started at %1 · %2").arg(page.trip.startedAt).arg(page.trip.duration) : ""
                AppButton {
                    text: qsTr("New trip")
                    iconName: "refresh-cw"
                    enabled: page.trip.active === true
                    Accessible.description: qsTr("Saves this trip and starts counting from zero")
                    onClicked: App.newTrip()
                }
            }

            // ---- The four numbers -------------------------------------------------
            component BigValue: ColumnLayout {
                id: big
                property string label
                property string value
                property string unit
                /// Why the value is missing, under the dash (empty: nothing to say).
                property string note
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.alignment: Qt.AlignTop // a note under one value mustn't shift the row
                spacing: 4
                Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.hairline; Layout.bottomMargin: Theme.s2 }
                Text {
                    Layout.fillWidth: true
                    text: big.label
                    color: Theme.textMuted
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontSmall
                    elide: Text.ElideRight
                }
                RowLayout {
                    spacing: 6
                    Text {
                        Layout.alignment: Qt.AlignBaseline
                        text: big.value
                        color: big.value === "—" ? Theme.textFaint : Theme.text
                        font.family: Theme.sans
                        font.pixelSize: Theme.fontDisplay
                        font.weight: Font.DemiBold
                        font.letterSpacing: -1
                        font.features: { "tnum": 1 }
                    }
                    Text {
                        Layout.alignment: Qt.AlignBaseline
                        visible: big.value !== "—"
                        text: big.unit
                        color: Theme.textMuted
                        font.family: Theme.sans
                        font.pixelSize: Theme.fontEmphasis
                    }
                }
                Text {
                    Layout.fillWidth: true
                    visible: big.note.length > 0
                    text: big.note
                    color: Theme.textMuted
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontSmall
                    wrapMode: Text.WordWrap
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.s5
                BigValue {
                    label: qsTr("Distance")
                    value: page.num(page.trip.distanceValue || 0, page.trip.distanceValue < 100 ? 1 : 0)
                    unit: page.trip.distanceUnit || ""
                }
                BigValue {
                    label: qsTr("Fuel used")
                    value: page.trip.fuelKnown ? page.num(page.trip.fuelValue, page.trip.fuelValue < 10 ? 2 : 1) : "—"
                    unit: page.trip.fuelUnit || ""
                    // One reason for the three empty fuel values, here where they start.
                    note: page.trip.fuelKnown ? ""
                        : page.trip.method === "needsEngineSize" ? qsTr("Needs the engine size, below")
                        : ["fuelRate", "maf", "speedDensity"].includes(page.trip.method) ? ""
                        : qsTr("Not available on this car")
                }
                BigValue {
                    label: qsTr("Average")
                    value: page.trip.fuelKnown && page.trip.averageValue > 0 ? page.num(page.trip.averageValue, 1) : "—"
                    unit: page.trip.consumptionUnit || ""
                    note: page.trip.averageNote || ""
                }
                BigValue {
                    // Standing still, L/100 km is meaningless: show the flow instead.
                    label: page.trip.moving ? qsTr("Right now") : qsTr("Right now, standing")
                    value: !page.trip.fuelKnown ? "—"
                         : page.trip.moving ? page.num(page.trip.instantValue, 1) : page.num(page.trip.flowValue, 1)
                    unit: (page.trip.moving ? page.trip.consumptionUnit : page.trip.flowUnit) || ""
                }
            }

            // ---- Times and speeds -------------------------------------------------
            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: Theme.s6
                rowSpacing: 0
                KeyValue { Layout.fillWidth: true; Layout.preferredWidth: 1; labelWidth: 160; label: qsTr("Driving"); value: page.trip.drivingTime || "" }
                KeyValue { Layout.fillWidth: true; Layout.preferredWidth: 1; labelWidth: 160; label: qsTr("Average speed")
                           value: page.trip.averageSpeedText || "—" }
                KeyValue { Layout.fillWidth: true; Layout.preferredWidth: 1; labelWidth: 160; label: qsTr("Idling"); value: page.trip.idleTime || "" }
                KeyValue { Layout.fillWidth: true; Layout.preferredWidth: 1; labelWidth: 160; label: qsTr("Top speed")
                           value: page.trip.maxSpeedText || "" }
                KeyValue { Layout.fillWidth: true; Layout.preferredWidth: 1; labelWidth: 160; label: qsTr("Cost")
                           visible: (page.trip.costText || "").length > 0; value: page.trip.costText || "" }
            }

            // ---- How fuel is measured ----------------------------------------------
            ColumnLayout {
                Layout.fillWidth: true
                Layout.maximumWidth: Theme.readingMaxWidth
                spacing: Theme.s3

                SectionTitle { Layout.fillWidth: true; text: qsTr("How fuel is measured") }

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: Theme.text
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontBody
                    lineHeight: 1.3
                    text: {
                        switch (page.trip.method) {
                        case "fuelRate": return qsTr("The engine computer reports its fuel rate, so fuel use is measured, not estimated.")
                        case "maf": return qsTr("Estimated from the air-flow sensor: fuel is the air the engine breathes divided by the air-fuel ratio. Expect up to 10–20 % error until it's calibrated with fill-ups.")
                        case "speedDensity": return qsTr("This car has no air-flow sensor, so fuel is estimated from manifold pressure, intake temperature, engine speed and the engine size. Expect up to 10–20 % error until it's calibrated with fill-ups.")
                        case "needsEngineSize": return qsTr("This car has no air-flow sensor. Enter the engine size and Obdaryn estimates fuel from manifold pressure, intake temperature and engine speed.")
                        default: return page.trip.diesel
                                 ? qsTr("This diesel doesn't report its fuel rate, and a diesel's air-fuel ratio varies too much to estimate fuel from air flow. Distance and times still work.")
                                 : qsTr("This car reports neither fuel rate, air flow nor manifold pressure, so fuel can't be estimated. Distance and times still work.")
                        }
                    }
                }

                // Engine size, only where the estimate needs it.
                RowLayout {
                    visible: page.trip.method === "speedDensity" || page.trip.method === "needsEngineSize"
                    spacing: Theme.s3
                    TextInput {
                        id: engineField
                        Layout.preferredWidth: 160
                        Layout.maximumWidth: 160
                        label: qsTr("Engine size (liters)")
                        placeholder: qsTr("e.g. 1.6")
                        text: page.trip.engineLiters > 0 ? page.num(page.trip.engineLiters, 1) : ""
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        onAccepted: engineSave.clicked()
                    }
                    AppButton {
                        id: engineSave
                        Layout.alignment: Qt.AlignBottom
                        implicitHeight: 36
                        text: qsTr("Save")
                        readonly property real liters: page.parseNumber(engineField.text)
                        enabled: liters >= 0.5 && liters <= 10 && Math.abs(liters - page.trip.engineLiters) > 0.001
                        onClicked: App.setEngineLiters(liters)
                    }
                    Text {
                        Layout.alignment: Qt.AlignBottom
                        Layout.bottomMargin: 9
                        Layout.fillWidth: true
                        text: qsTr("From the registration certificate: cm³ ÷ 1000.")
                        color: Theme.textFaint
                        font.family: Theme.sans
                        font.pixelSize: Theme.fontSmall
                        wrapMode: Text.WordWrap
                    }
                }

                // Calibration with fill-ups.
                Text {
                    Layout.fillWidth: true
                    visible: page.trip.fuelKnown === true || page.trip.method === "maf" || page.trip.method === "speedDensity"
                    wrapMode: Text.WordWrap
                    color: Theme.textMuted
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontBody
                    lineHeight: 1.3
                    text: !page.trip.saved
                          ? qsTr("Trips of the demo car and of recordings aren't saved, and fill-ups can't be recorded for them.")
                          : page.trip.calibrationFillUps > 0
                            ? qsTr("Calibrated with %n fill-up(s): estimates are multiplied by %1.", "", page.trip.calibrationFillUps).arg(page.num(page.trip.calibration, 2))
                              + (page.trip.fillUpStarted ? " " + qsTr("Since the last fill-up: %1.").arg(page.trip.sinceFillUpText) : "")
                            : page.trip.fillUpStarted
                              ? qsTr("Counting since your last fill-up: %1 so far. At the next fill-up, enter the amount the pump shows to calibrate. Every drive in between must be followed by Obdaryn.").arg(page.trip.sinceFillUpText)
                              : qsTr("Not calibrated yet. Fill the tank to the click and record the fill-up, drive normally with Obdaryn connected, then record the next fill-up: Obdaryn compares the pump with its estimate.")
                }

                RowLayout {
                    visible: page.trip.saved === true && !fillForm.visible
                    spacing: Theme.s2
                    AppButton {
                        text: qsTr("Record a fill-up")
                        iconName: "fuel"
                        onClicked: { fillForm.visible = true; litersField.text = ""; litersField.forceActiveFocus() }
                    }
                    AppButton {
                        visible: page.trip.calibrationFillUps > 0
                        variant: "quiet"
                        text: qsTr("Reset calibration")
                        onClicked: App.resetFuelCalibration()
                    }
                }

                // Inline, not a dialog: it's one number.
                RowLayout {
                    id: fillForm
                    visible: false
                    spacing: Theme.s3
                    TextInput {
                        id: litersField
                        Layout.preferredWidth: 180
                        Layout.maximumWidth: 180
                        label: qsTr("Amount at the pump (%1)").arg(page.trip.volumeUnit || "")
                        placeholder: qsTr("e.g. 38.5")
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        onAccepted: fillSave.clicked()
                    }
                    AppButton {
                        id: fillSave
                        Layout.alignment: Qt.AlignBottom
                        implicitHeight: 36
                        variant: "primary"
                        text: qsTr("Save fill-up")
                        readonly property real liters: page.parseNumber(litersField.text)
                        enabled: liters > 0 && liters <= 200
                        onClicked: { App.recordFillUp(liters); fillForm.visible = false }
                    }
                    AppButton {
                        Layout.alignment: Qt.AlignBottom
                        implicitHeight: 36
                        variant: "quiet"
                        text: qsTr("Cancel")
                        onClicked: fillForm.visible = false
                    }
                    Text {
                        Layout.alignment: Qt.AlignBottom
                        Layout.bottomMargin: 9
                        Layout.fillWidth: true
                        text: qsTr("Fill to the first click of the nozzle, every time.")
                        color: Theme.textFaint
                        font.family: Theme.sans
                        font.pixelSize: Theme.fontSmall
                        wrapMode: Text.WordWrap
                    }
                }

                // Fuel price, for the trip's cost. Per liter, or per gallon in US/UK units.
                RowLayout {
                    visible: page.trip.fuelKnown === true || page.trip.method === "maf" || page.trip.method === "speedDensity" || page.trip.method === "fuelRate"
                    Layout.topMargin: Theme.s2
                    spacing: Theme.s3
                    TextInput {
                        id: priceField
                        Layout.preferredWidth: 180
                        Layout.maximumWidth: 180
                        label: qsTr("Fuel price per %1").arg(page.trip.volumeUnit || "")
                        placeholder: qsTr("optional")
                        text: page.trip.fuelPrice > 0 ? page.num(page.trip.fuelPrice, 2) : ""
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        onAccepted: priceSave.clicked()
                    }
                    TextInput {
                        id: currencyField
                        Layout.preferredWidth: 90
                        Layout.maximumWidth: 90
                        label: qsTr("Currency")
                        text: page.trip.currency || ""
                        onAccepted: priceSave.clicked()
                    }
                    AppButton {
                        id: priceSave
                        Layout.alignment: Qt.AlignBottom
                        implicitHeight: 36
                        text: qsTr("Save")
                        readonly property real price: priceField.text.length ? page.parseNumber(priceField.text) : 0
                        enabled: (price >= 0 || priceField.text.length === 0)
                                 && (Math.abs(price - (page.trip.fuelPrice || 0)) > 0.0001 || currencyField.text !== page.trip.currency)
                        onClicked: App.setFuelPrice(isNaN(price) ? 0 : price, currencyField.text)
                    }
                }
            }

            // ---- Recent trips -------------------------------------------------------
            ColumnLayout {
                Layout.fillWidth: true
                Layout.bottomMargin: Theme.s6
                visible: page.trip.saved === true
                spacing: 0

                SectionTitle { Layout.fillWidth: true; text: qsTr("Recent trips"); Layout.bottomMargin: Theme.s2 }

                Text {
                    visible: App.tripHistory.length === 0
                    Layout.fillWidth: true
                    text: qsTr("Trips are saved here when you disconnect or start a new trip.")
                    color: Theme.textMuted
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontBody
                    wrapMode: Text.WordWrap
                }

                Repeater {
                    model: App.tripHistory
                    delegate: ColumnLayout {
                        id: row
                        required property var modelData
                        Layout.fillWidth: true
                        spacing: 0
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: Theme.s2
                            Layout.bottomMargin: Theme.s2
                            spacing: Theme.s4
                            component Cell: Text {
                                color: Theme.text
                                font.family: Theme.sans
                                font.pixelSize: Theme.fontBody
                                font.features: { "tnum": 1 }
                                elide: Text.ElideRight
                            }
                            Cell { Layout.fillWidth: true; text: row.modelData.when; color: Theme.textMuted }
                            Cell { Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight; text: row.modelData.distanceText }
                            Cell { Layout.preferredWidth: 80; horizontalAlignment: Text.AlignRight; text: row.modelData.fuelText || "—" }
                            Cell { Layout.preferredWidth: 120; horizontalAlignment: Text.AlignRight; text: row.modelData.averageText || "—" }
                            Cell { Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight; visible: (row.modelData.cost || "").length > 0
                                   text: row.modelData.cost || "" }
                            Cell { Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight; text: row.modelData.duration; color: Theme.textMuted }
                        }
                        Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.hairline }
                    }
                }
            }
        }
    }
}
