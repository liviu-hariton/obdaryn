// SPDX-License-Identifier: GPL-3.0-or-later
//
// Trouble codes: the list on the left, the plain-language explanation of the
// selected code on the right. Clearing lives here, behind a confirmation.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import Obdaryn

Item {
    id: page
    signal openPage(int p)
    signal clearRequested()
    property string selectedCode: ""

    readonly property bool ready: App.connectionState === App.Ready
    readonly property var info: selectedCode.length ? App.explainCode(selectedCode) : ({})

    // Keep a sensible selection as codes come and go.
    function ensureSelection() {
        const codes = App.dtcs.codes()
        if (!codes.includes(selectedCode))
            selectedCode = codes.length ? codes[0] : ""
    }
    Connections {
        target: App.dtcs
        function onCountsChanged() { Qt.callLater(page.ensureSelection) }
    }
    Component.onCompleted: Qt.callLater(ensureSelection)

    NotConnected {
        visible: !page.ready
        anchors.centerIn: parent
        width: Math.min(560, parent.width - 2 * Theme.s6)
        onConnectRequested: page.openPage(0)
    }

    ColumnLayout {
        visible: page.ready
        anchors.fill: parent
        anchors.margins: Theme.s6
        anchors.topMargin: Theme.s5
        spacing: Theme.s5

        PageHeader {
            Layout.fillWidth: true
            title: qsTr("Trouble codes")
            subtitle: App.milOn ? qsTr("The check-engine light is on.") : qsTr("The check-engine light is off.")
            AppButton {
                text: qsTr("Read again")
                iconName: "refresh-cw"
                busy: App.scanning
                enabled: !App.scanning
                onClicked: App.refreshCodes()
            }
            AppButton {
                variant: "danger"
                text: qsTr("Clear codes…")
                iconName: "eraser"
                enabled: !App.scanning && App.dtcs.count > 0
                onClicked: page.clearRequested()
            }
        }

        EmptyState {
            visible: App.dtcs.count === 0
            Layout.topMargin: Theme.s5
            iconName: "circle-check"
            iconColor: Theme.green
            title: qsTr("No trouble codes")
            message: qsTr("No module reports a stored, pending or permanent fault. If a warning light is on anyway, it may come from a module generic OBD-II can't reach yet (ABS, airbag), which a later version will scan.")
        }

        RowLayout {
            visible: App.dtcs.count > 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.s5

            // ---- List ---------------------------------------------------------
            ListView {
                id: codeList
                Layout.preferredWidth: Math.max(280, Math.min(380, page.width * 0.36))
                Layout.fillHeight: true
                clip: true
                model: App.dtcs
                spacing: 2
                keyNavigationEnabled: true
                focus: true
                Accessible.role: Accessible.List
                Accessible.name: qsTr("Trouble codes")

                section.property: "kind"
                section.delegate: Text {
                    required property string section
                    width: ListView.view.width
                    topPadding: Theme.s3
                    bottomPadding: Theme.s2
                    leftPadding: Theme.s3
                    text: section === "stored" ? qsTr("Stored: confirmed faults")
                        : section === "pending" ? qsTr("Pending: seen once, not confirmed yet")
                        : qsTr("Permanent: clear themselves after a repair")
                    color: Theme.textFaint
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontSmall
                    font.weight: Font.DemiBold
                }

                delegate: C.ItemDelegate {
                    id: item
                    required property int index
                    required property string code
                    required property string title
                    required property string kind
                    required property string kindLabel
                    readonly property bool current: page.selectedCode === code
                    width: ListView.view.width
                    implicitHeight: Math.max(58, contentItem.implicitHeight + 18)
                    hoverEnabled: true
                    activeFocusOnTab: true
                    onClicked: { page.selectedCode = code; codeList.currentIndex = index }
                    Accessible.name: code + " " + title
                    background: Rectangle {
                        radius: Theme.radius
                        color: item.current ? Theme.selected : item.hovered ? Theme.hover : "transparent"
                        border.width: item.visualFocus ? 2 : 0
                        border.color: Theme.blue
                    }
                    contentItem: ColumnLayout {
                        spacing: 2
                        RowLayout {
                            spacing: Theme.s2
                            Rectangle {
                                width: 7; height: 7; radius: 3.5
                                color: item.kind === "stored" ? Theme.red : item.kind === "pending" ? Theme.amber : Theme.blue
                            }
                            Text {
                                text: item.code
                                color: Theme.text
                                font.family: Theme.mono
                                font.pixelSize: Theme.fontBody + 1
                                font.weight: Font.DemiBold
                            }
                            Text {
                                text: item.kindLabel
                                color: Theme.textFaint
                                font.family: Theme.sans
                                font.pixelSize: Theme.fontSmall
                            }
                        }
                        Text {
                            Layout.fillWidth: true
                            Layout.leftMargin: 15
                            text: item.title
                            color: Theme.textMuted
                            font.family: Theme.sans
                            font.pixelSize: Theme.fontSmall + 1
                            wrapMode: Text.WordWrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                        }
                    }
                }
            }

            Rectangle { Layout.fillHeight: true; implicitWidth: 1; color: Theme.hairline }

            // ---- Explanation ------------------------------------------------------
            C.ScrollView {
                id: detailScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: availableWidth

                ColumnLayout {
                    width: Math.min(720, detailScroll.availableWidth)
                    spacing: Theme.s5

                    ColumnLayout {
                        spacing: Theme.s2
                        Text {
                            text: page.info.code || ""
                            color: Theme.text
                            font.family: Theme.mono
                            font.pixelSize: 34
                            font.weight: Font.Bold
                        }
                        Text {
                            Layout.fillWidth: true
                            text: page.info.known ? page.info.title
                                : page.info.manufacturerSpecific ? qsTr("Manufacturer-specific code")
                                : qsTr("Code not in the database")
                            color: Theme.text
                            font.family: Theme.sans
                            font.pixelSize: 20
                            font.weight: Font.DemiBold
                            wrapMode: Text.WordWrap
                        }
                        Text {
                            Layout.fillWidth: true
                            text: page.info.system || ""
                            color: Theme.textMuted
                            font.family: Theme.sans
                            font.pixelSize: Theme.fontBody
                        }
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: Theme.s2
                        Chip { visible: !!page.info.turnsOnMil; text: qsTr("Turns on the check-engine light"); iconName: "engine"; iconColor: Theme.amber }
                        Chip { visible: !!page.info.emissionsRelevant; text: qsTr("Matters for the ITP"); iconName: "list-checks"; iconColor: Theme.amber }
                        Chip { visible: !!page.info.limpModePossible; text: qsTr("Can trigger limp mode"); iconName: "triangle-alert"; iconColor: Theme.red }
                        Chip { visible: !!page.info.needsDriveCycle; text: qsTr("Needs a drive cycle to confirm a fix"); iconName: "activity" }
                    }

                    ColumnLayout {
                        visible: (page.info.symptoms || []).length > 0
                        Layout.fillWidth: true
                        spacing: Theme.s1
                        SectionTitle { text: qsTr("What you may notice"); Layout.bottomMargin: Theme.s2 }
                        Repeater {
                            model: page.info.symptoms || []
                            delegate: RowLayout {
                                required property string modelData
                                Layout.fillWidth: true
                                spacing: Theme.s3
                                Rectangle { width: 5; height: 5; radius: 2.5; color: Theme.textMuted; Layout.alignment: Qt.AlignTop; Layout.topMargin: 8 }
                                Text {
                                    Layout.fillWidth: true
                                    text: parent.modelData
                                    color: Theme.text
                                    font.family: Theme.sans
                                    font.pixelSize: Theme.fontBody
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }

                    // Likely causes, most likely first.
                    ColumnLayout {
                        visible: (page.info.causes || []).length > 0
                        Layout.fillWidth: true
                        spacing: 0
                        SectionTitle { text: qsTr("Likely causes"); Layout.bottomMargin: Theme.s2 }
                        Repeater {
                            model: page.info.causes || []
                            delegate: RowLayout {
                                id: cause
                                required property var modelData
                                Layout.fillWidth: true
                                spacing: Theme.s4
                                Text {
                                    Layout.preferredWidth: 64
                                    Layout.alignment: Qt.AlignTop
                                    Layout.topMargin: 10
                                    text: cause.modelData.likelihood === "high" ? qsTr("Likely")
                                        : cause.modelData.likelihood === "medium" ? qsTr("Possible")
                                        : qsTr("Rare")
                                    color: cause.modelData.likelihood === "high" ? Theme.text : Theme.textFaint
                                    font.family: Theme.sans
                                    font.pixelSize: Theme.fontSmall
                                    font.weight: Font.DemiBold
                                }
                                Text {
                                    Layout.fillWidth: true
                                    Layout.topMargin: 8
                                    Layout.bottomMargin: 8
                                    text: cause.modelData.text
                                    color: Theme.text
                                    font.family: Theme.sans
                                    font.pixelSize: Theme.fontBody
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        visible: !!page.info.repairDifficulty || (page.info.costMaxEur || 0) > 0
                        Layout.fillWidth: true
                        spacing: 0
                        SectionTitle { text: qsTr("Repair"); Layout.bottomMargin: Theme.s1 }
                        KeyValue {
                            label: qsTr("Difficulty")
                            value: page.info.repairDifficulty === "easy" ? qsTr("Easy")
                                 : page.info.repairDifficulty === "medium" ? qsTr("Medium")
                                 : page.info.repairDifficulty === "hard" ? qsTr("Hard") : (page.info.repairDifficulty || "")
                            note: page.info.diyPossible ? qsTr("Often possible to do yourself.") : qsTr("Usually a job for a workshop.")
                        }
                        KeyValue {
                            visible: (page.info.costMaxEur || 0) > 0
                            label: qsTr("Typical cost")
                            value: qsTr("€%1 – €%2").arg(page.info.costMinEur).arg(page.info.costMaxEur)
                            note: qsTr("Rough range for parts and labour; varies a lot by car and country.")
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.s2
                        SectionTitle { visible: page.info.known === true; text: qsTr("How the car detects it") }
                        Text {
                            Layout.fillWidth: true
                            visible: text.length > 0
                            text: page.info.known ? page.info.description
                                : page.info.manufacturerSpecific
                                  ? qsTr("Its meaning depends on the car's manufacturer. Brand-specific descriptions are planned; until then, search for this code together with the make and model.")
                                  : qsTr("Obdaryn doesn't have a description for this code yet.")
                            color: Theme.text
                            font.family: Theme.sans
                            font.pixelSize: Theme.fontBody + 1
                            lineHeight: 1.45
                            wrapMode: Text.WordWrap
                        }

                    }

                    Flow {
                        visible: (page.info.related || []).length > 0
                        Layout.fillWidth: true
                        spacing: Theme.s2
                        Text {
                            text: qsTr("Related codes")
                            color: Theme.textMuted
                            font.family: Theme.sans
                            font.pixelSize: Theme.fontSmall
                            height: 22
                            verticalAlignment: Text.AlignVCenter
                        }
                        Repeater {
                            model: page.info.related || []
                            delegate: Chip { required property string modelData; text: modelData }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        Layout.topMargin: Theme.s2
                        visible: page.info.known === true
                        text: App.language === "ro"
                              ? qsTr("Descriptions: obdex database (CC0). Available in English for now.")
                              : qsTr("Descriptions: obdex database (CC0). Guidance only; confirm with a workshop before replacing parts.")
                        color: Theme.textFaint
                        font.family: Theme.sans
                        font.pixelSize: Theme.fontSmall
                        wrapMode: Text.WordWrap
                    }

                    Item { Layout.preferredHeight: Theme.s5 }
                }
            }
        }
    }
}
