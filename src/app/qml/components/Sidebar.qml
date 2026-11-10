// SPDX-License-Identifier: GPL-3.0-or-later
//
// Left navigation: connection status, the screens grouped by job, settings.
import QtQuick
import QtQuick.Layouts
import Obdaryn

Rectangle {
    id: sidebar
    property int currentPage: 0
    signal navigate(int page)

    readonly property bool ready: App.connectionState === App.Ready
    readonly property bool busy: App.connectionState === App.Connecting || App.connectionState === App.Initializing

    color: Theme.sidebar
    implicitWidth: Theme.sidebarWidth

    Rectangle { // divider to the content
        anchors.right: parent.right
        width: 1
        height: parent.height
        color: Theme.hairline
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.s3
        anchors.topMargin: Theme.s5
        spacing: 2

        Text {
            text: "Obdaryn"
            color: Theme.text
            font.family: Theme.sans
            font.pixelSize: 17
            font.weight: Font.Bold
            font.letterSpacing: -0.2
            Layout.leftMargin: 10
            Layout.bottomMargin: Theme.s4
        }

        // ---- Connection status --------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            Layout.bottomMargin: Theme.s4
            implicitHeight: statusCol.implicitHeight + 2 * Theme.s3
            radius: Theme.radius
            color: Theme.surface
            border.width: 1
            border.color: Theme.hairline

            ColumnLayout {
                id: statusCol
                anchors.fill: parent
                anchors.margins: Theme.s3
                spacing: 2
                RowLayout {
                    spacing: 8
                    Rectangle {
                        width: 8; height: 8; radius: 4
                        color: sidebar.ready ? Theme.green
                             : sidebar.busy ? Theme.amber
                             : App.connectionState === App.NoVehicle || App.connectionState === App.Failed ? Theme.red
                             : Theme.unlit
                    }
                    Text {
                        Layout.fillWidth: true
                        text: sidebar.ready ? qsTr("Connected")
                            : sidebar.busy ? qsTr("Connecting…")
                            : App.connectionState === App.NoVehicle ? qsTr("Car not answering")
                            : App.connectionState === App.Failed ? qsTr("Connection failed")
                            : qsTr("Not connected")
                        color: Theme.text
                        font.family: Theme.sans
                        font.pixelSize: Theme.fontBody
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }
                }
                Text {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    visible: text.length > 0
                    text: App.demoMode && App.connectionState !== App.Disconnected ? qsTr("Demo car")
                        : sidebar.ready ? App.transportDescription : ""
                    color: Theme.textMuted
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontSmall
                    elide: Text.ElideRight
                }
                AppButton {
                    visible: App.connectionState !== App.Disconnected
                    Layout.topMargin: Theme.s2
                    Layout.fillWidth: true
                    implicitHeight: 28
                    variant: "secondary"
                    iconName: "unplug"
                    text: qsTr("Disconnect")
                    onClicked: App.disconnectAdapter()
                }
            }
        }

        // ---- Navigation -------------------------------------------------------
        component GroupLabel: Text {
            color: Theme.textFaint
            font.family: Theme.sans
            font.pixelSize: Theme.fontSmall
            font.weight: Font.DemiBold
            Layout.leftMargin: 10
            Layout.topMargin: Theme.s3
            Layout.bottomMargin: 4
        }

        GroupLabel { text: qsTr("Diagnose") }
        NavItem {
            Layout.fillWidth: true
            text: qsTr("Overview"); iconName: "layout-dashboard"
            current: sidebar.currentPage === 0; onClicked: sidebar.navigate(0)
        }
        NavItem {
            Layout.fillWidth: true
            text: qsTr("Trouble codes"); iconName: "triangle-alert"
            badge: App.dtcs.activeCodeCount
            Accessible.description: qsTr("%n codes stored or pending", "", App.dtcs.activeCodeCount)
            badgeColor: App.dtcs.storedCount > 0 ? Theme.redFill : Theme.amber
            current: sidebar.currentPage === 1; onClicked: sidebar.navigate(1)
        }
        NavItem {
            Layout.fillWidth: true
            text: qsTr("Readiness"); iconName: "list-checks"
            current: sidebar.currentPage === 2; onClicked: sidebar.navigate(2)
        }
        NavItem {
            Layout.fillWidth: true
            text: qsTr("Freeze frame"); iconName: "snowflake"
            current: sidebar.currentPage === 3; onClicked: sidebar.navigate(3)
        }

        GroupLabel { text: qsTr("Monitor") }
        NavItem {
            Layout.fillWidth: true
            text: qsTr("Live data"); iconName: "activity"
            current: sidebar.currentPage === 4; onClicked: sidebar.navigate(4)
        }

        GroupLabel { text: qsTr("Vehicle") }
        NavItem {
            Layout.fillWidth: true
            text: qsTr("Vehicle info"); iconName: "car-front"
            current: sidebar.currentPage === 5; onClicked: sidebar.navigate(5)
        }
        GroupLabel { text: qsTr("Adapter") }
        NavItem {
            Layout.fillWidth: true
            text: qsTr("Adapter log"); iconName: "terminal"
            current: sidebar.currentPage === 6; onClicked: sidebar.navigate(6)
        }

        Item { Layout.fillHeight: true }

        NavItem {
            Layout.fillWidth: true
            text: qsTr("Settings"); iconName: "settings"
            current: sidebar.currentPage === 7; onClicked: sidebar.navigate(7)
        }

        // The safety promise, always visible.
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.s3
            Layout.leftMargin: 10
            Layout.rightMargin: 6
            spacing: 8
            Icon { name: "shield-check"; size: 16; color: Theme.green; Layout.alignment: Qt.AlignTop }
            Text {
                Layout.fillWidth: true
                text: qsTr("Read-only. Obdaryn can't change anything in your car.")
                wrapMode: Text.WordWrap
                color: Theme.textMuted
                font.family: Theme.sans
                font.pixelSize: Theme.fontSmall
                lineHeight: 1.25
            }
        }
    }
}
