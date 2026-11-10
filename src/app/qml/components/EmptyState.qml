// SPDX-License-Identifier: GPL-3.0-or-later
//
// What a screen shows when there is nothing to list: what it means and what to do.
import QtQuick
import QtQuick.Layouts
import Obdaryn

ColumnLayout {
    id: empty
    property string iconName: "info"
    property color iconColor: Theme.textFaint
    property string title
    property string message
    default property alias actions: actionRow.data

    spacing: Theme.s2

    Icon { name: empty.iconName; color: empty.iconColor; size: 24; Layout.bottomMargin: Theme.s1 }
    Text {
        Layout.fillWidth: true
        text: empty.title
        color: Theme.text
        font.family: Theme.sans
        font.pixelSize: Theme.fontEmphasis
        font.weight: Font.DemiBold
        wrapMode: Text.WordWrap
    }
    Text {
        Layout.fillWidth: true
        Layout.maximumWidth: 560
        visible: text.length > 0
        text: empty.message
        color: Theme.textMuted
        font.family: Theme.sans
        font.pixelSize: Theme.fontBody
        lineHeight: 1.3
        wrapMode: Text.WordWrap
    }
    Row { id: actionRow; spacing: Theme.s2; Layout.topMargin: Theme.s2 }
}
