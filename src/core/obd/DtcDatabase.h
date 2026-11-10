// SPDX-License-Identifier: GPL-3.0-or-later
//
// Plain-language explanations for trouble codes.

#pragma once

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
    QString repairDifficulty;    ///< "easy" / "medium" / "hard" (may be empty)
    bool diyPossible = false;
    int costMinEur = 0, costMaxEur = 0; ///< rough repair cost range, 0 = unknown
    QStringList related;
};

/**
 * @brief Looks up code descriptions.
 *
 * Generic codes come from the obdex database (CC0-1.0, independently written —
 * not SAE J2012 text), compiled into the app from data/dtc_generic.json; see
 * tools/import_obdex.py. Manufacturer codes (P1xxx, B1xxx, ...) differ per
 * brand; brand packs will be added as separate files later.
 *
 * The descriptions are English for now. Translations can be layered on top
 * as data/dtc_generic_<lang>.json with the same structure.
 */
class DtcDatabase
{
public:
    static DtcDatabase &instance();

    DtcInfo lookup(const QString &code) const;
    int size() const { return int(m_codes.size()); }
    /// Credit line for the About screen.
    QString attribution() const { return m_attribution; }

    /// Loads an additional JSON file (same structure); later files win. Used by tests.
    bool loadFile(const QString &path);

private:
    DtcDatabase();
    QJsonObject m_codes;
    QString m_attribution;
};

} // namespace obdaryn
