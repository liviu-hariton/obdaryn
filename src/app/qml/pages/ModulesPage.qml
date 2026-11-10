// SPDX-License-Identifier: GPL-3.0-or-later
//
// All modules: asks every module the brand is known to have (ABS, airbag,
// body, cluster...) for its trouble codes, not just the engine.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import Obdaryn

Item {
    id: page
    signal openPage(int p)
    signal clearRequested(int row, string moduleName)
    signal obdClearRequested()   // engine / transmission: the full OBD-II clear, with all its consequences
    property bool showNotes: false
    property string openCode: ""  // "<moduleId>/<code>" whose explanation is unfolded

    /// Sets hex-like tokens (0x18, 7D1) in the mono face inside rich text.
    function monoHex(text) {
        return text.replace(/&/g, "&amp;").replace(/</g, "&lt;")
                   .replace(/\b(0x[0-9A-Fa-f]+|[0-9A-F]{3})\b/g,
                            '<font face="' + Theme.mono + '">$1</font>')
    }

    readonly property bool ready: App.connectionState === App.Ready
    readonly property var scan: App.moduleScan
    property string expandedId: ""

    /// The module's own warning lamp (ISO 2575), or "" when it has none.
    function telltaleOf(category) {
        return category === "engine" ? "engine" : category === "abs" ? "abs" : category === "airbag" ? "airbag"
             : category === "tpms" ? "tpms" : category === "steering" ? "steering" : ""
    }
    /// Telltale colour: airbag lamps are red, the others amber (ISO 2575).
    function telltaleColor(category) { return category === "airbag" ? Theme.red : Theme.amber }

    // When a scan ends, open the first module (other than engine/transmission,
    // which the Trouble codes screen already covers) that reported codes.
    Connections {
        target: App
        function onModuleScanChanged() {
            if (App.moduleScanRunning || page.expandedId.length)
                return
            for (let i = 0; i < page.scan.count; ++i) {
                const idx = page.scan.index(i, 0)
                const category = page.scan.data(idx, Qt.UserRole + 3)
                const codes = page.scan.data(idx, Qt.UserRole + 10)
                if (codes > 0 && category !== "engine" && category !== "transmission") {
                    page.expandedId = page.scan.data(idx, Qt.UserRole + 1)
                    return
                }
            }
        }
    }

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
        spacing: Theme.s4

        PageHeader {
            Layout.fillWidth: true
            title: qsTr("All modules")
            subtitle: qsTr("Trouble codes from every module this brand is known to have: ABS, airbag, body, instrument cluster and more, not just the engine.")
            AppButton {
                visible: App.moduleScanRunning
                text: qsTr("Stop")
                iconName: "x"
                onClicked: App.cancelModuleScan()
            }
            AppButton {
                visible: !App.moduleScanRunning
                variant: "primary"
                iconName: "cpu"
                text: page.scan.count > 0 ? qsTr("Scan again") : qsTr("Scan all modules")
                enabled: !App.scanning
                onClicked: App.scanModules()
            }
        }

        // Which module list is used, and why.
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.s3
            Text {
                text: qsTr("Module list")
                color: Theme.textMuted
                font.family: Theme.sans
                font.pixelSize: Theme.fontBody
            }
            C.ComboBox {
                id: profileBox
                Layout.fillWidth: true
                Layout.maximumWidth: 340
                Layout.minimumWidth: 200
                implicitHeight: 36
                model: App.vehicleProfiles
                textRole: "label"
                valueRole: "value"
                enabled: !App.moduleScanRunning
                currentIndex: Math.max(0, indexOfValue(App.profileId))
                onActivated: App.profileId = currentValue
                activeFocusOnTab: true
                Accessible.name: qsTr("Module list")
                font.family: Theme.sans
                font.pixelSize: Theme.fontBody
                contentItem: Text {
                    leftPadding: 10
                    rightPadding: 28
                    text: profileBox.displayText
                    color: Theme.text
                    font: profileBox.font
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                indicator: Icon {
                    x: profileBox.width - width - 10
                    anchors.verticalCenter: parent.verticalCenter
                    name: "chevron-right"
                    rotation: 90
                    size: 14
                    color: Theme.textMuted
                }
                background: Rectangle {
                    radius: Theme.radius
                    color: Theme.field
                    border.width: profileBox.visualFocus ? 2 : 1
                    border.color: profileBox.visualFocus ? Theme.blue : Theme.fieldBorder
                }
                delegate: C.ItemDelegate {
                    id: option
                    required property var modelData
                    required property int index
                    width: ListView.view.width
                    highlighted: profileBox.highlightedIndex === index
                    contentItem: Text {
                        text: option.modelData.label
                        color: Theme.text
                        font.family: Theme.sans
                        font.pixelSize: Theme.fontBody
                        elide: Text.ElideRight
                    }
                    background: Rectangle { color: option.highlighted ? Theme.hover : "transparent"; radius: Theme.radius - 2 }
                }
                popup.background: Rectangle {
                    radius: Theme.radius
                    color: Theme.surface
                    border.width: 1
                    border.color: Theme.hairline
                }
            }
            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: App.profileChosenByUser ? qsTr("chosen by you") : qsTr("matched from the VIN")
                color: Theme.textFaint
                font.family: Theme.sans
                font.pixelSize: Theme.fontSmall
                elide: Text.ElideRight
            }
            AppButton {
                visible: App.profileNotes.length > 0
                variant: "quiet"
                iconName: "info"
                text: page.showNotes ? qsTr("Hide details") : qsTr("About this list")
                onClicked: page.showNotes = !page.showNotes
            }
        }
        Text {
            Layout.fillWidth: true
            Layout.topMargin: -Theme.s2
            visible: page.showNotes && App.profileNotes.length > 0
            text: page.monoHex(App.profileNotes)
            textFormat: Text.StyledText
            color: Theme.textMuted
            font.family: Theme.sans
            font.pixelSize: Theme.fontSmall
            wrapMode: Text.WordWrap
        }

        // Progress while scanning, summary afterwards.
        ColumnLayout {
            Layout.fillWidth: true
            visible: page.scan.count > 0
            spacing: Theme.s2
            Text {
                Layout.fillWidth: true
                text: App.moduleScanRunning
                      ? qsTr("Asking module %1 of %2…").arg(Math.min(page.scan.doneCount + 1, page.scan.count)).arg(page.scan.count)
                      : qsTr("%1 of %2 modules answered · %3").arg(page.scan.answeredCount).arg(page.scan.count)
                            .arg(page.scan.codeCount === 0 ? qsTr("no trouble codes")
                                 : qsTr("%n code(s)", "", page.scan.codeCount) + " " + qsTr("in %n module(s)", "", page.scan.modulesWithCodes))
                        + (App.moduleScanTime.length && !App.moduleScanRunning ? " · " + qsTr("scanned at %1").arg(App.moduleScanTime) : "")
                color: Theme.textMuted
                font.family: Theme.sans
                font.pixelSize: Theme.fontBody
                font.features: { "tnum": 1 }
                elide: Text.ElideRight
            }
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 3
                radius: 1.5
                color: Theme.hairline
                visible: App.moduleScanRunning
                Rectangle {
                    height: parent.height
                    radius: parent.radius
                    color: Theme.blue
                    width: page.scan.count > 0 ? parent.width * page.scan.doneCount / page.scan.count : 0
                    Behavior on width { NumberAnimation { duration: Theme.normal; easing.type: Easing.OutExpo } }
                }
            }
        }

        // First visit: what a module scan is.
        EmptyState {
            visible: page.scan.count === 0
            Layout.topMargin: Theme.s4
            iconName: "cpu"
            title: qsTr("Scan every module")
            message: qsTr("Generic OBD-II only reaches the engine and transmission. A module scan asks each module directly, so codes behind the ABS, airbag or body warning lights show up too. It takes 5–15 seconds; the ignition must be ON and the engine may run. Live data pauses while it runs.")
        }

        Item { Layout.fillHeight: true; visible: page.scan.count === 0 } // keep the empty state at the top

        ListView {
            id: list
            visible: page.scan.count > 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: page.scan
            spacing: 0
            boundsBehavior: Flickable.StopAtBounds
            C.ScrollBar.vertical: C.ScrollBar {}

            delegate: Item {
                id: row
                required property int index
                required property string moduleId
                required property string name
                required property string category
                required property string address
                required property string state
                required property string protocol
                required property bool verified
                required property var dtcs
                required property int codeCount
                required property string partNumber
                required property string softwareVersion
                required property string error

                readonly property bool expandable: state === "answered"
                readonly property bool expanded: expandable && page.expandedId === moduleId

                width: ListView.view.width
                implicitHeight: header.height + (expanded ? detail.implicitHeight + Theme.s4 : 0)

                Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hairline }

                C.ItemDelegate {
                    id: header
                    width: parent.width
                    height: Math.max(56, implicitContentHeight + 20)   // grows with a wrapped error reason
                    hoverEnabled: row.expandable
                    activeFocusOnTab: true
                    enabled: row.expandable
                    onClicked: page.expandedId = row.expanded ? "" : row.moduleId
                    Accessible.name: row.name + ", " + statusText.text
                    background: Rectangle {
                        radius: Theme.radius
                        color: header.hovered ? Theme.hover : "transparent"
                        border.width: header.visualFocus ? 2 : 0
                        border.color: Theme.blue
                    }
                    contentItem: RowLayout {
                        spacing: Theme.s4

                        // State lamp.
                        Item {
                            Layout.preferredWidth: 20
                            Layout.preferredHeight: 20
                            Icon {
                                anchors.centerIn: parent
                                visible: row.state === "scanning"
                                name: "refresh-cw"
                                size: 16
                                color: Theme.blue
                                RotationAnimator on rotation {
                                    running: row.state === "scanning" && !Theme.reduceMotion
                                    from: 0; to: 360; duration: 1100; loops: Animation.Infinite
                                }
                            }
                            Icon {
                                anchors.centerIn: parent
                                visible: row.state !== "scanning"
                                size: 18
                                name: row.state === "obdonly" ? "circle-check"
                                    : row.state === "answered" ? (row.codeCount > 0 ? (page.telltaleOf(row.category) || "triangle-alert") : "circle-check")
                                    : row.state === "error" ? "circle-help"
                                    : "circle-dashed"
                                color: row.state === "obdonly" ? Theme.textFaint
                                     : row.state === "answered" ? (row.codeCount > 0 ? (page.telltaleOf(row.category) ? page.telltaleColor(row.category) : Theme.red) : Theme.green)
                                     : row.state === "error" ? Theme.amber
                                     : Theme.unlit
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                            spacing: 1
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.s2
                                Text {
                                    text: row.name
                                    color: row.state === "absent" || row.state === "waiting" ? Theme.textMuted : Theme.text
                                    font.family: Theme.sans
                                    font.pixelSize: Theme.fontBody
                                    font.weight: Font.DemiBold
                                }
                                Text {
                                    visible: !row.verified
                                    text: qsTr("unconfirmed address")
                                    color: Theme.textFaint
                                    font.family: Theme.sans
                                    font.pixelSize: Theme.fontSmall
                                }
                                Item { Layout.fillWidth: true }
                            }
                            Text {
                                Layout.fillWidth: true
                                text: row.address + (row.protocol.length ? " · " + row.protocol : "")
                                color: Theme.textFaint
                                font.family: Theme.mono
                                font.pixelSize: Theme.fontSmall
                            }
                            // Why a present module's codes couldn't be read.
                            Text {
                                Layout.fillWidth: true
                                visible: row.state === "error" && row.error.length > 0
                                text: row.error
                                color: Theme.amberInk
                                font.family: Theme.sans
                                font.pixelSize: Theme.fontSmall
                                wrapMode: Text.WordWrap
                            }
                        }
                        Text {
                            id: statusText
                            Layout.preferredWidth: 190
                            Layout.alignment: Qt.AlignVCenter
                            horizontalAlignment: Text.AlignRight
                            elide: Text.ElideRight
                            text: row.state === "waiting" ? qsTr("Waiting")
                                : row.state === "scanning" ? qsTr("Asking…")
                                : row.state === "absent" ? qsTr("Not found")
                                : row.state === "error" ? qsTr("Couldn't read codes")
                                : row.state === "obdonly" ? qsTr("Generic OBD-II only")
                                : row.codeCount === 0 ? qsTr("No codes")
                                : qsTr("%n code(s)", "", row.codeCount)
                            color: row.state === "answered" && row.codeCount > 0 ? Theme.redInk
                                 : row.state === "error" ? Theme.amberInk
                                 : Theme.textMuted
                            font.family: Theme.sans
                            font.pixelSize: Theme.fontBody
                            font.weight: row.state === "answered" && row.codeCount > 0 ? Font.DemiBold : Font.Normal
                        }
                        Icon {
                            name: "chevron-right"
                            size: 16
                            color: Theme.textFaint
                            opacity: row.expandable ? 1 : 0   // keep the column; the icon tint has no alpha
                            rotation: row.expanded ? 90 : 0
                            Behavior on rotation { NumberAnimation { duration: Theme.fast; easing.type: Easing.OutExpo } }
                        }
                    }
                }

                // Details: codes, identification, clearing.
                ColumnLayout {
                    id: detail
                    visible: row.expanded
                    anchors.top: header.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 20 + Theme.s4 + Theme.s3
                    anchors.rightMargin: Theme.s3
                    spacing: Theme.s3

                    Text {
                        visible: row.codeCount === 0
                        text: qsTr("This module reports no trouble codes.")
                        color: Theme.textMuted
                        font.family: Theme.sans
                        font.pixelSize: Theme.fontBody
                    }

                    Repeater {
                        model: row.dtcs
                        delegate: ColumnLayout {
                            id: code
                            required property var modelData
                            readonly property string key: row.moduleId + "/" + modelData.code
                            readonly property bool open: page.openCode === key
                            readonly property var info: { App.language; return open ? App.explainCode(modelData.code, App.moduleScanProfile, row.category) : ({}) }
                            Layout.fillWidth: true
                            spacing: Theme.s2

                            // The code line opens its plain-language explanation.
                            C.ItemDelegate {
                                id: codeButton
                                Layout.fillWidth: true
                                implicitHeight: codeLine.implicitHeight + Theme.s2 * 2
                                hoverEnabled: true
                                activeFocusOnTab: true
                                leftPadding: 0      // codes line up with the part-number line below
                                rightPadding: Theme.s2
                                onClicked: page.openCode = code.open ? "" : code.key
                                Accessible.name: code.modelData.displayCode + " " + code.modelData.title
                                background: Rectangle {
                                    radius: Theme.radius
                                    color: codeButton.hovered ? Theme.hover : "transparent"
                                    border.width: codeButton.visualFocus ? 2 : 0
                                    border.color: Theme.blue
                                }
                                contentItem: RowLayout {
                                    id: codeLine
                                    spacing: Theme.s4
                                    Text {
                                        Layout.preferredWidth: 104
                                        Layout.alignment: Qt.AlignTop
                                        text: code.modelData.displayCode
                                        color: Theme.text
                                        font.family: Theme.mono
                                        font.pixelSize: Theme.fontEmphasis
                                        font.weight: Font.DemiBold
                                    }
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: Theme.s1
                                        Text {
                                            Layout.fillWidth: true
                                            text: code.modelData.title
                                            color: Theme.text
                                            font.family: Theme.sans
                                            font.pixelSize: Theme.fontBody
                                            wrapMode: Text.WordWrap
                                        }
                                        Flow {
                                            Layout.fillWidth: true
                                            spacing: Theme.s2
                                            KindChip { kind: code.modelData.kind; label: code.modelData.kindLabel }
                                            Chip { visible: code.modelData.activeNow; text: qsTr("Present now"); dotColor: Theme.red }
                                            Chip {
                                                visible: code.modelData.warningLamp
                                                text: qsTr("Warning light requested")
                                                iconName: page.telltaleOf(row.category) || "triangle-alert"
                                                iconColor: page.telltaleColor(row.category)
                                            }
                                        }
                                    }
                                    Icon {
                                        Layout.alignment: Qt.AlignTop
                                        name: "chevron-right"
                                        size: 14
                                        color: Theme.textFaint
                                        rotation: code.open ? 90 : 0
                                    }
                                }
                            }

                            // Explanation, unfolded in place.
                            ColumnLayout {
                                visible: code.open
                                Layout.fillWidth: true
                                Layout.leftMargin: 104 + Theme.s4   // under the title, not the code
                                Layout.maximumWidth: 720            // keep a readable line length
                                spacing: Theme.s2
                                Text {
                                    Layout.fillWidth: true
                                    text: code.info.known ? code.info.description
                                        : qsTr("This code isn't in Obdaryn's descriptions for this brand yet. Search for it together with the make and model, or ask a workshop.")
                                    color: code.info.known ? Theme.text : Theme.textMuted
                                    font.family: Theme.sans
                                    font.pixelSize: Theme.fontBody
                                    lineHeight: 1.4
                                    wrapMode: Text.WordWrap
                                }
                                Repeater {
                                    model: code.open && code.info.known ? (code.info.causes || []).slice(0, 3) : []
                                    delegate: Text {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        text: (modelData.likelihood === "high" ? qsTr("Likely") : modelData.likelihood === "medium" ? qsTr("Possible") : qsTr("Rare"))
                                              + " · " + modelData.text
                                        color: Theme.textMuted
                                        font.family: Theme.sans
                                        font.pixelSize: Theme.fontSmall
                                        wrapMode: Text.WordWrap
                                    }
                                }
                                DescriptionSource {
                                    Layout.fillWidth: true
                                    Layout.topMargin: Theme.s2   // a note about the text, not another cause
                                    visible: code.open && code.info.known === true
                                    info: code.info
                                }
                            }
                        }
                    }

                    // Identification and the raw status byte, for power users.
                    Text {
                        Layout.fillWidth: true
                        visible: text.length > 0
                        textFormat: Text.StyledText
                        text: page.monoHex([row.partNumber.length ? qsTr("Part number %1").arg(row.partNumber) : "",
                                            row.softwareVersion.length ? qsTr("Software %1").arg(row.softwareVersion) : "",
                                            row.dtcs.length ? qsTr("Status byte %1").arg(row.dtcs.map(d => d.status).join(", ")) : ""]
                                           .filter(s => s.length).join(" · "))
                        color: Theme.textMuted
                        font.family: Theme.sans
                        font.pixelSize: Theme.fontSmall
                        wrapMode: Text.WordWrap
                    }

                    Row {
                        visible: row.codeCount > 0
                        spacing: Theme.s2
                        AppButton {
                            variant: "danger"
                            iconName: "eraser"
                            text: qsTr("Clear this module's codes…")
                            enabled: !App.moduleScanRunning && !App.scanning
                            // Engine and transmission codes are emission codes: use the
                            // OBD-II clear, whose dialog explains readiness and freeze frames.
                            onClicked: row.category === "engine" || row.category === "transmission"
                                       ? page.obdClearRequested()
                                       : page.clearRequested(row.index, row.name)
                        }
                    }
                }
            }

            footer: Text {
                width: ListView.view.width
                visible: !App.moduleScanRunning && page.scan.count > page.scan.answeredCount
                topPadding: Theme.s4
                bottomPadding: Theme.s4
                text: qsTr("\"Not found\" means nothing answered at that address. The module may not be fitted to this car, may use another address on this model, or may sit on a bus the OBD socket doesn't reach.")
                color: Theme.textFaint
                font.family: Theme.sans
                font.pixelSize: Theme.fontSmall
                wrapMode: Text.WordWrap
            }
        }
    }
}
