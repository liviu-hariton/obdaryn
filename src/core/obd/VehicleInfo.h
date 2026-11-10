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
};

/// Decodes a VIN. The model year is the most recent year matching the code
/// that is not after next year; European VINs do not always encode it.
VinInfo decodeVin(const QString &vin);

/// Parsers for service 09 answers. Each accepts the message from one ECU.
namespace VehicleInfoCodec {
QString vin(const EcuMessage &m, bool can);              ///< PID 02
QStringList calibrationIds(const EcuMessage &m, bool can); ///< PID 04, 16 chars each
QStringList cvns(const EcuMessage &m, bool can);           ///< PID 06, 4 bytes each, as hex
QString ecuName(const EcuMessage &m, bool can);           ///< PID 0A, "ECM-EngineControl"
}

} // namespace obdaryn
