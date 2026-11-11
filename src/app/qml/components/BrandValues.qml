// SPDX-License-Identifier: GPL-3.0-or-later
//
// Manufacturer values on the Live data screen: choose the model, find what
// the car's modules answer, then the values refresh at their own pace.
// Definitions come from OBDb, a community database, so the screen says
// plainly that they aren't checked on this car.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import Obdaryn

ColumnLayout {
    id: panel
    /// App.manufacturerSetup
    property var setup: ({})
    property string filter: ""
    readonly property bool finding: setup.state === "finding"

    spacing: Theme.s3

    // ---- Which car: model, year, and the button that asks the modules ----------
    Flow {
        Layout.fillWidth: true
        spacing: Theme.s3
        Dropdown {
            id: modelBox
            width: 220
            model: [{ label: qsTr("Choose the model…"), value: "" }]
                   .concat((panel.setup.models || []).map(m => ({ label: m, value: m })))
            value: panel.setup.model || ""
            enabled: !panel.finding
            Accessible.name: qsTr("Car model")
            onPicked: v => { if (v.length) App.setManufacturerModel(v) }
        }
        // The year decides which definitions apply. Most VINs tell it.
        Text {
            visible: panel.setup.yearFromVin === true
            height: 36
            verticalAlignment: Text.AlignVCenter
            //: Live data, manufacturer values: e.g. "2014, from the VIN"
            text: qsTr("%1, from the VIN").arg(panel.setup.year)
            color: Theme.textMuted
            font.family: Theme.sans
            font.pixelSize: Theme.fontBody
        }
        C.TextField {
            id: yearField
            visible: panel.setup.yearFromVin !== true
            width: 110
            height: 36
            text: panel.setup.year > 0 ? String(panel.setup.year) : ""
            placeholderText: qsTr("Model year")
            placeholderTextColor: Theme.textFaint
            inputMethodHints: Qt.ImhDigitsOnly
            validator: IntValidator { bottom: 1996; top: 2100 }
            color: Theme.text
            font.family: Theme.sans
            font.pixelSize: Theme.fontBody
            Accessible.name: qsTr("Model year")
            onEditingFinished: if (acceptableInput) App.setManufacturerYear(Number(text))
            background: Rectangle {
                radius: Theme.radius
                color: Theme.field
                border.width: yearField.activeFocus ? 2 : 1
                border.color: yearField.activeFocus ? Theme.blue : Theme.fieldBorder
            }
        }
        AppButton {
            visible: panel.setup.state === "found" || panel.setup.state === "none"
            implicitHeight: 36
            variant: "quiet"
            iconName: "refresh-cw"
            text: qsTr("Find again")
            onClicked: App.findManufacturerValues()
        }
    }
    Text {
        Layout.fillWidth: true
        Layout.maximumWidth: Theme.readingMaxWidth
        visible: panel.setup.state === "found"
        text: qsTr("From OBDb, a community database (CC BY-SA 4.0). These values aren't checked on your car: compare with a workshop tool before relying on one.")
        color: Theme.textMuted
        font.family: Theme.sans
        font.pixelSize: Theme.fontSmall
        wrapMode: Text.WordWrap
        lineHeight: 1.25
    }

    // ---- Before there is anything to list ----------------------------------------
    EmptyState {
        visible: panel.setup.state !== "found"
        Layout.fillWidth: true
        Layout.topMargin: Theme.s4
        iconName: panel.setup.state === "none" ? "info" : "cpu"
        title: panel.setup.state === "choose" ? qsTr("Choose your model")
             : panel.setup.state === "finding" ? qsTr("Asking the modules… %1 of %2").arg(panel.setup.progress).arg(panel.setup.total)
             : panel.setup.state === "none" ? qsTr("No module answered")
             : qsTr("Find the values your car reports")
        message: panel.setup.state === "choose"
                 ? qsTr("%1 values come from community definitions for each model. Pick yours above.").arg(panel.setup.make)
                 : panel.setup.state === "none"
                 ? qsTr("Your car may answer other requests than the ones listed for this model and year. Check the model and year, turn the ignition ON, and try again.")
                 : panel.setup.state === "finding"
                 ? qsTr("Each module is asked once whether it reports each value. It only reads.")
                 : qsTr("Obdaryn asks each module once which of the %1 %2 values it reports. This takes up to a minute and only reads; afterwards the values refresh on their own while this screen is open.")
                       .arg(panel.setup.make).arg(panel.setup.model)
        AppButton {
            visible: panel.setup.state === "ready"
            variant: "primary"
            iconName: "search"
            text: qsTr("Find values")
            onClicked: App.findManufacturerValues()
        }
    }

    // ---- The values ----------------------------------------------------------
    RowLayout {
        visible: panel.setup.state === "found"
        Layout.fillWidth: true
        Layout.topMargin: Theme.s2
        spacing: Theme.s4
        component Heading: Text {
            color: Theme.textFaint
            font.family: Theme.sans
            font.pixelSize: Theme.fontSmall
            font.weight: Font.DemiBold
        }
        Heading { Layout.fillWidth: true; text: qsTr("Value") }
        Heading { Layout.preferredWidth: 220; text: qsTr("Module") }
        Heading { Layout.preferredWidth: 150; horizontalAlignment: Text.AlignRight; text: qsTr("Reading") }
    }
    ListView {
        id: list
        visible: panel.setup.state === "found"
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        model: App.manufacturerValues
        boundsBehavior: Flickable.StopAtBounds
        C.ScrollBar.vertical: C.ScrollBar {}
        section.property: "group"
        section.delegate: Text {
            required property string section
            width: ListView.view.width
            height: 40
            verticalAlignment: Text.AlignBottom
            bottomPadding: 6
            text: section
            color: Theme.textMuted
            font.family: Theme.sans
            font.pixelSize: Theme.fontSmall
            font.weight: Font.DemiBold
        }

        delegate: Item {
            id: row
            required property string name
            required property string module
            required property string request
            required property string valueText
            required property string unit
            required property bool hasValue
            readonly property bool matches: panel.filter.length === 0
                || name.toLowerCase().indexOf(panel.filter.toLowerCase()) >= 0
                || module.toLowerCase().indexOf(panel.filter.toLowerCase()) >= 0

            width: ListView.view.width
            height: matches ? 48 : 0
            visible: matches

            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hairline }

            RowLayout {
                anchors.fill: parent
                spacing: Theme.s4
                Text {
                    Layout.fillWidth: true
                    text: row.name
                    textFormat: Text.PlainText // community data
                    color: Theme.text
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontBody
                    elide: Text.ElideRight
                }
                ColumnLayout {
                    Layout.preferredWidth: 220
                    Layout.maximumWidth: 220
                    spacing: 1
                    Text {
                        Layout.fillWidth: true
                        text: row.module
                        color: Theme.textMuted
                        font.family: Theme.sans
                        font.pixelSize: Theme.fontSmall
                        elide: Text.ElideRight
                    }
                    Text {
                        text: row.request
                        color: Theme.textFaint
                        font.family: Theme.mono
                        font.pixelSize: Theme.fontCaption
                    }
                }
                RowLayout {
                    Layout.preferredWidth: 150
                    Layout.fillWidth: false
                    spacing: 5
                    Item { Layout.fillWidth: true }
                    Text {
                        Layout.alignment: Qt.AlignBaseline
                        text: row.valueText
                        color: row.hasValue ? Theme.text : Theme.textFaint
                        font.family: Theme.sans
                        font.pixelSize: Theme.fontSection
                        font.weight: Font.DemiBold
                        font.features: { "tnum": 1 }
                    }
                    Text {
                        Layout.alignment: Qt.AlignBaseline
                        Layout.preferredWidth: 44
                        text: row.hasValue ? row.unit : "" // a dash needs no unit
                        color: Theme.textMuted
                        font.family: Theme.sans
                        font.pixelSize: Theme.fontSmall
                    }
                }
            }
        }
    }

    // Keeps the setup and the message at the top when there is no list.
    Item { visible: panel.setup.state !== "found"; Layout.fillHeight: true }
}
