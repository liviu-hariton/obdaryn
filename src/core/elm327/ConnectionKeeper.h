// SPDX-License-Identifier: GPL-3.0-or-later
//
// Keeps a session connected: reconnects after the link drops, and waits for
// the car to answer again after the ignition was turned off.

#pragma once

#include "elm327/ElmSession.h"

#include <QElapsedTimer>
#include <QList>
#include <QObject>
#include <QTimer>

namespace obdaryn {

/**
 * @brief Reconnection policy for one ElmSession.
 *
 * Cheap Wi-Fi adapters drop the link: the Vgate iCar Pro sleeps after about
 * 20 minutes without traffic or when the car's voltage drops, laptops roam to
 * another network, the engine start makes the voltage dip. The keeper:
 *
 *  - **Link lost** (the transport failed or closed, or the adapter stopped
 *    answering, see ElmSession::adapterUnresponsive): reopens the session
 *    after 2, 4, 8, 15, then 30 s, up to kMaxAttempts tries, then gives up.
 *    Only after the session was Ready once: a first connection that never
 *    worked is reported to the user instead of retried.
 *  - **Car silent** (NoVehicle, e.g. ignition off): asks the car again every
 *    5 s ("0100") until it answers, then the session is Ready again.
 *
 * It never acts after stop(): the user pressed Disconnect.
 */
class ConnectionKeeper : public QObject
{
    Q_OBJECT
public:
    explicit ConnectionKeeper(ElmSession *session, QObject *parent = nullptr);

    /// The user wants to be connected. `reconnect` false keeps only the
    /// wait-for-the-car behavior (replays have no link to lose).
    void start(bool reconnect = true);
    /// The user disconnected: stop everything.
    void stop();

    bool reconnecting() const { return m_reconnecting; }
    int attempt() const { return m_attempt; }
    static constexpr int kMaxAttempts = 8;
    /// Milliseconds until the next reconnect attempt (0 while one is running).
    qint64 nextAttemptInMs() const;
    bool waitingForVehicle() const { return m_vehicleTimer.isActive(); }
    /// True after the keeper gave up reconnecting.
    bool gaveUp() const { return m_gaveUp; }

    /// Tests use short delays.
    void setDelays(const QList<int> &reconnectMs, int vehicleRetryMs);

signals:
    void changed();

private:
    void onStateChanged(ElmSession::State state);
    void onUnresponsive();
    void scheduleReconnect();

    ElmSession *m_session;
    bool m_active = false;
    bool m_reconnect = true;
    bool m_wasReady = false;
    bool m_reconnecting = false;
    bool m_gaveUp = false;
    int m_attempt = 0;
    QList<int> m_delays{2000, 4000, 8000, 15000, 30000, 30000, 30000, 30000};
    QTimer m_reconnectTimer;
    QElapsedTimer m_reconnectClock;
    QTimer m_vehicleTimer;
};

} // namespace obdaryn
