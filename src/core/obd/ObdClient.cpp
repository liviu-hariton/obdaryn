// SPDX-License-Identifier: GPL-3.0-or-later

#include "obd/ObdClient.h"

#include <QTimer>

#include "obd/Pids.h"
#include "obd/VehicleInfo.h"

#include <QCoreApplication>
#include <QSet>

namespace obdaryn {

QByteArray hexCommand(const QList<quint8> &bytes)
{
    QByteArray raw;
    for (quint8 b : bytes)
        raw.append(char(b));
    return raw.toHex().toUpper();
}

void ObdClient::setResponseCountsEnabled(bool enabled)
{
    m_responseCountsEnabled = enabled;
    if (!enabled)
        m_responseCounts = ResponseCounts::Off;
    else if (m_responseCounts == ResponseCounts::Off)
        m_responseCounts = ResponseCounts::Unknown;
}

int ObdClient::expectedResponses(const QList<quint8> &group) const
{
    if (m_responseCounts == ResponseCounts::NotSupported || m_responseCounts == ResponseCounts::Off
        || !isCan() || !m_session->headersEnabled() || m_ecuPids.isEmpty())
        return 0;
    // One CAN frame holds 7 bytes: 41, then each PID and its data.
    int bytes = 1;
    for (quint8 pid : group) {
        const int len = PidTable::dataLength(pid);
        if (len < 0)
            return 0;
        bytes += 1 + len;
    }
    if (bytes > 7)
        return 0;
    int ecus = 0;
    for (auto it = m_ecuPids.cbegin(); it != m_ecuPids.cend(); ++it) {
        for (quint8 pid : group) {
            if (it.value().contains(pid)) {
                ++ecus;
                break;
            }
        }
    }
    return ecus >= 1 && ecus <= 15 ? ecus : 0;
}

QByteArray hexCommand(std::initializer_list<quint8> bytes)
{
    return hexCommand(QList<quint8>(bytes));
}

// ---------------------------------------------------------------------------
// Generic request

QString ObdClient::Response::errorText() const
{
    if (ok())
        return {};
    if (!negative.isEmpty() && negative.first().data.size() >= 3)
        return negativeResponseText(quint8(negative.first().data[2]));
    switch (reply.status) {
    case ElmSession::Reply::Status::NoData:
        return QCoreApplication::translate("ObdClient", "No module answered this request.");
    case ElmSession::Reply::Status::Timeout:
        return QCoreApplication::translate("ObdClient", "The adapter did not answer in time.");
    case ElmSession::Reply::Status::Blocked:
        return QCoreApplication::translate("ObdClient", "Blocked by Obdaryn's safety guard: %1")
            .arg(reply.error);
    case ElmSession::Reply::Status::NotConnected:
        return QCoreApplication::translate("ObdClient", "Not connected.");
    case ElmSession::Reply::Status::AdapterError:
        return QCoreApplication::translate("ObdClient", "Adapter error: %1").arg(reply.error);
    case ElmSession::Reply::Status::Ok:
        break;
    }
    return QCoreApplication::translate("ObdClient", "Unexpected answer.");
}

QString ObdClient::negativeResponseText(quint8 nrc)
{
    auto t = [](const char *s) { return QCoreApplication::translate("Nrc", s); };
    switch (nrc) {
    case 0x10: return t(QT_TRANSLATE_NOOP("Nrc", "The module rejected the request."));
    case 0x11: return t(QT_TRANSLATE_NOOP("Nrc", "The module does not support this service."));
    case 0x12: return t(QT_TRANSLATE_NOOP("Nrc", "The module does not support this sub-function."));
    case 0x13: return t(QT_TRANSLATE_NOOP("Nrc", "Wrong request length."));
    case 0x21: return t(QT_TRANSLATE_NOOP("Nrc", "The module is busy; try again."));
    case 0x22: return t(QT_TRANSLATE_NOOP("Nrc", "Conditions not correct (for example, the engine is running)."));
    case 0x31: return t(QT_TRANSLATE_NOOP("Nrc", "The requested value does not exist in this module."));
    case 0x33: return t(QT_TRANSLATE_NOOP("Nrc", "Access denied by the module."));
    case 0x78: return t(QT_TRANSLATE_NOOP("Nrc", "The module is still working on the answer."));
    case 0x7E:
    case 0x7F: return t(QT_TRANSLATE_NOOP("Nrc", "Not available in the current diagnostic session."));
    default:
        return t(QT_TRANSLATE_NOOP("Nrc", "Negative response 0x%1.")).arg(nrc, 2, 16, QLatin1Char('0'));
    }
}

ObdClient::ObdClient(QObject *parent) : QObject(parent), m_session(new ElmSession(this))
{
    // A new connection may be a different car: re-detect multi-PID support.
    connect(m_session, &ElmSession::stateChanged, this, [this](ElmSession::State s) {
        if (s == ElmSession::State::Connecting) {
            m_multiPidSupported = true;
            m_responseCounts = m_responseCountsEnabled ? ResponseCounts::Unknown : ResponseCounts::Off;
            m_ecuPids.clear();
        }
    });
}

void ObdClient::request(const QByteArray &command, ResponseCallback callback, ElmSession::Options options)
{
    const QByteArray bytes = QByteArray::fromHex(command.left(2));
    const quint8 sid = bytes.isEmpty() ? 0 : quint8(bytes[0]);

    m_session->send(command, [this, sid, callback = std::move(callback)](const ElmSession::Reply &r) {
        Response resp;
        resp.reply = r;
        if (r.ok()) {
            const auto messages = ResponseParser::parse(r.lines, m_session->protocol(),
                                                        m_session->headersEnabled());
            for (const EcuMessage &m : messages) {
                if (m.data.isEmpty())
                    continue;
                const quint8 first = quint8(m.data[0]);
                if (first == quint8(sid + 0x40)) {
                    resp.positive << m;
                } else if (first == 0x7F && m.data.size() >= 3 && quint8(m.data[1]) == sid) {
                    // 0x78 = "response pending": the real answer follows in the same reply.
                    if (quint8(m.data[2]) != 0x78)
                        resp.negative << m;
                }
            }
        }
        if (callback)
            callback(resp);
    }, options);
}

// ---------------------------------------------------------------------------
// Supported PIDs

struct ObdClient::SupportedState {
    quint8 service = 0x01;
    quint8 base = 0x00;
    QSet<quint8> pids;
    std::function<void(const QList<quint8> &)> callback;
};

void ObdClient::readSupportedPids(quint8 service, std::function<void(const QList<quint8> &)> callback)
{
    if (service == 0x01)
        m_ecuPids.clear();
    auto state = std::make_shared<SupportedState>();
    state->service = service;
    state->callback = std::move(callback);
    supportedStep(state);
}

void ObdClient::supportedStep(std::shared_ptr<SupportedState> st)
{
    // Service 02 needs a frame number after the PID: "02 00 00".
    const QByteArray cmd = st->service == 0x02 ? hexCommand({0x02, st->base, 0x00})
                                               : hexCommand({st->service, st->base});
    const int offset = st->service == 0x02 ? 3 : 2; // where the 4 bitmap bytes start

    request(cmd, [this, st, offset](const Response &r) {
        bool nextRange = false;
        for (const EcuMessage &m : r.positive) {
            if (m.data.size() < offset + 4 || quint8(m.data[1]) != st->base)
                continue;
            if (st->service == 0x01)
                m_ecuPids[m.ecu].insert(st->base); // it answers this bitmap request too
            for (int i = 0; i < 32; ++i) {
                const quint8 byte = quint8(m.data[offset + i / 8]);
                if (byte & (0x80 >> (i % 8))) {
                    const quint8 pid = quint8(st->base + i + 1);
                    if (st->service == 0x01)
                        m_ecuPids[m.ecu].insert(pid);
                    if (PidTable::isSupportBitmap(pid))
                        nextRange = true;
                    else
                        st->pids.insert(pid);
                }
            }
        }
        if (nextRange && st->base < 0xE0) {
            st->base += 0x20;
            supportedStep(st);
            return;
        }
        QList<quint8> sorted(st->pids.begin(), st->pids.end());
        std::sort(sorted.begin(), sorted.end());
        st->callback(sorted);
    });
}

// ---------------------------------------------------------------------------
// Live values

struct ObdClient::PidBatchState {
    QList<QList<quint8>> groups;
    int index = 0;
    QList<PidReading> readings;
    std::function<void(const QList<PidReading> &)> callback;
};

void ObdClient::readPids(const QList<quint8> &pids, std::function<void(const QList<PidReading> &)> callback)
{
    auto st = std::make_shared<PidBatchState>();
    st->callback = std::move(callback);

    // Group PIDs whose answer length is known (needed to split a combined
    // answer); everything else is requested on its own.
    const bool batch = isCan() && m_multiPidSupported;
    QList<quint8> current;
    for (quint8 pid : pids) {
        if (batch && PidTable::dataLength(pid) > 0) {
            current << pid;
            if (current.size() == 6) {
                st->groups << current;
                current.clear();
            }
        } else {
            st->groups << QList<quint8>{pid};
        }
    }
    if (!current.isEmpty())
        st->groups << current;

    pidBatchStep(st);
}

void ObdClient::pidBatchStep(std::shared_ptr<PidBatchState> st)
{
    if (st->index >= st->groups.size()) {
        st->callback(st->readings);
        return;
    }
    const QList<quint8> group = st->groups[st->index];
    QList<quint8> cmd{0x01};
    cmd += group;
    QByteArray command = hexCommand(cmd);
    const int expected = expectedResponses(group);
    if (expected > 0)
        command += QByteArray::number(expected, 16).toUpper();

    request(command, [this, st, group, expected](const Response &r) {
        if (expected > 0) {
            if (r.reply.status == ElmSession::Reply::Status::AdapterError) {
                // "?": this adapter doesn't understand the digit. Plain requests from now on.
                m_responseCounts = ResponseCounts::NotSupported;
                pidBatchStep(st); // the same group again, without it
                return;
            }
            if (r.ok())
                m_responseCounts = ResponseCounts::Working;
        }
        QSet<quint8> answered;
        for (const EcuMessage &m : r.positive) {
            // Walk "41 <pid> <data...> <pid> <data...>".
            qsizetype i = 1;
            while (i < m.data.size()) {
                const quint8 pid = quint8(m.data[i]);
                const int len = PidTable::dataLength(pid);
                if (len < 0 || i + 1 + len > m.data.size())
                    break;
                st->readings << PidReading{pid, m.data.mid(i + 1, len), m.ecu};
                answered.insert(pid);
                i += 1 + len;
            }
        }

        // Some ECUs only answer the first PID of a combined request. Detect it
        // once, then fall back to one PID per request for this connection.
        if (group.size() > 1 && r.ok() && answered.size() <= 1 && m_multiPidSupported) {
            m_multiPidSupported = false;
            for (quint8 pid : group) {
                if (!answered.contains(pid))
                    st->groups.insert(st->index + 1, QList<quint8>{pid});
            }
        }
        ++st->index;
        pidBatchStep(st);
    });
}

void ObdClient::readMonitorTests(std::function<void(const QList<MonitorTest> &, const QString &)> callback)
{
    if (!isCan()) {
        QTimer::singleShot(0, this, [callback] {
            callback({}, tr("Test results are read on CAN cars only for now (most cars from 2008 on)."));
        });
        return;
    }
    readSupportedPids(0x06, [this, callback](const QList<quint8> &mids) {
        if (mids.isEmpty()) {
            callback({}, tr("The car doesn't report any test results."));
            return;
        }
        auto results = std::make_shared<QList<MonitorTest>>();
        auto remaining = std::make_shared<int>(int(mids.size()));
        for (quint8 mid : mids) {
            request(hexCommand({0x06, mid}), [results, remaining, callback](const Response &r) {
                for (const EcuMessage &m : r.positive)
                    *results += MonitorTestCodec::fromResponse(m);
                if (--*remaining == 0)
                    callback(*results, QString());
            });
        }
    });
}

void ObdClient::readMonitorStatus(std::function<void(const MonitorStatus &)> callback)
{
    request("0101", [callback = std::move(callback)](const Response &r) {
        // Several ECUs may answer; the engine ECU (sorted first, 7E8) has the
        // full picture, the others usually report all zeros.
        MonitorStatus best;
        for (const EcuMessage &m : r.positive) {
            if (m.data.size() < 6 || quint8(m.data[1]) != 0x01)
                continue;
            const MonitorStatus s = decodeMonitorStatus(m.data.mid(2, 4));
            if (!best.valid || s.monitors.size() > best.monitors.size())
                best = s;
        }
        callback(best);
    });
}

// ---------------------------------------------------------------------------
// Trouble codes

void ObdClient::readDtcs(DtcKind kind, std::function<void(const QList<Dtc> &, const QString &)> callback)
{
    const char *cmd = kind == DtcKind::Stored ? "03" : kind == DtcKind::Pending ? "07" : "0A";
    request(cmd, [this, kind, callback = std::move(callback)](const Response &r) {
        QList<Dtc> dtcs;
        for (const EcuMessage &m : r.positive) {
            for (const QString &code : DtcCodec::fromResponse(m, isCan()))
                dtcs << Dtc{code, m.ecu, kind};
        }
        // "NO DATA" here usually just means "no codes" on older cars.
        const bool noAnswer = r.reply.status == ElmSession::Reply::Status::NoData;
        callback(dtcs, r.ok() || noAnswer ? QString() : r.errorText());
    }, ElmSession::Options{.timeoutMs = 4000});
}

void ObdClient::readAllDtcs(std::function<void(const QList<Dtc> &, const QString &)> callback)
{
    auto all = std::make_shared<QList<Dtc>>();
    auto errors = std::make_shared<QStringList>();
    auto collect = [all, errors](const QList<Dtc> &d, const QString &e) {
        *all += d;
        if (!e.isEmpty())
            *errors << e;
    };
    readDtcs(DtcKind::Stored, collect);
    readDtcs(DtcKind::Pending, collect);
    // Permanent codes exist on cars from ~2010 (US) onward; many EU cars answer NO DATA.
    readDtcs(DtcKind::Permanent, [collect, all, errors, callback = std::move(callback)](
                                     const QList<Dtc> &d, const QString &e) {
        collect(d, e);
        errors->removeDuplicates();
        callback(*all, errors->join(QStringLiteral(" ")));
    });
}

struct ObdClient::FreezeState {
    FreezeFrame frame;
    QList<quint8> pids;
    int index = 0;
    std::function<void(const FreezeFrame &)> callback;
};

void ObdClient::readFreezeFrame(std::function<void(const FreezeFrame &)> callback)
{
    auto st = std::make_shared<FreezeState>();
    st->callback = std::move(callback);

    // PID 02 of service 02 = the code that caused freeze frame 0.
    request("020200", [this, st](const Response &r) {
        for (const EcuMessage &m : r.positive) {
            if (m.data.size() >= 5 && quint8(m.data[1]) == 0x02) {
                const quint8 a = quint8(m.data[3]);
                const quint8 b = quint8(m.data[4]);
                if (a || b) {
                    st->frame.available = true;
                    st->frame.dtc = DtcCodec::decode(a, b);
                    break;
                }
            }
        }
        if (!st->frame.available) {
            st->callback(st->frame);
            return;
        }
        readSupportedPids(0x02, [this, st](const QList<quint8> &pids) {
            for (quint8 pid : pids) {
                const PidDefinition *def = PidTable::find(pid);
                if (def && def->isNumeric())
                    st->pids << pid;
            }
            freezeStep(st);
        });
    });
}

void ObdClient::freezeStep(std::shared_ptr<FreezeState> st)
{
    if (st->index >= st->pids.size()) {
        st->callback(st->frame);
        return;
    }
    const quint8 pid = st->pids[st->index++];
    request(hexCommand({0x02, pid, 0x00}), [this, st, pid](const Response &r) {
        for (const EcuMessage &m : r.positive) {
            if (m.data.size() > 3 && quint8(m.data[1]) == pid) {
                st->frame.values << PidReading{pid, m.data.mid(3), m.ecu};
                break;
            }
        }
        freezeStep(st);
    });
}

void ObdClient::clearDtcs(std::function<void(ClearResult, const QString &)> callback)
{
    // Safety check first: never clear with the engine running.
    readPids({0x0C}, [this, callback = std::move(callback)](const QList<PidReading> &readings) {
        for (const PidReading &rd : readings) {
            double rpm = 0;
            const PidDefinition *def = PidTable::find(0x0C);
            if (def && def->value(rd.raw, &rpm) && rpm > 0) {
                callback(ClearResult::EngineRunning,
                         tr("The engine is running (%1 rpm). Switch the engine off, leave the "
                            "ignition ON, and try again.").arg(qRound(rpm)));
                return;
            }
        }

        request("04", [callback](const Response &r) {
            if (r.ok()) {
                callback(ClearResult::Cleared, {});
            } else if (!r.negative.isEmpty()) {
                callback(ClearResult::Rejected, r.errorText());
            } else {
                callback(ClearResult::NoResponse, r.errorText());
            }
        }, ElmSession::Options{.timeoutMs = 6000, .userConfirmedClear = true});
    });
}

// ---------------------------------------------------------------------------
// Vehicle information

struct ObdClient::InfoState {
    VehicleInformation info;
    QList<quint8> pids;
    int index = 0;
    std::function<void(const VehicleInformation &)> callback;
};

void ObdClient::readVehicleInfo(std::function<void(const VehicleInformation &)> callback)
{
    auto st = std::make_shared<InfoState>();
    st->callback = std::move(callback);
    readSupportedPids(0x09, [this, st](const QList<quint8> &supported) {
        // 02 VIN, 04 calibration IDs, 06 CVNs, 0A ECU names.
        for (quint8 pid : {quint8(0x02), quint8(0x04), quint8(0x06), quint8(0x0A)}) {
            if (supported.contains(pid))
                st->pids << pid;
        }
        // Some cars don't report support but still answer the VIN.
        if (!st->pids.contains(0x02))
            st->pids.prepend(0x02);
        infoStep(st);
    });
}

void ObdClient::infoStep(std::shared_ptr<InfoState> st)
{
    if (st->index >= st->pids.size()) {
        st->callback(st->info);
        return;
    }
    const quint8 pid = st->pids[st->index++];
    request(hexCommand({0x09, pid}), [this, st, pid](const Response &r) {
        const bool can = isCan();
        for (const EcuMessage &m : r.positive) {
            if (m.data.size() < 2 || quint8(m.data[1]) != pid)
                continue;
            switch (pid) {
            case 0x02:
                if (st->info.vin.isEmpty())
                    st->info.vin = VehicleInfoCodec::vin(m, can);
                break;
            case 0x04:
                st->info.calibrationIds += VehicleInfoCodec::calibrationIds(m, can);
                break;
            case 0x06:
                st->info.cvns += VehicleInfoCodec::cvns(m, can);
                break;
            case 0x0A:
                st->info.ecuNames.insert(m.ecu, VehicleInfoCodec::ecuName(m, can));
                break;
            default:
                break;
            }
        }
        infoStep(st);
    }, ElmSession::Options{.timeoutMs = 4000});
}

void ObdClient::readBatteryVoltage(std::function<void(double)> callback)
{
    m_session->send("ATRV", [callback = std::move(callback)](const ElmSession::Reply &r) {
        double volts = 0;
        if (r.ok() && !r.lines.isEmpty()) {
            QString s = r.lines.first().trimmed();
            s.remove(QLatin1Char('V'), Qt::CaseInsensitive);
            volts = s.toDouble();
        }
        callback(volts);
    });
}

} // namespace obdaryn
