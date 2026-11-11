// SPDX-License-Identifier: GPL-3.0-or-later
//
// Left navigation: connection status, the screens grouped by job, settings.
import QtQuick
import QtQuick.Controls.Basic as C
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

    // Short windows (the 680 px minimum) get tighter gaps between groups.
    readonly property bool compact: height < 760

    // Scrolls only when the content can't fit (a short window while recording
    // or replaying); otherwise Settings and the read-only note stay at the bottom.
    Flickable {
        id: flick
        anchors.fill: parent
        contentHeight: column.height + column.y + Theme.s3
        boundsBehavior: Flickable.StopAtBounds
        clip: contentHeight > height
        C.ScrollBar.vertical: C.ScrollBar { policy: flick.contentHeight > flick.height ? C.ScrollBar.AsNeeded : C.ScrollBar.AlwaysOff }

    ColumnLayout {
        id: column
        x: Theme.s3
        y: sidebar.compact ? Theme.s4 : Theme.s5
        width: flick.width - 2 * Theme.s3
        height: Math.max(flick.height - y - Theme.s3, implicitHeight)
        spacing: 2

        Text {
            text: "Obdaryn"
            color: Theme.text
            font.family: Theme.sans
            font.pixelSize: 17
            font.weight: Font.Bold
            font.letterSpacing: -0.2
            Layout.leftMargin: 10
            Layout.bottomMargin: sidebar.compact ? Theme.s3 : Theme.s4
        }

        // ---- Connection status --------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            Layout.bottomMargin: sidebar.compact ? Theme.s2 : Theme.s4
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
                        // Blue for a replay: talking, but not to a car.
                        color: sidebar.ready ? (App.replayMode ? Theme.blue : Theme.green)
                             : sidebar.busy ? Theme.amber
                             : App.connectionState === App.NoVehicle || App.connectionState === App.Failed ? Theme.red
                             : Theme.unlit
                    }
                    Text {
                        Layout.fillWidth: true
                        text: sidebar.ready ? (App.replayMode ? qsTr("Replaying a recording") : qsTr("Connected"))
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
                    textFormat: Text.PlainText // a recording's car name comes from the file
                    color: Theme.textMuted
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontSmall
                    // A replay names the car and the date: wrap rather than cut the date.
                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                }
                // Where the replay is; it loops at the end.
                Text {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    visible: sidebar.ready && App.replayMode
                    text: App.replayPosition
                    color: Theme.textMuted
                    font.family: Theme.mono
                    font.pixelSize: Theme.fontSmall
                    font.features: { "tnum": 1 }
                }

                // ---- Recording ------------------------------------------------
                // A red dot and the running time, like any recorder: the one
                // place in the sidebar that is red while nothing is wrong.
                RowLayout {
                    visible: App.recording
                    Layout.topMargin: Theme.s2
                    spacing: 8
                    Rectangle {
                        id: recDot
                        width: 8; height: 8; radius: 4
                        color: Theme.red
                        SequentialAnimation on opacity {
                            running: App.recording && !Theme.reduceMotion
                            loops: Animation.Infinite
                            NumberAnimation { to: 0.35; duration: 700; easing.type: Easing.InOutSine }
                            NumberAnimation { to: 1; duration: 700; easing.type: Easing.InOutSine }
                            onStopped: recDot.opacity = 1
                        }
                    }
                    Text {
                        text: qsTr("Recording")
                        color: Theme.redInk
                        font.family: Theme.sans
                        font.pixelSize: Theme.fontSmall
                        font.weight: Font.DemiBold
                    }
                    Text {
                        text: App.recordingTime
                        color: Theme.textMuted
                        font.family: Theme.mono
                        font.pixelSize: Theme.fontSmall
                        font.features: { "tnum": 1 }
                    }
                }
                // Connected to a car: record and disconnect share one row, so the
                // box stays short enough for the minimum window height.
                RowLayout {
                    visible: sidebar.ready && !App.replayMode
                    Layout.topMargin: Theme.s2
                    Layout.fillWidth: true
                    spacing: Theme.s2
                    AppButton {
                        Layout.fillWidth: true
                        implicitHeight: 28
                        variant: "secondary"
                        iconName: App.recording ? "square" : "circle-dot"
                        text: App.recording ? qsTr("Stop") : qsTr("Record")
                        Accessible.name: App.recording ? qsTr("Stop recording") : qsTr("Record session")
                        Accessible.description: App.recording ? qsTr("Saves the recording") : qsTr("Saves everything the car says, to replay later without it")
                        C.ToolTip.visible: hovered
                        C.ToolTip.delay: 600
                        C.ToolTip.text: Accessible.description
                        onClicked: App.recording ? App.stopRecording() : App.startRecording()
                    }
                    AppButton {
                        implicitHeight: 28
                        variant: "secondary"
                        iconName: "unplug"
                        Accessible.name: qsTr("Disconnect")
                        C.ToolTip.visible: hovered
                        C.ToolTip.delay: 600
                        C.ToolTip.text: qsTr("Disconnect")
                        onClicked: App.disconnectAdapter()
                    }
                }
                // Connecting, failed, or replaying: one full-width button.
                AppButton {
                    visible: App.connectionState !== App.Disconnected && !(sidebar.ready && !App.replayMode)
                    Layout.topMargin: Theme.s2
                    Layout.fillWidth: true
                    implicitHeight: 28
                    variant: "secondary"
                    iconName: "unplug"
                    text: App.replayMode ? qsTr("Stop replay") : qsTr("Disconnect")
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
            Layout.topMargin: sidebar.compact ? Theme.s2 : Theme.s3
            Layout.bottomMargin: sidebar.compact ? 2 : 4
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
            text: qsTr("All modules"); iconName: "cpu"
            badge: App.moduleScan.extraCodeCount
            badgeColor: App.moduleScan.extraStoredCount > 0 ? Theme.redFill : Theme.amber
            Accessible.description: qsTr("%n codes in modules other than the engine and transmission", "", App.moduleScan.extraCodeCount)
            current: sidebar.currentPage === 8; onClicked: sidebar.navigate(8)
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
        NavItem {
            Layout.fillWidth: true
            text: qsTr("Trip"); iconName: "fuel"
            current: sidebar.currentPage === 9; onClicked: sidebar.navigate(9)
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
            Layout.topMargin: sidebar.compact ? Theme.s2 : Theme.s3
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
}
