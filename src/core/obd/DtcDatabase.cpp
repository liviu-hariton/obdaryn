// SPDX-License-Identifier: GPL-3.0-or-later

#include "obd/DtcDatabase.h"

#include "obd/Dtc.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

namespace obdaryn {

DtcDatabase &DtcDatabase::instance()
{
    static DtcDatabase db;
    return db;
}

DtcDatabase::DtcDatabase()
{
    // Parsed once, on first use (≈ 10 k entries, a few hundred ms at most).
    loadFile(QStringLiteral(":/obdaryn/data/dtc_generic.json"));
}

bool DtcDatabase::loadFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    const QJsonObject codes = root.value(QStringLiteral("codes")).toObject();
    for (auto it = codes.begin(); it != codes.end(); ++it)
        m_codes.insert(it.key(), it.value());

    const QJsonObject source = root.value(QStringLiteral("source")).toObject();
    if (!source.isEmpty()) {
        m_attribution = QStringLiteral("%1 (%2), %3")
                            .arg(source.value(QStringLiteral("name")).toString(),
                                 source.value(QStringLiteral("url")).toString(),
                                 source.value(QStringLiteral("license")).toString());
    }
    return !codes.isEmpty();
}

DtcInfo DtcDatabase::lookup(const QString &rawCode) const
{
    DtcInfo info;
    info.code = rawCode.trimmed().toUpper();
    info.system = DtcCodec::systemName(info.code);
    info.manufacturerSpecific = DtcCodec::isManufacturerSpecific(info.code);

    const QJsonObject e = m_codes.value(info.code).toObject();
    if (e.isEmpty())
        return info;

    info.known = true;
    info.title = e.value(QStringLiteral("t")).toString();
    info.description = e.value(QStringLiteral("d")).toString();
    for (const QJsonValue &c : e.value(QStringLiteral("c")).toArray()) {
        const QJsonArray pair = c.toArray();
        if (pair.size() == 2)
            info.causes.append({pair[0].toString(), pair[1].toString()});
    }
    for (const QJsonValue &s : e.value(QStringLiteral("s")).toArray())
        info.symptoms << s.toString();

    const QJsonArray flags = e.value(QStringLiteral("f")).toArray();
    info.turnsOnMil = flags.contains(QStringLiteral("mil"));
    info.emissionsRelevant = flags.contains(QStringLiteral("emissions"));
    info.needsDriveCycle = flags.contains(QStringLiteral("drive_cycle"));
    info.limpModePossible = flags.contains(QStringLiteral("limp_mode"));

    const QJsonObject r = e.value(QStringLiteral("r")).toObject();
    info.repairDifficulty = r.value(QStringLiteral("difficulty")).toString();
    info.diyPossible = r.value(QStringLiteral("diy")).toBool();
    const QJsonArray eur = r.value(QStringLiteral("eur")).toArray();
    if (eur.size() == 2) {
        info.costMinEur = eur[0].toInt();
        info.costMaxEur = eur[1].toInt();
    }
    for (const QJsonValue &rel : e.value(QStringLiteral("rel")).toArray())
        info.related << rel.toString();
    return info;
}

} // namespace obdaryn
