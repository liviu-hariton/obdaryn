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
//  10 Charts (several values over time)        11 Help
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
    Timer { id: bulbTimer; interval: Theme.bulbCheck; onTriggered: window.bulbCheck = false }
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

    // Replaying a recording from the menu (the connect screen has its own button).
    function openRecording() { recordingDialog.open() }
    FileDialog {
        id: recordingDialog
        title: qsTr("Open a recording")
        nameFilters: [qsTr("Obdaryn recordings (*.obdrec)"), qsTr("All files (*)")]
        onAccepted: { App.replayRecording(selectedFile); window.page = 0 }
    }
    // About Obdaryn: its own small window, centred over this one.
    AboutWindow { id: aboutWindow }
    function showAbout() {
        aboutWindow.x = window.x + (window.width - aboutWindow.width) / 2
        aboutWindow.y = window.y + Math.max(60, (window.height - aboutWindow.height) / 3)
        aboutWindow.show()
        aboutWindow.raise()
        aboutWindow.requestActivate()
    }
    /// ⌘W closes the window in front: the About window if it is, otherwise this one.
    function closeFrontWindow() {
        if (aboutWindow.visible && aboutWindow.active)
            aboutWindow.close()
        else
            window.close()
    }

    /// Help, scrolled to one of its sections (1 = Supported cars).
    function showHelpSection(index) {
        window.page = 11
        Qt.callLater(() => helpPage.showSection(index))
    }

    // ---- The macOS menu bar ----------------------------------------------------
    // Native, so it lives in the top bar like any Mac app's. It carries the
    // keyboard shortcuts there; on other systems the shortcuts below do.
    readonly property bool nativeMenus: Qt.platform.os === "osx"
    Component.onCompleted: {
        if (!nativeMenus)
            return
        const bar = menuComponent.createObject(window, { app: window, window: window })
        // Developer check: "--print-menus" prints the menu bar's structure.
        if (Qt.application.arguments.indexOf("--print-menus") >= 0) {
            for (const m of bar.menus) {
                const items = []
                for (const i of m.items)
                    items.push(i.separator ? "—" : i.text + (i.shortcut ? " [" + i.shortcut + "]" : "") + (i.role ? " (role " + i.role + ")" : "") + (i.enabled ? "" : " (off)"))
                console.log("menu", m.title + ":", items.join(" | "))
            }
        }
    }
    Component { id: menuComponent; AppMenuBar {} }

    // Keyboard: ⌘1…⌘9 follow the sidebar order, ⌘, opens Settings, ⌘R scans again, ⌘S saves the report.
    readonly property var sidebarOrder: [0, 1, 8, 2, 3, 4, 10, 9, 5, 6]
    Repeater {
        model: window.nativeMenus ? 0 : Math.min(9, window.sidebarOrder.length) // ⌘1–⌘9
        delegate: Item {
            required property int index
            Shortcut { sequence: "Ctrl+" + (index + 1); onActivated: window.page = window.sidebarOrder[index] }
        }
    }
    Shortcut { enabled: !window.nativeMenus; sequences: [StandardKey.Preferences]; onActivated: window.page = 7 }
    Shortcut { enabled: !window.nativeMenus; sequences: [StandardKey.HelpContents]; onActivated: window.page = 11 }
    Shortcut { sequences: [StandardKey.Refresh, "Ctrl+R"]; enabled: !window.nativeMenus && App.connectionState === App.Ready; onActivated: App.runFullScan() }
    Shortcut { sequences: [StandardKey.Save]; enabled: !window.nativeMenus && App.connectionState === App.Ready && !App.scanning; onActivated: window.requestReport() }

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
            ChartsPage { current: window.page === 10; onOpenPage: p => window.page = p }
            HelpPage { id: helpPage; onOpenPage: p => window.page = p }
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
