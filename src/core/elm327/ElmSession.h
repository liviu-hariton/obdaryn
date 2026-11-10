// SPDX-License-Identifier: GPL-3.0-or-later
//
// Talks the ELM327 text protocol: one command at a time, replies end with '>'.

#pragma once

#include "obd/Protocol.h"
#include "transport/Transport.h"

#include <QElapsedTimer>
#include <QObject>
#include <QQueue>
#include <QStringList>
#include <QTimer>

#include <functional>
#include <optional>

namespace obdaryn {

/**
 * @brief Owns the conversation with an ELM327 adapter.
 *
 * The ELM327 works like a very small terminal: you type a command followed by
 * a carriage return, it answers with one or more lines and finally prints the
 * '>' prompt to say it is ready for the next command. It can only handle one
 * command at a time, so ElmSession keeps a queue and sends the next command
 * only after the previous prompt arrived (or timed out).
 *
 * Every command passes through RequestGuard before it reaches the wire.
 *
 * Typical use:
 * @code
 *   session.setTransport(new TcpTransport("192.168.0.10", 35000));
 *   session.open();                    // connects and initializes
 *   session.send("010C", [](const ElmSession::Reply &r) { ... });
 * @endcode
 */
class ElmSession : public QObject
{
    Q_OBJECT
public:
    /// Connection lifecycle, as shown in the UI.
    enum class State {
        Disconnected,  ///< nothing open
        Connecting,    ///< opening the transport (TCP connect, ...)
        Initializing,  ///< adapter answers; sending the AT setup commands
        NoVehicle,     ///< adapter is fine, but no ECU answered (ignition off?)
        Ready,         ///< adapter and vehicle are talking
        Failed,        ///< could not connect or initialize; see lastError()
    };
    Q_ENUM(State)

    /// The result of one command.
    struct Reply {
        enum class Status {
            Ok,            ///< got data lines (or "OK" for AT commands)
            NoData,        ///< adapter said "NO DATA": no ECU answered
            AdapterError,  ///< "?", "UNABLE TO CONNECT", "CAN ERROR", ...
            Timeout,       ///< no '>' prompt within the timeout
            Blocked,       ///< refused by RequestGuard, never sent
            NotConnected,  ///< session closed before the command could run
        };
        QByteArray command;
        Status status = Status::Ok;
        QStringList lines;   ///< data lines, echo and "SEARCHING..." removed
        QString error;       ///< text for AdapterError / Blocked / Timeout
        qint64 elapsedMs = 0;
        bool ok() const { return status == Status::Ok; }
    };
    using Callback = std::function<void(const Reply &)>;

    /// Per-command options.
    struct Options {
        int timeoutMs = 2500;
        /// Must be true for 0x04 / 0x14, and only after the user confirmed.
        bool userConfirmedClear = false;
    };

    explicit ElmSession(QObject *parent = nullptr);
    ~ElmSession() override;

    /// Takes ownership of the transport. Call before open().
    void setTransport(Transport *transport);
    Transport *transport() const { return m_transport; }

    /// Opens the transport and runs the initialization sequence.
    void open();
    /// Closes everything. Pending commands finish with Status::NotConnected.
    void close();

    /// Queues a command. The callback always runs exactly once (asynchronously).
    void send(const QByteArray &command, Callback callback, Options options);
    void send(const QByteArray &command, Callback callback) { send(command, std::move(callback), Options{}); }

    /// Retries the vehicle handshake ("0100" + "AT DPN"), e.g. after ignition ON.
    void detectVehicle();

    State state() const { return m_state; }
    QString lastError() const { return m_lastError; }
    Protocol protocol() const { return m_protocol; }
    QString adapterVersion() const { return m_adapterVersion; }
    QString adapterDescription() const { return m_adapterDescription; }
    /// True once "AT H1" succeeded: replies then carry the ECU address.
    bool headersEnabled() const { return m_headersEnabled; }
    int queuedCommands() const { return int(m_queue.size()) + (m_inFlight ? 1 : 0); }

signals:
    void stateChanged(obdaryn::ElmSession::State state);
    /// Adapter version, protocol, ... changed.
    void infoChanged();
    /// Raw traffic for the adapter console. direction is ">" (sent), "<" (received) or "!" (blocked / error).
    void trafficLogged(const QString &direction, const QString &text);
    /// One command finished: everything the adapter printed before its '>' prompt,
    /// unparsed. Used by SessionRecorder; blocked commands never get here because
    /// they never reach the adapter. On a timeout rawReply is empty.
    void exchanged(const QByteArray &command, const QByteArray &rawReply, qint64 elapsedMs, bool timedOut);

private:
    struct Pending {
        QByteArray command;
        Callback callback;
        Options options;
    };

    void setState(State state, const QString &error = {});
    void runInitialization();
    void processNext();
    void onReceived(const QByteArray &data);
    void finishInFlight(const QByteArray &rawReply);
    void onTimeout();
    void failAll(Reply::Status status, const QString &error);
    Reply buildReply(const Pending &p, const QByteArray &raw) const;

    Transport *m_transport = nullptr;
    State m_state = State::Disconnected;
    QString m_lastError;
    Protocol m_protocol = Protocol::Unknown;
    QString m_adapterVersion;
    QString m_adapterDescription;
    bool m_headersEnabled = false;

    QQueue<Pending> m_queue;
    std::optional<Pending> m_inFlight;
    QByteArray m_rxBuffer;
    QTimer m_timeoutTimer;
    QTimer m_resyncTimer;
    QElapsedTimer m_commandClock;
    bool m_resyncing = false; ///< discarding a late reply after a timeout
};

} // namespace obdaryn
