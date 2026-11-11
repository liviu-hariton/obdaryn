// SPDX-License-Identifier: GPL-3.0-or-later

#include "elm327/ConnectionKeeper.h"

namespace obdaryn {

ConnectionKeeper::ConnectionKeeper(ElmSession *session, QObject *parent) : QObject(parent), m_session(session)
{
    m_reconnectTimer.setSingleShot(true);
    connect(&m_reconnectTimer, &QTimer::timeout, this, [this] {
        if (!m_active)
            return;
        ++m_attempt;
        emit changed();
        m_session->open(); // runs the whole handshake again
    });

    m_vehicleTimer.setInterval(5000);
    connect(&m_vehicleTimer, &QTimer::timeout, this, [this] {
        // One question at a time: "0100" can take up to 15 s on a sleepy bus.
        if (m_active && m_session->state() == ElmSession::State::NoVehicle && m_session->queuedCommands() == 0)
            m_session->detectVehicle();
    });

    // Queued: these fire from inside the session's reply handling, and the
    // reactions (closing, reopening) must not run in the middle of it.
    connect(session, &ElmSession::stateChanged, this, &ConnectionKeeper::onStateChanged, Qt::QueuedConnection);
    connect(session, &ElmSession::adapterUnresponsive, this, &ConnectionKeeper::onUnresponsive, Qt::QueuedConnection);
}

void ConnectionKeeper::setDelays(const QList<int> &reconnectMs, int vehicleRetryMs)
{
    m_delays = reconnectMs;
    m_vehicleTimer.setInterval(vehicleRetryMs);
}

void ConnectionKeeper::start(bool reconnect)
{
    m_active = true;
    m_reconnect = reconnect;
    m_wasReady = false;
    m_reconnecting = false;
    m_gaveUp = false;
    m_attempt = 0;
    m_reconnectTimer.stop();
    m_vehicleTimer.stop();
    emit changed();
}

void ConnectionKeeper::stop()
{
    m_active = false;
    m_reconnecting = false;
    m_reconnectTimer.stop();
    m_vehicleTimer.stop();
    emit changed();
}

qint64 ConnectionKeeper::nextAttemptInMs() const
{
    if (!m_reconnectTimer.isActive())
        return 0;
    return qMax<qint64>(0, m_reconnectTimer.interval() - m_reconnectClock.elapsed());
}

void ConnectionKeeper::onStateChanged(ElmSession::State state)
{
    if (!m_active)
        return;
    switch (state) {
    case ElmSession::State::Ready:
        m_wasReady = true;
        m_reconnecting = false;
        m_attempt = 0;
        m_reconnectTimer.stop();
        m_vehicleTimer.stop();
        break;
    case ElmSession::State::NoVehicle:
        // The adapter is there; the car isn't answering. Keep asking.
        m_reconnecting = false;
        if (!m_vehicleTimer.isActive())
            m_vehicleTimer.start();
        break;
    case ElmSession::State::Failed:
    case ElmSession::State::Disconnected:
        m_vehicleTimer.stop();
        if (m_reconnect && m_wasReady)
            scheduleReconnect();
        break;
    case ElmSession::State::Connecting:
    case ElmSession::State::Initializing:
        break;
    }
    emit changed();
}

void ConnectionKeeper::onUnresponsive()
{
    if (!m_active || !m_reconnect || m_session->state() != ElmSession::State::Ready)
        return;
    // Drop the dead link; the Disconnected state that follows schedules the reconnect.
    m_session->close();
}

void ConnectionKeeper::scheduleReconnect()
{
    if (m_reconnectTimer.isActive())
        return;
    if (m_attempt >= kMaxAttempts) {
        m_reconnecting = false;
        m_gaveUp = true;
        m_active = false;
        return;
    }
    m_reconnecting = true;
    m_reconnectTimer.setInterval(m_delays.value(m_attempt, m_delays.isEmpty() ? 30000 : m_delays.last()));
    m_reconnectClock.start();
    m_reconnectTimer.start();
}

} // namespace obdaryn
