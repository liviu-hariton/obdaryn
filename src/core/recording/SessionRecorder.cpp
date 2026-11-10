// SPDX-License-Identifier: GPL-3.0-or-later

#include "recording/SessionRecorder.h"

#include "elm327/ElmSession.h"
#include "safety/RequestGuard.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QJsonDocument>

namespace obdaryn {

SessionRecorder::SessionRecorder(ElmSession *session, QObject *parent) : QObject(parent), m_session(session)
{
    connect(session, &ElmSession::exchanged, this, &SessionRecorder::onExchange);
    connect(session, &ElmSession::stateChanged, this, [this](ElmSession::State state) {
        // A new connection starts from scratch: forget the old car's answers,
        // and end a running recording when the connection goes away.
        if (state == ElmSession::State::Connecting) {
            m_latest.clear();
            m_order.clear();
            m_target = {};
        }
        if (state == ElmSession::State::Disconnected || state == ElmSession::State::Failed)
            stop();
    });
}

SessionRecorder::~SessionRecorder()
{
    stop();
}

bool SessionRecorder::start(const QString &path, const QJsonObject &metadata, QString *error)
{
    stop();
    m_file.setFileName(path);
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (error)
            *error = m_file.errorString();
        return false;
    }
    m_path = path;
    m_count = 0;

    QJsonObject header = metadata;
    header.insert(QStringLiteral("format"), QLatin1String(Recording::kFormatName));
    header.insert(QStringLiteral("version"), Recording::kFormatVersion);
    header.insert(QStringLiteral("created"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    header.insert(QStringLiteral("protocol"), int(m_session->protocol()));
    header.insert(QStringLiteral("headers"), m_session->headersEnabled());
    header.insert(QStringLiteral("adapter"), m_session->adapterVersion());
    header.insert(QStringLiteral("adapterDescription"), m_session->adapterDescription());
    if (const Transport *t = m_session->transport())
        header.insert(QStringLiteral("transport"), t->description());
    m_file.write(QJsonDocument(header).toJson(QJsonDocument::Compact) + '\n');

    m_clock.start();
    // What the car said before Record was pressed (see the class comment).
    for (const QString &key : std::as_const(m_order)) {
        RecordedExchange e = m_latest.value(key);
        e.t = 0;
        e.snapshot = true;
        write(e);
    }
    m_file.flush();
    emit recordingChanged();
    return true;
}

void SessionRecorder::stop()
{
    if (!m_file.isOpen())
        return;
    m_file.write(QJsonDocument(QJsonObject{{QStringLiteral("end"), m_clock.elapsed()}}).toJson(QJsonDocument::Compact) + '\n');
    m_file.close();
    emit recordingChanged();
}

void SessionRecorder::onExchange(const QByteArray &command, const QByteArray &raw, qint64 elapsedMs, bool timedOut)
{
    RecordedExchange e;
    e.command = RequestGuard::normalize(command);
    e.target = m_target.header; // the module this command went to
    e.raw = raw;
    e.ms = int(elapsedMs);
    e.timedOut = timedOut;
    m_target.apply(e.command);

    const QString key = e.key();
    if (!m_latest.contains(key))
        m_order << key;
    m_latest.insert(key, e);

    if (m_file.isOpen()) {
        e.t = m_clock.elapsed();
        write(e);
        m_file.flush(); // keep the file usable if the laptop dies mid-drive
    }
}

void SessionRecorder::write(const RecordedExchange &e)
{
    QJsonObject o{{QStringLiteral("t"), e.t},
                  {QStringLiteral("c"), QString::fromLatin1(e.command)},
                  {QStringLiteral("r"), QString::fromLatin1(e.raw)},
                  {QStringLiteral("ms"), e.ms}};
    if (!e.target.isEmpty())
        o.insert(QStringLiteral("h"), e.target);
    if (e.timedOut)
        o.insert(QStringLiteral("x"), true);
    if (e.snapshot)
        o.insert(QStringLiteral("s"), true);
    m_file.write(QJsonDocument(o).toJson(QJsonDocument::Compact) + '\n');
    ++m_count;
}

} // namespace obdaryn
