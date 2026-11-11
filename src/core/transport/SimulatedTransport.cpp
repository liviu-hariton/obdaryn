// SPDX-License-Identifier: GPL-3.0-or-later

#include "transport/SimulatedTransport.h"

#include "safety/RequestGuard.h"

#include <QSet>
#include <QTimer>

#include <algorithm>
#include <cmath>

namespace obdaryn {

namespace {

// PIDs the simulated engine ECU supports in service 01 (current data).
const QSet<quint8> kEnginePids = {
    0x01, 0x03, 0x04, 0x05, 0x06, 0x07, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x11,
    0x13, 0x14, 0x15, 0x1C, 0x1F, 0x21, 0x2E, 0x2F, 0x30, 0x31, 0x33, 0x3C,
    0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x49, 0x4A, 0x4C, 0x51,
};

// PIDs available in the freeze frame (service 02).
const QSet<quint8> kFreezePids = {
    0x02, 0x04, 0x05, 0x06, 0x07, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x11,
};

// The transmission only knows PID 01 (its own monitor status).
const QSet<quint8> kTransmissionPids = {0x01};

const QByteArray kVin = "KMHDH41EBEU000001"; // 10th character 'E' = model year 2014

/// Builds the 4-byte "supported PIDs" bitmap for PIDs base+1 .. base+32.
QByteArray supportedMask(const QSet<quint8> &pids, quint8 base)
{
    QByteArray mask(4, '\0');
    bool hasHigher = false;
    for (quint8 pid : pids) {
        if (pid > base + 0x20)
            hasHigher = true;
        if (pid <= base || pid > base + 0x20)
            continue;
        const int index = pid - base - 1; // 0..31
        mask[index / 8] = char(quint8(mask[index / 8]) | (0x80 >> (index % 8)));
    }
    // The last bit says "the next range (base+0x20) is supported too".
    if (hasHigher)
        mask[3] = char(quint8(mask[3]) | 0x01);
    return mask;
}

QByteArray u16(int value)
{
    value = std::clamp(value, 0, 0xFFFF);
    QByteArray b;
    b.append(char((value >> 8) & 0xFF));
    b.append(char(value & 0xFF));
    return b;
}

QByteArray u8(double value)
{
    return QByteArray(1, char(std::clamp(int(std::lround(value)), 0, 255)));
}

QByteArray percent(double pct) { return u8(pct * 255.0 / 100.0); }

QByteArray dtcBytes(const QList<quint16> &codes)
{
    QByteArray b;
    for (quint16 c : codes)
        b.append(u16(c));
    return b;
}

/// ASCII text padded with zero bytes to a fixed length.
QByteArray padded(const QByteArray &text, int length)
{
    QByteArray b = text.left(length);
    b.append(QByteArray(length - b.size(), '\0'));
    return b;
}

} // namespace

SimulatedTransport::SimulatedTransport(QObject *parent) : Transport(parent) {}

QString SimulatedTransport::description() const
{
    return tr("Built-in demo car");
}

void SimulatedTransport::open()
{
    m_clock.start();
    resetAdapterSettings();
    QTimer::singleShot(m_latencyMs, this, [this] {
        m_open = true;
        emit opened();
    });
}

void SimulatedTransport::close()
{
    if (!m_open)
        return;
    m_open = false;
    emit closed();
}

void SimulatedTransport::resetAdapterSettings()
{
    m_echo = true;
    m_headers = false;
    m_spaces = true;
    m_linefeeds = false;
    m_protocolFound = false;
    m_header = "7DF";
}

double SimulatedTransport::seconds() const
{
    return m_clock.isValid() ? m_clock.elapsed() / 1000.0 : 0.0;
}

void SimulatedTransport::write(const QByteArray &data)
{
    if (!m_open || m_asleep)
        return;
    m_rxBuffer.append(data);

    // The ELM327 executes one command per carriage return.
    qsizetype cr;
    while ((cr = m_rxBuffer.indexOf('\r')) >= 0) {
        const QByteArray line = m_rxBuffer.left(cr);
        m_rxBuffer.remove(0, cr + 1);
        const QByteArray reply = handleLine(line);

        QTimer::singleShot(m_latencyMs, this, [this, reply] {
            if (!m_open)
                return;
            // Deliver in two chunks: real Wi-Fi adapters split replies at
            // arbitrary points, and the session must cope with that.
            const qsizetype half = reply.size() / 2;
            emit received(reply.left(half));
            emit received(reply.mid(half));
        });
    }
}

QByteArray SimulatedTransport::handleLine(const QByteArray &line)
{
    const QByteArray eol = m_linefeeds ? "\r\n" : "\r";
    QByteArray out;
    if (m_echo)
        out += line + eol;

    const QByteArray n = RequestGuard::normalize(line.trimmed());
    QByteArray body;
    if (n.isEmpty())
        body = "?";
    else if (n.startsWith("AT"))
        body = handleAt(n.mid(2));
    else
        body = handleObd(n);

    body.replace("\r", eol);
    out += body + eol + eol + ">";
    return out;
}

QByteArray SimulatedTransport::handleAt(const QByteArray &body)
{
    auto flag = [&](const char *prefix, bool &target) -> bool {
        if (body == QByteArray(prefix) + "0") { target = false; return true; }
        if (body == QByteArray(prefix) + "1") { target = true;  return true; }
        return false;
    };

    if (body == "Z" || body == "WS" || body == "D") {
        resetAdapterSettings();
        return body == "D" ? "OK" : "\rELM327 v1.5";
    }
    if (flag("E", m_echo) || flag("L", m_linefeeds) || flag("S", m_spaces) || flag("H", m_headers))
        return "OK";
    if (body == "I")
        return "ELM327 v1.5";
    if (body == "@1")
        return "OBDII to RS232 Interpreter";
    // Like many "v1.5" clones: some v1.4 read commands work, others answer "?".
    if (body == "IGN")
        return m_carAnswering ? "ON" : "OFF";
    if (body == "CS")
        return "T:00 R:00";
    if (body == "@2" || body == "PPS" || body == "KW")
        return "?";
    if (body == "RV") {
        const double v = m_engineRunning ? 14.2 + 0.05 * std::sin(seconds()) : 12.4;
        return QByteArray::number(v, 'f', 1) + "V";
    }
    if (body == "DPN")
        return m_protocolFound ? "A6" : "0";
    if (body == "DP")
        return m_protocolFound ? "AUTO, ISO 15765-4 (CAN 11/500)" : "AUTO";
    if (body.startsWith("SH")) {
        m_header = body.mid(2);
        return "OK";
    }
    static const QList<QByteArray> okPrefixes = {
        "SP", "TP", "AT", "CAF", "CFC", "ST", "CRA", "D0", "D1", "AL", "NL", "PC", "AR", "CP", "CEA", "FC",
    };
    for (const QByteArray &p : okPrefixes) {
        if (body.startsWith(p))
            return "OK";
    }
    return "?";
}

QByteArray SimulatedTransport::handleObd(const QByteArray &normalized)
{
    QByteArray hex = normalized;
    if (hex.size() % 2 == 1) {
        if (m_rejectsResponseCounts)
            return "?";
        hex.chop(1); // optional response-count digit
    }
    const QByteArray req = QByteArray::fromHex(hex);
    if (req.isEmpty())
        return "?";

    QByteArray prefix;
    if (!m_protocolFound) {
        prefix = "SEARCHING...\r"; // the first request triggers the protocol search
        m_protocolFound = true;
    }

    if (!m_carAnswering)
        return prefix + "NO DATA"; // ignition off: the adapter answers, the car doesn't

    QList<EcuReply> replies;
    if (m_header == "7DF") {
        // Functional OBD-II request: every emission-related ECU may answer.
        replies += engineReply(req);
        replies += transmissionReply(req);
    } else {
        // Physical request to one module (module scan).
        replies += moduleReply(m_header, req);
    }

    if (replies.isEmpty())
        return prefix + "NO DATA";

    QByteArray out = prefix;
    for (int i = 0; i < replies.size(); ++i) {
        if (i > 0)
            out += "\r";
        out += formatFrames(replies[i]);
    }
    return out;
}

QList<SimulatedTransport::EcuReply> SimulatedTransport::engineReply(const QByteArray &req)
{
    const QString ecu = QStringLiteral("7E8");
    const quint8 sid = quint8(req[0]);
    auto reply = [&](const QByteArray &payload) { return QList<EcuReply>{{ecu, payload}}; };
    auto negative = [&](quint8 nrc) {
        return reply(QByteArray("\x7F", 1) + char(sid) + char(nrc));
    };

    switch (sid) {
    case 0x01: { // current data, possibly several PIDs in one request
        QByteArray payload(1, char(0x41));
        for (int i = 1; i < req.size(); ++i) {
            const quint8 pid = quint8(req[i]);
            if (pid % 0x20 == 0) {
                payload += char(pid) + supportedMask(kEnginePids, pid);
            } else if (kEnginePids.contains(pid)) {
                payload += char(pid) + pidData(pid);
            }
        }
        return payload.size() > 1 ? reply(payload) : QList<EcuReply>{};
    }
    case 0x02: { // freeze frame: 02 <pid> <frame>
        if (req.size() < 3 || quint8(req[2]) != 0x00 || m_stored.isEmpty())
            return {};
        const quint8 pid = quint8(req[1]);
        QByteArray data;
        if (pid % 0x20 == 0)
            data = supportedMask(kFreezePids, pid);
        else if (kFreezePids.contains(pid))
            data = freezeFrameData(pid);
        if (data.isEmpty())
            return {};
        return reply(QByteArray(1, char(0x42)) + char(pid) + char(0x00) + data);
    }
    case 0x03:
        return reply(QByteArray(1, char(0x43)) + char(m_stored.size()) + dtcBytes(m_stored));
    case 0x07:
        return reply(QByteArray(1, char(0x47)) + char(m_pending.size()) + dtcBytes(m_pending));
    case 0x0A:
        return reply(QByteArray(1, char(0x4A)) + char(m_permanent.size()) + dtcBytes(m_permanent));
    case 0x04:
        if (m_engineRunning)
            return negative(0x22); // conditionsNotCorrect: engine must be off
        m_stored.clear();
        m_pending.clear();
        m_monitorsIncomplete = 0xE5; // every supported monitor back to "not complete"
        return reply(QByteArray(1, char(0x44)));
    case 0x09: {
        if (req.size() < 2)
            return negative(0x12);
        const quint8 pid = quint8(req[1]);
        QByteArray head = QByteArray(1, char(0x49)) + char(pid);
        switch (pid) {
        case 0x00: return reply(head + supportedMask({0x02, 0x04, 0x06, 0x0A}, 0x00));
        case 0x02: return reply(head + char(0x01) + kVin);
        case 0x04: return reply(head + char(0x01) + padded("6AG1EU3B2KFA1000", 16));
        case 0x06: return reply(head + char(0x01) + QByteArray::fromHex("3A1FC207"));
        case 0x0A: return reply(head + char(0x01) + padded(QByteArray("ECM\0-EngineControl", 18), 20));
        default:   return {};
        }
    }
    default:
        return negative(0x11); // serviceNotSupported
    }
}

QList<SimulatedTransport::EcuReply> SimulatedTransport::transmissionReply(const QByteArray &req)
{
    const QString ecu = QStringLiteral("7E9");
    const quint8 sid = quint8(req[0]);
    auto reply = [&](const QByteArray &payload) { return QList<EcuReply>{{ecu, payload}}; };

    switch (sid) {
    case 0x01: {
        QByteArray payload(1, char(0x41));
        for (int i = 1; i < req.size(); ++i) {
            const quint8 pid = quint8(req[i]);
            if (pid == 0x00)
                payload += char(pid) + supportedMask(kTransmissionPids, pid);
            else if (pid == 0x01)
                payload += char(pid) + QByteArray::fromHex("00000000");
        }
        return payload.size() > 1 ? reply(payload) : QList<EcuReply>{};
    }
    case 0x03: return reply(QByteArray::fromHex("4300"));
    case 0x07: return reply(QByteArray::fromHex("4700"));
    case 0x0A: return reply(QByteArray::fromHex("4A00"));
    case 0x04: return m_engineRunning ? QList<EcuReply>{} : reply(QByteArray::fromHex("44"));
    case 0x09:
        if (req.size() >= 2 && quint8(req[1]) == 0x00)
            return reply(QByteArray::fromHex("4900") + supportedMask({0x0A}, 0x00));
        if (req.size() >= 2 && quint8(req[1]) == 0x0A)
            return reply(QByteArray::fromHex("490A01")
                         + padded(QByteArray("TCM\0-TransmisionCtl", 19), 20));
        return {};
    default:
        return {};
    }
}

QList<SimulatedTransport::EcuReply> SimulatedTransport::moduleReply(const QByteArray &header, const QByteArray &req)
{
    const quint8 sid = quint8(req[0]);
    auto reply = [](const char *ecu, const QByteArray &payload) {
        return QList<EcuReply>{{QString::fromLatin1(ecu), payload}};
    };
    auto negative = [&](const char *ecu, quint8 nrc) {
        return reply(ecu, QByteArray("\x7F", 1) + char(sid) + char(nrc));
    };
    auto kwpDtcs = [](const QList<QPair<quint16, quint8>> &codes) {
        QByteArray p = QByteArray(1, char(0x58)) + char(codes.size());
        for (const auto &c : codes)
            p += u16(c.first) + char(c.second);
        return p;
    };
    const bool clearAll = (sid == 0x14);

    if (header == "7E0") { // engine, KWP2000 on CAN like a 2014 Hyundai
        switch (sid) {
        case 0x18: {
            QList<QPair<quint16, quint8>> codes;
            for (quint16 c : m_stored) codes << qMakePair(c, quint8(0xE0));   // present now, MIL on
            for (quint16 c : m_pending) codes << qMakePair(c, quint8(0x40));  // maturing
            return reply("7E8", kwpDtcs(codes));
        }
        case 0x1A: return reply("7E8", QByteArray::fromHex("5A87") + "39101-2B330");
        case 0x14:
            if (m_engineRunning) return negative("7E8", 0x22);
            m_stored.clear(); m_pending.clear(); m_monitorsIncomplete = 0xE5;
            return reply("7E8", QByteArray::fromHex("54FF00"));
        case 0x19: return negative("7E8", 0x11);
        default:   return engineReply(req);
        }
    }
    if (header == "7E1") { // transmission, KWP2000
        switch (sid) {
        case 0x18: return reply("7E9", kwpDtcs({}));
        case 0x1A: return reply("7E9", QByteArray::fromHex("5A87") + "95440-26050");
        case 0x14: return reply("7E9", QByteArray::fromHex("54FF00"));
        case 0x19: return negative("7E9", 0x11);
        default:   return transmissionReply(req);
        }
    }
    if (header == "7D1") { // ABS / ESC, KWP2000
        switch (sid) {
        // C1203: front-right wheel-speed sensor open/short (Hyundai-specific), present now.
        case 0x18: return reply("7D9", kwpDtcs(m_absFault ? QList<QPair<quint16, quint8>>{{0x5203, 0xE0}}
                                                         : QList<QPair<quint16, quint8>>{}));
        case 0x1A: return reply("7D9", QByteArray::fromHex("5A87") + "58920-3X000");
        case 0x14:
            if (clearAll) m_absFault = false; // a real active fault would come straight back
            return reply("7D9", QByteArray::fromHex("54FF00"));
        default:   return negative("7D9", 0x11);
        }
    }
    if (header == "7D2") { // airbag, UDS
        switch (sid) {
        case 0x19:
            // 59 02 <mask> then B1346 (driver airbag stage 1, resistance too high: the
            // classic Elantra MD clock-spring fault), failure type 00, status 0x88.
            return reply("7DA", QByteArray::fromHex("5902FF")
                                    + (m_airbagFault ? QByteArray::fromHex("93460088") : QByteArray()));
        case 0x22:
            if (req.size() >= 3 && quint8(req[1]) == 0xF1 && quint8(req[2]) == 0x87)
                return reply("7DA", QByteArray::fromHex("62F187") + "95910-3X100");
            if (req.size() >= 3 && quint8(req[1]) == 0xF1 && quint8(req[2]) == 0x89)
                return reply("7DA", QByteArray::fromHex("62F189") + "1.04");
            return negative("7DA", 0x31);
        case 0x14:
            m_airbagFault = false;
            return reply("7DA", QByteArray::fromHex("54"));
        default:   return negative("7DA", 0x11);
        }
    }
    if (header == "7A0") { // body control, KWP2000, no codes
        switch (sid) {
        case 0x18: return reply("7A8", kwpDtcs({}));
        case 0x1A: return reply("7A8", QByteArray::fromHex("5A87") + "95400-3X010");
        default:   return negative("7A8", 0x11);
        }
    }
    if (header == "7C6") { // instrument cluster, UDS, no codes
        switch (sid) {
        case 0x19: return reply("7CE", QByteArray::fromHex("5902FF"));
        case 0x22:
            if (req.size() >= 3 && quint8(req[2]) == 0x87) return reply("7CE", QByteArray::fromHex("62F187") + "94011-3X120");
            if (req.size() >= 3 && quint8(req[2]) == 0x89) return reply("7CE", QByteArray::fromHex("62F189") + "2.00");
            return negative("7CE", 0x31);
        default:   return negative("7CE", 0x11);
        }
    }
    if (header == "7D6") { // TPMS: answers keep-alive, ignores code requests (seen on real modules)
        if (sid == 0x3E)
            return reply("7DE", QByteArray::fromHex("7E00"));
        return {};
    }
    return {}; // steering, climate...: not fitted in the demo car
}

QByteArray SimulatedTransport::joinBytes(const QByteArray &bytes) const
{
    // toHex('\0') means "no separator".
    return bytes.toHex(m_spaces ? ' ' : '\0').toUpper();
}

QByteArray SimulatedTransport::formatFrames(const EcuReply &reply) const
{
    const QByteArray &p = reply.payload;
    const QByteArray sep = m_spaces ? " " : "";
    QList<QByteArray> lines;

    if (p.size() <= 7) {
        // ISO-TP single frame: PCI byte = length, padded to 8 bytes.
        if (m_headers) {
            QByteArray frame = QByteArray(1, char(p.size())) + p;
            frame.append(QByteArray(8 - frame.size(), char(0xAA)));
            lines << reply.header.toLatin1() + sep + joinBytes(frame);
        } else {
            lines << joinBytes(p);
        }
    } else {
        // ISO-TP first frame + consecutive frames.
        QList<QByteArray> chunks;
        chunks << p.left(6);
        for (qsizetype i = 6; i < p.size(); i += 7)
            chunks << p.mid(i, 7);

        if (m_headers) {
            QByteArray first = QByteArray(1, char(0x10 | ((p.size() >> 8) & 0x0F)));
            first += char(p.size() & 0xFF);
            first += chunks[0];
            lines << reply.header.toLatin1() + sep + joinBytes(first);
            for (int i = 1; i < chunks.size(); ++i) {
                QByteArray cf = QByteArray(1, char(0x20 | (i & 0x0F))) + chunks[i];
                cf.append(QByteArray(8 - cf.size(), char(0xAA)));
                lines << reply.header.toLatin1() + sep + joinBytes(cf);
            }
        } else {
            // Without headers the ELM327 prints the length, then "n: data".
            lines << QByteArray::number(int(p.size()), 16).toUpper().rightJustified(3, '0');
            for (int i = 0; i < chunks.size(); ++i)
                lines << QByteArray::number(i & 0x0F, 16).toUpper() + ":" + sep + joinBytes(chunks[i]);
        }
    }
    return lines.join("\r");
}

QByteArray SimulatedTransport::pidData(quint8 pid) const
{
    const double t = seconds();
    const bool on = m_engineRunning;
    // A slow "driving" wave between 0 and 1 that drives rpm, load, throttle...
    const double drive = on ? 0.5 + 0.5 * std::sin(t * 0.45) : 0.0;
    const double rpm = on ? 780.0 + 2400.0 * drive * drive : 0.0;
    const double speed = on ? std::max(0.0, 75.0 * std::sin(t * 0.08)) : 0.0;
    const double coolant = on ? std::min(89.0, 38.0 + t * 0.9) : 31.0;

    switch (pid) {
    case 0x01: {
        const bool mil = !m_stored.isEmpty();
        QByteArray d;
        d += char((mil ? 0x80 : 0x00) | std::min<int>(int(m_stored.size()), 0x7F));
        d += char(0x07);               // misfire / fuel / components monitors available & complete
        d += char(0xE5);               // catalyst, evap, O2 sensor, O2 heater, EGR/VVT available
        d += char(m_monitorsIncomplete);
        return d;
    }
    case 0x03: return QByteArray(1, char(on ? 0x02 : 0x01)) + char(0x00); // closed / open loop
    case 0x04: return percent(on ? 15 + 45 * drive : 0);
    case 0x05: return u8(coolant + 40);
    case 0x06: return u8(128 + (on ? 4.0 * std::sin(t * 1.7) : 0) * 1.28);
    case 0x07: return u8(128 + 3.1 * 1.28);
    case 0x0B: return u8(on ? 30 + 55 * drive : 101);
    case 0x0C: return u16(int(rpm * 4));
    case 0x0D: return u8(speed);
    case 0x0E: return u8(((on ? 6 + 18 * drive : 0) + 64) * 2);
    case 0x0F: return u8(27 + 40);
    case 0x11: return percent(on ? 13 + 50 * drive : 12);
    case 0x13: return QByteArray(1, char(0x03)); // bank 1 sensors 1 and 2 present
    case 0x14: return u8((on ? 0.45 + 0.4 * std::sin(t * 6.0) : 0.0) * 200) + char(0x80);
    case 0x15: return u8((on ? 0.68 : 0.0) * 200) + char(0xFF);
    case 0x1C: return QByteArray(1, char(0x06)); // EOBD (Europe)
    case 0x1F: return u16(on ? int(t) : 0);
    case 0x21: return u16(0);
    case 0x2E: return percent(on ? 8 : 0);
    case 0x2F: return percent(62);
    case 0x30: return u8(m_stored.isEmpty() ? 0 : 14);
    case 0x31: return u16(m_stored.isEmpty() ? 0 : 1450);
    case 0x33: return u8(101);
    case 0x3C: return u16(int(((on ? 420 + 260 * drive : 40) + 40) * 10));
    case 0x41: return QByteArray::fromHex("0007E5") + char(m_monitorsIncomplete);
    case 0x42: return u16(on ? int(14150 + 60 * std::sin(t)) : 12400);
    case 0x43: return u16(int((on ? 18 + 50 * drive : 0) * 255 / 100));
    case 0x44: return u16(int((on ? 1.0 + 0.02 * std::sin(t * 2) : 1.0) * 32768));
    case 0x45: return percent(on ? 5 + 45 * drive : 0);
    case 0x46: return u8(16 + 40);
    case 0x47: return percent(on ? 13 + 50 * drive : 12);
    case 0x49: return percent(on ? 15 + 55 * drive : 15);
    case 0x4A: return percent(on ? 7 + 27 * drive : 7);
    case 0x4C: return percent(on ? 10 + 45 * drive : 0);
    case 0x51: return QByteArray(1, char(0x01)); // gasoline
    default:   return {};
    }
}

QByteArray SimulatedTransport::freezeFrameData(quint8 pid) const
{
    // The snapshot the ECU stored when P0420 / P0133 was set.
    switch (pid) {
    case 0x02: return u16(m_stored.isEmpty() ? 0 : m_stored.first());
    case 0x04: return percent(42);
    case 0x05: return u8(86 + 40);
    case 0x06: return u8(128 + 2.3 * 1.28);
    case 0x07: return u8(128 + 4.7 * 1.28);
    case 0x0B: return u8(58);
    case 0x0C: return u16(2150 * 4);
    case 0x0D: return u8(64);
    case 0x0E: return u8((14 + 64) * 2);
    case 0x0F: return u8(31 + 40);
    case 0x11: return percent(24);
    default:   return {};
    }
}

} // namespace obdaryn
