// SPDX-License-Identifier: GPL-3.0-or-later
//
// The one button. Variants:
//   "primary"   main action of a screen (filled blue)
//   "secondary" everything else (outlined)
//   "danger"    destructive / consequential actions (red outline, red fill when emphasized)
//   "quiet"     toolbar and inline actions (no border until hovered)
import QtQuick
import QtQuick.Controls.Basic as C
import Obdaryn

C.AbstractButton {
    id: control
    property string variant: "secondary"
    property string iconName: ""
    property bool emphasized: false   // danger + emphasized = filled red (inside the confirm dialog)
    property bool busy: false
    property bool iconTrailing: false   // e.g. "All live data ›"

    readonly property bool filled: variant === "primary" || (variant === "danger" && emphasized)
    readonly property color fillColor: variant === "primary" ? Theme.blueFill : Theme.redFill
    readonly property color inkColor: filled ? "#FFFFFF"
        : variant === "danger" ? Theme.redInk
        : Theme.text

    implicitHeight: 34
    implicitWidth: Math.max(implicitHeight, row.implicitWidth + (text.length ? 28 : 16))
    focusPolicy: Qt.StrongFocus
    activeFocusOnTab: true   // macOS only tabs between text fields unless asked
    hoverEnabled: true
    opacity: enabled ? 1 : 0.45
    Accessible.role: Accessible.Button
    Accessible.name: text

    background: Rectangle {
        radius: Theme.radius
        color: control.filled
               ? (control.down ? Qt.darker(control.fillColor, 1.15)
                  : control.hovered ? (control.variant === "primary" ? Theme.blueFillHover : Qt.lighter(control.fillColor, 1.08))
                  : control.fillColor)
               : (control.down ? Theme.selected : control.hovered ? Theme.hover : "transparent")
        border.width: control.filled || (control.variant === "quiet" && !control.hovered) ? 0 : 1
        border.color: control.variant === "danger" ? Theme.red : Theme.fieldBorder
        Behavior on color { ColorAnimation { duration: Theme.fast } }

        // Keyboard focus ring, drawn outside the button.
        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            radius: Theme.radius + 3
            color: "transparent"
            border.width: 2
            border.color: Theme.blue
            visible: control.visualFocus
        }
    }

    contentItem: Item {
        implicitWidth: row.implicitWidth
        implicitHeight: row.implicitHeight
        Row {
            id: row
            anchors.centerIn: parent
            spacing: 8
            C.BusyIndicator {
                visible: control.busy
                running: control.busy
                width: 16; height: 16
                anchors.verticalCenter: parent.verticalCenter
            }
            Icon {
                visible: control.iconName.length > 0 && !control.busy && !control.iconTrailing
                name: control.iconName
                color: control.inkColor
                size: 16
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                visible: control.text.length > 0
                text: control.text
                color: control.inkColor
                font.family: Theme.sans
                font.pixelSize: Theme.fontBody
                font.weight: Font.DemiBold
                anchors.verticalCenter: parent.verticalCenter
            }
            Icon {
                visible: control.iconName.length > 0 && control.iconTrailing
                name: control.iconName
                color: control.inkColor
                size: 16
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}
