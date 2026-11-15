// SPDX-License-Identifier: GPL-3.0-or-later
//
// Service 09 (vehicle information) and VIN decoding.

#pragma once

#include "obd/ResponseParser.h"

#include <QString>
#include <QStringList>

namespace obdaryn {

/// What can be learned from the 17-character VIN alone.
struct VinInfo {
    QString vin;
    bool valid = false;            ///< 17 chars, no I / O / Q
    QString wmi;                   ///< World Manufacturer Identifier (first 3 chars)
    QString manufacturer;          ///< e.g. "Hyundai", empty if unknown
    QString region;                ///< translated, e.g. "South Korea"
    int modelYear = 0;             ///< from the 10th character; 0 if not encoded
    bool checkDigitValid = false;  ///< 9th character check (mandatory in North America only)
    /// Whether a wrong check digit means something: true for VINs from North
    /// America (1-5) and China (L), where it is mandatory. Elsewhere the 9th
    /// character is often a letter or a factory code, so a mismatch is normal.
    bool checkDigitRequired = false;
};

/// Decodes a VIN. The model year is the most recent year matching the code
/// that is not after next year; European VINs do not always encode it.
VinInfo decodeVin(const QString &vin);

/**
 * @brief In-use performance tracking (service 09, PID 08 petrol / 0B diesel).
 *
 * For each emission monitor the engine computer counts how often the car was
 * driven in conditions where the monitor could run ("conditions") and how
 * often it actually finished ("completed"). The ratio shows whether a monitor
 * runs as often as the law expects.
 *
 * The counters survive clearing codes (service 04): they reset only when the
 * engine computer loses its memory or is reprogrammed. Low counters next to a
 * high mileage therefore hint at a recent battery disconnect or reflash, which
 * is worth knowing on a car for sale.
 */
struct InUsePerformance {
    bool available = false;
    bool compressionIgnition = false; ///< diesel layout (PID 0B)
    QString ecu;                      ///< which module answered, e.g. "7E8"
    int obdConditions = -1;           ///< times the general monitoring conditions were met (OBDCOND)
    int ignitionCycles = -1;          ///< engine starts counted by the ECU (IGNCNTR)
    struct Monitor {
        QString name;                 ///< translated, e.g. "Catalyst, bank 1"
        int completed = 0;
        int conditions = 0;
        /// completed / conditions, or -1 when the conditions were never met.
        double ratio() const { return conditions > 0 ? double(completed) / conditions : -1.0; }
        /// Both counters zero: usually a monitor this car doesn't have (bank 2, secondary air).
        bool unused() const { return completed == 0 && conditions == 0; }
    };
    QList<Monitor> monitors;
};

/// Parsers for service 09 answers. Each accepts the message from one ECU.
namespace VehicleInfoCodec {
QString vin(const EcuMessage &m, bool can);              ///< PID 02
QStringList calibrationIds(const EcuMessage &m, bool can); ///< PID 04, 16 chars each
QStringList cvns(const EcuMessage &m, bool can);           ///< PID 06, 4 bytes each, as hex
QString ecuName(const EcuMessage &m, bool can);           ///< PID 0A, "ECM-EngineControl"
/// PID 08 (pid = 0x08, petrol) or 0B (diesel). available is false when the answer is too short.
InUsePerformance performanceTracking(const EcuMessage &m, bool can, quint8 pid);
}

} // namespace obdaryn
