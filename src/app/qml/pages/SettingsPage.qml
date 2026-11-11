// SPDX-License-Identifier: GPL-3.0-or-later
//
// Settings: language, appearance, motion, demo options, about.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import Obdaryn

Item {
    id: page

    C.ScrollView {
        id: scroll
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: Math.min(Theme.readingMaxWidth, scroll.availableWidth - 2 * Theme.s6)
            x: (scroll.availableWidth - width) / 2
            spacing: Theme.s6

            PageHeader {
                Layout.fillWidth: true
                Layout.topMargin: Theme.s5
                title: qsTr("Settings")
            }

            component SettingRow: RowLayout {
                property alias label: labelText.text
                property alias description: descText.text
                default property alias control: slot.data
                Layout.fillWidth: true
                spacing: Theme.s5
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        id: labelText
                        Layout.fillWidth: true
                        color: Theme.text
                        font.family: Theme.sans
                        font.pixelSize: Theme.fontBody
                        font.weight: Font.Medium
                    }
                    Text {
                        id: descText
                        Layout.fillWidth: true
                        visible: text.length > 0
                        color: Theme.textMuted
                        font.family: Theme.sans
                        font.pixelSize: Theme.fontSmall
                        wrapMode: Text.WordWrap
                    }
                }
                Item {
                    id: slot
                    implicitWidth: children.length ? children[0].implicitWidth : 0
                    implicitHeight: children.length ? children[0].implicitHeight : 0
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.s4
                SectionTitle { text: qsTr("General") }
                SettingRow {
                    label: qsTr("Language")
                    description: qsTr("Trouble-code descriptions stay in English for now.")
                    Segmented {
                        model: [{ value: "en", label: "English" }, { value: "ro", label: "Română" }]
                        value: App.language
                        onPicked: v => App.setLanguage(v)
                    }
                }
                SettingRow {
                    label: qsTr("Appearance")
                    description: qsTr("Dark is easier on the eyes in the car at night.")
                    Segmented {
                        model: [{ value: "system", label: qsTr("System") }, { value: "light", label: qsTr("Light") },
                                { value: "dark", label: qsTr("Dark") }]
                        value: Theme.settings.appearance
                        onPicked: v => Theme.settings.appearance = v
                    }
                }
                ToggleSwitch {
                    Layout.fillWidth: true
                    text: qsTr("Reduce motion")
                    description: qsTr("Skip the start-up light check and other animations.")
                    checked: Theme.settings.reduceMotion
                    onToggled: Theme.settings.reduceMotion = checked
                }
            }

            ColumnLayout {
                // Not only while Ready: with the ignition off the car doesn't answer,
                // and the switch must stay reachable to turn it back on.
                visible: App.demoMode && App.connectionState !== App.Disconnected
                Layout.fillWidth: true
                spacing: Theme.s4
                SectionTitle { text: qsTr("Demo car") }
                ToggleSwitch {
                    Layout.fillWidth: true
                    text: qsTr("Engine running")
                    description: qsTr("Turn the simulated engine off to try clearing codes. Obdaryn refuses while it runs, as with a real car.")
                    checked: App.demoEngineRunning
                    onToggled: App.demoEngineRunning = checked
                }
                ToggleSwitch {
                    Layout.fillWidth: true
                    text: qsTr("Ignition on")
                    description: qsTr("Turn the ignition off and the car stops answering, as when you switch a real car off. Obdaryn waits and picks up again by itself when it's back on.")
                    checked: App.demoIgnitionOn
                    onToggled: App.demoIgnitionOn = checked
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.s3
                SectionTitle { text: qsTr("About") }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Obdaryn %1 · free and open-source software under the GNU GPL v3 or later.").arg(App.appVersion)
                    color: Theme.text
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontBody
                    wrapMode: Text.WordWrap
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.s3
                    Icon { name: "shield-check"; color: Theme.green; size: 18; Layout.alignment: Qt.AlignTop }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Read-only by design. A safety guard checks every command and refuses anything that could write to, code, reset or flash a module in your car. The only change Obdaryn can make is clearing trouble codes, and only after you confirm.")
                        color: Theme.textMuted
                        font.family: Theme.sans
                        font.pixelSize: Theme.fontBody
                        lineHeight: 1.3
                        wrapMode: Text.WordWrap
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Trouble-code descriptions: %1 codes from %2.").arg(App.dtcDatabaseSize).arg(App.dtcAttribution)
                    color: Theme.textFaint
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontSmall
                    wrapMode: Text.WordWrap
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Fonts: Inter and JetBrains Mono (SIL Open Font License). Icons: Lucide (ISC).")
                    color: Theme.textFaint
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontSmall
                    wrapMode: Text.WordWrap
                }
            }
            Item { Layout.preferredHeight: Theme.s6 }
        }
    }
}
