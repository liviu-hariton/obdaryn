// SPDX-License-Identifier: GPL-3.0-or-later
//
// The application window: sidebar + the current screen.
//
// Screens (index → file):
//   0 Overview (doubles as the connect screen)   4 Live data
//   1 Trouble codes                              5 Vehicle info
//   2 Readiness                                  6 Adapter log
//   3 Freeze frame                               7 Settings
//   8 All modules (full-vehicle scan)          9 Trip (distance and fuel)
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Dialogs
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
        function onModuleClearFinished(success, message) { toast.show(success, message) }
        function onNotice(success, message) { toast.show(success, message) }
    }

    // Shared by the Overview and Trouble codes screens.
    function requestClear() { clearDialog.moduleRow = -1; clearDialog.moduleName = ""; clearDialog.open() }
    function requestModuleClear(row, name) { clearDialog.moduleRow = row; clearDialog.moduleName = name; clearDialog.open() }

    // The shareable report: a native save dialog, PDF by default, or a web page.
    function requestReport() {
        reportDialog.selectedFile = App.suggestedReportUrl()
        reportDialog.open()
    }
    FileDialog {
        id: reportDialog
        title: qsTr("Save report")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("PDF document (*.pdf)"), qsTr("Web page (*.html)")]
        defaultSuffix: selectedNameFilter.index === 1 ? "html" : "pdf"
        onAccepted: App.saveReport(selectedFile)
    }

    // Keyboard: ⌘1…⌘9 follow the sidebar order, ⌘, opens Settings, ⌘R scans again, ⌘S saves the report.
    readonly property var sidebarOrder: [0, 1, 8, 2, 3, 4, 9, 5, 6]
    Repeater {
        model: window.sidebarOrder.length
        delegate: Item {
            required property int index
            Shortcut { sequence: "Ctrl+" + (index + 1); onActivated: window.page = window.sidebarOrder[index] }
        }
    }
    Shortcut { sequences: [StandardKey.Preferences]; onActivated: window.page = 7 }
    Shortcut { sequences: [StandardKey.Refresh, "Ctrl+R"]; enabled: App.connectionState === App.Ready; onActivated: App.runFullScan() }
    Shortcut { sequences: [StandardKey.Save]; enabled: App.connectionState === App.Ready && !App.scanning; onActivated: window.requestReport() }

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
                onOpenModules: window.page = 8
                onSaveReportRequested: window.requestReport()
            }
            CodesPage { id: codesPage; objectName: "codesPage"; onOpenPage: p => window.page = p; onClearRequested: window.requestClear() }
            ReadinessPage { bulbCheck: window.bulbCheck; onOpenPage: p => window.page = p }
            FreezeFramePage { onOpenPage: p => window.page = p }
            LivePage { bulbCheck: window.bulbCheck; onOpenPage: p => window.page = p }
            VehiclePage { onOpenPage: p => window.page = p }
            LogPage {}
            SettingsPage {}
            ModulesPage {
                objectName: "modulesPage"
                onOpenPage: p => window.page = p
                onClearRequested: (row, name) => window.requestModuleClear(row, name)
                onObdClearRequested: window.requestClear()
            }
            TripPage { onOpenPage: p => window.page = p }
        }
    }

    ClearCodesDialog {
        id: clearDialog
        onConfirmed: moduleRow >= 0 ? App.clearModuleCodes(moduleRow) : App.clearCodes()
    }

    Toast {
        id: toast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: Theme.sidebarWidth / 2
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.s5
    }
}
