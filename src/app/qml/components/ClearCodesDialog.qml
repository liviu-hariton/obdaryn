// SPDX-License-Identifier: GPL-3.0-or-later
//
// Confirmation before clearing trouble codes. A modal is justified here: the
// action changes the car's state (it erases data and resets the readiness
// monitors), so the user must read the consequences before it happens.
import QtQuick
import QtQuick.Controls.Basic as C
import QtQuick.Effects
import QtQuick.Layouts
import Obdaryn

C.Popup {
    id: dialog
    signal confirmed()
    /// Set to clear one module (UDS/KWP 0x14) instead of the OBD-II codes.
    property int moduleRow: -1
    property string moduleName: ""
    readonly property bool moduleMode: moduleRow >= 0

    modal: true
    focus: true
    anchors.centerIn: C.Overlay.overlay
    width: Math.min(560, (parent ? parent.width : 600) - 2 * Theme.s5)
    padding: Theme.s5
    closePolicy: C.Popup.CloseOnEscape | C.Popup.CloseOnPressOutside

    onOpened: cancelButton.forceActiveFocus()

    C.Overlay.modal: Rectangle { color: Theme.dark ? "#B3000000" : "#59000000" }

    background: Rectangle {
        radius: Theme.radiusLarge
        color: Theme.surface
        border.width: 1
        border.color: Theme.hairline
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowBlur: 0.8
            shadowVerticalOffset: 10
            shadowColor: "#55000000"
        }
    }

    contentItem: ColumnLayout {
        spacing: Theme.s4

        Text {
            Layout.fillWidth: true
            text: dialog.moduleMode ? qsTr("Clear the codes in %1?").arg(dialog.moduleName) : qsTr("Clear trouble codes?")
            color: Theme.text
            font.family: Theme.sans
            font.pixelSize: 20
            font.weight: Font.Bold
            Accessible.role: Accessible.Heading
        }
        Text {
            Layout.fillWidth: true
            text: qsTr("Do this after a repair, not to hide a fault. Before clearing, know that it:")
            color: Theme.textMuted
            font.family: Theme.sans
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
        }

        Repeater {
            model: dialog.moduleMode ? [
                qsTr("Turns this module's warning light off if the fault is gone. A fault that is still present comes back at once."),
                qsTr("Erases the module's fault history, which a workshop may want to see. Note the codes first."),
                qsTr("Clearing never repairs anything: a %1 fault usually needs a fix before it stays away.").arg(dialog.moduleName),
            ] : [
                qsTr("Turns the check-engine light off. If the fault is still there, the light comes back."),
                qsTr("Erases the freeze frame, the snapshot of sensor values taken when the fault happened."),
                qsTr("Resets all readiness monitors. The car fails the ITP emissions check until it has been driven for a few days."),
                qsTr("Does not remove permanent codes. They clear themselves once the car re-tests the system."),
            ]
            delegate: RowLayout {
                required property string modelData
                Layout.fillWidth: true
                spacing: Theme.s3
                Rectangle { width: 5; height: 5; radius: 2.5; color: Theme.textMuted; Layout.alignment: Qt.AlignTop; Layout.topMargin: 8 }
                Text {
                    Layout.fillWidth: true
                    text: parent.modelData
                    color: Theme.text
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontBody
                    lineHeight: 1.3
                    wrapMode: Text.WordWrap
                }
            }
        }

        // The state the car must be in.
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: req.implicitHeight + 2 * Theme.s3
            radius: Theme.radius
            color: Theme.hover
            RowLayout {
                id: req
                anchors.fill: parent
                anchors.margins: Theme.s3
                spacing: Theme.s3
                Icon { name: "info"; color: Theme.blue; size: 18; Layout.alignment: Qt.AlignTop }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Engine off, ignition ON. Obdaryn checks the engine speed first and won't clear while the engine runs.")
                    color: Theme.text
                    font.family: Theme.sans
                    font.pixelSize: Theme.fontBody
                    wrapMode: Text.WordWrap
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.s2
            spacing: Theme.s2
            Item { Layout.fillWidth: true }
            AppButton {
                id: cancelButton
                text: qsTr("Cancel")
                onClicked: dialog.close()
            }
            AppButton {
                variant: "danger"
                emphasized: true
                iconName: "eraser"
                text: qsTr("Clear codes")
                onClicked: { dialog.close(); dialog.confirmed() }
            }
        }
    }
}
