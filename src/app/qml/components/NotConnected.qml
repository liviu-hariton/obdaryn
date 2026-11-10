// SPDX-License-Identifier: GPL-3.0-or-later
//
// Shown by data screens while no car is connected.
import QtQuick
import Obdaryn

EmptyState {
    id: notConnected
    signal connectRequested()
    iconName: "plug-zap"
    title: qsTr("Connect to a car to see this")
    message: qsTr("Obdaryn reads this information from the car while the adapter is connected and the ignition is ON.")
    AppButton {
        variant: "primary"
        text: qsTr("Go to connection")
        onClicked: notConnected.connectRequested()
    }
}
