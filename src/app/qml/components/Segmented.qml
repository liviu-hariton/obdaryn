// SPDX-License-Identifier: GPL-3.0-or-later
//
// A segmented choice (e.g. English | Română). model: [{ value, label }].
import QtQuick
import QtQuick.Controls.Basic as C
import Obdaryn

Rectangle {
    id: seg
    property var model: []
    property string value
    signal picked(string value)

    implicitHeight: 32
    implicitWidth: row.implicitWidth + 4
    radius: Theme.radius
    color: Theme.hover
    Accessible.role: Accessible.PageTabList

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 2
        Repeater {
            model: seg.model
            delegate: C.AbstractButton {
                id: opt
                required property var modelData
                readonly property bool on: modelData.value === seg.value
                implicitHeight: 28
                implicitWidth: label.implicitWidth + 28
                focusPolicy: Qt.StrongFocus
                activeFocusOnTab: true
                checkable: false
                Accessible.role: Accessible.PageTab
                Accessible.name: modelData.label
                Accessible.checked: on
                onClicked: seg.picked(modelData.value)
                background: Rectangle {
                    radius: Theme.radius - 1
                    color: opt.on ? Theme.surface : "transparent"
                    border.width: opt.visualFocus ? 2 : (opt.on ? 1 : 0)
                    border.color: opt.visualFocus ? Theme.blue : Theme.hairline
                }
                contentItem: Text {
                    id: label
                    text: opt.modelData.label
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: opt.on ? Theme.text : Theme.textMuted
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontBody
                    font.weight: opt.on ? Font.DemiBold : Font.Medium
                }
            }
        }
    }
}
