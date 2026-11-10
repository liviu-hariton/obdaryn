// SPDX-License-Identifier: GPL-3.0-or-later
//
// Readiness monitors: has the car finished its self-tests since codes were
// last cleared? An emissions inspection (in Romania, the ITP) needs them done.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import Obdaryn

Item {
    id: page
    property bool bulbCheck: false
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
            spacing: Theme.s5

            Item { Layout.preferredHeight: 0 }

            PageHeader {
                Layout.fillWidth: true
                Layout.topMargin: Theme.s5
                title: qsTr("Readiness")
                subtitle: qsTr("The engine computer's self-tests. They run while you drive and reset whenever codes are cleared or the battery is disconnected.")
                AppButton { text: qsTr("Read again"); iconName: "refresh-cw"; onClicked: App.refreshReadiness() }
            }

            // Verdict.
            Rectangle {
                Layout.fillWidth: true
                visible: App.readinessValid
                implicitHeight: verdict.implicitHeight + 2 * Theme.s5
                radius: Theme.radiusLarge
                color: Theme.surface
                border.width: 1
                border.color: Theme.hairline
                RowLayout {
                    id: verdict
                    anchors.fill: parent
                    anchors.margins: Theme.s5
                    spacing: Theme.s4
                    Telltale {
                        Layout.alignment: Qt.AlignTop
                        size: 34
                        iconName: App.likelyReadyForInspection ? "circle-check" : "list-checks"
                        lit: true
                        litColor: App.likelyReadyForInspection ? Theme.green : Theme.amber
                        bulbCheck: page.bulbCheck
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.s1
                        Text {
                            Layout.fillWidth: true
                            text: App.likelyReadyForInspection ? qsTr("Likely ready for the ITP") : qsTr("Not ready for the ITP yet")
                            color: Theme.text
                            font.family: Theme.sans
                            font.pixelSize: 20
                            font.weight: Font.Bold
                            wrapMode: Text.WordWrap
                        }
                        Text {
                            Layout.fillWidth: true
                            text: App.milOn
                                  ? qsTr("The check-engine light is on, which fails the emissions check on its own. Fix the fault first.")
                                  : App.incompleteMonitors === 0
                                    ? qsTr("All monitors have completed and the check-engine light is off.")
                                    : App.incompleteMonitors === 1
                                      ? qsTr("One monitor hasn't finished. Most inspections accept one; drive a little more to be safe.")
                                      : qsTr("%1 monitors haven't finished. Drive normally for a few days (city and highway) and check again.").arg(App.incompleteMonitors)
                            color: Theme.textMuted
                            font.family: Theme.sans
                            font.pixelSize: Theme.fontBody
                            lineHeight: 1.3
                            wrapMode: Text.WordWrap
                        }
                        Text {
                            Layout.fillWidth: true
                            Layout.topMargin: Theme.s1
                            text: qsTr("Guidance only: inspection rules differ by country and model year.")
                            color: Theme.textFaint
                            font.family: Theme.sans
                            font.pixelSize: Theme.fontSmall
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }

            EmptyState {
                visible: !App.readinessValid
                iconName: "circle-help"
                title: qsTr("The car didn't report its monitors")
                message: qsTr("Older or non-standard cars may not support this. Try reading again with the ignition ON.")
            }

            // Monitors.
            ColumnLayout {
                visible: App.readinessValid
                Layout.fillWidth: true
                spacing: 0
                SectionTitle {
                    Layout.fillWidth: true
                    Layout.bottomMargin: Theme.s2
                    text: App.dieselEngine ? qsTr("Monitors (diesel)") : qsTr("Monitors")
                    detail: qsTr("%1 of %2 complete").arg(App.monitors.length - App.incompleteMonitors).arg(App.monitors.length)
                }
                Repeater {
                    model: App.monitors
                    delegate: Item {
                        id: mon
                        required property var modelData
                        Layout.fillWidth: true
                        implicitHeight: 48
                        Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hairline }
                        RowLayout {
                            anchors.fill: parent
                            spacing: Theme.s4
                            Telltale {
                                size: 18
                                iconName: mon.modelData.complete ? "circle-check" : "circle-dashed"
                                lit: true
                                litColor: mon.modelData.complete ? Theme.green : Theme.amber
                                bulbCheck: page.bulbCheck
                            }
                            Text {
                                Layout.fillWidth: true
                                text: mon.modelData.name
                                color: Theme.text
                                font.family: Theme.sans
                                font.pixelSize: Theme.fontBody
                                elide: Text.ElideRight
                            }
                            Text {
                                text: mon.modelData.continuous ? qsTr("Runs all the time") : ""
                                color: Theme.textFaint
                                font.family: Theme.sans
                                font.pixelSize: Theme.fontSmall
                            }
                            Text {
                                Layout.preferredWidth: 130
                                horizontalAlignment: Text.AlignRight
                                text: mon.modelData.complete ? qsTr("Complete") : qsTr("Not complete")
                                color: mon.modelData.complete ? Theme.textMuted : Theme.amberInk
                                font.family: Theme.sans
                                font.pixelSize: Theme.fontBody
                                font.weight: mon.modelData.complete ? Font.Normal : Font.DemiBold
                            }
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: Theme.s6 }
        }
    }
}
