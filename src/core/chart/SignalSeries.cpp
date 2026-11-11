// SPDX-License-Identifier: GPL-3.0-or-later

#include "chart/SignalSeries.h"

#include <algorithm>
#include <limits>

namespace obdaryn {

namespace {
auto byTime = [](const SignalSeries::Sample &s, qint64 t) { return s.t < t; };
} // namespace

void SignalSeries::add(qint64 t, double v)
{
    // Polls arrive in order; a late one (clock jitter) is inserted where it belongs.
    if (m_samples.isEmpty() || t >= m_samples.last().t) {
        m_samples.append({t, v});
        return;
    }
    const auto it = std::lower_bound(m_samples.begin(), m_samples.end(), t, byTime);
    m_samples.insert(it, {t, v});
}

void SignalSeries::dropBefore(qint64 t)
{
    const auto it = std::lower_bound(m_samples.begin(), m_samples.end(), t, byTime);
    m_samples.erase(m_samples.begin(), it);
}

bool SignalSeries::range(qint64 from, qint64 to, double *lo, double *hi) const
{
    bool any = false;
    double a = std::numeric_limits<double>::max(), b = std::numeric_limits<double>::lowest();
    for (auto it = std::lower_bound(m_samples.cbegin(), m_samples.cend(), from, byTime);
         it != m_samples.cend() && it->t <= to; ++it) {
        a = std::min(a, it->v);
        b = std::max(b, it->v);
        any = true;
    }
    if (any) {
        *lo = a;
        *hi = b;
    }
    return any;
}

bool SignalSeries::valueAt(qint64 t, qint64 maxAgeMs, double *v) const
{
    auto it = std::upper_bound(m_samples.cbegin(), m_samples.cend(), t,
                               [](qint64 time, const Sample &s) { return time < s.t; });
    if (it == m_samples.cbegin())
        return false;
    --it; // the last sample at or before t
    if (t - it->t > maxAgeMs)
        return false;
    *v = it->v;
    return true;
}

bool SignalSeries::latest(double *v) const
{
    if (m_samples.isEmpty())
        return false;
    *v = m_samples.last().v;
    return true;
}

QList<QPointF> SignalSeries::envelope(qint64 from, qint64 to, int columns) const
{
    QList<QPointF> out;
    if (to <= from || columns < 1)
        return out;
    auto it = std::lower_bound(m_samples.cbegin(), m_samples.cend(), from, byTime);
    const double span = double(to - from);
    int column = -1;
    Sample lo{}, hi{};
    auto flush = [&] {
        if (column < 0)
            return;
        // Min and max of the column, in the order they happened.
        if (lo.t <= hi.t) {
            out << QPointF(lo.t, lo.v);
            if (hi.t != lo.t)
                out << QPointF(hi.t, hi.v);
        } else {
            out << QPointF(hi.t, hi.v) << QPointF(lo.t, lo.v);
        }
    };
    for (; it != m_samples.cend() && it->t <= to; ++it) {
        const int c = std::min(columns - 1, int((it->t - from) / span * columns));
        if (c != column) {
            flush();
            column = c;
            lo = hi = *it;
            continue;
        }
        if (it->v < lo.v)
            lo = *it;
        if (it->v > hi.v)
            hi = *it;
    }
    flush();
    return out;
}

} // namespace obdaryn
