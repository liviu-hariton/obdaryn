// SPDX-License-Identifier: GPL-3.0-or-later

#include "transport/ReplayTransport.h"

#include "obd/Pids.h"
#include "safety/RequestGuard.h"

#include <QTimer>

#include <algorithm>

namespace obdaryn {

namespace {

/// Longest delay we reproduce; a recorded 5 s protocol search isn't worth waiting for again.
constexpr int kMaxDelayMs = 1500;

QByteArray hexSpaced(const QByteArray &bytes)
{
    return bytes.toHex(' ').toUpper();
}

/// "18DAF110" → "18 DA F1 10"; 11-bit headers stay as they are.
QByteArray headerText(const QString &ecu)
{
    if (ecu.size() == 8)
        return hexSpaced(QByteArray::fromHex(ecu.toLatin1()));
    return ecu.toLatin1();
}

/// One ECU's payload as ISO-TP frames, the way an ELM327 prints them with headers on.
QByteArray canFrames(const QString &ecu, const QByteArray &payload)
{
    const QByteArray head = headerText(ecu) + ' ';
    if (payload.size() <= 7)
        return head + hexSpaced(QByteArray(1, char(payload.size())) + payload) + '\r';

    QByteArray out = head + hexSpaced(QByteArray(1, char(0x10 | ((payload.size() >> 8) & 0x0F)))
                                      + char(payload.size() & 0xFF) + payload.left(6)) + '\r';
    quint8 sequence = 1;
    for (qsizetype i = 6; i < payload.size(); i += 7) {
        out += head + hexSpaced(QByteArray(1, char(0x20 | sequence)) + payload.mid(i, 7)) + '\r';
        sequence = (sequence + 1) & 0x0F;
    }
    return out;
}

} // namespace

ReplayTransport::ReplayTransport(Recording recording, const QString &name, QObject *parent)
    : Transport(parent), m_recording(std::move(recording)), m_name(name)
{
    // Exchanges are written in time order, so each list ends up sorted.
    for (qsizetype i = 0; i < m_recording.exchanges.size(); ++i) {
        const RecordedExchange &e = m_recording.exchanges[i];
        m_byKey[e.key()] << i;
        m_keyTimes[e.key()] << e.t;
    }
    m_samples = m_recording.pidSamples();
    for (qsizetype i = 0; i < m_samples.size(); ++i) {
        const Recording::PidSample &s = m_samples[i];
        m_pidIndex[s.ecu][s.pid] << i;
        m_pidTimes[s.ecu][s.pid] << s.t;
    }
}

void ReplayTransport::open()
{
    m_open = true;
    m_rxBuffer.clear();
    m_target = {};
    m_clock.start();
    QTimer::singleShot(0, this, [this] {
        if (m_open)
            emit opened();
    });
}

void ReplayTransport::close()
{
    if (!m_open)
        return;
    m_open = false;
    emit closed();
}

qint64 ReplayTransport::positionMs() const
{
    if (!m_clock.isValid())
        return 0;
    const qint64 now = m_clock.elapsed();
    return m_recording.durationMs > 0 ? now % (m_recording.durationMs + 1) : now;
}

bool ReplayTransport::hasModuleRequests() const
{
    return std::any_of(m_recording.exchanges.cbegin(), m_recording.exchanges.cend(),
                       [](const RecordedExchange &e) { return !e.target.isEmpty() && !e.command.startsWith("AT"); });
}

qsizetype ReplayTransport::pick(const QList<qint64> &times, qint64 now)
{
    // The last entry at or before `now`; before the first one, the first one.
    const auto it = std::upper_bound(times.cbegin(), times.cend(), now);
    return it == times.cbegin() ? 0 : (it - times.cbegin()) - 1;
}

void ReplayTransport::write(const QByteArray &data)
{
    if (!m_open)
        return;
    m_rxBuffer.append(data);
    qsizetype cr;
    while ((cr = m_rxBuffer.indexOf('\r')) >= 0) {
        const QByteArray command = RequestGuard::normalize(m_rxBuffer.left(cr).trimmed());
        m_rxBuffer.remove(0, cr + 1);
        if (command.isEmpty())
            continue;

        int delay = 0;
        bool silent = false;
        const QByteArray reply = answer(command, &delay, &silent);
        if (silent)
            continue; // the car didn't answer this either: let the session time out
        QTimer::singleShot(m_instant ? 0 : qBound(0, delay, kMaxDelayMs), this, [this, reply] {
            if (m_open)
                emit received(reply + "\r>");
        });
    }
}

QByteArray ReplayTransport::answer(const QByteArray &command, int *delayMs, bool *silent)
{
    const QString target = m_target.header;
    m_target.apply(command);

    // Never pretend that codes were cleared.
    const bool clear = !command.startsWith("AT") && (command.startsWith("04") || command.startsWith("14"));
    if (clear) {
        *delayMs = 50;
        return "NO DATA\r";
    }

    if (target.isEmpty() && command.startsWith("01") && command.size() >= 4) {
        const QByteArray rebuilt = rebuildLiveData(command, delayMs);
        if (!rebuilt.isEmpty())
            return rebuilt;
    }

    const QString key = target + QLatin1Char('|') + QString::fromLatin1(command);
    const auto found = m_byKey.constFind(key);
    if (found != m_byKey.cend()) {
        const RecordedExchange &e = m_recording.exchanges[found->at(pick(m_keyTimes.value(key), positionMs()))];
        *delayMs = e.ms;
        *silent = e.timedOut;
        return e.raw;
    }

    *delayMs = 20;
    return command.startsWith("AT") ? QByteArray("OK\r") : QByteArray("NO DATA\r");
}

QByteArray ReplayTransport::rebuildLiveData(const QByteArray &command, int *delayMs) const
{
    // Only CAN with headers can be rebuilt faithfully; other cases use exact matches.
    if (!isCan(m_recording.protocol()) || !m_recording.headersOn())
        return {};
    const QByteArray pids = QByteArray::fromHex(command.mid(2));
    if (pids.isEmpty())
        return {};
    for (char pid : pids) {
        if (PidTable::dataLength(quint8(pid)) < 0)
            return {};
    }

    const qint64 now = positionMs();
    QStringList ecus = m_pidIndex.keys();
    ecus.sort(); // engine (7E8) first, as a real bus usually answers
    QByteArray out;
    int delay = 0;
    for (const QString &ecu : std::as_const(ecus)) {
        const auto &index = m_pidIndex[ecu];
        const auto &times = m_pidTimes[ecu];
        QByteArray payload(1, char(0x41));
        for (char pid : pids) {
            const auto it = index.constFind(quint8(pid));
            if (it == index.cend())
                continue; // this ECU never reported this value
            const Recording::PidSample &s = m_samples[it->at(pick(times.value(quint8(pid)), now))];
            payload += pid + s.raw;
            delay = qMax(delay, s.ms);
        }
        if (payload.size() > 1)
            out += canFrames(ecu, payload);
    }
    *delayMs = delay;
    return out;
}

} // namespace obdaryn
