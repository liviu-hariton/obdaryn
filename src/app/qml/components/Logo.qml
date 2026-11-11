// SPDX-License-Identifier: GPL-3.0-or-later
//
// The Obdaryn logo mark: the OBD-II port on a graphite tile, its pin 16
// (battery power) lit amber. Below 32px the simplified drawing (outline and
// the lit pin) stays legible; the dark variants carry a hairline edge so the
// tile holds on dark grounds. Brand amber appears only here, as that pin.
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
        source: "qrc:/brand/logo" + (logo.size < 32 ? "-small" : "") + (Theme.dark ? "-dark" : "") + ".svg"
        sourceSize: Qt.size(logo.size * 2, logo.size * 2) // crisp on Retina
        smooth: true
        mipmap: true
    }
}
