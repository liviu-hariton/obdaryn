// SPDX-License-Identifier: GPL-3.0-or-later

#include "models/ManufacturerValuesModel.h"

#include "obd/ModuleAddressing.h"
#include "obd/Units.h"

#include <QLocale>

#include <algorithm>

namespace obdaryn {

ManufacturerValuesModel::ManufacturerValuesModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ManufacturerValuesModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

QHash<int, QByteArray> ManufacturerValuesModel::roleNames() const
{
    return {{NameRole, "name"},          {GroupRole, "group"},       {ModuleRole, "module"},
            {RequestRole, "request"},    {ValueTextRole, "valueText"}, {UnitRole, "unit"},
            {HasValueRole, "hasValue"}};
}

QVariant ManufacturerValuesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return {};
    const Row &r = m_rows[index.row()];
    const ManufacturerSignal &s = r.signal;
    switch (role) {
    case NameRole: return s.displayName();
    case GroupRole: return s.group.isEmpty() ? tr("Other") : ManufacturerData::groupName(s.group);
    case ModuleRole: return r.module;
    case RequestRole: return r.request;
    case HasValueRole: return r.hasValue;
    case ValueTextRole:
        if (!r.hasValue)
            return QStringLiteral("—");
        if (s.isState()) {
            const bool on = r.metric != 0;
            return s.unit == QLatin1String("offon") ? (on ? tr("On") : tr("Off")) : (on ? tr("Yes") : tr("No"));
        }
        {
            const Units::Shown shown = Units::show(r.metric, s.metricUnit(), s.decimals());
            return QLocale().toString(shown.value, 'f', shown.decimals);
        }
    case UnitRole:
        return s.isState() || !*s.metricUnit() ? QString() : Units::unitFor(s.metricUnit());
    default:
        return {};
    }
}

void ManufacturerValuesModel::setCommands(const QList<ManufacturerCommand> &commands, const QHash<quint16, QString> &moduleNames)
{
    beginResetModel();
    m_rows.clear();
    m_rowsOfCommand.clear();
    for (const ManufacturerCommand &c : commands) {
        const QString hex = QString::fromLatin1(ModuleAddressing::hex3(c.header));
        const QString module = moduleNames.value(c.header, hex);
        // "7E0 21 01": the request as the adapter log shows it, spaced.
        QString request = hex;
        const QByteArray req = c.request();
        for (int i = 0; i + 1 < req.size(); i += 2)
            request += QLatin1Char(' ') + QString::fromLatin1(req.mid(i, 2));
        for (const ManufacturerSignal &s : c.signalList) {
            Row r;
            r.commandKey = c.key();
            r.signal = s;
            r.module = module;
            r.request = request;
            m_rows << r;
        }
    }
    sortRows();
    endResetModel();
    emit countChanged();
}

void ManufacturerValuesModel::sortRows()
{
    // Grouped as OBDb groups them, then alphabetically by the name as shown,
    // so the order follows the UI language.
    std::stable_sort(m_rows.begin(), m_rows.end(), [](const Row &a, const Row &b) {
        const int g = QString::localeAwareCompare(ManufacturerData::groupName(a.signal.group),
                                                  ManufacturerData::groupName(b.signal.group));
        return g != 0 ? g < 0 : QString::localeAwareCompare(a.signal.displayName(), b.signal.displayName()) < 0;
    });
    m_rowsOfCommand.clear();
    for (int i = 0; i < m_rows.size(); ++i)
        m_rowsOfCommand[m_rows[i].commandKey] << i;
}

void ManufacturerValuesModel::clear()
{
    setCommands({}, {});
}

void ManufacturerValuesModel::applyResult(const ManufacturerCommand &command, const ManufacturerReader::Result &result)
{
    if (!result.answered)
        return;
    const QList<int> rows = m_rowsOfCommand.value(command.key());
    for (int row : rows) {
        Row &r = m_rows[row];
        r.hasValue = false;
        for (const ManufacturerReader::Value &v : result.values) {
            if (v.signal >= 0 && v.signal < command.signalList.size() && command.signalList[v.signal].id == r.signal.id) {
                r.metric = r.signal.toMetric(v.value);
                r.hasValue = true;
                break;
            }
        }
        emit dataChanged(index(row), index(row), {ValueTextRole, HasValueRole});
    }
}

void ManufacturerValuesModel::refreshTexts()
{
    if (m_rows.isEmpty())
        return;
    // Names may have changed language: sort again, then show every text anew.
    beginResetModel();
    sortRows();
    endResetModel();
}

} // namespace obdaryn
