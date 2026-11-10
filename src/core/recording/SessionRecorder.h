// SPDX-License-Identifier: GPL-3.0-or-later
//
// Writes everything an ElmSession says and hears to a recording file.

#pragma once

#include "recording/Recording.h"

#include <QElapsedTimer>
#include <QFile>
#include <QHash>
#include <QJsonObject>
#include <QObject>

namespace obdaryn {

class ElmSession;

/**
 * @brief Records a session so it can be replayed later without the car.
 *
 * The recorder listens to ElmSession::exchanged() all the time, but only
 * writes while a recording runs. While idle it remembers the latest answer to
 * every command (the adapter reset, protocol detection, the VIN...). When a
 * recording starts, those answers are written first as "snapshot" entries,
 * so a replay can go through the whole connection handshake even if the user
 * pressed Record long after connecting.
 *
 * Recordings contain the VIN and the car's answers, nothing else about the
 * user. They are plain text, so they can be inspected before being shared.
 */
class SessionRecorder : public QObject
{
    Q_OBJECT
public:
    explicit SessionRecorder(ElmSession *session, QObject *parent = nullptr);
    ~SessionRecorder() override;

    /**
     * @brief Starts writing to `path` (overwritten if it exists).
     * @param metadata extra header fields, e.g. {"vin": ..., "app": ...}
     */
    bool start(const QString &path, const QJsonObject &metadata, QString *error = nullptr);
    /// Finishes the file. Safe to call when not recording.
    void stop();

    bool isRecording() const { return m_file.isOpen(); }
    QString path() const { return m_path; }
    qint64 elapsedMs() const { return isRecording() ? m_clock.elapsed() : 0; }
    int exchangeCount() const { return m_count; }

signals:
    void recordingChanged();

private:
    void onExchange(const QByteArray &command, const QByteArray &raw, qint64 elapsedMs, bool timedOut);
    void write(const RecordedExchange &e);

    ElmSession *m_session;
    QFile m_file;
    QString m_path;
    QElapsedTimer m_clock;
    AdapterTarget m_target;
    QHash<QString, RecordedExchange> m_latest; ///< latest answer per target + command
    QStringList m_order;                        ///< keys of m_latest, first seen first
    int m_count = 0;
};

} // namespace obdaryn
