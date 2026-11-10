// SPDX-License-Identifier: GPL-3.0-or-later
//
// Diagnostic Trouble Codes (DTCs): decoding the 2-byte form into "P0420".

#pragma once

#include "obd/ResponseParser.h"

#include <QString>
#include <QStringList>

namespace obdaryn {

/// Which memory a code was read from.
enum class DtcKind {
    Stored,    ///< service 03: confirmed fault, usually turns the check-engine light on
    Pending,   ///< service 07: seen once, will be stored if it happens again
    Permanent, ///< service 0A: cannot be cleared by a scan tool; disappears by
               ///< itself after the car re-checks the fault and finds it fixed
};

struct Dtc {
    QString code;   ///< e.g. "P0420"
    QString ecu;    ///< address of the module that reported it, e.g. "7E8"
    DtcKind kind = DtcKind::Stored;
};

namespace DtcCodec {

/**
 * @brief Converts the two raw bytes of a DTC to text.
 *
 * Bits 15–14 select the letter (P powertrain, C chassis, B body, U network),
 * bits 13–12 the first digit, and the remaining three nibbles are hex digits:
 * 0x04 0x20 → "P0420", 0xC1 0x23 → "U0123".
 */
QString decode(quint8 a, quint8 b);

/**
 * @brief Extracts all codes from a positive answer to service 03, 07 or 0A.
 *
 * On CAN the answer is: SID+0x40, count, then 2 bytes per code.
 * On older protocols each frame is: SID+0x40 and 6 bytes (3 codes), padded with 00 00.
 */
QStringList fromResponse(const EcuMessage &message, bool can);

/// True for manufacturer-specific codes (P1xxx, P30xx–P33xx, B1/B2, C1/C2, U1/U2).
bool isManufacturerSpecific(const QString &code);

/// Translated name of the system a code belongs to, e.g. "Ignition system or misfire".
QString systemName(const QString &code);

} // namespace DtcCodec

} // namespace obdaryn
