// SPDX-License-Identifier: GPL-3.0-or-later
//
// A stroke icon from qrc:/icons, tinted with any colour. Decorative by default;
// set Accessible.name when the icon is the only label of a control.
import QtQuick
import Obdaryn

Image {
    id: icon
    property string name: ""
    property color color: Theme.text
    property int size: 18

    width: size
    height: size
    sourceSize: Qt.size(size * Screen.devicePixelRatio, size * Screen.devicePixelRatio)
    source: name.length ? "image://icon/" + name + "/" + Theme.hex(color) : ""
    fillMode: Image.PreserveAspectFit
    smooth: true
    asynchronous: false
    Accessible.ignored: true
}
