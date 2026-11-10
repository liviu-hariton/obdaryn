// SPDX-License-Identifier: GPL-3.0-or-later
//
// One row in the sidebar.
import QtQuick
import QtQuick.Controls.Basic as C
import Obdaryn

C.AbstractButton {
    id: item
    property string iconName
    property bool current: false
    property int badge: 0          // e.g. number of stored codes
    property color badgeColor: Theme.red
    property string shortcutHint: ""

    implicitHeight: 32
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    activeFocusOnTab: true
    Accessible.role: Accessible.PageTab
    Accessible.name: text

    background: Rectangle {
        radius: Theme.radius
        color: item.current ? Theme.selected : item.hovered ? Theme.hover : "transparent"
        Rectangle {
            anchors.fill: parent
            anchors.margins: -2
            radius: Theme.radius + 2
            color: "transparent"
            border.width: 2
            border.color: Theme.blue
            visible: item.visualFocus
        }
    }

    contentItem: Row {
        spacing: 10
        leftPadding: 10
        rightPadding: 10
        Icon {
            name: item.iconName
            size: 16
            color: item.current ? Theme.text : Theme.textMuted
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            width: item.width - 16 - 10 - 20 - (badgeBox.visible ? badgeBox.width + 8 : 0)
            text: item.text
            elide: Text.ElideRight
            color: item.current ? Theme.text : Theme.textMuted
            font.family: Theme.sans
            font.pixelSize: Theme.fontBody
            font.weight: item.current ? Font.DemiBold : Font.Medium
            anchors.verticalCenter: parent.verticalCenter
        }
        Rectangle {
            id: badgeBox
            visible: item.badge > 0
            height: 18
            width: Math.max(18, badgeText.implicitWidth + 10)
            radius: 9
            color: item.badgeColor
            anchors.verticalCenter: parent.verticalCenter
            Text {
                id: badgeText
                anchors.centerIn: parent
                text: item.badge
                color: "#FFFFFF"
                font.family: Theme.sans
                font.pixelSize: Theme.fontCaption
                font.weight: Font.Bold
                font.features: { "tnum": 1 }
            }
        }
    }
}
