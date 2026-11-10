// SPDX-License-Identifier: GPL-3.0-or-later
//
// "Where does this explanation come from?" under a code's description.
// Generic codes: the obdex database. Brand codes: Obdaryn's brand pack, with how
// sure the meaning is and links to the public sources (official ones marked).
// Always guidance: the user confirms with a workshop before replacing parts.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import Obdaryn

ColumnLayout {
    id: src
    property var info: ({})
    readonly property bool brand: !!info.source && info.source !== "generic"
    readonly property var refs: info.references || []

    spacing: Theme.s1

    /// Short, distinguishable name of a source: NHTSA documents by their id
    /// ("NHTSA MC-11036632"), everything else by its site.
    function linkLabel(url) {
        const m = /^https?:\/\/(?:www\.)?([^\/]+)(.*)$/.exec(url)
        if (!m)
            return url
        if (m[1] === "static.nhtsa.gov") {
            const file = m[2].split("/").pop().replace(/\.pdf$/i, "")
            return "NHTSA " + file.split("-").slice(0, 2).join("-")
        }
        return m[1]
    }

    Text {
        Layout.fillWidth: true
        text: (src.brand
               ? qsTr("Description: %1, written from public sources. %2")
                     .arg(src.info.sourceLabel)
                     .arg(src.info.confidence === "official"
                          ? qsTr("The meaning is confirmed by an official %1 document.").arg(src.info.brandName)
                          : qsTr("The meaning is confirmed by at least two independent sources."))
               : qsTr("Description: obdex database (CC0)."))
              + " " + (src.info.language === "en" && App.language !== "en" ? qsTr("Available in English for now.") + " " : "")
              + qsTr("Guidance only; confirm with a workshop before replacing parts.")
        color: Theme.textFaint
        font.family: Theme.sans
        font.pixelSize: Theme.fontSmall
        wrapMode: Text.WordWrap
    }

    // Source links: keyboard reachable, official ones marked. The label sits
    // beside the Flow so wrapped links line up with the first one.
    RowLayout {
        Layout.fillWidth: true
        visible: src.brand && src.refs.length > 0
        spacing: Theme.s2

        Text {
            Layout.alignment: Qt.AlignTop
            text: qsTr("Sources:")
            color: Theme.textFaint
            font.family: Theme.sans
            font.pixelSize: Theme.fontSmall
            height: 20
            verticalAlignment: Text.AlignVCenter
        }
        Flow {
            Layout.fillWidth: true
            spacing: Theme.s2
            Repeater {
                model: src.refs.slice(0, 5)
                delegate: C.AbstractButton {
                    id: link
                    required property var modelData
                    required property int index
                    objectName: "sourceLink" + index
                    activeFocusOnTab: true
                    hoverEnabled: true
                    implicitHeight: 20
                    implicitWidth: label.implicitWidth + 4
                    onClicked: Qt.openUrlExternally(modelData.url)
                    Keys.onReturnPressed: clicked()
                    Keys.onEnterPressed: clicked()
                    Accessible.role: Accessible.Link
                    Accessible.name: (modelData.official ? qsTr("Official document") + ": " : "") + modelData.url
                    C.ToolTip.visible: hovered || visualFocus
                    C.ToolTip.delay: 400
                    C.ToolTip.text: modelData.url + "\n" + qsTr("Opens in your browser; needs internet, which this computer doesn't have while joined to the adapter's Wi-Fi.")
                    background: Item {
                        // Focus ring outside the link, like every other control (DESIGN.md).
                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: -3
                            radius: 5
                            color: "transparent"
                            border.width: 2
                            border.color: Theme.blue
                            visible: link.visualFocus
                        }
                    }
                    contentItem: Text {
                        id: label
                        leftPadding: 2
                        rightPadding: 2
                        text: src.linkLabel(link.modelData.url) + (link.modelData.official ? " (" + qsTr("official") + ")" : "")
                        color: Theme.blueInk
                        font.family: Theme.sans
                        font.pixelSize: Theme.fontSmall
                        font.underline: true
                        verticalAlignment: Text.AlignVCenter
                    }
                    HoverHandler { cursorShape: Qt.PointingHandCursor }
                }
            }
        }
    }
}
