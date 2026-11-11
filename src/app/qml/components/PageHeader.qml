// SPDX-License-Identifier: GPL-3.0-or-later
//
// Title row of a screen: title, optional one-line subtitle, actions on the right.
import QtQuick
import QtQuick.Layouts
import Obdaryn

RowLayout {
    id: header
    property alias title: titleText.text
    property alias subtitle: subtitleText.text
    property alias subtitleFont: subtitleText.font
    default property alias actions: actionRow.data

    spacing: Theme.s4

    ColumnLayout {
        Layout.fillWidth: true
        // Take what the actions leave: a long subtitle wraps instead of
        // pushing the actions off the right edge (Romanian runs longer).
        Layout.minimumWidth: 0
        Layout.preferredWidth: 0
        spacing: 4
        Text {
            id: titleText
            Layout.fillWidth: true
            color: Theme.text
            font.family: Theme.sans
            font.pixelSize: Theme.fontTitle
            font.weight: Font.Bold
            font.letterSpacing: -0.4
            elide: Text.ElideRight
            Accessible.role: Accessible.Heading
        }
        Text {
            id: subtitleText
            Layout.fillWidth: true
            visible: text.length > 0
            color: Theme.textMuted
            font.family: Theme.sans
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
        }
    }
    Row {
        id: actionRow
        spacing: Theme.s2
        Layout.alignment: Qt.AlignTop
    }
}
