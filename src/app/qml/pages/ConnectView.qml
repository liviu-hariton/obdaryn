// SPDX-License-Identifier: GPL-3.0-or-later
//
// Connecting to the adapter. Shown in place of the Overview until a car answers.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Dialogs
import QtQuick.Layouts
import Obdaryn

Item {
    id: view
    readonly property int state: App.connectionState
    readonly property bool busy: state === App.Connecting || state === App.Initializing

    // The list may be stale if files were added or removed in Finder meanwhile.
    onVisibleChanged: if (visible) App.refreshRecordings()

    FileDialog {
        id: fileDialog
        title: qsTr("Open a recording")
        nameFilters: [qsTr("Obdaryn recordings (*.obdrec)"), qsTr("All files (*)")]
        onAccepted: App.replayRecording(selectedFile)
    }

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

            // ---- Recordings ------------------------------------------------------
            // Sessions recorded from the sidebar, replayed as if the car were here.
            Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.hairline; Layout.topMargin: Theme.s2 }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.s3
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Replay a recording")
                    color: Theme.text
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontBody
                    font.weight: Font.DemiBold
                    Accessible.role: Accessible.Heading
                }
                AppButton {
                    variant: "quiet"
                    iconName: "folder-open"
                    implicitHeight: 30
                    text: qsTr("Open a file…")
                    enabled: !view.busy
                    onClicked: fileDialog.open()
                }
            }

            Text {
                Layout.fillWidth: true
                Layout.topMargin: -Theme.s3
                text: App.recordings.length > 0
                      ? qsTr("A replay answers exactly like the car did, so every screen works without it. Codes can't be cleared in a replay.")
                      : qsTr("Nothing recorded yet. While connected, press Record session in the sidebar: Obdaryn saves everything the car says, and you can replay it here without the car.")
                color: Theme.textMuted
                font.family: Theme.sans
                font.pixelSize: Theme.fontBody
                wrapMode: Text.WordWrap
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: -Theme.s2
                visible: App.recordings.length > 0
                spacing: 0
                Repeater {
                    model: App.recordings
                    delegate: ColumnLayout {
                        id: rec
                        required property var modelData
                        required property int index
                        Layout.fillWidth: true
                        spacing: 0
                        Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.hairline; visible: rec.index > 0 }
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: Theme.s3
                            Layout.bottomMargin: Theme.s3
                            spacing: Theme.s3
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text {
                                    Layout.fillWidth: true
                                    text: rec.modelData.title
                                    // Recordings can come from anyone: never interpret them as markup.
                                    textFormat: Text.PlainText
                                    color: Theme.text
                                    font.family: Theme.sans
                                    font.pixelSize: Theme.fontBody
                                    font.weight: Font.DemiBold
                                    elide: Text.ElideRight
                                }
                                Text {
                                    Layout.fillWidth: true
                                    //: Recording list: when it was made · how long it is
                                    text: qsTr("%1 · %2").arg(rec.modelData.when).arg(rec.modelData.duration)
                                    textFormat: Text.PlainText
                                    color: Theme.textMuted
                                    font.family: Theme.sans
                                    font.pixelSize: Theme.fontSmall
                                    font.features: { "tnum": 1 }
                                    elide: Text.ElideRight
                                }
                            }
                            AppButton {
                                variant: "quiet"
                                iconName: "file-down"
                                implicitHeight: 30
                                text: qsTr("CSV")
                                Accessible.name: qsTr("Export the live data of %1 as CSV").arg(rec.modelData.title)
                                C.ToolTip.visible: hovered
                                C.ToolTip.delay: 600
                                C.ToolTip.text: qsTr("Save the live data as a CSV file next to the recording")
                                onClicked: App.exportRecordingCsv(rec.modelData.path)
                            }
                            AppButton {
                                iconName: "play"
                                implicitHeight: 30
                                text: qsTr("Replay")
                                Accessible.name: qsTr("Replay %1, %2").arg(rec.modelData.title).arg(rec.modelData.when)
                                enabled: !view.busy
                                onClicked: App.replayRecording(rec.modelData.path)
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.s2
                Icon { name: "info"; size: 14; color: Theme.textFaint; Layout.alignment: Qt.AlignTop; Layout.topMargin: 2 }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Saved in %1. A recording includes the car's VIN, so check it before sharing.").arg(App.recordingsFolder)
                    textFormat: Text.PlainText
                    color: Theme.textFaint
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontSmall
                    wrapMode: Text.WordWrap
                }
                AppButton {
                    variant: "quiet"
                    implicitHeight: 26
                    text: qsTr("Show folder")
                    onClicked: App.showRecordingsFolder()
                }
            }

            Item { Layout.preferredHeight: Theme.s6 }
        }
    }
}
