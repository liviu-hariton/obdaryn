// SPDX-License-Identifier: GPL-3.0-or-later
//
// Vehicle bus protocols, numbered exactly like the ELM327 does (AT SP / AT DPN).

#pragma once

#include <QString>

namespace obdaryn {

/**
 * @brief The bus protocols an ELM327 adapter can speak to a car.
 *
 * The numbers match the ELM327 protocol numbers, so the reply to "AT DPN"
 * can be converted directly with protocolFromDpn().
 *
 * In practice: every car sold in the EU since ~2008 (and the US since 2008)
 * uses CAN (6–9). The older protocols (1–5) are kept for 2001–2008 cars.
 */
enum class Protocol : int {
    Unknown   = -1,
    Auto      = 0,   ///< adapter is still searching
    J1850Pwm  = 1,   ///< SAE J1850 PWM 41.6 kbaud (older Ford)
    J1850Vpw  = 2,   ///< SAE J1850 VPW 10.4 kbaud (older GM)
    Iso9141   = 3,   ///< ISO 9141-2 (older European / Asian cars)
    Kwp5Baud  = 4,   ///< ISO 14230-4 KWP2000, 5-baud init
    KwpFast   = 5,   ///< ISO 14230-4 KWP2000, fast init
    Can11_500 = 6,   ///< ISO 15765-4 CAN, 11-bit IDs, 500 kbit/s (most modern cars)
    Can29_500 = 7,   ///< ISO 15765-4 CAN, 29-bit IDs, 500 kbit/s
    Can11_250 = 8,   ///< ISO 15765-4 CAN, 11-bit IDs, 250 kbit/s
    Can29_250 = 9,   ///< ISO 15765-4 CAN, 29-bit IDs, 250 kbit/s
    J1939     = 10,  ///< SAE J1939 (heavy trucks) — "A"
    UserCan1  = 11,  ///< user-defined CAN — "B"
    UserCan2  = 12,  ///< user-defined CAN — "C"
};

/// True for every CAN-based protocol (ISO 15765-4, J1939, user CAN).
bool isCan(Protocol p);

/// True when the protocol uses 29-bit (extended) CAN identifiers.
bool uses29BitIds(Protocol p);

/**
 * @brief Parses the reply to "AT DPN".
 *
 * The ELM327 answers with one hex digit (1–C), prefixed with "A" when the
 * protocol was found automatically, e.g. "A6" → Protocol::Can11_500.
 */
Protocol protocolFromDpn(const QString &reply);

/// Human-readable, translated protocol name for the UI.
QString protocolDisplayName(Protocol p);

} // namespace obdaryn
