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

            // ---- Self-test results (service 06) -------------------------------------
            // The numbers behind the monitors. Read on request: it takes a few
            // seconds and most people only need it when chasing a problem.
            ColumnLayout {
                id: tests
                Layout.fillWidth: true
                spacing: 0
                readonly property var groups: App.monitorTests
                readonly property int failed: count("failed")
                readonly property int close: count("close")
                function count(verdict) {
                    let n = 0
                    for (const g of groups)
                        for (const t of g.tests)
                            if (t.verdict === verdict) n++
                    return n
                }
                function verdictText(v) {
                    return v === "failed" ? qsTr("Failed") : v === "close" ? qsTr("Close to the limit")
                         : v === "passed" ? qsTr("Passed") : qsTr("Not run yet")
                }
                function verdictColor(v) {
                    return v === "failed" ? Theme.redInk : v === "close" ? Theme.amberInk
                         : v === "passed" ? Theme.textMuted : Theme.textFaint
                }

                SectionTitle {
                    Layout.fillWidth: true
                    Layout.bottomMargin: Theme.s2
                    text: qsTr("Self-test results")
                    detail: !App.monitorTestsRead ? ""
                          : tests.failed + tests.close === 0 ? qsTr("all within their limits")
                          : [tests.failed ? qsTr("%n failed", "", tests.failed) : "",
                             tests.close ? qsTr("%n close to the limit", "", tests.close) : ""].filter(s => s.length).join(" · ")
                    AppButton {
                        variant: "quiet"
                        iconName: "refresh-cw"
                        text: App.monitorTestsRead ? qsTr("Read again") : qsTr("Read test results")
                        busy: App.readingMonitorTests
                        enabled: !App.readingMonitorTests && !App.scanning && !App.moduleScanRunning
                        onClicked: App.readMonitorTests()
                    }
                }

                Text {
                    Layout.fillWidth: true
                    Layout.bottomMargin: Theme.s3
                    visible: !App.monitorTestsRead || App.monitorTestsError.length > 0
                    text: App.monitorTestsError.length > 0 ? App.monitorTestsError
                        : qsTr("Each monitor keeps the result of its last test next to the limits the car judges it by. A result close to its limit points to a part that's wearing out, before the check-engine light comes on. Results reset when codes are cleared.")
                    color: Theme.textMuted
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontBody
                    wrapMode: Text.WordWrap
                    lineHeight: 1.3
                }

                Repeater {
                    model: tests.groups
                    delegate: ColumnLayout {
                        id: group
                        required property var modelData
                        Layout.fillWidth: true
                        spacing: 0

                        // Monitor name, with its worst result.
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: Theme.s3
                            Layout.bottomMargin: Theme.s1
                            Text {
                                Layout.fillWidth: true
                                text: group.modelData.monitor
                                color: Theme.text
                                font.family: Theme.sans
                                font.pixelSize: Theme.fontBody
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                            }
                        }

                        Repeater {
                            model: group.modelData.tests
                            delegate: Item {
                                id: test
                                required property var modelData
                                Layout.fillWidth: true
                                // Long Romanian test names wrap to two lines instead of being cut.
                                implicitHeight: Math.max(40, testRow.implicitHeight + 14)
                                Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hairline }
                                RowLayout {
                                    id: testRow
                                    anchors.fill: parent
                                    spacing: Theme.s4
                                    Text {
                                        Layout.fillWidth: true
                                        text: test.modelData.name
                                        color: Theme.textMuted
                                        font.family: Theme.sans
                                        font.pixelSize: Theme.fontBody
                                        wrapMode: Text.WordWrap
                                        maximumLineCount: 2
                                        elide: Text.ElideRight
                                    }
                                    // Where the value sits between its limits.
                                    Item {
                                        Layout.preferredWidth: 120
                                        Layout.preferredHeight: 12
                                        visible: test.modelData.verdict !== "notRun"
                                        Rectangle {
                                            anchors.verticalCenter: parent.verticalCenter
                                            width: parent.width; height: 2; radius: 1
                                            color: Theme.hairline
                                        }
                                        Repeater { // limit ticks
                                            model: [test.modelData.minPosition, test.modelData.maxPosition].filter(p => p >= 0)
                                            delegate: Rectangle {
                                                required property real modelData
                                                x: modelData * (120 - 1)
                                                width: 1; height: 12
                                                color: Theme.textFaint
                                            }
                                        }
                                        Rectangle {
                                            x: test.modelData.position * (120 - 8)
                                            anchors.verticalCenter: parent.verticalCenter
                                            width: 8; height: 8; radius: 4
                                            color: test.modelData.verdict === "failed" ? Theme.red
                                                 : test.modelData.verdict === "close" ? Theme.amber : Theme.green
                                        }
                                    }
                                    Text {
                                        Layout.preferredWidth: 90
                                        horizontalAlignment: Text.AlignRight
                                        text: test.modelData.value
                                        color: Theme.text
                                        font.family: Theme.sans
                                        font.pixelSize: Theme.fontBody
                                        font.features: { "tnum": 1 }
                                    }
                                    Text {
                                        Layout.preferredWidth: 130
                                        text: test.modelData.limits
                                        color: Theme.textFaint
                                        font.family: Theme.sans
                                        font.pixelSize: Theme.fontSmall
                                        font.features: { "tnum": 1 }
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        Layout.preferredWidth: 130
                                        horizontalAlignment: Text.AlignRight
                                        text: tests.verdictText(test.modelData.verdict)
                                        color: tests.verdictColor(test.modelData.verdict)
                                        font.family: Theme.sans
                                        font.pixelSize: Theme.fontBody
                                        font.weight: test.modelData.verdict === "failed" || test.modelData.verdict === "close" ? Font.DemiBold : Font.Normal
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: Theme.s6 }
        }
    }
}
