// SPDX-License-Identifier: GPL-3.0-or-later

#include "obd/DtcDatabase.h"

#include "obd/Dtc.h"
#include "vehicles/ModuleCatalog.h"

#include <QDir>
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
    QFile ro(QStringLiteral(":/obdaryn/data/dtc_generic_ro.json"));
    if (ro.open(QIODevice::ReadOnly))
        m_codesRo = QJsonDocument::fromJson(ro.readAll()).object().value(QStringLiteral("codes")).toObject();
    const QDir brands(QStringLiteral(":/obdaryn/data/dtc/brands"));
    for (const QString &name : brands.entryList({QStringLiteral("*.json")}, QDir::Files))
        loadBrandFile(brands.filePath(name));
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

bool DtcDatabase::loadBrandFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    const QString brand = root.value(QStringLiteral("brand")).toString();
    if (brand.isEmpty())
        return false;
    BrandPack &pack = m_brands[brand];
    pack.label = root.value(QStringLiteral("label")).toString();
    pack.brandName = root.value(QStringLiteral("brandName")).toString();
    for (auto it = root.begin(); it != root.end(); ++it) {
        if (it.key().startsWith(QLatin1String("label_")))
            pack.localizedLabels.insert(it.key().mid(6), it.value().toString());
    }
    const QJsonObject codes = root.value(QStringLiteral("codes")).toObject();
    for (auto it = codes.begin(); it != codes.end(); ++it)
    {
        // "C1604@steering": upper-case the code, keep the module as written.
        const QString key = it.key();
        const qsizetype at = key.indexOf(QLatin1Char('@'));
        pack.codes.insert(at < 0 ? key.toUpper() : key.left(at).toUpper() + key.mid(at), it.value());
    }
    return !codes.isEmpty();
}

void DtcDatabase::fillFrom(DtcInfo &info, const QJsonObject &e) const
{
    info.known = true;
    info.title = e.value(QStringLiteral("t")).toString();
    info.description = e.value(QStringLiteral("d")).toString();
    info.causes.clear();
    for (const QJsonValue &c : e.value(QStringLiteral("c")).toArray()) {
        const QJsonArray pair = c.toArray();
        if (pair.size() == 2)
            info.causes.append({pair[0].toString(), pair[1].toString()});
    }
    info.symptoms.clear();
    for (const QJsonValue &s : e.value(QStringLiteral("s")).toArray())
        info.symptoms << s.toString();
}

DtcInfo DtcDatabase::lookup(const QString &rawCode, const QString &brand, const QString &language,
                            const QString &module) const
{
    DtcInfo info;
    info.code = rawCode.trimmed().toUpper();
    info.system = DtcCodec::systemName(info.code);
    info.manufacturerSpecific = DtcCodec::isManufacturerSpecific(info.code);

    const QJsonObject generic = m_codes.value(info.code).toObject();
    QJsonObject brandEntry;
    if (!brand.isEmpty()) {
        const QJsonObject &codes = m_brands.value(brand).codes;
        if (!module.isEmpty())
            brandEntry = codes.value(info.code + QLatin1Char('@') + module).toObject();
        if (brandEntry.isEmpty()) {
            brandEntry = codes.value(info.code).toObject();
            // Wrong module: better no brand text than a confidently wrong meaning.
            if (!module.isEmpty() && brandEntry.value(QStringLiteral("module")).toString() != module)
                brandEntry = QJsonObject();
        }
    }

    const bool useBrand = !brandEntry.isEmpty()
        && (info.manufacturerSpecific || generic.isEmpty()
            || brandEntry.value(QStringLiteral("override")).toBool());

    if (useBrand) {
        // Text in the requested language when the pack has it, else English.
        const QString lang = brandEntry.contains(language) ? language : QStringLiteral("en");
        fillFrom(info, brandEntry.value(lang).toObject());
        info.language = lang;
        info.source = brand;
        const BrandPack pack = m_brands.value(brand);
        info.sourceLabel = pack.localizedLabels.value(language, pack.label);
        info.confidence = brandEntry.value(QStringLiteral("confidence")).toString();
        info.module = brandEntry.value(QStringLiteral("module")).toString();
        // The brand knows which module the code belongs to: name it ("Airbag (SRS)").
        if (!info.module.isEmpty())
            info.system = ModuleCatalog::categoryName(info.module);
        info.brandName = pack.brandName;
        for (const QJsonValue &v : brandEntry.value(QStringLiteral("src")).toArray()) {
            // {"u": url, "o": true} (official) or a plain URL string.
            const QJsonObject ref = v.toObject();
            info.references << (ref.isEmpty() ? v.toString() : ref.value(QStringLiteral("u")).toString());
            info.referenceOfficial << ref.value(QStringLiteral("o")).toBool();
        }
        const QJsonArray flags = brandEntry.value(QStringLiteral("f")).toArray();
        info.turnsOnMil = flags.contains(QStringLiteral("mil"));
        info.emissionsRelevant = flags.contains(QStringLiteral("emissions"));
        info.limpModePossible = flags.contains(QStringLiteral("limp_mode"));
        info.warningLamp = flags.contains(QStringLiteral("warning_lamp"));
        return info;
    }

    if (generic.isEmpty())
        return info;

    fillFrom(info, generic);
    info.language = QStringLiteral("en");
    if (language == QLatin1String("ro")) {
        // Romanian title for every code; full text where it has been translated.
        const QJsonObject ro = m_codesRo.value(info.code).toObject();
        if (ro.contains(QStringLiteral("d"))) {
            fillFrom(info, ro);
            info.language = QStringLiteral("ro");
        } else if (ro.contains(QStringLiteral("t"))) {
            info.title = ro.value(QStringLiteral("t")).toString();
        }
    }
    info.source = QStringLiteral("generic");
    info.sourceLabel = QStringLiteral("obdex");

    const QJsonArray flags = generic.value(QStringLiteral("f")).toArray();
    info.turnsOnMil = flags.contains(QStringLiteral("mil"));
    info.emissionsRelevant = flags.contains(QStringLiteral("emissions"));
    info.needsDriveCycle = flags.contains(QStringLiteral("drive_cycle"));
    info.limpModePossible = flags.contains(QStringLiteral("limp_mode"));

    const QJsonObject r = generic.value(QStringLiteral("r")).toObject();
    info.repairDifficulty = r.value(QStringLiteral("difficulty")).toString();
    info.diyPossible = r.value(QStringLiteral("diy")).toBool();
    const QJsonArray eur = r.value(QStringLiteral("eur")).toArray();
    if (eur.size() == 2) {
        info.costMinEur = eur[0].toInt();
        info.costMaxEur = eur[1].toInt();
    }
    for (const QJsonValue &rel : generic.value(QStringLiteral("rel")).toArray())
        info.related << rel.toString();
    return info;
}

} // namespace obdaryn
