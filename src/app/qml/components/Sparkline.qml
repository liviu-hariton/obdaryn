// SPDX-License-Identifier: GPL-3.0-or-later
//
// The recent history of one live value (oldest left, newest right). It shows
// real samples, scaled to their own range, so a change is visible at a glance.
// On connect it "sweeps in" from the left once (the bulb-check moment).
import QtQuick
import QtQuick.Shapes
import Obdaryn

Item {
    id: spark
    property var values: []          // numbers
    property color color: Theme.blue
    property int capacity: 120       // samples the full width represents
    property real reveal: 1.0        // 0..1, animated by the bulb check
    property bool sweep: false       // set true to replay the sweep-in

    implicitHeight: 28
    clip: true

    onSweepChanged: if (sweep && !Theme.reduceMotion) sweepAnimation.restart()
    NumberAnimation {
        id: sweepAnimation
        target: spark
        property: "reveal"
        from: 0
        to: 1
        duration: Theme.sweep
        easing.type: Easing.OutExpo
    }

    readonly property var points: {
        const v = spark.values
        const n = v.length
        if (n < 2 || width <= 0)
            return []
        let lo = Infinity, hi = -Infinity
        for (let i = 0; i < n; ++i) { lo = Math.min(lo, v[i]); hi = Math.max(hi, v[i]) }
        if (hi - lo < 1e-9) { lo -= 1; hi += 1 }   // flat line in the middle
        const step = width / (capacity - 1)
        const x0 = width - (n - 1) * step          // newest sample on the right edge
        const pad = 2
        const out = []
        for (let j = 0; j < n; ++j)
            out.push(Qt.point(x0 + j * step, pad + (height - 2 * pad) * (1 - (v[j] - lo) / (hi - lo))))
        return out
    }

    // Faint baseline: the full width is the time window, even before it fills.
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairline }

    Item {
        width: parent.width * spark.reveal
        height: parent.height
        clip: true
        Shape {
            width: spark.width
            height: spark.height
            visible: spark.points.length > 1
            ShapePath {
                strokeColor: spark.color
                strokeWidth: 1.5
                fillColor: "transparent"
                joinStyle: ShapePath.RoundJoin
                capStyle: ShapePath.RoundCap
                PathPolyline { path: spark.points }
            }
        }
    }
}
