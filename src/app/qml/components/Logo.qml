// SPDX-License-Identifier: GPL-3.0-or-later
//
// The Obdaryn logo mark: the engine telltale lit amber on its graphite tile.
// It is the one place the amber engine appears outside of state, and always
// inside the tile, so it never reads as a lit check-engine lamp.
import QtQuick
import Obdaryn

Item {
    id: logo
    property int size: 24
    implicitWidth: size
    implicitHeight: size
    Accessible.role: Accessible.Graphic
    Accessible.name: "Obdaryn"

    Image {
        anchors.fill: parent
        source: "qrc:/brand/logo.svg"
        sourceSize: Qt.size(logo.size * 2, logo.size * 2) // crisp on Retina
        smooth: true
        mipmap: true
    }
    // On the dark sidebar the graphite tile needs an edge to stay a tile.
    Rectangle {
        anchors.fill: parent
        radius: logo.size * 0.225
        color: "transparent"
        border.width: 1
        border.color: Theme.hairline
        visible: Theme.dark
    }
}
