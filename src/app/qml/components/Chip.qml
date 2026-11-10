// SPDX-License-Identifier: GPL-3.0-or-later
//
// A small labelled tag. A coloured dot plus text, so meaning never relies on colour alone.
import QtQuick
import Obdaryn

Rectangle {
    id: chip
    property alias text: label.text
    property color dotColor: "transparent"
    property string iconName: ""
    property color iconColor: Theme.textMuted

    // Never wider than the container (long Romanian labels elide).
    readonly property real maxWidth: parent ? parent.width : 100000
    implicitHeight: 22
    implicitWidth: Math.min(row.implicitWidth + 16, maxWidth)
    radius: 11
    color: "transparent"
    border.width: 1
    border.color: Theme.hairline

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 6
        Rectangle {
            visible: chip.dotColor.a > 0
            width: 7; height: 7; radius: 3.5
            color: chip.dotColor
            anchors.verticalCenter: parent.verticalCenter
        }
        Icon {
            visible: chip.iconName.length > 0
            name: chip.iconName
            size: 13
            color: chip.iconColor
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            id: label
            width: Math.min(implicitWidth, chip.maxWidth - 16 - (chip.dotColor.a > 0 ? 13 : 0) - (chip.iconName.length ? 19 : 0))
            elide: Text.ElideRight
            color: Theme.textMuted
            font.family: Theme.sans
            font.pixelSize: Theme.fontSmall
            font.weight: Font.Medium
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
