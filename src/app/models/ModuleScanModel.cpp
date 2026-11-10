// SPDX-License-Identifier: GPL-3.0-or-later

#include "models/ModuleScanModel.h"

#include "obd/DtcDatabase.h"

namespace obdaryn {

ModuleScanModel::ModuleScanModel(ModuleScanner *scanner, QObject *parent)
    : QAbstractListModel(parent), m_scanner(scanner)
{
    connect(scanner, &ModuleScanner::started, this, [this] {
        beginResetModel();
        endResetModel();
        emit summaryChanged();
    });
    connect(scanner, &ModuleScanner::moduleUpdated, this, [this](int row) {
        emit dataChanged(index(row), index(row));
        emit summaryChanged();
    });
    connect(scanner, &ModuleScanner::finished, this, &ModuleScanModel::summaryChanged);
}

int ModuleScanModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_scanner->results().size());
}

QVariant ModuleScanModel::data(const QModelIndex &index, int role) const
{
    const auto &results = m_scanner->results();
    if (!index.isValid() || index.row() >= results.size())
        return {};
    const ModuleResult &r = results[index.row()];

    switch (role) {
    case ModuleIdRole: return r.module.id;
    case NameRole: return r.module.displayName();
    case CategoryRole: return r.module.category;
    case AddressRole: return r.module.addressText();
    case StateRole:
        switch (r.state) {
        case ModuleResult::State::Waiting: return QStringLiteral("waiting");
        case ModuleResult::State::Scanning: return QStringLiteral("scanning");
        case ModuleResult::State::Absent: return QStringLiteral("absent");
        case ModuleResult::State::Answered: return QStringLiteral("answered");
        case ModuleResult::State::ObdOnly: return QStringLiteral("obdonly");
        case ModuleResult::State::Error: return QStringLiteral("error");
        }
        return {};
    case ProtocolRole:
        if (r.state != ModuleResult::State::Answered)
            return QString();
        return r.protocol == DiagProtocol::Uds ? QStringLiteral("UDS") : QStringLiteral("KWP2000");
    case VerifiedRole: return r.module.verified;
    case SourceRole: return r.module.source;
    case DtcsRole: {
        QVariantList list;
        for (const ModuleDtc &d : r.dtcs) {
            const DtcInfo info = DtcDatabase::instance().lookup(d.code);
            const QString kind = d.kind == DtcKind::Pending ? QStringLiteral("pending") : QStringLiteral("stored");
            list << QVariantMap{
                {QStringLiteral("code"), d.code},
                {QStringLiteral("displayCode"), d.displayCode()},
                {QStringLiteral("title"), info.known ? info.title
                                          : info.manufacturerSpecific ? tr("Manufacturer-specific code (%1)").arg(info.system)
                                          : info.system},
                {QStringLiteral("known"), info.known},
                {QStringLiteral("kind"), kind},
                {QStringLiteral("kindLabel"), d.kind == DtcKind::Pending ? tr("Pending") : tr("Stored")},
                {QStringLiteral("activeNow"), d.activeNow},
                {QStringLiteral("warningLamp"), d.warningLamp},
                {QStringLiteral("status"), QStringLiteral("0x") + QStringLiteral("%1").arg(d.status, 2, 16, QLatin1Char('0')).toUpper()},
            };
        }
        return list;
    }
    case CodeCountRole: return int(r.dtcs.size());
    case PartNumberRole: return r.partNumber;
    case SoftwareRole: return r.softwareVersion;
    case ErrorRole: return r.error;
    default: return {};
    }
}

QHash<int, QByteArray> ModuleScanModel::roleNames() const
{
    return {
        {ModuleIdRole, "moduleId"}, {NameRole, "name"}, {CategoryRole, "category"},
        {AddressRole, "address"}, {StateRole, "state"}, {ProtocolRole, "protocol"},
        {VerifiedRole, "verified"}, {SourceRole, "source"}, {DtcsRole, "dtcs"},
        {CodeCountRole, "codeCount"}, {PartNumberRole, "partNumber"},
        {SoftwareRole, "softwareVersion"}, {ErrorRole, "error"},
    };
}

int ModuleScanModel::answeredCount() const
{
    int n = 0;
    for (const ModuleResult &r : m_scanner->results())
        n += r.state == ModuleResult::State::Answered;
    return n;
}

int ModuleScanModel::codeCount() const
{
    int n = 0;
    for (const ModuleResult &r : m_scanner->results())
        n += int(r.dtcs.size());
    return n;
}

int ModuleScanModel::extraCodeCount() const
{
    int n = 0;
    for (const ModuleResult &r : m_scanner->results()) {
        if (r.module.category != QLatin1String("engine") && r.module.category != QLatin1String("transmission"))
            n += int(r.dtcs.size());
    }
    return n;
}

int ModuleScanModel::extraStoredCount() const
{
    int n = 0;
    for (const ModuleResult &r : m_scanner->results()) {
        if (r.module.category == QLatin1String("engine") || r.module.category == QLatin1String("transmission"))
            continue;
        for (const ModuleDtc &d : r.dtcs)
            n += d.kind == DtcKind::Stored;
    }
    return n;
}

int ModuleScanModel::modulesWithCodes() const
{
    int n = 0;
    for (const ModuleResult &r : m_scanner->results())
        n += !r.dtcs.isEmpty();
    return n;
}

int ModuleScanModel::doneCount() const
{
    int n = 0;
    for (const ModuleResult &r : m_scanner->results())
        n += r.state != ModuleResult::State::Waiting && r.state != ModuleResult::State::Scanning;
    return n;
}

void ModuleScanModel::retranslate()
{
    if (rowCount() > 0)
        emit dataChanged(index(0), index(rowCount() - 1), {NameRole, DtcsRole});
}

} // namespace obdaryn
