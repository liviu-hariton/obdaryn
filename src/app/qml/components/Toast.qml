// SPDX-License-Identifier: GPL-3.0-or-later
//
// A short message at the bottom of the window (e.g. "Codes cleared").
import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import Obdaryn

Rectangle {
    id: toast
    property bool success: true
    property string message: ""

    function show(ok, text) {
        success = ok
        message = text
        visible = true
        hideTimer.restart()
    }

    visible: false
    width: Math.min(560, parent ? parent.width - 2 * Theme.s5 : 560)
    implicitHeight: row.implicitHeight + 2 * Theme.s3
    radius: Theme.radiusLarge
    color: Theme.surface
    border.width: 1
    border.color: Theme.hairline
    Accessible.role: Accessible.AlertMessage
    Accessible.name: message

    // A soft, offset shadow so the toast reads above the content.
    layer.enabled: visible
    layer.effect: MultiEffect {
        shadowEnabled: true
        shadowBlur: 0.7
        shadowVerticalOffset: 6
        shadowColor: Theme.dark ? "#99000000" : "#33000000"
    }

    Timer { id: hideTimer; interval: 7000; onTriggered: toast.visible = false }

    RowLayout {
        id: row
        anchors.fill: parent
        anchors.margins: Theme.s3
        spacing: Theme.s3
        Icon {
            name: toast.success ? "circle-check" : "triangle-alert"
            color: toast.success ? Theme.green : Theme.amber
            size: 18
            Layout.alignment: Qt.AlignTop
        }
        Text {
            Layout.fillWidth: true
            text: toast.message
            color: Theme.text
            font.family: Theme.sans
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
        }
        AppButton {
            variant: "quiet"
            iconName: "x"
            implicitHeight: 26
            Accessible.name: qsTr("Dismiss")
            onClicked: toast.visible = false
        }
    }
}
