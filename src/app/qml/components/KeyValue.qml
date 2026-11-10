// SPDX-License-Identifier: GPL-3.0-or-later
//
// A "label: value" row with a hairline under it, for detail lists.
import QtQuick
import QtQuick.Layouts
import Obdaryn

Item {
    id: kv
    property string label
    property string value
    property string note: ""          // small explanation under the value
    property bool mono: false
    property bool labelMono: false   // e.g. module addresses such as 7E8
    property bool copyable: false
    property int labelWidth: 220

    implicitHeight: Math.max(44, col.implicitHeight + 2 * Theme.s3)
    Layout.fillWidth: true

    RowLayout {
        anchors.fill: parent
        spacing: Theme.s4
        Text {
            Layout.preferredWidth: kv.labelWidth
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: Theme.s3
            text: kv.label
            color: Theme.textMuted
            font.family: kv.labelMono ? Theme.mono : Theme.sans
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
        }
        ColumnLayout {
            id: col
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: Theme.s3
            spacing: 2
            Text {
                Layout.fillWidth: true
                text: kv.value.length ? kv.value : "—"
                color: Theme.text
                font.family: kv.mono ? Theme.mono : Theme.sans
                font.pixelSize: Theme.fontBody
                font.weight: Font.Medium
                font.features: { "tnum": 1 }
                wrapMode: Text.WrapAnywhere
            }
            Text {
                Layout.fillWidth: true
                visible: kv.note.length > 0
                text: kv.note
                color: Theme.textFaint
                font.family: Theme.sans
                font.pixelSize: Theme.fontSmall
                wrapMode: Text.WordWrap
            }
        }
        AppButton {
            visible: kv.copyable && kv.value.length > 0
            variant: "quiet"
            iconName: "copy"
            implicitHeight: 28
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: Theme.s2
            Accessible.name: qsTr("Copy %1").arg(kv.label)
            onClicked: App.copyToClipboard(kv.value)
        }
    }
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairline }
}
