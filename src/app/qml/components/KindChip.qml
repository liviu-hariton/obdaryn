// SPDX-License-Identifier: GPL-3.0-or-later
//
// Which memory a trouble code came from: stored (red), pending (amber), permanent (blue).
import QtQuick
import Obdaryn

Chip {
    property string kind: "stored"
    property string label: ""
    text: label
    dotColor: kind === "stored" ? Theme.red : kind === "pending" ? Theme.amber : Theme.blue
}
