// SPDX-License-Identifier: GPL-3.0-or-later
//
// Heading of a section inside a screen, with optional trailing detail/actions.
import QtQuick
import QtQuick.Layouts
import Obdaryn

RowLayout {
    property alias text: label.text
    property alias detail: detailText.text
    default property alias actions: actionRow.data
    spacing: Theme.s3

    Text {
        id: label
        color: Theme.text
        font.family: Theme.sans
        font.pixelSize: Theme.fontSection
        font.weight: Font.DemiBold
        font.letterSpacing: -0.2
        Accessible.role: Accessible.Heading
    }
    Text {
        id: detailText
        visible: text.length > 0
        color: Theme.textFaint
        font.family: Theme.sans
        font.pixelSize: Theme.fontSmall
        font.features: { "tnum": 1 }
        Layout.alignment: Qt.AlignBaseline
    }
    Item { Layout.fillWidth: true }
    Row { id: actionRow; spacing: Theme.s2 }
}
