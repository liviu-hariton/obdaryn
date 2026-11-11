// SPDX-License-Identifier: GPL-3.0-or-later
//
// A styled drop-down list. Model entries are {label, value}; `value` is the
// selected one (match by value, so the list can change under it).
import QtQuick
import QtQuick.Controls.Basic as C
import Obdaryn

C.ComboBox {
    id: box
    property string value: ""
    signal picked(string value)

    implicitHeight: 36
    textRole: "label"
    valueRole: "value"
    currentIndex: Math.max(0, indexOfValue(value))
    onActivated: box.picked(currentValue)
    activeFocusOnTab: true
    font.family: Theme.sans
    font.pixelSize: Theme.fontBody
    contentItem: Text {
        leftPadding: 10
        rightPadding: 28
        text: box.displayText
        textFormat: Text.PlainText // labels may come from files (recording names)
        color: Theme.text
        font: box.font
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    indicator: Icon {
        x: box.width - width - 10
        anchors.verticalCenter: parent.verticalCenter
        name: "chevron-right"
        rotation: 90
        size: 14
        color: Theme.textMuted
    }
    background: Rectangle {
        radius: Theme.radius
        color: Theme.field
        border.width: box.visualFocus ? 2 : 1
        border.color: box.visualFocus ? Theme.blue : Theme.fieldBorder
    }
    delegate: C.ItemDelegate {
        id: option
        required property var modelData
        required property int index
        width: ListView.view.width
        highlighted: box.highlightedIndex === index
        contentItem: Text {
            text: option.modelData.label
            textFormat: Text.PlainText
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
