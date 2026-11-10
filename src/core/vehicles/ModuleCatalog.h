// SPDX-License-Identifier: GPL-3.0-or-later
//
// Which control modules a car has, and how to address them, per brand.

#pragma once

#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

namespace obdaryn {

/// Diagnostic protocol a module speaks for reading trouble codes.
enum class DiagProtocol {
    Auto, ///< try UDS (0x19) first, then KWP2000 (0x18)
    Uds,  ///< ISO 14229: 0x19 ReadDTCInformation, 0x22 ReadDataByIdentifier
    Kwp,  ///< ISO 14230 KWP2000 on CAN: 0x18 ReadDTCByStatus, 0x1A ReadEcuIdentification
};

/**
 * @brief One control module (ECU) and its CAN addresses.
 *
 * Modules other than the engine don't answer generic OBD-II requests sent to
 * everyone (7DF). Each must be asked directly: the adapter sends to the
 * module's request ID ("tx") and listens for its response ID ("rx").
 */
struct ModuleDef {
    QString id;          ///< stable key, e.g. "hk-abs"
    QString category;    ///< engine, transmission, abs, airbag, steering, body, cluster, ...
    quint16 tx = 0;      ///< 11-bit request ID, e.g. 0x7D1
    quint16 rx = 0;      ///< 11-bit response ID, e.g. 0x7D9
    DiagProtocol dtc = DiagProtocol::Auto;
    bool verified = false; ///< address confirmed on real cars of this brand
    QString source;      ///< where the address comes from

    /// Translated name, e.g. "ABS / stability control".
    QString displayName() const;
    /// "7D1 → 7D9"
    QString addressText() const;
    /// Most brands answer on request ID + 8 and the ELM327 handles flow control
    /// itself; other offsets need the flow-control header set explicitly.
    bool needsFlowControlHeader() const { return rx != tx + 8; }
};

/// A brand's list of modules.
struct VehicleProfile {
    QString id;          ///< "hyundai-kia", "generic", ...
    QString label;       ///< "Hyundai / Kia / Genesis"
    QString notes;       ///< caveats shown in the UI (English)
    QHash<QString, QString> localizedNotes; ///< "ro" → notes in that language ("notes_ro" in JSON)
    /// Notes in the given language, falling back to English.
    QString notesFor(const QString &language) const { return localizedNotes.value(language, notes); }
    QStringList makes;   ///< manufacturer names as decoded from the VIN
    QStringList wmi;     ///< VIN prefixes (2–3 characters)
    QList<ModuleDef> modules;
};

/**
 * @brief The built-in brand profiles (one JSON file per brand in data/vehicles).
 *
 * Profiles are plain JSON so contributors can add a brand without touching
 * C++. Addresses marked "verified": false come from community lists and are
 * shown as such in the UI until someone confirms them on a real car.
 */
class ModuleCatalog
{
public:
    static const QList<VehicleProfile> &profiles();
    static const VehicleProfile *find(const QString &id);
    /// Best profile for a car: by VIN prefix, then by manufacturer name, else "generic".
    static const VehicleProfile &forVehicle(const QString &manufacturer, const QString &vin);
    /// Translated name of a module category.
    static QString categoryName(const QString &category);
    /// Parses one profile (used by the loader and by tests). Empty id on error.
    static VehicleProfile parse(const QByteArray &json);
};

} // namespace obdaryn
