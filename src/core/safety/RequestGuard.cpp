// SPDX-License-Identifier: GPL-3.0-or-later

#include "safety/RequestGuard.h"

#include <QList>

namespace obdaryn {

namespace {

GuardVerdict allow() { return {true, {}}; }
GuardVerdict deny(const QString &why) { return {false, why}; }

bool isHex(const QByteArray &s)
{
    for (char c : s) {
        const bool digit = c >= '0' && c <= '9';
        const bool letter = c >= 'A' && c <= 'F';
        if (!digit && !letter)
            return false;
    }
    return true;
}

// AT commands accepted exactly as written (after normalization).
// Each one only configures how the adapter formats or reports data.
const QList<QByteArray> kExactAtCommands = {
    "Z",            // reset adapter
    "WS",           // warm start
    "D",            // restore adapter defaults
    "E0", "E1",     // echo off / on
    "L0", "L1",     // linefeeds off / on
    "S0", "S1",     // spaces off / on
    "H0", "H1",     // show CAN/KWP headers off / on
    "D0", "D1",     // show CAN data length off / on
    "AT0", "AT1", "AT2", // adaptive timing
    "CAF0", "CAF1", // CAN auto formatting off / on
    "CFC1",         // CAN flow control on (the default; never turn it off)
    "AR",           // automatic receive address
    "AL", "NL",     // allow / forbid long messages
    "DP", "DPN",    // describe protocol (text / number)
    "RV",           // read battery voltage at the OBD pin
    "I", "@1", "@2",// adapter identification (read only)
    "PC",           // close protocol
    "CRA",          // reset the receive-address filter
    // ISO-TP flow control. When a module answers with several CAN frames, the
    // adapter must reply "continue" (a flow-control frame) to the module's own
    // address. Brands whose answer ID is not request ID + 8 (VW group,
    // Renault/Dacia, GM) need the adapter told where to send it. Only the
    // standard frame content is allowed: 30 00 00 = continue, no block limit,
    // no delay, which is what the ELM327 sends by default anyway.
    "FCSM0",        // flow control: automatic (default)
    "FCSM1",        // flow control: user-defined header, fixed data below
    "FCSD300000",   // flow control data: exactly 30 00 00
};

// AT commands followed by a hex argument: prefix → allowed argument lengths.
struct ParamCommand {
    QByteArray prefix;
    QList<int> argLengths;
};
const QList<ParamCommand> kParamAtCommands = {
    {"SPA", {1}},          // set protocol, auto-search fallback (e.g. SPA6)
    {"SP", {1}},           // set protocol (0 = auto)
    {"TPA", {1}},          // try protocol, auto fallback
    {"TP", {1}},           // try protocol
    {"SH", {3, 6, 8}},     // set request header (11-bit, KWP, 29-bit)
    {"CRA", {3, 8}},       // only accept responses from this ID
    {"CP", {2}},           // CAN priority byte for 29-bit IDs
    {"CEA", {2}},          // CAN extended address
    {"ST", {2}},           // response timeout (x 4 ms)
    {"FCSH", {3, 8}},      // flow-control header (where "continue" frames go)
};

} // namespace

QByteArray RequestGuard::normalize(const QByteArray &command)
{
    QByteArray out;
    out.reserve(command.size());
    for (char c : command) {
        if (c == ' ' || c == '\t')
            continue;
        out.append((c >= 'a' && c <= 'z') ? char(c - 'a' + 'A') : c);
    }
    return out;
}

bool RequestGuard::isClearService(quint8 serviceId)
{
    return serviceId == 0x04 || serviceId == 0x14;
}

GuardVerdict RequestGuard::check(const QByteArray &command, bool userConfirmedClear)
{
    // Control characters inside a command would let one "command" smuggle in a
    // second one (the ELM327 executes each CR-terminated line).
    for (char c : command) {
        if (c == '\r' || c == '\n' || c == '\0')
            return deny(QStringLiteral("control characters are not allowed inside a command"));
    }

    const QByteArray n = normalize(command);
    if (n.isEmpty())
        return deny(QStringLiteral("empty line would repeat the previous command"));

    if (n.startsWith("AT"))
        return checkAdapterCommand(n.mid(2));
    return checkVehicleRequest(n, userConfirmedClear);
}

GuardVerdict RequestGuard::checkAdapterCommand(const QByteArray &body)
{
    if (kExactAtCommands.contains(body))
        return allow();

    for (const ParamCommand &pc : kParamAtCommands) {
        if (!body.startsWith(pc.prefix))
            continue;
        const QByteArray arg = body.mid(pc.prefix.size());
        if (isHex(arg) && pc.argLengths.contains(int(arg.size())))
            return allow();
    }
    return deny(QStringLiteral("adapter command AT %1 is not on the allowlist")
                    .arg(QString::fromLatin1(body)));
}

GuardVerdict RequestGuard::checkVehicleRequest(const QByteArray &n, bool userConfirmedClear)
{
    if (!isHex(n))
        return deny(QStringLiteral("vehicle requests must be hexadecimal"));

    // The ELM327 accepts one extra hex digit at the end: the number of
    // responses to wait for (e.g. "010C1"). Strip it before decoding bytes.
    QByteArray hex = n;
    if (hex.size() % 2 == 1)
        hex.chop(1);

    const QByteArray bytes = QByteArray::fromHex(hex);
    if (bytes.isEmpty())
        return deny(QStringLiteral("request has no service byte"));
    if (bytes.size() > 7)
        return deny(QStringLiteral("request longer than one CAN frame"));

    const quint8 sid = quint8(bytes[0]);
    const int len = int(bytes.size());

    switch (sid) {
    // --- OBD-II (SAE J1979) read services --------------------------------
    case 0x01: // current data
    case 0x02: // freeze frame data
    case 0x06: // on-board monitoring test results
    case 0x09: // vehicle information
        return len >= 2 ? allow() : deny(QStringLiteral("service needs a PID"));
    case 0x03: // stored DTCs
    case 0x07: // pending DTCs
    case 0x0A: // permanent DTCs
        return len == 1 ? allow() : deny(QStringLiteral("DTC request takes no parameters"));

    // --- UDS (ISO 14229) read services ------------------------------------
    case 0x19: // ReadDTCInformation — every sub-function is a report
        return len >= 2 ? allow() : deny(QStringLiteral("0x19 needs a sub-function"));
    case 0x22: // ReadDataByIdentifier — one or more 2-byte DIDs
        return (len >= 3 && (len - 1) % 2 == 0)
                   ? allow() : deny(QStringLiteral("0x22 needs 2-byte identifiers"));
    case 0x3E: { // TesterPresent — keep-alive only
        if (len == 1)
            return allow();
        const quint8 sub = quint8(bytes[1]);
        return (len == 2 && (sub == 0x00 || sub == 0x80))
                   ? allow() : deny(QStringLiteral("0x3E only with sub-function 00/80"));
    }

    // --- KWP2000 (ISO 14230) read services --------------------------------
    case 0x18: // ReadDiagnosticTroubleCodesByStatus
        return allow();
    case 0x1A: // ReadEcuIdentification
    case 0x21: // ReadDataByLocalIdentifier
        return len == 2 ? allow() : deny(QStringLiteral("service takes one identifier byte"));

    // --- Clearing (user-confirmed only) -----------------------------------
    case 0x04: // OBD-II clear emission DTCs (also resets readiness monitors)
        if (!userConfirmedClear)
            return deny(QStringLiteral("clearing codes needs explicit user confirmation"));
        return len == 1 ? allow() : deny(QStringLiteral("0x04 takes no parameters"));
    case 0x14: // UDS / KWP ClearDiagnosticInformation (a module's DTC memory)
        if (!userConfirmedClear)
            return deny(QStringLiteral("clearing codes needs explicit user confirmation"));
        return (len == 3 || len == 4)
                   ? allow() : deny(QStringLiteral("0x14 needs a DTC group"));

    default:
        // Everything else is refused: 0x08 on-board control, 0x10 session
        // control, 0x11 ECU reset, 0x23 read memory by address, 0x27 security
        // access, 0x28 communication control, 0x2E/0x3B writes, 0x2F/0x30 IO
        // control, 0x31 routines, 0x34–0x37 downloads, 0x85 DTC setting, ...
        return deny(QStringLiteral("service 0x%1 is not allowed (read-only tool)")
                        .arg(sid, 2, 16, QLatin1Char('0')).toUpper());
    }
}

} // namespace obdaryn
