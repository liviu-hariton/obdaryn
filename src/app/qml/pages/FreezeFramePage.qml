// SPDX-License-Identifier: GPL-3.0-or-later
//
// Freeze frame: the sensor snapshot the engine computer saved when a code was stored.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import Obdaryn

Item {
    id: page
    signal openPage(int p)
    readonly property bool ready: App.connectionState === App.Ready
    readonly property var codeInfo: App.freezeFrameDtc.length ? App.explainCode(App.freezeFrameDtc) : ({})

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
            spacing: Theme.s5

            PageHeader {
                Layout.fillWidth: true
                Layout.topMargin: Theme.s5
                title: qsTr("Freeze frame")
                subtitle: qsTr("What the sensors read at the moment a fault was stored. It helps tell whether the problem happened cold, hot, idling or under load.")
                AppButton { text: qsTr("Read again"); iconName: "refresh-cw"; onClicked: App.refreshFreezeFrame() }
            }

            EmptyState {
                visible: !App.freezeFrameAvailable
                iconName: "snowflake"
                title: qsTr("No freeze frame stored")
                message: qsTr("The car stores one when a fault is confirmed. Clearing codes erases it.")
            }

            RowLayout {
                visible: App.freezeFrameAvailable
                Layout.fillWidth: true
                spacing: Theme.s4
                Text {
                    text: App.freezeFrameDtc
                    color: Theme.text
                    font.family: Theme.mono
                    font.pixelSize: 22
                    font.weight: Font.Bold
                }
                Text {
                    Layout.fillWidth: true
                    text: page.codeInfo.known ? page.codeInfo.title : (page.codeInfo.system || "")
                    color: Theme.textMuted
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontEmphasis
                    elide: Text.ElideRight
                }
            }

            ColumnLayout {
                visible: App.freezeFrameAvailable
                Layout.fillWidth: true
                spacing: 0
                Repeater {
                    model: App.freezeFrameValues
                    delegate: Item {
                        id: row
                        required property var modelData
                        Layout.fillWidth: true
                        implicitHeight: 44
                        Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hairline }
                        RowLayout {
                            anchors.fill: parent
                            spacing: Theme.s4
                            Text {
                                Layout.fillWidth: true
                                text: row.modelData.name
                                color: Theme.text
                                font.family: Theme.sans
                                font.pixelSize: Theme.fontBody
                                elide: Text.ElideRight
                            }
                            Text {
                                text: row.modelData.value
                                color: Theme.text
                                font.family: Theme.sans
                                font.pixelSize: Theme.fontEmphasis
                                font.weight: Font.DemiBold
                                font.features: { "tnum": 1 }
                            }
                            Text {
                                Layout.preferredWidth: 56
                                text: row.modelData.unit
                                color: Theme.textMuted
                                font.family: Theme.sans
                                font.pixelSize: Theme.fontBody
                            }
                        }
                    }
                }
            }
            Item { Layout.preferredHeight: Theme.s6 }
        }
    }
}
