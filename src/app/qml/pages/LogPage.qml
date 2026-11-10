// SPDX-License-Identifier: GPL-3.0-or-later
//
// Adapter log: every command Obdaryn sent and every answer, for troubleshooting.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import Obdaryn

Item {
    id: page

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.s6
        anchors.topMargin: Theme.s5
        spacing: Theme.s4

        PageHeader {
            Layout.fillWidth: true
            title: qsTr("Adapter log")
            subtitle: qsTr("Every command sent to the adapter (→), every answer (←) and every blocked or failed command (!). Copy it into a bug report if something looks wrong.")
            AppButton { variant: "quiet"; iconName: "copy"; text: qsTr("Copy log"); onClicked: App.copyToClipboard(App.trafficLog.toText()) }
            AppButton { variant: "quiet"; iconName: "x"; text: qsTr("Clear log"); onClicked: App.trafficLog.clear() }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Theme.radiusLarge
            color: Theme.surface
            border.width: 1
            border.color: Theme.hairline

            EmptyState {
                visible: App.trafficLog.count === 0
                anchors.centerIn: parent
                width: Math.min(420, parent.width - 48)
                iconName: "terminal"
                title: qsTr("Nothing yet")
                message: qsTr("Traffic appears here as soon as Obdaryn connects to an adapter.")
            }

            ListView {
                id: log
                anchors.fill: parent
                anchors.margins: Theme.s3
                clip: true
                model: App.trafficLog
                boundsBehavior: Flickable.StopAtBounds
                C.ScrollBar.vertical: C.ScrollBar {}
                // Follow new lines unless the user scrolled up to read.
                property bool follow: true
                onMovementEnded: follow = atYEnd
                onCountChanged: if (follow) Qt.callLater(positionViewAtEnd)

                delegate: RowLayout {
                    id: line
                    required property string time
                    required property string direction
                    required property string text
                    width: ListView.view.width
                    spacing: Theme.s3
                    Text {
                        text: line.time
                        color: Theme.textFaint
                        font.family: Theme.mono
                        font.pixelSize: Theme.fontSmall
                        Layout.alignment: Qt.AlignTop
                    }
                    Text {
                        Layout.preferredWidth: 14
                        Layout.alignment: Qt.AlignTop
                        text: line.direction === ">" ? "→" : line.direction === "<" ? "←" : "!"
                        color: line.direction === ">" ? Theme.blue : line.direction === "<" ? Theme.textMuted : Theme.amberInk
                        font.family: Theme.mono
                        font.pixelSize: Theme.fontSmall
                        font.weight: Font.Bold
                    }
                    Text {
                        Layout.fillWidth: true
                        text: line.text
                        color: line.direction === "!" ? Theme.amberInk : Theme.text
                        font.family: Theme.mono
                        font.pixelSize: Theme.fontSmall
                        wrapMode: Text.WrapAnywhere
                    }
                }
            }
        }
    }
}
