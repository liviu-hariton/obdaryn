// SPDX-License-Identifier: GPL-3.0-or-later
//
// A time-stamped series of one live value, for charts.

#pragma once

#include <QList>
#include <QPointF>

namespace obdaryn {

/**
 * @brief Samples of one signal over time (milliseconds, metric values).
 *
 * Charts may hold thousands of samples per signal (ten minutes at several
 * readings a second), more than there are pixels. envelope() reduces them to
 * at most two points per pixel column, the lowest and the highest, so a spike
 * that lasted one sample still shows, and drawing stays cheap.
 */
class SignalSeries
{
public:
    struct Sample {
        qint64 t = 0;
        double v = 0;
    };

    void add(qint64 t, double v);
    void clear() { m_samples.clear(); }
    /// Drops samples older than `t` (keeps memory bounded for live charts).
    void dropBefore(qint64 t);
    bool isEmpty() const { return m_samples.isEmpty(); }
    const QList<Sample> &samples() const { return m_samples; }

    /// Lowest and highest value between `from` and `to`; false if none.
    bool range(qint64 from, qint64 to, double *lo, double *hi) const;
    /// The last value at or before `t`, if a sample is at most `maxAgeMs` older.
    bool valueAt(qint64 t, qint64 maxAgeMs, double *v) const;
    /// The newest value; false if empty.
    bool latest(double *v) const;

    /**
     * @brief Points to draw for the window [from, to] in `columns` pixel columns.
     *
     * Each point is (t, v). Per column: its lowest and highest sample, in time
     * order, so the line still goes through every extreme.
     */
    QList<QPointF> envelope(qint64 from, qint64 to, int columns) const;

private:
    QList<Sample> m_samples; ///< oldest first
};

} // namespace obdaryn
