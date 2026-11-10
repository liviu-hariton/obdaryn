// SPDX-License-Identifier: GPL-3.0-or-later
//
// Labelled single-line text field.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import Obdaryn

ColumnLayout {
    id: input
    property alias label: labelText.text
    property alias text: field.text
    property alias placeholder: field.placeholderText
    property alias validator: field.validator
    property alias inputMethodHints: field.inputMethodHints
    property bool mono: false
    signal accepted()

    spacing: 6

    Text {
        id: labelText
        color: Theme.textMuted
        font.family: Theme.sans
        font.pixelSize: Theme.fontSmall
        font.weight: Font.Medium
    }
    C.TextField {
        id: field
        Layout.fillWidth: true
        implicitHeight: 36
        leftPadding: 10
        rightPadding: 10
        color: Theme.text
        placeholderTextColor: Theme.textFaint
        selectionColor: Theme.blueFill
        selectedTextColor: "#FFFFFF"
        font.family: input.mono ? Theme.mono : Theme.sans
        font.pixelSize: Theme.fontBody
        Accessible.name: input.label
        onAccepted: input.accepted()
        background: Rectangle {
            radius: Theme.radius
            color: Theme.field
            border.width: field.activeFocus ? 2 : 1
            border.color: field.activeFocus ? Theme.blue : Theme.fieldBorder
        }
    }
}
