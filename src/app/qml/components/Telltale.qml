// SPDX-License-Identifier: GPL-3.0-or-later
//
// An instrument-cluster style warning lamp: the icon is drawn in its telltale
// colour when lit and in a dim "unlit" grey when off. During the ignition bulb
// check (Main.bulbCheck) every telltale lights, exactly like a real cluster's
// self-test at key-on.
import QtQuick
import Obdaryn

Item {
    id: lamp
    property string iconName: "engine"
    property bool lit: false
    property color litColor: Theme.amber
    property int size: 28
    property bool bulbCheck: false

    readonly property bool shownLit: lit || bulbCheck

    implicitWidth: size
    implicitHeight: size

    Icon {
        anchors.centerIn: parent
        name: lamp.iconName
        size: lamp.size
        color: lamp.shownLit ? lamp.litColor : Theme.unlit
        Behavior on color { ColorAnimation { duration: Theme.normal; easing.type: Easing.OutExpo } }
    }
}
