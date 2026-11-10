// SPDX-License-Identifier: GPL-3.0-or-later
//
// Plain-language explanations for trouble codes: generic and per brand.

#pragma once

#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

namespace obdaryn {

/// Everything Obdaryn knows about one code.
struct DtcInfo {
    QString code;
    bool known = false;          ///< false: no entry, only the generic system name
    bool manufacturerSpecific = false;
    QString title;               ///< e.g. "Catalyst System Efficiency Below Threshold Bank 1"
    QString description;         ///< what the car checks and what the code means
    QString system;              ///< translated subsystem, always filled
    QList<QPair<QString, QString>> causes; ///< (likelihood "high"/"medium"/"low", text)
    QStringList symptoms;
    bool turnsOnMil = false;     ///< usually lights the check-engine lamp
    bool emissionsRelevant = false; ///< matters for the emissions test / ITP
    bool needsDriveCycle = false;   ///< a drive cycle is needed to confirm a repair
    bool limpModePossible = false;
    bool warningLamp = false;    ///< lights a module's own warning lamp (ABS, airbag, ...)
    QString repairDifficulty;    ///< "easy" / "medium" / "hard" (may be empty)
    bool diyPossible = false;
    int costMinEur = 0, costMaxEur = 0; ///< rough repair cost range, 0 = unknown
    QStringList related;

    // Where the text comes from.
    QString source;              ///< "generic" (obdex) or a brand pack id, e.g. "hyundai-kia"
    QString sourceLabel;         ///< "obdex" / "Obdaryn Hyundai/Kia pack"
    QString language;            ///< language of title/description ("en", "ro")
    QString confidence;          ///< brand packs: "official" or "two-sources"
    QString module;              ///< brand packs: module category the code belongs to
    QStringList references;      ///< brand packs: public sources for the meaning
    QList<bool> referenceOfficial; ///< same order as references: true for a Hyundai/Kia (OEM) document
    QString brandName;           ///< brand packs: "Hyundai/Kia"
};

/**
 * @brief Looks up code descriptions.
 *
 * Two layers:
 *  - Generic codes from the obdex database (CC0-1.0, independently written,
 *    not SAE J2012 text), compiled in from data/dtc_generic.json. English.
 *  - Brand packs (data/dtc/brands/<brand>.json), keyed by the vehicle-profile
 *    id (e.g. "hyundai-kia"). They describe manufacturer-specific codes such as
 *    C1611 or B1650 in English and Romanian, written for Obdaryn from public
 *    sources that each entry lists. Keys are "C1604" or, when the meaning
 *    differs per module, "C1604@steering". A brand entry is used when the code is
 *    manufacturer-specific, when the generic layer doesn't know it, or when
 *    the entry says "override": true.
 */
class DtcDatabase
{
public:
    static DtcDatabase &instance();

    /// @param brand    vehicle-profile id of the car ("hyundai-kia"), or empty
    /// @param language "en" or "ro"; brand text falls back to English
    /// @param module   category of the module that reported the code ("abs",
    ///                 "steering", ...), or empty when unknown. Manufacturer codes
    ///                 can mean different things in different modules (Hyundai
    ///                 C1604 is an ABS hardware fault but an MDPS over-discharge),
    ///                 so a brand entry for another module is never used.
    DtcInfo lookup(const QString &code, const QString &brand = {}, const QString &language = {},
                   const QString &module = {}) const;
    int size() const { return int(m_codes.size()); }
    /// Number of codes a brand pack describes (0 if there is none).
    int brandSize(const QString &brand) const { return int(m_brands.value(brand).codes.size()); }
    /// Credit line for the About screen.
    QString attribution() const { return m_attribution; }

    /// Loads an additional generic JSON file (same structure); later files win. Used by tests.
    bool loadFile(const QString &path);
    /// Loads a brand pack file. Used at start-up and by tests.
    bool loadBrandFile(const QString &path);

private:
    DtcDatabase();
    void fillFrom(DtcInfo &info, const QJsonObject &e) const;

    struct BrandPack {
        QString label;
        QHash<QString, QString> localizedLabels; ///< "ro" → label ("label_ro" in JSON)
        QString brandName;
        QJsonObject codes;
    };
    QJsonObject m_codes;
    QHash<QString, BrandPack> m_brands;
    QString m_attribution;
};

} // namespace obdaryn
