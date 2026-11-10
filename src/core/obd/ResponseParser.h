// SPDX-License-Identifier: GPL-3.0-or-later
//
// Turns the ELM327's text lines into per-ECU byte messages.

#pragma once

#include "obd/Protocol.h"

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

namespace obdaryn {

/// One complete answer from one ECU.
struct EcuMessage {
    /// Address of the responding ECU: "7E8" (11-bit CAN), "18DAF110" (29-bit CAN),
    /// "10" (legacy protocols: source address). Empty when headers are off.
    QString ecu;
    /// Reassembled payload, starting with the response service ID (e.g. 0x41).
    QByteArray data;
    /// Legacy (non-CAN) protocols send long answers as several independent
    /// frames that each repeat the service ID; they are kept here one by one.
    /// For CAN this holds a single entry equal to data.
    QList<QByteArray> frames;
};

/**
 * @brief Parses ELM327 reply lines into EcuMessage objects.
 *
 * Handles:
 *  - CAN with headers: "7E8 06 41 0C 1A F8 AA AA" — header, ISO-TP PCI byte,
 *    data, padding. Multi-frame answers (first frame 0x1N, consecutive frames
 *    0x2N) are reassembled per ECU, using the length from the first frame.
 *  - 29-bit CAN with headers: "18 DA F1 10 06 41 ...".
 *  - CAN without headers: "41 0C 1A F8", or the ELM327 multi-frame layout
 *    "014" / "0: 49 02 01 ..." / "1: ...".
 *  - Legacy protocols with headers: "48 6B 10 41 0C 1A F8 C4" — 3 header
 *    bytes and a trailing checksum byte.
 *
 * Messages are returned sorted by ECU address, so the engine (7E8) comes first.
 */
class ResponseParser
{
public:
    static QList<EcuMessage> parse(const QStringList &lines, Protocol protocol, bool headersOn);

    /// Parses a "41 0C 1A F8" style string (spaces optional) into bytes; empty on error.
    static QByteArray hexBytes(const QString &text);
};

} // namespace obdaryn
