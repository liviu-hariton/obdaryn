// SPDX-License-Identifier: GPL-3.0-or-later

#include "models/TrafficLogModel.h"

namespace obdaryn {

TrafficLogModel::TrafficLogModel(QObject *parent) : QAbstractListModel(parent) {}

int TrafficLogModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_entries.size());
}

QVariant TrafficLogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.size())
        return {};
    const Entry &e = m_entries[index.row()];
    switch (role) {
    case TimeRole: return e.time.toString(QStringLiteral("HH:mm:ss.zzz"));
    case DirectionRole: return e.direction;
    case TextRole: return e.text;
    default: return {};
    }
}

QHash<int, QByteArray> TrafficLogModel::roleNames() const
{
    return {{TimeRole, "time"}, {DirectionRole, "direction"}, {TextRole, "text"}};
}

void TrafficLogModel::append(const QString &direction, const QString &text)
{
    if (m_entries.size() >= kMaxEntries) {
        // Drop the oldest tenth in one go; cheaper than one row at a time.
        const int drop = kMaxEntries / 10;
        beginRemoveRows({}, 0, drop - 1);
        m_entries.remove(0, drop);
        endRemoveRows();
    }
    const int row = int(m_entries.size());
    beginInsertRows({}, row, row);
    m_entries.append({QDateTime::currentDateTime(), direction, text});
    endInsertRows();
    emit countChanged();
}

void TrafficLogModel::clear()
{
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
}

QString TrafficLogModel::toText() const
{
    QString out;
    for (const Entry &e : m_entries)
        out += QStringLiteral("%1 %2 %3\n").arg(e.time.toString(QStringLiteral("HH:mm:ss.zzz")), e.direction, e.text);
    return out;
}

} // namespace obdaryn
