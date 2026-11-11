// SPDX-License-Identifier: GPL-3.0-or-later
//
// Help: how to connect, what to do in which car state, what clearing really
// does, the screens, troubleshooting, words, privacy and safety. A reading
// page: one column at reading width, jump links at the top.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Layouts
import Obdaryn

Item {
    id: page
    signal openPage(int p)

    // Building blocks, so the content below reads like the text it is.
    component Para: Text {
        Layout.fillWidth: true
        color: Theme.text
        font.family: Theme.sans
        font.pixelSize: Theme.fontBody
        lineHeight: 1.35
        wrapMode: Text.WordWrap
    }
    component Bullets: ColumnLayout {
        id: bullets
        property var items: []
        Layout.fillWidth: true
        spacing: 6
        Repeater {
            model: bullets.items
            delegate: RowLayout {
                required property string modelData
                Layout.fillWidth: true
                spacing: Theme.s2
                Rectangle { Layout.alignment: Qt.AlignTop; Layout.topMargin: 8; width: 4; height: 4; radius: 2; color: Theme.textMuted }
                Para { text: modelData }
            }
        }
    }
    /// Label on the left, explanation on the right, hairline between rows.
    component Rows: ColumnLayout {
        id: rows
        property var items: []    // [{ label, text }]
        property int labelWidth: 220
        Layout.fillWidth: true
        spacing: 0
        Repeater {
            model: rows.items
            delegate: ColumnLayout {
                required property var modelData
                Layout.fillWidth: true
                spacing: 0
                Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.hairline }
                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: Theme.s3
                    Layout.bottomMargin: Theme.s3
                    spacing: Theme.s4
                    Text {
                        Layout.preferredWidth: rows.labelWidth
                        Layout.alignment: Qt.AlignTop
                        text: modelData.label
                        color: Theme.text
                        font.family: Theme.sans
                        font.pixelSize: Theme.fontBody
                        font.weight: Font.DemiBold
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
        spacing: Theme.s3
        Text {
            id: heading
            Layout.fillWidth: true
            Layout.topMargin: Theme.s4
            color: Theme.text
            font.family: Theme.sans
            font.pixelSize: Theme.fontSection
            font.weight: Font.DemiBold
            wrapMode: Text.WordWrap
            Accessible.role: Accessible.Heading
        }
    }

    function jumpTo(item) {
        scroll.contentItem.contentY = Math.max(0, Math.min(item.y - Theme.s4, content.height - scroll.height))
    }

    C.ScrollView {
        id: scroll
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            id: content
            width: Math.min(Theme.readingMaxWidth, scroll.availableWidth - 2 * Theme.s6)
            x: (scroll.availableWidth - width) / 2
            spacing: Theme.s4

            PageHeader {
                Layout.fillWidth: true
                Layout.topMargin: Theme.s5
                title: qsTr("Help")
                subtitle: qsTr("How to connect, what to do in which state of the car, and what each screen tells you.")
            }

            // Jump links: the sections are long, people come for one of them.
            Flow {
                Layout.fillWidth: true
                spacing: 6
                Repeater {
                    model: [
                        { label: qsTr("Start here"), target: start },
                        { label: qsTr("Which car state"), target: states },
                        { label: qsTr("Clearing codes"), target: clearing },
                        { label: qsTr("The screens"), target: screens },
                        { label: qsTr("When something doesn't work"), target: trouble },
                        { label: qsTr("Words"), target: words },
                        { label: qsTr("Privacy and safety"), target: safety },
                    ]
                    delegate: AppButton {
                        required property var modelData
                        variant: "quiet"
                        implicitHeight: 28
                        text: modelData.label
                        onClicked: page.jumpTo(modelData.target)
                    }
                }
            }

            Section {
                id: start
                title: qsTr("Start here")
                Para { text: qsTr("Obdaryn reads what the car's computers report through an ELM327-compatible Wi-Fi adapter such as the Vgate iCar Pro. It only reads: it can't change anything in the car.") }
                Bullets {
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
                        { label: qsTr("Live data"), text: qsTr("Every value the car reports, with its recent history. Pin values to the Overview.") },
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
                        { label: qsTr("The connection drops"), text: qsTr("The adapter may have fallen asleep or the Wi-Fi changed. Obdaryn reconnects by itself; if it gives up, press its button or turn the ignition off and on, then connect.") },
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
                    labelWidth: 200
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
                Para {
                    Layout.bottomMargin: Theme.s6
                    text: qsTr("Obdaryn %1 is free and open-source software (GPL-3.0). Source code and the full guide: github.com/liviu-hariton/obdaryn").arg(App.appVersion)
                    color: Theme.textFaint
                    font.pixelSize: Theme.fontSmall
                }
            }
        }
    }
}
