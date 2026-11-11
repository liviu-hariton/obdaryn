// SPDX-License-Identifier: GPL-3.0-or-later
//
// Shown by data screens while no car is connected.
import QtQuick
import Obdaryn

EmptyState {
    id: notConnected
    signal connectRequested()
    // A lost link being restored, or a car that went quiet, say so: they are
    // not the same as never having connected.
    readonly property bool reconnecting: App.connectionState === App.Reconnecting
    readonly property bool carSilent: App.connectionState === App.NoVehicle
    iconName: reconnecting ? "refresh-cw" : "plug-zap"
    title: reconnecting ? qsTr("Reconnecting to the adapter…")
         : carSilent ? qsTr("The car isn't answering")
         : qsTr("Connect to a car to see this")
    message: reconnecting || carSilent ? App.statusMessage
           : qsTr("Obdaryn reads this information from the car while the adapter is connected and the ignition is ON.")
    AppButton {
        variant: notConnected.reconnecting || notConnected.carSilent ? "secondary" : "primary"
        text: notConnected.reconnecting || notConnected.carSilent ? qsTr("Disconnect") : qsTr("Go to connection")
        onClicked: notConnected.reconnecting || notConnected.carSilent ? App.disconnectAdapter() : notConnected.connectRequested()
    }
}
