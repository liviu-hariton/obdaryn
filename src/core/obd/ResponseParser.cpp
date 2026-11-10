// SPDX-License-Identifier: GPL-3.0-or-later

#include "obd/ResponseParser.h"

#include <QMap>
#include <QRegularExpression>

namespace obdaryn {

namespace {

/// Reassembly state for one ECU on CAN.
struct IsoTpAssembly {
    QByteArray data;
    int expectedLength = -1; ///< from the first frame; -1 = single frame(s)
    QList<QByteArray> singles;
};

bool isHexToken(const QString &t)
{
    static const QRegularExpression re(QStringLiteral("^[0-9A-Fa-f]+$"));
    return re.match(t).hasMatch();
}

QStringList tokensOf(const QString &line)
{
    QStringList tokens = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    // With "AT S0" (no spaces) the adapter sends one long hex string. Split it
    // into bytes; an odd length means an 11-bit CAN header (3 hex digits) in front.
    if (tokens.size() == 1 && tokens.first().size() > 3 && isHexToken(tokens.first())) {
        const QString s = tokens.first();
        tokens.clear();
        qsizetype i = 0;
        if (s.size() % 2 == 1) {
            tokens << s.left(3);
            i = 3;
        }
        for (; i + 1 < s.size(); i += 2)
            tokens << s.mid(i, 2);
    }
    return tokens;
}

QByteArray bytesOf(const QStringList &tokens, qsizetype from)
{
    QByteArray out;
    for (qsizetype i = from; i < tokens.size(); ++i) {
        bool ok = false;
        const uint v = tokens[i].toUInt(&ok, 16);
        if (!ok || v > 0xFF)
            return {};
        out.append(char(v));
    }
    return out;
}

/// Feeds one CAN frame (PCI byte first) into the per-ECU assembly.
void feedIsoTp(IsoTpAssembly &a, const QByteArray &frame)
{
    if (frame.isEmpty())
        return;
    const quint8 pci = quint8(frame[0]);
    switch (pci >> 4) {
    case 0x0: { // single frame: low nibble = length
        const int len = pci & 0x0F;
        a.singles << frame.mid(1, len);
        break;
    }
    case 0x1: { // first frame: 12-bit length, then 6 data bytes
        if (frame.size() < 2)
            return;
        a.expectedLength = ((pci & 0x0F) << 8) | quint8(frame[1]);
        a.data = frame.mid(2);
        break;
    }
    case 0x2: // consecutive frame: low nibble = sequence number, then 7 bytes
        if (a.expectedLength >= 0)
            a.data += frame.mid(1);
        break;
    default: // 0x3 = flow control (sent by the adapter itself), ignore
        break;
    }
}

} // namespace

QByteArray ResponseParser::hexBytes(const QString &text)
{
    return bytesOf(tokensOf(text.trimmed()), 0);
}

QList<EcuMessage> ResponseParser::parse(const QStringList &lines, Protocol protocol, bool headersOn)
{
    QList<EcuMessage> result;
    const bool can = isCan(protocol) || protocol == Protocol::Unknown || protocol == Protocol::Auto;

    if (!can) {
        // Legacy protocols: every line is a complete frame.
        QMap<QString, EcuMessage> byEcu;
        for (const QString &line : lines) {
            const QStringList tokens = tokensOf(line);
            QByteArray bytes = bytesOf(tokens, 0);
            if (bytes.isEmpty())
                continue;
            QString ecu;
            if (headersOn) {
                if (bytes.size() < 5)
                    continue;              // header (3) + at least 1 data + checksum
                ecu = tokens[2].toUpper(); // third header byte = source address
                bytes = bytes.mid(3, bytes.size() - 4);
            }
            EcuMessage &m = byEcu[ecu];
            m.ecu = ecu;
            m.frames << bytes;
            m.data += bytes;
        }
        return byEcu.values();
    }

    // CAN.
    QMap<QString, IsoTpAssembly> byEcu;
    IsoTpAssembly noHeader;          // used when headers are off
    bool noHeaderMultiFrame = false;

    for (const QString &line : lines) {
        const QStringList tokens = tokensOf(line);
        if (tokens.isEmpty())
            continue;

        // Headers off, multi-frame: "014" then "0: 49 02 01 ...".
        if (tokens.size() == 1 && tokens.first().size() == 3 && isHexToken(tokens.first())) {
            noHeaderMultiFrame = true;
            noHeader.expectedLength = tokens.first().toInt(nullptr, 16);
            continue;
        }
        if (tokens.first().endsWith(QLatin1Char(':'))) {
            noHeaderMultiFrame = true;
            noHeader.data += bytesOf(tokens, 1);
            continue;
        }

        const QString first = tokens.first();
        if (first.size() == 3 && isHexToken(first)) {
            // 11-bit CAN header, e.g. "7E8".
            feedIsoTp(byEcu[first.toUpper()], bytesOf(tokens, 1));
        } else if (uses29BitIds(protocol) && headersOn && tokens.size() >= 5) {
            // 29-bit CAN header: 4 bytes, e.g. "18 DA F1 10".
            const QString ecu = (tokens[0] + tokens[1] + tokens[2] + tokens[3]).toUpper();
            feedIsoTp(byEcu[ecu], bytesOf(tokens, 4));
        } else {
            // Headers off, single frame: just the data bytes.
            noHeader.singles << bytesOf(tokens, 0);
        }
    }

    auto finish = [&result](const QString &ecu, IsoTpAssembly &a) {
        if (a.expectedLength >= 0) {
            EcuMessage m{ecu, a.data.left(a.expectedLength), {}};
            m.frames << m.data;
            result << m;
        }
        for (const QByteArray &s : a.singles) {
            if (s.isEmpty())
                continue;
            EcuMessage m{ecu, s, {s}};
            result << m;
        }
    };

    for (auto it = byEcu.begin(); it != byEcu.end(); ++it)
        finish(it.key(), it.value());
    if (noHeaderMultiFrame || !noHeader.singles.isEmpty())
        finish(QString(), noHeader);
    return result;
}

} // namespace obdaryn
