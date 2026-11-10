// SPDX-License-Identifier: GPL-3.0-or-later
//
// A recorded diagnostic session: every command sent to the adapter and its raw
// reply, with timing. Written by SessionRecorder, replayed by ReplayTransport.

#pragma once

#include "obd/Protocol.h"

#include <QByteArray>
#include <QJsonObject>
#include <QList>
#include <QString>

namespace obdaryn {

/**
 * @brief Which module the adapter is talking to, as set by "AT SH".
 *
 * The same request means different things for different modules ("1902FF"
 * to the ABS module or to the airbag module), so recordings remember the
 * target of every request and replay looks answers up by target + command.
 * An empty header means the default: all OBD-II ECUs (functional 7DF).
 */
struct AdapterTarget
{
    QString header;
    /// Updates the target after `command` (normalized: upper case, no spaces).
    void apply(const QByteArray &command);
};

/// One command and the adapter's answer to it.
struct RecordedExchange
{
    qint64 t = 0;          ///< ms since the recording started (0 for snapshot entries)
    QString target;        ///< AdapterTarget::header when the command was sent
    QByteArray command;    ///< normalized, e.g. "010C" or "ATSH7D1"
    QByteArray raw;        ///< everything the adapter printed before '>'
    int ms = 0;            ///< how long the adapter took to answer
    bool timedOut = false; ///< no answer at all
    bool snapshot = false; ///< an answer from before the recording started (see SessionRecorder)

    /// Lookup key used by replay: target + command.
    QString key() const { return target + QLatin1Char('|') + QString::fromLatin1(command); }
};

/**
 * @brief A recording loaded from disk.
 *
 * File format (".obdrec", JSON Lines, one object per line, UTF-8):
 *  - line 1, the header: {"format":"obdaryn-recording","version":1,"created":...,
 *    "protocol":6,"headers":true,"vin":...,...}
 *  - one line per exchange: {"t":1520,"c":"010C","r":"7E8 04 41 0C 1A F8\r","ms":48}
 *    plus "h" (target header) when a module is addressed, "x":true for a
 *    timeout and "s":true for snapshot entries;
 *  - a last line {"end":<ms>} when the recording was stopped cleanly.
 *
 * Every line is flushed as it is written, so a recording that was cut short
 * (laptop battery, crash) still loads: it just has no "end" line.
 */
class Recording
{
public:
    static constexpr int kFormatVersion = 1;
    static constexpr const char *kFormatName = "obdaryn-recording";
    static constexpr const char *kSuffix = "obdrec";

    /// Loads a file; on failure returns false and explains why in *error.
    bool load(const QString &path, QString *error = nullptr);
    /// Reads only the header and duration (fast; for lists of recordings).
    static bool peek(const QString &path, QJsonObject *header, qint64 *durationMs);

    QJsonObject header;
    QList<RecordedExchange> exchanges;
    qint64 durationMs = 0;

    Protocol protocol() const { return static_cast<Protocol>(header.value(QStringLiteral("protocol")).toInt(-1)); }
    bool headersOn() const { return header.value(QStringLiteral("headers")).toBool(); }

    /// One live-data value (service 01) taken out of a recorded reply.
    struct PidSample {
        qint64 t = 0;
        QString ecu;      ///< which module answered, e.g. "7E8"
        quint8 pid = 0;
        QByteArray raw;   ///< the PID's data bytes (A, B, ...)
        int ms = 0;       ///< answer time of the exchange it came from
    };
    /// Every live-data value in the recording, oldest first. Only requests sent
    /// to all OBD-II ECUs count; the module scan's direct requests don't.
    QList<PidSample> pidSamples() const;

    /**
     * @brief The live data as CSV: one row per moment, one column per value.
     *
     * Comma-separated with '.' decimals (what spreadsheet imports expect),
     * time in seconds from the start. A cell is empty when that value wasn't
     * read at that moment: values are polled in rotation, and an empty cell is
     * more honest than repeating an old value.
     */
    QString toCsv() const;
};

} // namespace obdaryn
