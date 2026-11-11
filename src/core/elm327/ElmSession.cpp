// SPDX-License-Identifier: GPL-3.0-or-later

#include "elm327/ElmSession.h"

#include "safety/RequestGuard.h"

#include <algorithm>

namespace obdaryn {

namespace {

// Lines the ELM327 prints instead of data when something went wrong.
const QStringList kErrorPrefixes = {
    QStringLiteral("?"),
    QStringLiteral("UNABLE TO CONNECT"),
    QStringLiteral("CAN ERROR"),
    QStringLiteral("BUS ERROR"),
    QStringLiteral("BUS BUSY"),
    QStringLiteral("FB ERROR"),
    QStringLiteral("DATA ERROR"),
    QStringLiteral("<DATA ERROR"),
    QStringLiteral("<RX ERROR"),
    QStringLiteral("RX ERROR"),
    QStringLiteral("BUFFER FULL"),
    QStringLiteral("STOPPED"),
    QStringLiteral("LV RESET"),
    QStringLiteral("ACT ALERT"),
    QStringLiteral("ERR"),
};

bool isErrorLine(const QString &line)
{
    for (const QString &prefix : kErrorPrefixes) {
        if (line.startsWith(prefix))
            return true;
    }
    return false;
}

} // namespace

ElmSession::ElmSession(QObject *parent) : QObject(parent)
{
    m_timeoutTimer.setSingleShot(true);
    connect(&m_timeoutTimer, &QTimer::timeout, this, &ElmSession::onTimeout);

    // After a timeout the adapter may still print a late answer. We wait a
    // little for its '>' instead of sending a bare CR to interrupt it — a bare
    // CR would make the ELM327 *repeat* the previous command.
    m_resyncTimer.setSingleShot(true);
    m_resyncTimer.setInterval(1500);
    connect(&m_resyncTimer, &QTimer::timeout, this, [this] {
        m_resyncing = false;
        m_rxBuffer.clear();
        processNext();
    });
}

ElmSession::~ElmSession()
{
    // Callbacks may point at objects that are being destroyed too; drop them silently.
    m_queue.clear();
    m_inFlight.reset();
}

void ElmSession::setTransport(Transport *transport)
{
    if (m_transport) {
        m_transport->disconnect(this);
        m_transport->close();
        m_transport->deleteLater();
    }
    m_transport = transport;
    if (!m_transport)
        return;
    m_transport->setParent(this);

    connect(m_transport, &Transport::opened, this, &ElmSession::runInitialization);
    connect(m_transport, &Transport::received, this, &ElmSession::onReceived);
    connect(m_transport, &Transport::failed, this, [this](const QString &message) {
        emit trafficLogged(QStringLiteral("!"), message);
        failAll(Reply::Status::NotConnected, message);
        setState(State::Failed, message);
    });
    connect(m_transport, &Transport::closed, this, [this] {
        failAll(Reply::Status::NotConnected, tr("Connection closed"));
        if (m_state != State::Failed)
            setState(State::Disconnected);
    });
}

void ElmSession::open()
{
    if (!m_transport) {
        setState(State::Failed, tr("No adapter selected"));
        return;
    }
    m_protocol = Protocol::Unknown;
    m_adapterVersion.clear();
    m_adapterDescription.clear();
    m_headersEnabled = false;
    m_rxBuffer.clear();
    m_health = {};
    m_latencies.clear();
    m_consecutiveTimeouts = 0;
    m_unansweredLiveRequests = 0;
    emit infoChanged();
    setState(State::Connecting);
    emit trafficLogged(QStringLiteral("!"), tr("Connecting to %1").arg(m_transport->description()));
    m_transport->open();
}

void ElmSession::close()
{
    failAll(Reply::Status::NotConnected, tr("Disconnected"));
    if (m_transport)
        m_transport->close();
    setState(State::Disconnected);
}

void ElmSession::setState(State state, const QString &error)
{
    if (!error.isEmpty())
        m_lastError = error;
    else if (state != State::Failed)
        m_lastError.clear();
    if (m_state == state)
        return;
    m_state = state;
    emit stateChanged(state);
}

void ElmSession::runInitialization()
{
    setState(State::Initializing);

    // Each step is queued in order; the queue guarantees they run one by one.
    // ATZ resets the adapter (echo comes back on), so it must be first.
    send("ATZ", [this](const Reply &r) {
        if (!r.ok()) {
            setState(State::Failed, tr("The adapter did not answer the reset command (%1).").arg(r.error));
            failAll(Reply::Status::NotConnected, m_lastError);
            return;
        }
        for (const QString &line : r.lines) {
            if (line.contains(QLatin1String("ELM327"), Qt::CaseInsensitive))
                m_adapterVersion = line.trimmed();
        }
        emit infoChanged();
    }, Options{.timeoutMs = 5000});

    send("ATE0", [](const Reply &) {});   // echo off: we don't need our own commands back
    send("ATL0", [](const Reply &) {});   // no linefeeds, CR only
    send("ATS1", [](const Reply &) {});   // spaces between bytes (easier to parse and log)
    send("ATH1", [this](const Reply &r) { // headers on: tells us WHICH ECU answered
        m_headersEnabled = r.ok() && r.lines.contains(QStringLiteral("OK"));
    });
    send("ATAT1", [](const Reply &) {});  // adaptive timing: shorter waits once the car is known
    send("ATSP0", [](const Reply &) {});  // automatic protocol detection
    send("AT@1", [this](const Reply &r) {
        if (r.ok() && !r.lines.isEmpty())
            m_adapterDescription = r.lines.first().trimmed();
        emit infoChanged();
    });
    detectVehicle();
}

void ElmSession::detectVehicle()
{
    // "0100" asks every OBD ECU which PIDs it supports. It is the standard way
    // to wake the bus; the first time it also makes the adapter search for the
    // protocol, which can take several seconds on older cars.
    send("0100", [this](const Reply &r) {
        if (r.ok()) {
            send("ATDPN", [this](const Reply &dpn) {
                m_protocol = dpn.ok() && !dpn.lines.isEmpty()
                                 ? protocolFromDpn(dpn.lines.first()) : Protocol::Unknown;
                emit infoChanged();
                m_unansweredLiveRequests = 0;
                m_lastVehicleAnswer.start();
                setState(State::Ready);
            });
            return;
        }
        if (r.status == Reply::Status::NotConnected)
            return;
        setState(State::NoVehicle,
                 tr("The adapter is connected, but the car does not answer. "
                    "Turn the ignition ON (engine may stay off) and retry."));
    }, Options{.timeoutMs = 15000});
}

void ElmSession::send(const QByteArray &command, Callback callback, Options options)
{
    const GuardVerdict verdict = RequestGuard::check(command, options.userConfirmedClear);
    if (!verdict.allowed) {
        emit trafficLogged(QStringLiteral("!"),
                           tr("Blocked \"%1\": %2").arg(QString::fromLatin1(command), verdict.reason));
        Reply r;
        r.command = command;
        r.status = Reply::Status::Blocked;
        r.error = verdict.reason;
        // Always asynchronous, so callers see the same behavior on every path.
        QTimer::singleShot(0, this, [callback = std::move(callback), r] {
            if (callback)
                callback(r);
        });
        return;
    }

    m_queue.enqueue({command, std::move(callback), options});
    processNext();
}

void ElmSession::processNext()
{
    if (m_inFlight || m_resyncing || m_queue.isEmpty())
        return;
    if (!m_transport || !m_transport->isOpen()) {
        failAll(Reply::Status::NotConnected, tr("Not connected"));
        return;
    }

    m_inFlight = m_queue.dequeue();
    m_rxBuffer.clear();
    emit trafficLogged(QStringLiteral(">"), QString::fromLatin1(m_inFlight->command));
    m_commandClock.start();
    m_timeoutTimer.start(m_inFlight->options.timeoutMs);
    m_transport->write(m_inFlight->command + '\r');
}

void ElmSession::onReceived(const QByteArray &data)
{
    m_rxBuffer.append(data);

    const qsizetype prompt = m_rxBuffer.indexOf('>');
    if (prompt < 0)
        return; // reply not complete yet

    const QByteArray reply = m_rxBuffer.left(prompt);
    m_rxBuffer.remove(0, prompt + 1);

    if (m_resyncing) {
        // The late answer to a command that already timed out: throw it away.
        m_resyncTimer.stop();
        m_resyncing = false;
        m_rxBuffer.clear();
        processNext();
        return;
    }
    if (!m_inFlight) {
        m_rxBuffer.clear(); // unsolicited output (e.g. a banner after power-up)
        return;
    }
    finishInFlight(reply);
}

ElmSession::Reply ElmSession::buildReply(const Pending &p, const QByteArray &raw) const
{
    Reply r;
    r.command = p.command;
    r.elapsedMs = m_commandClock.elapsed();

    QByteArray text = raw;
    text.replace('\0', QByteArray()); // some clones emit NUL bytes
    text.replace('\n', '\r');

    const QString echo = QString::fromLatin1(p.command).trimmed().toUpper();
    bool sawNoData = false;
    QStringList errors;

    for (const QByteArray &rawLine : text.split('\r')) {
        const QString line = QString::fromLatin1(rawLine).trimmed();
        if (line.isEmpty())
            continue;
        if (line.toUpper() == echo || QString(line).remove(QLatin1Char(' ')).toUpper()
                                          == QString(echo).remove(QLatin1Char(' ')))
            continue; // our own command echoed back
        if (line.startsWith(QLatin1String("SEARCHING")))
            continue;
        if (line.startsWith(QLatin1String("BUS INIT"))) {
            if (line.contains(QLatin1String("ERROR")))
                errors << line;
            continue;
        }
        if (line == QLatin1String("NO DATA")) {
            sawNoData = true;
            continue;
        }
        if (isErrorLine(line)) {
            errors << line;
            continue;
        }
        r.lines << line;
    }

    if (!r.lines.isEmpty())
        r.status = Reply::Status::Ok; // partial errors next to data still count as data
    else if (!errors.isEmpty()) {
        r.status = Reply::Status::AdapterError;
        r.error = errors.join(QStringLiteral("; "));
    } else if (sawNoData) {
        r.status = Reply::Status::NoData;
        r.error = QStringLiteral("NO DATA");
    } else {
        r.status = Reply::Status::NoData; // prompt with nothing before it
    }
    return r;
}

void ElmSession::finishInFlight(const QByteArray &rawReply)
{
    m_timeoutTimer.stop();
    Pending p = std::move(*m_inFlight);
    m_inFlight.reset();

    const Reply r = buildReply(p, rawReply);
    QString logged = r.lines.join(QStringLiteral(" | "));
    if (logged.isEmpty())
        logged = r.error.isEmpty() ? QStringLiteral("(empty)") : r.error;
    emit trafficLogged(QStringLiteral("<"),
                       QStringLiteral("%1   [%2 ms]").arg(logged).arg(r.elapsedMs));
    emit exchanged(p.command, rawReply, r.elapsedMs, false);
    recordReply(r);

    if (p.callback)
        p.callback(r);
    processNext();
}

void ElmSession::onTimeout()
{
    if (!m_inFlight)
        return;
    Pending p = std::move(*m_inFlight);
    m_inFlight.reset();

    Reply r;
    r.command = p.command;
    r.status = Reply::Status::Timeout;
    r.error = tr("No answer within %1 ms").arg(p.options.timeoutMs);
    r.elapsedMs = m_commandClock.elapsed();
    emit trafficLogged(QStringLiteral("!"), QStringLiteral("%1: %2")
                                                 .arg(QString::fromLatin1(p.command), r.error));
    emit exchanged(p.command, QByteArray(), r.elapsedMs, true);
    recordReply(r);

    m_resyncing = true;
    m_resyncTimer.start();
    if (p.callback)
        p.callback(r);
}

void ElmSession::recordReply(const Reply &r)
{
    ++m_health.commands;
    switch (r.status) {
    case Reply::Status::Timeout: ++m_health.timeouts; break;
    case Reply::Status::AdapterError:
        // A "?" to an AT command only means the adapter lacks it (the adapter
        // check asks on purpose); errors on requests to the car are what matter.
        if (!r.command.startsWith("AT"))
            ++m_health.errors;
        break;
    case Reply::Status::NoData: ++m_health.noData; break;
    default: break;
    }
    if (r.status != Reply::Status::Timeout) {
        m_latencies << r.elapsedMs;
        if (m_latencies.size() > 100)
            m_latencies.removeFirst();
    }

    // Watchdog 1: the adapter itself stopped answering.
    m_consecutiveTimeouts = r.status == Reply::Status::Timeout ? m_consecutiveTimeouts + 1 : 0;
    if (m_state == State::Ready && m_consecutiveTimeouts == kUnresponsiveTimeouts)
        emit adapterUnresponsive();

    // Watchdog 2: the adapter answers, but the car doesn't (ignition off).
    // Only live-data requests count: a module scan legitimately gets many
    // "NO DATA" from modules a car doesn't have.
    if (!r.command.startsWith("01"))
        return;
    if (r.ok()) {
        m_unansweredLiveRequests = 0;
        m_lastVehicleAnswer.start();
        return;
    }
    if (r.status == Reply::Status::NoData || r.status == Reply::Status::AdapterError) {
        ++m_unansweredLiveRequests;
        if (m_state == State::Ready && m_unansweredLiveRequests >= 5 && m_lastVehicleAnswer.isValid()
            && m_lastVehicleAnswer.elapsed() >= m_silentAfterMs) {
            setState(State::NoVehicle, tr("The car stopped answering, probably because the ignition was turned off. "
                                          "Obdaryn picks up again by itself as soon as the car answers."));
            emit vehicleSilent();
        }
    }
}

ElmSession::Health ElmSession::health() const
{
    Health h = m_health;
    if (!m_latencies.isEmpty()) {
        QList<qint64> sorted = m_latencies;
        std::sort(sorted.begin(), sorted.end());
        qint64 sum = 0;
        for (qint64 v : std::as_const(sorted))
            sum += v;
        h.averageMs = double(sum) / sorted.size();
        h.p95Ms = double(sorted[qMin(sorted.size() - 1, qsizetype(sorted.size() * 0.95))]);
    }
    return h;
}

void ElmSession::failAll(Reply::Status status, const QString &error)
{
    m_timeoutTimer.stop();
    m_resyncTimer.stop();
    m_resyncing = false;

    QList<Pending> victims;
    if (m_inFlight) {
        victims << std::move(*m_inFlight);
        m_inFlight.reset();
    }
    while (!m_queue.isEmpty())
        victims << m_queue.dequeue();

    for (Pending &p : victims) {
        Reply r;
        r.command = p.command;
        r.status = status;
        r.error = error;
        if (p.callback)
            p.callback(r);
    }
}

} // namespace obdaryn
