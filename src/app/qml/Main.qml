// SPDX-License-Identifier: GPL-3.0-or-later
//
// The application window: sidebar + the current screen.
//
// Screens (index → file):
//   0 Overview (doubles as the connect screen)   4 Live data
//   1 Trouble codes                              5 Vehicle info
//   2 Readiness                                  6 Adapter log
//   3 Freeze frame                               7 Settings
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import Obdaryn

C.ApplicationWindow {
    id: window
    width: 1320
    height: 860
    minimumWidth: 1040
    minimumHeight: 680
    visible: true
    title: "Obdaryn"
    color: Theme.window

    property int page: 0

    // The ignition "bulb check": when the car first answers, every telltale
    // lights briefly and the live traces sweep in, then the real state shows.
    property bool bulbCheck: false
    Timer { id: bulbTimer; interval: 1200; onTriggered: window.bulbCheck = false }
    Connections {
        target: App
        function onConnectionChanged() {
            if (App.connectionState === App.Ready && !Theme.reduceMotion) {
                window.bulbCheck = true
                bulbTimer.restart()
            }
        }
        function onClearFinished(success, message) { toast.show(success, message) }
    }

    // Shared by the Overview and Trouble codes screens.
    function requestClear() { clearDialog.open() }

    // Keyboard: ⌘1…⌘7 switch screens, ⌘, opens Settings, ⌘R scans again.
    Shortcut { sequence: "Ctrl+1"; onActivated: window.page = 0 }
    Shortcut { sequence: "Ctrl+2"; onActivated: window.page = 1 }
    Shortcut { sequence: "Ctrl+3"; onActivated: window.page = 2 }
    Shortcut { sequence: "Ctrl+4"; onActivated: window.page = 3 }
    Shortcut { sequence: "Ctrl+5"; onActivated: window.page = 4 }
    Shortcut { sequence: "Ctrl+6"; onActivated: window.page = 5 }
    Shortcut { sequence: "Ctrl+7"; onActivated: window.page = 6 }
    Shortcut { sequences: [StandardKey.Preferences]; onActivated: window.page = 7 }
    Shortcut { sequences: [StandardKey.Refresh, "Ctrl+R"]; enabled: App.connectionState === App.Ready; onActivated: App.runFullScan() }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Sidebar {
            Layout.fillHeight: true
            currentPage: window.page
            onNavigate: page => window.page = page
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: window.page

            OverviewPage {
                bulbCheck: window.bulbCheck
                onOpenPage: p => window.page = p
                onOpenCode: code => { codesPage.selectedCode = code; window.page = 1 }
            }
            CodesPage { id: codesPage; onOpenPage: p => window.page = p; onClearRequested: window.requestClear() }
            ReadinessPage { bulbCheck: window.bulbCheck; onOpenPage: p => window.page = p }
            FreezeFramePage { onOpenPage: p => window.page = p }
            LivePage { bulbCheck: window.bulbCheck; onOpenPage: p => window.page = p }
            VehiclePage { onOpenPage: p => window.page = p }
            LogPage {}
            SettingsPage {}
        }
    }

    ClearCodesDialog {
        id: clearDialog
        onConfirmed: App.clearCodes()
    }

    Toast {
        id: toast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: Theme.sidebarWidth / 2
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.s5
    }
}
