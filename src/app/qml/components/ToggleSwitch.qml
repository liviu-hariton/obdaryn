// SPDX-License-Identifier: GPL-3.0-or-later
//
// On/off switch with a label and an optional explanation.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import Obdaryn

C.AbstractButton {
    id: sw
    property string description: ""
    checkable: true
    focusPolicy: Qt.StrongFocus
    activeFocusOnTab: true
    hoverEnabled: true
    implicitHeight: Math.max(36, col.implicitHeight)
    Accessible.role: Accessible.CheckBox
    Accessible.name: text
    Accessible.checked: checked

    background: Item {}
    contentItem: RowLayout {
        spacing: Theme.s4
        ColumnLayout {
            id: col
            Layout.fillWidth: true
            spacing: 2
            Text {
                Layout.fillWidth: true
                text: sw.text
                color: Theme.text
                font.family: Theme.sans
                font.pixelSize: Theme.fontBody
                font.weight: Font.Medium
                wrapMode: Text.WordWrap
            }
            Text {
                Layout.fillWidth: true
                visible: sw.description.length > 0
                text: sw.description
                color: Theme.textMuted
                font.family: Theme.sans
                font.pixelSize: Theme.fontSmall
                wrapMode: Text.WordWrap
            }
        }
        Rectangle {
            Layout.alignment: Qt.AlignVCenter
            implicitWidth: 38
            implicitHeight: 22
            radius: 11
            color: sw.checked ? Theme.blueFill : Theme.unlit
            border.width: sw.visualFocus ? 2 : 0
            border.color: Theme.text
            Behavior on color { ColorAnimation { duration: Theme.fast } }
            Rectangle {
                width: 18; height: 18; radius: 9
                y: 2
                x: sw.checked ? parent.width - width - 2 : 2
                color: "#FFFFFF"
                Behavior on x { NumberAnimation { duration: Theme.fast; easing.type: Easing.OutExpo } }
            }
        }
    }
}
