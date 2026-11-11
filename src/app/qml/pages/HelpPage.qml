// SPDX-License-Identifier: GPL-3.0-or-later
//
// Help: how to connect, what to do in which car state, what clearing really
// does, the screens, troubleshooting, words, privacy and safety.
//
// A reading page. In a wide window an "On this page" rail stays on the left
// and follows the reading position; in a narrow one the same links sit under
// the title. Prose and label rows share one measure, so the right edge is even.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import Obdaryn

Item {
    id: page
    objectName: "helpPage"
    signal openPage(int p)

    // ---- Measures ------------------------------------------------------------
    readonly property int measure: 620          // prose and rows: about 80 characters
    readonly property int railWidth: 200
    readonly property int railGap: Theme.s6
    /// The rail needs room for itself plus the full measure, with gutters.
    readonly property bool wide: width >= railWidth + railGap + measure + 2 * Theme.s7
    readonly property int columnWidth: Math.min(measure, width - 2 * Theme.s6)
    readonly property int groupWidth: wide ? railWidth + railGap + columnWidth : columnWidth
    readonly property real groupX: Math.max(Theme.s6, (width - groupWidth) / 2)
    readonly property real columnX: wide ? groupX + railWidth + railGap : groupX

    readonly property var sections: [start, states, clearing, screens, trouble, words, safety]
    /// The section being read: the last one whose heading has reached the top area.
    property int current: 0
    function updateCurrent() {
        if (jump.running)
            return // a jump decides when it lands
        const y = scroll.contentItem.contentY + Theme.s7 * 2
        let index = 0
        for (let i = 0; i < sections.length; ++i) {
            if (sections[i].y <= y)
                index = i
        }
        // At the very end, the last section is the one on screen even if it's short.
        if (scroll.contentItem.contentY >= content.height - scroll.height - 2)
            index = sections.length - 1
        current = index
    }

    // ---- Building blocks, so the content below reads like the text it is -----
    component Para: Text {
        Layout.fillWidth: true
        color: Theme.text
        font.family: Theme.sans
        font.pixelSize: Theme.fontBody
        lineHeight: 1.4
        wrapMode: Text.WordWrap
    }
    component Bullets: ColumnLayout {
        id: bullets
        property var items: []
        Layout.fillWidth: true
        spacing: Theme.s2
        Repeater {
            model: bullets.items
            delegate: RowLayout {
                required property string modelData
                Layout.fillWidth: true
                spacing: Theme.s3
                Rectangle { Layout.alignment: Qt.AlignTop; Layout.topMargin: 8; Layout.leftMargin: 2; width: 4; height: 4; radius: 2; color: Theme.textMuted }
                Para { text: modelData }
            }
        }
    }
    /// Numbered steps: the order matters (plug in, ignition, Wi-Fi, connect).
    component Steps: ColumnLayout {
        id: steps
        property var items: []
        Layout.fillWidth: true
        spacing: Theme.s2
        Repeater {
            model: steps.items
            delegate: RowLayout {
                required property string modelData
                required property int index
                Layout.fillWidth: true
                spacing: Theme.s3
                Text {
                    Layout.alignment: Qt.AlignTop
                    Layout.preferredWidth: 14
                    text: index + 1
                    color: Theme.textMuted
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontBody
                    font.weight: Font.DemiBold
                    font.features: { "tnum": 1 }
                    horizontalAlignment: Text.AlignRight
                    lineHeight: 1.4
                }
                Para { text: modelData }
            }
        }
    }
    /// Label on the left, explanation on the right, hairline between rows.
    component Rows: ColumnLayout {
        id: rows
        property var items: []    // [{ label, text }]
        property int labelWidth: 170
        Layout.fillWidth: true
        spacing: 0
        Repeater {
            model: rows.items
            delegate: ColumnLayout {
                required property var modelData
                required property int index
                Layout.fillWidth: true
                spacing: 0
                Rectangle { visible: index > 0; Layout.fillWidth: true; implicitHeight: 1; color: Theme.hairline }
                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: Theme.s3
                    Layout.bottomMargin: Theme.s3
                    spacing: Theme.s4
                    Text {
                        Layout.preferredWidth: rows.labelWidth
                        Layout.maximumWidth: rows.labelWidth
                        Layout.alignment: Qt.AlignTop
                        text: modelData.label
                        color: Theme.text
                        font.family: Theme.sans
                        font.pixelSize: Theme.fontBody
                        font.weight: Font.DemiBold
                        lineHeight: 1.4
                        wrapMode: Text.WordWrap
                    }
                    Para { text: modelData.text; color: Theme.textMuted }
                }
            }
        }
    }
    component Section: ColumnLayout {
        property alias title: heading.text
        Layout.fillWidth: true
        Layout.topMargin: Theme.s7 // generous above, tight below: the heading belongs to what follows
        spacing: Theme.s3
        Text {
            id: heading
            Layout.fillWidth: true
            Layout.bottomMargin: 2
            color: Theme.text
            font.family: Theme.sans
            font.pixelSize: Theme.fontSection
            font.weight: Font.DemiBold
            font.letterSpacing: -0.2
            wrapMode: Text.WordWrap
            Accessible.role: Accessible.Heading
        }
    }
    /// One "On this page" entry: the rail and the narrow link row share it.
    component TocItem: C.AbstractButton {
        id: toc
        property bool current: false
        property bool fill: true
        implicitHeight: Math.max(30, label.implicitHeight + 10)
        implicitWidth: label.implicitWidth + 20
        hoverEnabled: true
        focusPolicy: Qt.StrongFocus
        Accessible.role: Accessible.Link
        Accessible.name: text
        background: Rectangle {
            radius: Theme.radius
            color: toc.current ? Theme.selected : toc.hovered ? Theme.hover : "transparent"
            Rectangle {
                anchors.fill: parent
                anchors.margins: -2
                radius: Theme.radius + 2
                color: "transparent"
                border.width: 2
                border.color: Theme.blue
                visible: toc.visualFocus
            }
        }
        contentItem: Text {
            id: label
            leftPadding: 10
            rightPadding: 10
            text: toc.text
            color: toc.current || toc.hovered ? Theme.text : Theme.textMuted
            font.family: Theme.sans
            font.pixelSize: Theme.fontBody
            font.weight: toc.current ? Font.DemiBold : Font.Medium
            wrapMode: toc.fill ? Text.WordWrap : Text.NoWrap
            verticalAlignment: Text.AlignVCenter
            lineHeight: 1.2
        }
    }
    component TocLabel: Text {
        text: qsTr("On this page")
        color: Theme.textFaint
        font.family: Theme.sans
        font.pixelSize: Theme.fontSmall
        font.weight: Font.DemiBold
        leftPadding: 10
    }

    // Smooth jumps (none under Reduce motion: Theme.normal is 0 then).
    NumberAnimation {
        id: jump
        property int section: -1 // the one clicked: it stays highlighted even if the page can't scroll that far
        target: scroll.contentItem
        property: "contentY"
        duration: Theme.normal
        easing.type: Easing.OutExpo
        onFinished: if (section >= 0) { page.current = section; section = -1 }
    }
    function jumpTo(item) {
        jump.stop()
        jump.section = sections.indexOf(item)
        jump.to = Math.max(0, Math.min(item.y - Theme.s5, content.height - scroll.height))
        jump.start()
    }
    /// For screenshots and tests: jump straight to a section by index.
    function showSection(index) {
        scroll.contentItem.contentY = Math.max(0, Math.min(sections[index].y - Theme.s5, content.height - scroll.height))
    }

    // ---- The rail (wide windows): stays put while the text scrolls ------------
    ColumnLayout {
        visible: page.wide
        x: page.groupX
        y: Theme.s5 + 4
        width: page.railWidth
        spacing: 2
        TocLabel { Layout.bottomMargin: Theme.s1 }
        Repeater {
            model: page.sections
            delegate: TocItem {
                required property var modelData
                required property int index
                Layout.fillWidth: true
                text: modelData.title
                current: index === page.current
                onClicked: page.jumpTo(modelData)
            }
        }
    }

    C.ScrollView {
        id: scroll
        anchors.fill: parent
        contentWidth: availableWidth
        // Keyboard reading: arrows, Page Up / Down, Home / End.
        focus: true
        activeFocusOnTab: true
        Keys.onPressed: e => {
            const f = scroll.contentItem
            const page = scroll.height * 0.9
            const max = Math.max(0, content.height - scroll.height)
            let y = f.contentY
            if (e.key === Qt.Key_Down) y += 40
            else if (e.key === Qt.Key_Up) y -= 40
            else if (e.key === Qt.Key_PageDown || e.key === Qt.Key_Space) y += page
            else if (e.key === Qt.Key_PageUp) y -= page
            else if (e.key === Qt.Key_Home) y = 0
            else if (e.key === Qt.Key_End) y = max
            else return
            f.contentY = Math.max(0, Math.min(max, y))
            e.accepted = true
        }
        Connections {
            target: scroll.contentItem
            function onContentYChanged() { page.updateCurrent() }
        }

        ColumnLayout {
            id: content
            x: page.columnX
            width: page.columnWidth
            spacing: 0

            PageHeader {
                Layout.fillWidth: true
                Layout.topMargin: Theme.s5
                title: qsTr("Help")
                subtitle: qsTr("How to connect, what to do in which state of the car, and what each screen tells you.")
            }

            // Narrow windows: the same links, under the title.
            ColumnLayout {
                visible: !page.wide
                Layout.fillWidth: true
                Layout.topMargin: Theme.s4
                spacing: Theme.s1
                TocLabel {}
                Flow {
                    Layout.fillWidth: true
                    spacing: 2
                    Repeater {
                        model: page.sections
                        delegate: TocItem {
                            required property var modelData
                            fill: false
                            text: modelData.title
                            onClicked: page.jumpTo(modelData)
                        }
                    }
                }
            }

            Section {
                id: start
                title: qsTr("Start here")
                Para { text: qsTr("Obdaryn reads what the car's computers report through an ELM327-compatible Wi-Fi adapter such as the Vgate iCar Pro. It only reads: it can't change anything in the car.") }
                Steps {
                    items: [
                        qsTr("Plug the adapter into the OBD-II socket, under the dashboard on the driver's side."),
                        qsTr("Turn the ignition ON. The engine can stay off; start it for live data with the engine running."),
                        qsTr("Join the adapter's Wi-Fi (usually \"V-LINK\", no password). This computer has no internet while it's connected; that's normal."),
                        qsTr("Press Connect. The first time, macOS asks to let Obdaryn find devices on the local network: allow it."),
                        qsTr("Close other OBD apps first (Car Scanner, Torque): a Wi-Fi adapter takes one connection at a time."),
                    ]
                }
                Para { text: qsTr("No car nearby? Try the demo car, or replay a recording: every screen works with both."); color: Theme.textMuted }
            }

            Section {
                id: states
                title: qsTr("What to do in which car state")
                Rows {
                    items: [
                        { label: qsTr("Ignition ON, engine off"), text: qsTr("Read trouble codes, the freeze frame, readiness and self-test results, vehicle info, and scan all modules. The only state in which codes can be cleared.") },
                        { label: qsTr("Engine running, parked"), text: qsTr("Everything above except clearing, plus meaningful live data: temperatures, fuel trims, charging voltage.") },
                        { label: qsTr("Driving"), text: qsTr("Live data, charts, the trip computer and recording a session. A passenger watches the screen, never the driver.") },
                        { label: qsTr("Not connected"), text: qsTr("Replay or chart recordings, export their data as CSV, and look through saved trips.") },
                    ]
                }
                Para { text: qsTr("With the ignition ON and the engine off, the car drains its battery. Keep such sessions under 20–30 minutes, or start the engine. Obdaryn shows the battery voltage on the Overview."); color: Theme.textMuted }
            }

            Section {
                id: clearing
                title: qsTr("What clearing codes really does")
                Para { text: qsTr("Clearing is the only thing Obdaryn does that changes anything in the car, and only after you confirm with the engine off. It doesn't repair anything:") }
                Bullets {
                    items: [
                        qsTr("The warning light goes off, but comes back if the fault is still there, sometimes after a few drives."),
                        qsTr("The freeze frame is erased. Read it, or save a report, first."),
                        qsTr("Every readiness monitor resets. The car fails the ITP emissions check until it has been driven enough, usually 50–150 km of mixed driving over a few days. Never clear codes right before an inspection."),
                        qsTr("Permanent codes stay. They clear themselves once the car has re-tested the system and found it fixed."),
                    ]
                }
            }

            Section {
                id: screens
                title: qsTr("The screens")
                Rows {
                    items: [
                        { label: qsTr("Overview"), text: qsTr("The check-engine light, readiness, battery, codes and the main live values at a glance. Save report… makes a PDF or web page to share.") },
                        { label: qsTr("Trouble codes"), text: qsTr("Codes from the engine and transmission, each explained: what it means, likely causes, whether it matters for the ITP.") },
                        { label: qsTr("All modules"), text: qsTr("Asks ABS, airbag, body and the other modules for their codes, using the module list of your car's brand.") },
                        { label: qsTr("Readiness"), text: qsTr("Which self-tests have finished since codes were last cleared, and the self-test results with their limits.") },
                        { label: qsTr("Freeze frame"), text: qsTr("The sensor values saved when a fault was stored.") },
                        { label: qsTr("Live data"), text: qsTr("Every value the car reports, with its recent history. Pin values to the Overview. On Hyundai and Kia cars, the brand's own values too, from the OBDb community: choose the model, then Find values.") },
                        { label: qsTr("Charts"), text: qsTr("Several values over time, or two recordings compared.") },
                        { label: qsTr("Trip"), text: qsTr("Distance, fuel and consumption. Calibrate the fuel estimate with two fill-ups.") },
                        { label: qsTr("Vehicle info"), text: qsTr("The VIN and what it tells, software versions and the modules that answer.") },
                        { label: qsTr("Adapter log"), text: qsTr("What the adapter supports, how fast it answers, and every command and answer, for troubleshooting.") },
                    ]
                }
            }

            Section {
                id: trouble
                title: qsTr("When something doesn't work")
                Rows {
                    items: [
                        { label: qsTr("Can't reach the adapter"), text: qsTr("Check that this computer is on the adapter's Wi-Fi; macOS sometimes jumps back to your usual network. On macOS 15 and later, allow Obdaryn under System Settings → Privacy & Security → Local Network.") },
                        { label: qsTr("The adapter refuses"), text: qsTr("Another app is connected to it, often a phone app. Close it and connect again.") },
                        { label: qsTr("The car isn't answering"), text: qsTr("Turn the ignition ON. If you switched it off, Obdaryn picks up again by itself when it's back on.") },
                        { label: qsTr("The connection drops"), text: qsTr("The adapter may have fallen asleep or the Wi-Fi changed. Obdaryn reconnects by itself; if it gives up, turn the ignition off and on, then press Connect again.") },
                        { label: qsTr("A module shows \"Not found\""), text: qsTr("Your model may not have it, or uses another address. Addresses marked \"unconfirmed\" come from community lists.") },
                        { label: qsTr("Live data is slow"), text: qsTr("Cheap adapters and older cars answer slowly. Pin fewer values to the Overview; pinned and charted values refresh first.") },
                        { label: qsTr("A code came back"), text: qsTr("The fault is still there. Clearing only turns the light off; the cause needs fixing.") },
                    ]
                }
            }

            Section {
                id: words
                title: qsTr("Words you'll see")
                Rows {
                    items: [
                        { label: qsTr("Check-engine light (MIL)"), text: qsTr("The engine warning lamp, turned on by the engine computer for faults that affect emissions.") },
                        { label: qsTr("Trouble code (DTC)"), text: qsTr("A five-character code such as P0420: P powertrain, C chassis, B body, U network.") },
                        { label: qsTr("Stored, pending, permanent"), text: qsTr("Stored: confirmed. Pending: seen once, stored if it happens again. Permanent: can't be cleared by a tool, clears itself once fixed.") },
                        { label: qsTr("Freeze frame"), text: qsTr("A snapshot of sensor values taken when a code was stored.") },
                        { label: qsTr("Readiness monitors"), text: qsTr("Self-tests of the emission systems. Incomplete monitors can fail an emissions inspection.") },
                        { label: qsTr("Self-test results (Mode 06)"), text: qsTr("The numbers behind the monitors, each with the limits the car judges it by.") },
                        { label: qsTr("ITP"), text: qsTr("The periodic technical inspection in Romania, which includes an emissions check.") },
                        { label: qsTr("VIN"), text: qsTr("The car's 17-character serial number.") },
                    ]
                }
            }

            Section {
                id: safety
                title: qsTr("Privacy and safety")
                Bullets {
                    items: [
                        qsTr("Obdaryn sends nothing online. It talks only to the adapter."),
                        qsTr("Recordings and reports stay on this computer and include the VIN. Check them before sharing."),
                        qsTr("A safety guard checks every command and refuses anything that could write to, code, reset or flash a module. The only change it allows is clearing trouble codes, after you confirm, with the engine off."),
                        qsTr("Code explanations are guidance from public sources. Confirm with a workshop before replacing parts."),
                    ]
                }
            }

            // ---- About -------------------------------------------------------
            Rectangle { Layout.fillWidth: true; Layout.topMargin: Theme.s7; implicitHeight: 1; color: Theme.hairline }
            Para {
                id: about
                Layout.topMargin: Theme.s4
                Layout.bottomMargin: Theme.s7
                readonly property string url: "https://github.com/liviu-hariton/obdaryn"
                //: %1 = version, %2 = a link to the project page
                text: qsTr("Obdaryn %1 is free and open-source software (GPL-3.0). Source code and the full guide: %2")
                      .arg(App.appVersion)
                      .arg("<a href=\"" + url + "\">github.com/liviu-hariton/obdaryn</a>")
                textFormat: Text.StyledText
                color: Theme.textMuted
                font.pixelSize: Theme.fontSmall
                linkColor: Theme.blue
                onLinkActivated: link => Qt.openUrlExternally(link)
                HoverHandler { cursorShape: about.hoveredLink.length ? Qt.PointingHandCursor : Qt.ArrowCursor }
            }
        }
    }
}
