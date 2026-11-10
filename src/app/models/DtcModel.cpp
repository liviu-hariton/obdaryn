// SPDX-License-Identifier: GPL-3.0-or-later

#include "models/DtcModel.h"

#include "obd/DtcDatabase.h"

#include <QSet>

#include <algorithm>

namespace obdaryn {

DtcModel::DtcModel(QObject *parent) : QAbstractListModel(parent) {}

int DtcModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_dtcs.size());
}

QVariant DtcModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_dtcs.size())
        return {};
    const Dtc &d = m_dtcs[index.row()];

    switch (role) {
    case CodeRole: return d.code;
    case KindRole:
        return d.kind == DtcKind::Stored ? QStringLiteral("stored")
             : d.kind == DtcKind::Pending ? QStringLiteral("pending") : QStringLiteral("permanent");
    case KindLabelRole:
        return d.kind == DtcKind::Stored ? tr("Stored")
             : d.kind == DtcKind::Pending ? tr("Pending") : tr("Permanent");
    case EcuRole: return d.ecu;
    case EcuNameRole: return m_ecuNames.value(d.ecu);
    default: break;
    }

    // Standard OBD-II addresses: 7E8 is the engine, 7E9 the transmission.
    const QString module = d.ecu == QLatin1String("7E8") ? QStringLiteral("engine")
                         : d.ecu == QLatin1String("7E9") ? QStringLiteral("transmission") : QString();
    const DtcInfo info = DtcDatabase::instance().lookup(d.code, m_brand, m_language, module);
    switch (role) {
    case TitleRole:
        if (info.known)
            return info.title;
        return info.manufacturerSpecific
                   ? tr("Manufacturer-specific code (%1)").arg(info.system)
                   : info.system;
    case SystemRole: return info.system;
    case KnownRole: return info.known;
    case ManufacturerSpecificRole: return info.manufacturerSpecific;
    case EmissionsRole: return info.emissionsRelevant;
    default: return {};
    }
}

QHash<int, QByteArray> DtcModel::roleNames() const
{
    return {
        {CodeRole, "code"}, {KindRole, "kind"}, {KindLabelRole, "kindLabel"}, {EcuRole, "ecu"},
        {EcuNameRole, "ecuName"}, {TitleRole, "title"}, {SystemRole, "system"},
        {KnownRole, "known"}, {ManufacturerSpecificRole, "manufacturerSpecific"},
        {EmissionsRole, "emissionsRelevant"},
    };
}

QStringList DtcModel::codes() const
{
    QStringList out;
    for (const Dtc &d : m_dtcs) {
        if (!out.contains(d.code))
            out << d.code;
    }
    return out;
}

QVariantList DtcModel::grouped() const
{
    QVariantList out;
    QHash<QString, int> indexOf;
    for (int i = 0; i < m_dtcs.size(); ++i) {
        const Dtc &d = m_dtcs[i];
        const QModelIndex idx = index(i);
        const QVariantMap kind{{QStringLiteral("kind"), data(idx, KindRole)},
                               {QStringLiteral("label"), data(idx, KindLabelRole)}};
        auto it = indexOf.constFind(d.code);
        if (it == indexOf.constEnd()) {
            indexOf.insert(d.code, int(out.size()));
            out << QVariantMap{{QStringLiteral("code"), d.code},
                               {QStringLiteral("title"), data(idx, TitleRole)},
                               {QStringLiteral("ecu"), d.ecu},
                               {QStringLiteral("ecuName"), data(idx, EcuNameRole)},
                               {QStringLiteral("kinds"), QVariantList{kind}}};
        } else {
            QVariantMap entry = out[*it].toMap();
            QVariantList kinds = entry.value(QStringLiteral("kinds")).toList();
            kinds << kind;
            entry.insert(QStringLiteral("kinds"), kinds);
            out[*it] = entry;
        }
    }
    return out;
}

int DtcModel::activeCodeCount() const
{
    QSet<QString> codes;
    for (const Dtc &d : m_dtcs) {
        if (d.kind != DtcKind::Permanent)
            codes.insert(d.code);
    }
    return int(codes.size());
}

void DtcModel::setDtcs(const QList<Dtc> &dtcs)
{
    beginResetModel();
    m_dtcs = dtcs;
    // Stored first, then pending, then permanent; codes alphabetical inside.
    std::stable_sort(m_dtcs.begin(), m_dtcs.end(), [](const Dtc &a, const Dtc &b) {
        if (a.kind != b.kind)
            return int(a.kind) < int(b.kind);
        return a.code < b.code;
    });
    endResetModel();
    emit countsChanged();
}

void DtcModel::setEcuNames(const QHash<QString, QString> &names)
{
    m_ecuNames = names;
    if (!m_dtcs.isEmpty())
        emit dataChanged(index(0), index(int(m_dtcs.size()) - 1), {EcuNameRole});
}

void DtcModel::retranslate()
{
    if (!m_dtcs.isEmpty())
        emit dataChanged(index(0), index(int(m_dtcs.size()) - 1), {KindLabelRole, TitleRole, SystemRole});
}

void DtcModel::setContext(const QString &brand, const QString &language)
{
    if (brand == m_brand && language == m_language)
        return;
    m_brand = brand;
    m_language = language;
    retranslate();
}

int DtcModel::countOf(DtcKind kind) const
{
    return int(std::count_if(m_dtcs.begin(), m_dtcs.end(), [kind](const Dtc &d) { return d.kind == kind; }));
}

} // namespace obdaryn
