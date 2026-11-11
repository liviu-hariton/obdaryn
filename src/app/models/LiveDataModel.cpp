// SPDX-License-Identifier: GPL-3.0-or-later

#include "models/LiveDataModel.h"

#include "obd/Pids.h"

#include <QLocale>
#include <QSettings>

#include <algorithm>
#include <cmath>
#include <limits>

namespace obdaryn {

namespace {

// Shown on the dashboard until the user pins something else.
const QList<quint8> kDefaultPinned = {0x0C, 0x0D, 0x05, 0x04, 0x42, 0x11, 0x0B, 0x0F, 0x06, 0x07};
const QString kPinnedKey = QStringLiteral("liveData/pinned");

// How many non-pinned PIDs ride along with each polling cycle.
constexpr int kRotationPerCycle = 6;

} // namespace

LiveDataModel::LiveDataModel(QObject *parent) : QAbstractListModel(parent)
{
    QSettings settings;
    if (settings.contains(kPinnedKey)) {
        for (const QVariant &v : settings.value(kPinnedKey).toList())
            m_pinned.insert(quint8(v.toInt()));
    } else {
        m_pinned = QSet<quint8>(kDefaultPinned.begin(), kDefaultPinned.end());
    }
}

int LiveDataModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

QVariant LiveDataModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return {};
    const Row &row = m_rows[index.row()];
    const PidDefinition *def = PidTable::find(row.pid);
    if (!def)
        return {};

    switch (role) {
    case PidRole: return row.pid;
    case PidHexRole: return QStringLiteral("%1").arg(row.pid, 2, 16, QLatin1Char('0')).toUpper();
    case NameRole: return def->translatedName();
    case UnitRole: return def->translatedUnit();
    case ValueRole: return row.hasValue ? row.value : std::numeric_limits<double>::quiet_NaN();
    case ValueTextRole:
        return row.hasValue ? QLocale().toString(row.value, 'f', def->decimals) : QStringLiteral("—");
    case RawRole: return QString::fromLatin1(row.raw.toHex(' ').toUpper());
    case MinimumRole: return def->minimum;
    case MaximumRole: return def->maximum;
    case HistoryRole: {
        QVariantList list;
        list.reserve(row.history.size());
        for (double v : row.history)
            list << v;
        return list;
    }
    case PinnedRole: return m_pinned.contains(row.pid);
    case HasValueRole: return row.hasValue;
    default: return {};
    }
}

QHash<int, QByteArray> LiveDataModel::roleNames() const
{
    return {
        {PidRole, "pid"}, {PidHexRole, "pidHex"}, {NameRole, "name"}, {UnitRole, "unit"},
        {ValueRole, "value"}, {ValueTextRole, "valueText"}, {RawRole, "raw"},
        {MinimumRole, "minimum"}, {MaximumRole, "maximum"}, {HistoryRole, "history"},
        {PinnedRole, "pinned"}, {HasValueRole, "hasValue"},
    };
}

void LiveDataModel::setSupportedPids(const QList<quint8> &pids)
{
    beginResetModel();
    m_rows.clear();
    m_supported = QSet<quint8>(pids.cbegin(), pids.cend());
    for (quint8 pid : pids) {
        const PidDefinition *def = PidTable::find(pid);
        if (def && def->isNumeric())
            m_rows << Row{pid, 0, false, {}, {}};
    }
    // Most useful values first (the order the dashboard shows them), then by PID.
    std::stable_sort(m_rows.begin(), m_rows.end(), [](const Row &a, const Row &b) {
        const qsizetype ia = kDefaultPinned.indexOf(a.pid), ib = kDefaultPinned.indexOf(b.pid);
        const qsizetype ra = ia < 0 ? 1000 + a.pid : ia, rb = ib < 0 ? 1000 + b.pid : ib;
        return ra < rb;
    });
    m_rotation = 0;
    endResetModel();
    emit countChanged();
    emit pinnedChanged();
}

void LiveDataModel::clear()
{
    setSupportedPids({});
}

int LiveDataModel::rowOf(quint8 pid) const
{
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows[i].pid == pid)
            return i;
    }
    return -1;
}

QList<quint8> LiveDataModel::nextPollBatch()
{
    QList<quint8> batch;
    QList<quint8> others;
    for (quint8 pid : std::as_const(m_required)) {
        if (m_supported.contains(pid))
            batch << pid;
    }
    std::sort(batch.begin(), batch.end());
    for (const Row &r : std::as_const(m_rows)) {
        if (batch.contains(r.pid))
            continue;
        if (m_pinned.contains(r.pid))
            batch << r.pid;
        else
            others << r.pid;
    }
    // Rotate through the rest so every value refreshes every few cycles.
    for (int i = 0; i < qMin(kRotationPerCycle, int(others.size())); ++i)
        batch << others[(m_rotation + i) % others.size()];
    if (!others.isEmpty())
        m_rotation = (m_rotation + kRotationPerCycle) % int(others.size());
    return batch;
}

void LiveDataModel::applyReading(quint8 pid, const QByteArray &raw)
{
    const int i = rowOf(pid);
    const PidDefinition *def = PidTable::find(pid);
    double value = 0;
    if (i < 0 || !def || !def->value(raw, &value) || !std::isfinite(value))
        return;

    Row &row = m_rows[i];
    row.value = value;
    row.hasValue = true;
    row.raw = raw;
    row.history << value;
    if (row.history.size() > kHistoryLength)
        row.history.removeFirst();

    const QModelIndex idx = index(i);
    emit dataChanged(idx, idx, {ValueRole, ValueTextRole, RawRole, HistoryRole, HasValueRole});
}

void LiveDataModel::setPinned(int pid, bool pinned)
{
    const quint8 p = quint8(pid);
    if (pinned == m_pinned.contains(p))
        return;
    if (pinned)
        m_pinned.insert(p);
    else
        m_pinned.remove(p);
    savePinned();
    const int i = rowOf(p);
    if (i >= 0)
        emit dataChanged(index(i), index(i), {PinnedRole});
    emit pinnedChanged();
}

QVariantList LiveDataModel::pinnedRows() const
{
    QVariantList out;
    for (int i = 0; i < m_rows.size(); ++i) {
        if (!m_pinned.contains(m_rows[i].pid))
            continue;
        QVariantMap m;
        const QModelIndex idx = index(i);
        const auto names = roleNames();
        for (auto it = names.begin(); it != names.end(); ++it)
            m.insert(QString::fromLatin1(it.value()), data(idx, it.key()));
        m.insert(QStringLiteral("row"), i);
        out << m;
    }
    return out;
}

QString LiveDataModel::toCsv() const
{
    QString csv = QStringLiteral("pid,name,value,unit\n");
    for (int i = 0; i < m_rows.size(); ++i) {
        const QModelIndex idx = index(i);
        csv += QStringLiteral("%1,\"%2\",%3,%4\n")
                   .arg(data(idx, PidHexRole).toString(), data(idx, NameRole).toString(),
                        m_rows[i].hasValue ? QString::number(m_rows[i].value) : QString(),
                        data(idx, UnitRole).toString());
    }
    return csv;
}

void LiveDataModel::setSamplesPerSecond(double v)
{
    if (qFuzzyCompare(v + 1.0, m_samplesPerSecond + 1.0))
        return;
    m_samplesPerSecond = v;
    emit samplesPerSecondChanged();
}

void LiveDataModel::retranslate()
{
    if (!m_rows.isEmpty())
        emit dataChanged(index(0), index(int(m_rows.size()) - 1), {NameRole, UnitRole, ValueTextRole});
    emit pinnedChanged();
}

void LiveDataModel::savePinned() const
{
    QVariantList list;
    for (quint8 p : m_pinned)
        list << int(p);
    QSettings().setValue(kPinnedKey, list);
}

} // namespace obdaryn
