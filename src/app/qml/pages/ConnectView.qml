// SPDX-License-Identifier: GPL-3.0-or-later
//
// Connecting to the adapter. Shown in place of the Overview until a car answers.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import Obdaryn

Item {
    id: view
    readonly property int state: App.connectionState
    readonly property bool busy: state === App.Connecting || state === App.Initializing

    C.ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: Math.min(620, view.width - 2 * Theme.s6)
            x: (view.width - width) / 2
            spacing: Theme.s5

            Item { Layout.preferredHeight: Theme.s7 }

            ColumnLayout {
                spacing: Theme.s2
                Text {
                    text: qsTr("Connect to your car")
                    color: Theme.text
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontTitle + 4
                    font.weight: Font.Bold
                    font.letterSpacing: -0.6
                    Accessible.role: Accessible.Heading
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Obdaryn talks to the car through a Wi-Fi OBD-II adapter such as the Vgate iCar Pro. It only reads: it can't change anything in the car.")
                    color: Theme.textMuted
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontEmphasis
                    lineHeight: 1.3
                    wrapMode: Text.WordWrap
                }
            }

            // The steps really are a sequence, so they are numbered.
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                Repeater {
                    model: [
                        { title: qsTr("Plug the adapter into the OBD-II socket"),
                          detail: qsTr("Under the dashboard on the driver's side, usually left of the steering column.") },
                        { title: qsTr("Turn the ignition ON"),
                          detail: qsTr("The engine can stay off. Start it only if you want live data with the engine running.") },
                        { title: qsTr("Join the adapter's Wi-Fi on this Mac"),
                          detail: qsTr("Usually called \"V-LINK\" or \"WiFi_OBDII\". This Mac has no internet while it's connected.") },
                        { title: qsTr("Connect"),
                          detail: qsTr("The first time, macOS asks to let Obdaryn find devices on the local network. Allow it.") },
                    ]
                    delegate: RowLayout {
                        id: step
                        required property var modelData
                        required property int index
                        Layout.fillWidth: true
                        spacing: Theme.s4
                        Rectangle {
                            Layout.alignment: Qt.AlignTop
                            Layout.topMargin: Theme.s3
                            width: 24; height: 24; radius: 12
                            color: "transparent"
                            border.width: 1.5
                            border.color: Theme.fieldBorder
                            Text {
                                anchors.centerIn: parent
                                text: step.index + 1
                                color: Theme.textMuted
                                font.family: Theme.sans
                                font.pixelSize: Theme.fontSmall
                                font.weight: Font.DemiBold
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: Theme.s3
                            Layout.bottomMargin: Theme.s3
                            spacing: 2
                            Text {
                                Layout.fillWidth: true
                                text: step.modelData.title
                                color: Theme.text
                                font.family: Theme.sans
                                font.pixelSize: Theme.fontBody + 1
                                font.weight: Font.DemiBold
                                wrapMode: Text.WordWrap
                            }
                            Text {
                                Layout.fillWidth: true
                                text: step.modelData.detail
                                color: Theme.textMuted
                                font.family: Theme.sans
                                font.pixelSize: Theme.fontBody
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }
            }

            // Address + connect.
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.s3
                TextInput {
                    id: hostField
                    Layout.fillWidth: true
                    label: qsTr("Adapter address")
                    text: App.adapterHost
                    mono: true
                    onAccepted: connectButton.clicked()
                }
                TextInput {
                    id: portField
                    Layout.preferredWidth: 110
                    label: qsTr("Port")
                    text: App.adapterPort
                    mono: true
                    validator: IntValidator { bottom: 1; top: 65535 }
                    inputMethodHints: Qt.ImhDigitsOnly
                    onAccepted: connectButton.clicked()
                }
                AppButton {
                    id: connectButton
                    Layout.alignment: Qt.AlignBottom
                    implicitHeight: 36
                    variant: "primary"
                    iconName: "wifi"
                    text: view.busy ? qsTr("Connecting…") : qsTr("Connect")
                    busy: view.busy
                    enabled: !view.busy && hostField.text.length > 0
                    onClicked: App.connectToAdapter(hostField.text, parseInt(portField.text))
                }
            }

            // Progress and errors, each with what to do next.
            Rectangle {
                Layout.fillWidth: true
                visible: view.busy || view.state === App.NoVehicle || view.state === App.Failed
                implicitHeight: statusRow.implicitHeight + 2 * Theme.s4
                radius: Theme.radiusLarge
                color: Theme.surface
                border.width: 1
                border.color: view.state === App.Failed || view.state === App.NoVehicle ? Theme.red : Theme.hairline

                RowLayout {
                    id: statusRow
                    anchors.fill: parent
                    anchors.margins: Theme.s4
                    spacing: Theme.s3
                    Icon {
                        Layout.alignment: Qt.AlignTop
                        visible: !view.busy
                        name: "triangle-alert"
                        color: Theme.red
                        size: 20
                    }
                    C.BusyIndicator {
                        visible: view.busy
                        running: view.busy
                        Layout.preferredWidth: 20
                        Layout.preferredHeight: 20
                        Layout.alignment: Qt.AlignTop
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.s1
                        Text {
                            Layout.fillWidth: true
                            text: view.state === App.NoVehicle ? qsTr("The car isn't answering")
                                : view.state === App.Failed ? qsTr("Couldn't reach the adapter")
                                : App.statusMessage
                            color: Theme.text
                            font.family: Theme.sans
                            font.pixelSize: Theme.fontBody
                            font.weight: Font.DemiBold
                            wrapMode: Text.WordWrap
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: !view.busy
                            text: App.statusMessage
                            color: Theme.textMuted
                            font.family: Theme.sans
                            font.pixelSize: Theme.fontBody
                            wrapMode: Text.WordWrap
                        }
                        Row {
                            visible: view.state === App.NoVehicle
                            Layout.topMargin: Theme.s2
                            spacing: Theme.s2
                            AppButton { text: qsTr("Try again"); iconName: "refresh-cw"; onClicked: App.retryVehicle() }
                        }
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.hairline; Layout.topMargin: Theme.s2 }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.s4
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        text: qsTr("No car nearby?")
                        color: Theme.text
                        font.family: Theme.sans
                        font.pixelSize: Theme.fontBody
                        font.weight: Font.DemiBold
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("The demo car is a simulated 2014 Hyundai Elantra with a few faults, live values and a freeze frame.")
                        color: Theme.textMuted
                        font.family: Theme.sans
                        font.pixelSize: Theme.fontBody
                        wrapMode: Text.WordWrap
                    }
                }
                AppButton {
                    text: qsTr("Try the demo car")
                    iconName: "play"
                    enabled: !view.busy
                    onClicked: App.connectDemo()
                }
            }

            Item { Layout.preferredHeight: Theme.s6 }
        }
    }
}
