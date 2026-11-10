// SPDX-License-Identifier: GPL-3.0-or-later
//
// Plays a recorded session back as if the car were connected.

#pragma once

#include "recording/Recording.h"
#include "transport/Transport.h"

#include <QElapsedTimer>
#include <QHash>
#include <QList>

namespace obdaryn {

/**
 * @brief A pretend adapter that answers from a recording.
 *
 * Every layer above (ElmSession, ObdClient, the screens) runs exactly as with
 * a real car, so a drive recorded once can be studied, demonstrated or used to
 * debug Obdaryn at a desk.
 *
 * Answers follow the recording's clock: a request sent 90 s into the replay
 * gets the answer the car gave around 90 s into the recording (the latest one
 * at or before that moment). At the end, the replay loops.
 *
 *  - Most requests are looked up by target module + command, so a module scan
 *    replays per module.
 *  - Live data (service 01 to all ECUs) is rebuilt value by value, because
 *    the app may ask for a different combination of values than it did while
 *    recording (other pinned values, another rotation).
 *  - Settings the recording doesn't contain ("AT ..." commands) answer OK;
 *    unknown requests answer NO DATA.
 *  - Clearing codes (04, 14) never replays a success: there is no car to clear.
 */
class ReplayTransport : public Transport
{
    Q_OBJECT
public:
    explicit ReplayTransport(Recording recording, const QString &name, QObject *parent = nullptr);

    void open() override;
    void close() override;
    bool isOpen() const override { return m_open; }
    void write(const QByteArray &data) override;
    QString description() const override { return m_name; }

    /// Answer immediately instead of with the recorded delay (tests).
    void setInstant(bool instant) { m_instant = instant; }

    qint64 durationMs() const { return m_recording.durationMs; }
    /// Position in the recording, in ms (wraps around at the end).
    qint64 positionMs() const;
    const Recording &recording() const { return m_recording; }
    /// True when the recording contains requests to individual modules (a module scan).
    bool hasModuleRequests() const;

private:
    QByteArray answer(const QByteArray &command, int *delayMs, bool *silent);
    QByteArray rebuildLiveData(const QByteArray &command, int *delayMs) const;
    /// Index of the entry in `times` that was current at `now`.
    static qsizetype pick(const QList<qint64> &times, qint64 now);

    Recording m_recording;
    QString m_name;
    bool m_open = false;
    bool m_instant = false;
    QElapsedTimer m_clock;
    QByteArray m_rxBuffer;
    AdapterTarget m_target;

    /// key (target|command) → indexes into m_recording.exchanges, by time.
    QHash<QString, QList<qsizetype>> m_byKey;
    QHash<QString, QList<qint64>> m_keyTimes;

    /// Live data: ECU → PID → samples, by time.
    QList<Recording::PidSample> m_samples;
    QHash<QString, QHash<quint8, QList<qsizetype>>> m_pidIndex;
    QHash<QString, QHash<quint8, QList<qint64>>> m_pidTimes;
};

} // namespace obdaryn
