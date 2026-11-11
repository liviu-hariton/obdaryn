// SPDX-License-Identifier: GPL-3.0-or-later
//
// About Obdaryn: a small window in the macOS manner. The mark, the name, the
// version (selectable, with a button to copy it for a bug report), what the app
// is, the read-only promise, and the licence and credits in small print.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import QtQuick.Window
import Obdaryn

Window {
    id: about
    objectName: "aboutWindow"
    title: qsTr("About Obdaryn")
    // A fixed size, close button only: an About window isn't resized or minimised.
    flags: Qt.Window | Qt.CustomizeWindowHint | Qt.WindowTitleHint | Qt.WindowCloseButtonHint
    readonly property int fixedWidth: 400
    readonly property int fixedHeight: content.implicitHeight + 2 * Theme.s6
    width: fixedWidth
    height: fixedHeight
    minimumWidth: fixedWidth
    maximumWidth: fixedWidth
    minimumHeight: fixedHeight
    maximumHeight: fixedHeight
    color: Theme.window

    readonly property string projectUrl: "https://github.com/liviu-hariton/obdaryn"
    readonly property string versionInfo: qsTr("Version %1").arg(App.appVersion)

    Shortcut { sequences: ["Esc", StandardKey.Close]; onActivated: about.close() }

    ColumnLayout {
        id: content
        x: Theme.s6
        y: Theme.s6
        width: about.fixedWidth - 2 * Theme.s6
        spacing: 0

        Logo { size: 80; Layout.alignment: Qt.AlignHCenter }

        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Theme.s4
            text: "Obdaryn"
            color: Theme.text
            font.family: Theme.sans
            font.pixelSize: 22
            font.weight: Font.Bold
            font.letterSpacing: -0.3
        }
        // Selectable, so the version can be copied into a bug report by hand too.
        TextEdit {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Theme.s1
            readOnly: true
            selectByMouse: true
            text: about.versionInfo
            color: Theme.textMuted
            selectionColor: Theme.blueFill
            selectedTextColor: Theme.onFill
            font.family: Theme.sans
            font.pixelSize: Theme.fontBody
            font.features: { "tnum": 1 }
        }
        TextEdit {
            Layout.alignment: Qt.AlignHCenter
            readOnly: true
            selectByMouse: true
            text: App.buildInfo
            color: Theme.textFaint
            selectionColor: Theme.blueFill
            selectedTextColor: Theme.onFill
            font.family: Theme.sans
            font.pixelSize: Theme.fontSmall
            font.features: { "tnum": 1 }
        }

        Text {
            Layout.fillWidth: true
            Layout.topMargin: Theme.s4
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Free, open-source OBD-II diagnostics.")
            color: Theme.text
            font.family: Theme.sans
            font.pixelSize: Theme.fontBody
            lineHeight: 1.3
            wrapMode: Text.WordWrap
        }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.maximumWidth: content.width
            Layout.topMargin: Theme.s2
            spacing: 6
            Icon { name: "shield-check"; size: 15; color: Theme.green; Layout.alignment: Qt.AlignTop; Layout.topMargin: 1 }
            Text {
                Layout.maximumWidth: content.width - 21
                text: qsTr("Read-only. Obdaryn can't change anything in your car.")
                color: Theme.textMuted
                font.family: Theme.sans
                font.pixelSize: Theme.fontSmall
                wrapMode: Text.WordWrap
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Theme.s5
            spacing: Theme.s2
            AppButton {
                text: qsTr("Project page")
                onClicked: Qt.openUrlExternally(about.projectUrl)
            }
            AppButton {
                id: copyButton
                variant: "quiet"
                iconName: copied.running ? "check" : "copy"
                text: copied.running ? qsTr("Copied") : qsTr("Copy version info")
                onClicked: {
                    App.copyToClipboard("Obdaryn " + App.appVersion + " · " + App.buildInfo)
                    copied.restart()
                }
                Timer { id: copied; interval: 1600 }
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.topMargin: Theme.s5; implicitHeight: 1; color: Theme.hairline }

        Text {
            id: licence
            Layout.fillWidth: true
            Layout.topMargin: Theme.s4
            horizontalAlignment: Text.AlignHCenter
            textFormat: Text.StyledText
            text: qsTr("Free software under the %1.")
                  .arg("<a href=\"" + about.projectUrl + "/blob/main/LICENSE\">" + qsTr("GNU GPL v3 or later") + "</a>")
            linkColor: Theme.blueInk
            color: Theme.textMuted
            font.family: Theme.sans
            font.pixelSize: Theme.fontSmall
            wrapMode: Text.WordWrap
            onLinkActivated: link => Qt.openUrlExternally(link)
            HoverHandler { cursorShape: licence.hoveredLink.length ? Qt.PointingHandCursor : Qt.ArrowCursor }
        }
        Text {
            Layout.fillWidth: true
            Layout.topMargin: Theme.s2
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Code explanations from obdex (CC0). Manufacturer values from OBDb (CC BY-SA 4.0). Fonts: Inter and JetBrains Mono (SIL Open Font License). Icons: Lucide (ISC). Built with Qt.")
            color: Theme.textFaint
            font.family: Theme.sans
            font.pixelSize: Theme.fontSmall
            lineHeight: 1.3
            wrapMode: Text.WordWrap
        }
    }
}
