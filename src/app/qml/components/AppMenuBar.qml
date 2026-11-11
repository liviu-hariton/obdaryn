// SPDX-License-Identifier: GPL-3.0-or-later
//
// The macOS menu bar: the app's commands where a Mac user looks for them, with
// their keyboard shortcuts listed. Native (Qt.labs.platform), so items with a
// role (About, Settings, Quit) go to the Obdaryn menu in any language.
// Main.qml creates it on macOS only; elsewhere the window's own shortcuts serve.
import QtQuick
import Qt.labs.platform as P
import Obdaryn

P.MenuBar {
    id: bar
    /// The application window (Main.qml): pages, dialogs and the focused item.
    required property var app

    readonly property bool ready: App.connectionState === App.Ready
    readonly property bool connected: App.connectionState !== App.Disconnected
                                      && App.connectionState !== App.Failed

    // Edit commands go to whatever text field has the keyboard focus.
    function focused(method) {
        const item = bar.app.activeFocusItem
        return item && typeof item[method] === "function" ? item : null
    }

    // ---- Obdaryn (the application menu: macOS moves the role items there) ----
    P.Menu {
        title: qsTr("Obdaryn")
        P.MenuItem {
            text: qsTr("About Obdaryn")
            role: P.MenuItem.AboutRole
            onTriggered: bar.app.showAbout()
        }
        P.MenuItem {
            text: qsTr("Settings…")
            role: P.MenuItem.PreferencesRole
            shortcut: StandardKey.Preferences
            onTriggered: bar.app.page = 7
        }
        P.MenuItem {
            text: qsTr("Quit Obdaryn")
            role: P.MenuItem.QuitRole
            shortcut: StandardKey.Quit
            onTriggered: Qt.quit()
        }
    }

    // ---- File: connecting, recordings, the report -------------------------------
    P.Menu {
        title: qsTr("File")
        P.MenuItem {
            text: qsTr("Connect to Adapter…")
            enabled: !bar.connected
            onTriggered: bar.app.page = 0 // the connect screen
        }
        P.MenuItem {
            text: qsTr("Try the Demo Car")
            enabled: !bar.connected
            onTriggered: { App.connectDemo(); bar.app.page = 0 }
        }
        P.MenuItem {
            text: qsTr("Replay a Recording…")
            enabled: !bar.connected
            shortcut: StandardKey.Open
            onTriggered: bar.app.openRecording()
        }
        P.MenuSeparator {}
        P.MenuItem {
            text: App.recording ? qsTr("Stop Recording") : qsTr("Start Recording")
            enabled: bar.ready && !App.replayMode
            shortcut: "Ctrl+Shift+R"
            onTriggered: App.recording ? App.stopRecording() : App.startRecording()
        }
        P.MenuItem {
            text: qsTr("Save Report…")
            enabled: bar.ready && !App.scanning
            shortcut: StandardKey.Save
            onTriggered: bar.app.requestReport()
        }
        P.MenuSeparator {}
        P.MenuItem {
            text: qsTr("Disconnect")
            enabled: bar.connected
            onTriggered: App.disconnectAdapter()
        }
        P.MenuItem {
            text: qsTr("Close Window")
            shortcut: StandardKey.Close
            onTriggered: bar.app.closeFrontWindow()
        }
    }

    // ---- Edit: for the text fields --------------------------------------------
    P.Menu {
        title: qsTr("Edit")
        P.MenuItem { text: qsTr("Undo"); shortcut: StandardKey.Undo; onTriggered: { const i = bar.focused("undo"); if (i) i.undo() } }
        P.MenuItem { text: qsTr("Redo"); shortcut: StandardKey.Redo; onTriggered: { const i = bar.focused("redo"); if (i) i.redo() } }
        P.MenuSeparator {}
        P.MenuItem { text: qsTr("Cut"); shortcut: StandardKey.Cut; onTriggered: { const i = bar.focused("cut"); if (i) i.cut() } }
        P.MenuItem { text: qsTr("Copy"); shortcut: StandardKey.Copy; onTriggered: { const i = bar.focused("copy"); if (i) i.copy() } }
        P.MenuItem { text: qsTr("Paste"); shortcut: StandardKey.Paste; onTriggered: { const i = bar.focused("paste"); if (i) i.paste() } }
        P.MenuItem { text: qsTr("Select All"); shortcut: StandardKey.SelectAll; onTriggered: { const i = bar.focused("selectAll"); if (i) i.selectAll() } }
    }

    // ---- Car: what Obdaryn asks the car --------------------------------------------
    P.Menu {
        title: qsTr("Car")
        P.MenuItem {
            text: qsTr("Scan Again")
            enabled: bar.ready && !App.scanning
            shortcut: StandardKey.Refresh
            onTriggered: App.runFullScan()
        }
        P.MenuItem {
            text: qsTr("Scan All Modules")
            enabled: bar.ready && !App.scanning && !App.moduleScanRunning
            onTriggered: { bar.app.page = 8; App.scanModules() }
        }
        P.MenuItem {
            text: qsTr("Read Self-Test Results")
            enabled: bar.ready && !App.scanning && !App.readingMonitorTests
            onTriggered: { bar.app.page = 2; App.readMonitorTests() }
        }
        P.MenuSeparator {}
        P.MenuItem {
            // Opens the same confirmation as the button: consequences first, engine off.
            text: qsTr("Clear Trouble Codes…")
            enabled: bar.ready && !App.replayMode && !App.scanning && App.dtcs.count > 0
            onTriggered: { bar.app.page = 1; bar.app.requestClear() }
        }
    }

    // ---- View: the screens, in sidebar order (⌘1–⌘9) ------------------------------
    P.Menu {
        title: qsTr("View")
        Instantiator {
            model: bar.app.sidebarOrder
            delegate: P.MenuItem {
                required property int modelData
                required property int index
                text: [qsTr("Overview"), qsTr("Trouble Codes"), qsTr("Readiness"), qsTr("Freeze Frame"),
                       qsTr("Live Data"), qsTr("Vehicle Info"), qsTr("Adapter Log"), qsTr("Settings"),
                       qsTr("All Modules"), qsTr("Trip"), qsTr("Charts"), qsTr("Help")][modelData]
                shortcut: index < 9 ? "Ctrl+" + (index + 1) : ""
                checkable: true
                checked: bar.app.page === modelData
                onTriggered: bar.app.page = modelData
            }
            onObjectAdded: (index, object) => viewMenu.insertItem(index, object)
            onObjectRemoved: (index, object) => viewMenu.removeItem(object)
        }
        id: viewMenu
    }

    // ---- Window --------------------------------------------------------------
    P.Menu {
        title: qsTr("Window")
        P.MenuItem {
            text: qsTr("Minimize")
            shortcut: "Ctrl+M"
            onTriggered: bar.app.showMinimized()
        }
        P.MenuItem {
            text: qsTr("Zoom")
            onTriggered: bar.app.visibility === Window.Maximized ? bar.app.showNormal() : bar.app.showMaximized()
        }
    }

    // ---- Help -----------------------------------------------------------------
    P.Menu {
        title: qsTr("Help")
        P.MenuItem {
            text: qsTr("Obdaryn Help")
            shortcut: StandardKey.HelpContents
            onTriggered: bar.app.page = 11
        }
        P.MenuItem {
            text: qsTr("Supported Cars")
            onTriggered: bar.app.showHelpSection(1)
        }
        P.MenuSeparator {}
        P.MenuItem {
            text: qsTr("Obdaryn on GitHub")
            onTriggered: Qt.openUrlExternally("https://github.com/liviu-hariton/obdaryn")
        }
        P.MenuItem {
            text: qsTr("Report a Problem")
            onTriggered: Qt.openUrlExternally("https://github.com/liviu-hariton/obdaryn/issues")
        }
    }
}
