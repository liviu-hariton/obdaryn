// SPDX-License-Identifier: GPL-3.0-or-later
//
// The single gatekeeper between Obdaryn and the vehicle.

#pragma once

#include <QByteArray>
#include <QString>

namespace obdaryn {

/// Outcome of a safety check on one outgoing adapter command.
struct GuardVerdict {
    bool allowed = false;  ///< true when the command may be sent
    QString reason;        ///< short English explanation, written to the traffic log
};

/**
 * @brief Decides whether a command may be sent to the ELM327.
 *
 * Obdaryn is a read-only tool. It reads diagnostics and, only after the user
 * explicitly confirms, clears trouble codes. Anything that could change how a
 * vehicle module behaves — writes, coding, adaptations, routines, actuator
 * tests, security access, ECU resets, session changes or flashing — is rejected
 * here, no matter which part of the app asks. ElmSession calls check() for
 * every single line it writes, so this class is the one place to audit.
 *
 * Allowed vehicle requests (first byte = service ID):
 *   OBD-II (SAE J1979):  01 live data, 02 freeze frame, 03 / 07 / 0A trouble
 *                        codes, 06 on-board monitor results, 09 vehicle info
 *   UDS (ISO 14229):     19 ReadDTCInformation, 22 ReadDataByIdentifier,
 *                        3E TesterPresent (sub-function 00 / 80 only)
 *   KWP2000 (ISO 14230): 18 ReadDTCByStatus, 1A ReadEcuIdentification,
 *                        21 ReadDataByLocalIdentifier
 *   Clearing — only when userConfirmedClear is true:
 *                        04 (OBD-II clear), 14 (UDS/KWP ClearDiagnosticInformation)
 *
 * Adapter (AT) commands are allowlisted too, because a few of them can put
 * arbitrary bytes on the bus (AT WM, AT FC SD, AT RTR) or permanently change
 * the adapter (AT @3, AT PP ...). An empty line is refused as well: the ELM327
 * repeats the previous command when it receives a bare carriage return, which
 * could silently repeat a "clear codes" request.
 */
class RequestGuard
{
public:
    /**
     * @param command            the text that would be sent, without the trailing CR,
     *                           e.g. "ATZ", "AT SH 7E0", "010C", "22 F1 90".
     * @param userConfirmedClear true only for a clear request the user confirmed.
     */
    static GuardVerdict check(const QByteArray &command, bool userConfirmedClear = false);

    /// True for the services that erase trouble codes (0x04, 0x14).
    static bool isClearService(quint8 serviceId);

    /// Uppercases and removes spaces, e.g. "at sh 7e0" → "ATSH7E0".
    static QByteArray normalize(const QByteArray &command);

private:
    static GuardVerdict checkAdapterCommand(const QByteArray &normalized);
    static GuardVerdict checkVehicleRequest(const QByteArray &normalized, bool userConfirmedClear);
};

} // namespace obdaryn
