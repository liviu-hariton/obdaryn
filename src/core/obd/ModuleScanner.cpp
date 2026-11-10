// SPDX-License-Identifier: GPL-3.0-or-later

#include "obd/ModuleScanner.h"

#include "obd/ObdClient.h"
#include "obd/Pids.h"

#include <memory>

namespace obdaryn {

namespace {

QByteArray hex3(quint16 id)
{
    return QByteArray::number(id, 16).toUpper().rightJustified(3, '0');
}

// Per-request timeout for module requests. Absent modules cost this much.
constexpr int kModuleTimeoutMs = 1500;
// The keep-alive probe of a silent address. A real ELM327 answers NO DATA in
// ~0.2 s; this only bounds adapters that stay quiet instead.
constexpr int kPingTimeoutMs = 800;

} // namespace

// ---------------------------------------------------------------------------
// Decoding

QString ModuleDtc::displayCode() const
{
    if (!hasFailureType || failureType == 0)
        return code;
    return QStringLiteral("%1-%2").arg(code).arg(failureType, 2, 16, QLatin1Char('0')).toUpper();
}

namespace ModuleDtcCodec {

QList<ModuleDtc> fromUds(const QByteArray &p)
{
    QList<ModuleDtc> out;
    // 59 02 <availability mask> then [DTC high, DTC middle, failure type, status] ...
    if (p.size() < 3 || quint8(p[0]) != 0x59)
        return out;
    for (qsizetype i = 3; i + 3 < p.size(); i += 4) {
        const quint8 status = quint8(p[i + 3]);
        // Bits: 0 test failed now, 2 pending, 3 confirmed, 5 failed since clear, 7 warning lamp.
        if ((status & (0x01 | 0x04 | 0x08 | 0x20 | 0x80)) == 0)
            continue; // the module lists it but nothing has failed
        ModuleDtc d;
        d.code = DtcCodec::decode(quint8(p[i]), quint8(p[i + 1]));
        d.hasFailureType = true;
        d.failureType = quint8(p[i + 2]);
        d.status = status;
        d.kind = (status & 0x08) ? DtcKind::Stored : (status & 0x04) ? DtcKind::Pending : DtcKind::Stored;
        d.activeNow = status & 0x01;
        d.warningLamp = status & 0x80;
        out << d;
    }
    return out;
}

QList<ModuleDtc> fromKwp(const QByteArray &p)
{
    QList<ModuleDtc> out;
    // 58 <count> then [DTC high, DTC low, status] ...
    if (p.size() < 2 || quint8(p[0]) != 0x58)
        return out;
    for (qsizetype i = 2; i + 2 < p.size(); i += 3) {
        const quint8 a = quint8(p[i]), b = quint8(p[i + 1]), status = quint8(p[i + 2]);
        if (a == 0 && b == 0)
            continue;
        ModuleDtc d;
        d.code = DtcCodec::decode(a, b);
        d.status = status;
        // Bits 6-5 "storage state": 11 present now, 01 stored but not present,
        // 10 maturing (pending), 00 not detected (some modules still list it).
        const int storage = (status >> 5) & 0x03;
        d.kind = storage == 0x02 ? DtcKind::Pending : DtcKind::Stored;
        d.activeNow = storage == 0x03;
        d.warningLamp = status & 0x80;
        out << d;
    }
    return out;
}

QString asciiField(const QByteArray &bytes)
{
    QString s;
    for (char c : bytes) {
        if (c >= 0x20 && c < 0x7F)
            s.append(QLatin1Char(c));
    }
    return s.trimmed();
}

} // namespace ModuleDtcCodec

// ---------------------------------------------------------------------------
// Scanning

ModuleScanner::ModuleScanner(ObdClient *client, QObject *parent) : QObject(parent), m_client(client) {}

void ModuleScanner::scan(const VehicleProfile &profile)
{
    if (m_running)
        return;
    m_profileId = profile.id;
    m_results.clear();
    for (const ModuleDef &m : profile.modules)
        m_results << ModuleResult{m, ModuleResult::State::Waiting, DiagProtocol::Auto, {}, {}, {}, {}};
    m_running = true;
    m_cancel = false;
    m_index = -1;
    m_obdResponders.clear();
    emit started(int(m_results.size()));

    // Which modules answer standard OBD-II? (Functional address 7DF.)
    m_client->request("0100", [this](const ObdClient::Response &r) {
        for (const EcuMessage &m : r.positive) {
            bool ok = false;
            const uint id = m.ecu.toUInt(&ok, 16);
            if (ok && m.ecu.size() == 3)
                m_obdResponders.insert(quint16(id));
        }
        scanNext();
    });
}

void ModuleScanner::cancel()
{
    m_cancel = true;
}

void ModuleScanner::scanNext()
{
    ++m_index;
    if (m_cancel || m_index >= m_results.size()) {
        // Remaining modules stay "Waiting" after a cancel.
        restoreAddressing([this] {
            m_running = false;
            emit finished();
        });
        return;
    }

    const int i = m_index;
    m_results[i].state = ModuleResult::State::Scanning;
    emit moduleUpdated(i);

    address(m_results[i].module, [this, i](bool ok) {
        if (!ok) {
            m_results[i].state = ModuleResult::State::Error;
            m_results[i].error = tr("The adapter didn't accept the module address.");
            emit moduleUpdated(i);
            scanNext();
            return;
        }
        readDtcs(i, m_results[i].module.dtc, false, [this, i] {
            if (m_results[i].state != ModuleResult::State::Answered) {
                emit moduleUpdated(i);
                scanNext();
                return;
            }
            readIdentification(i, [this, i] {
                emit moduleUpdated(i);
                scanNext();
            });
        });
    });
}

void ModuleScanner::address(const ModuleDef &m, std::function<void(bool)> done)
{
    ElmSession *s = m_client->session();
    // The commands run in order; only the header decides success, and done()
    // is called exactly once, from the last command.
    auto headerOk = std::make_shared<bool>(false);
    s->send("ATSH" + hex3(m.tx), [headerOk](const ElmSession::Reply &r) { *headerOk = r.ok(); });
    // Only listen to this module's answers.
    s->send("ATCRA" + hex3(m.rx), [](const ElmSession::Reply &) {});
    if (m.needsFlowControlHeader()) {
        // Multi-frame answers need a standard "continue" frame sent to the module's own ID.
        s->send("ATFCSH" + hex3(m.tx), [](const ElmSession::Reply &) {});
        s->send("ATFCSD300000", [](const ElmSession::Reply &) {});
        s->send("ATFCSM1", [headerOk, done](const ElmSession::Reply &) { done(*headerOk); });
    } else {
        s->send("ATFCSM0", [headerOk, done](const ElmSession::Reply &) { done(*headerOk); });
    }
}

void ModuleScanner::restoreAddressing(std::function<void()> done)
{
    ElmSession *s = m_client->session();
    s->send("ATFCSM0", [](const ElmSession::Reply &) {});
    s->send("ATCRA", [](const ElmSession::Reply &) {});                       // listen to everyone again
    s->send("ATSH7DF", [done](const ElmSession::Reply &) { done(); });       // functional OBD-II address
}

void ModuleScanner::readDtcs(int i, DiagProtocol protocol, bool isFallback, std::function<void()> done)
{
    const bool uds = protocol != DiagProtocol::Kwp; // Auto tries UDS first
    const QByteArray cmd = uds ? QByteArray("1902FF") : QByteArray("1802FF00");

    m_client->request(cmd, [this, i, uds, isFallback, done](const ObdClient::Response &r) {
        ModuleResult &res = m_results[i];
        if (r.ok()) {
            res.state = ModuleResult::State::Answered;
            res.protocol = uds ? DiagProtocol::Uds : DiagProtocol::Kwp;
            res.dtcs = uds ? ModuleDtcCodec::fromUds(r.positive.first().data)
                           : ModuleDtcCodec::fromKwp(r.positive.first().data);
            res.error.clear();
            done();
            return;
        }
        if (!isFallback) {
            if (r.reply.status == ElmSession::Reply::Status::NotConnected) {
                res.state = ModuleResult::State::Error;
                res.error = r.errorText();
                done();
                return;
            }
            if (r.negative.isEmpty()) {
                // Silence. Some modules ignore requests they don't support
                // instead of refusing them, so don't conclude "absent" yet.
                if (m_obdResponders.contains(res.module.rx)) {
                    res.state = ModuleResult::State::ObdOnly;
                    res.error.clear();
                    done();
                    return;
                }
                // One keep-alive (TesterPresent) tells "not fitted" from "fitted but silent".
                m_client->request("3E00", [this, i, uds, done](const ObdClient::Response &ping) {
                    ModuleResult &res = m_results[i];
                    if (ping.positive.isEmpty() && ping.negative.isEmpty()) {
                        res.state = ModuleResult::State::Absent;
                        done();
                        return;
                    }
                    readDtcs(i, uds ? DiagProtocol::Kwp : DiagProtocol::Uds, true, done);
                }, ElmSession::Options{.timeoutMs = kPingTimeoutMs});
                return;
            }
            // The module is there but refused this protocol: try the other one.
            readDtcs(i, uds ? DiagProtocol::Kwp : DiagProtocol::Uds, true, done);
            return;
        }
        res.state = ModuleResult::State::Error;
        res.error = r.negative.isEmpty()
                        ? tr("The module answers, but not to trouble-code requests. Scan again with the ignition ON; if it persists, this module needs the brand's own diagnostic tool.")
                        : tr("%1 Scan again with the ignition ON; if it persists, this module needs the brand's own diagnostic tool.").arg(r.errorText());
        done();
    }, ElmSession::Options{.timeoutMs = kModuleTimeoutMs});
}

void ModuleScanner::readIdentification(int i, std::function<void()> done)
{
    const bool uds = m_results[i].protocol == DiagProtocol::Uds;
    // UDS: F187 spare-part number, F189 software version. KWP: 0x87 spare-part number.
    const QByteArray partCmd = uds ? QByteArray("22F187") : QByteArray("1A87");
    const int skip = uds ? 3 : 2; // "62 F1 87" or "5A 87"

    m_client->request(partCmd, [this, i, uds, skip, done](const ObdClient::Response &r) {
        if (r.ok())
            m_results[i].partNumber = ModuleDtcCodec::asciiField(r.positive.first().data.mid(skip));
        if (!uds) {
            done();
            return;
        }
        m_client->request("22F189", [this, i, done](const ObdClient::Response &r2) {
            if (r2.ok())
                m_results[i].softwareVersion = ModuleDtcCodec::asciiField(r2.positive.first().data.mid(3));
            done();
        }, ElmSession::Options{.timeoutMs = kModuleTimeoutMs});
    }, ElmSession::Options{.timeoutMs = kModuleTimeoutMs});
}

// ---------------------------------------------------------------------------
// Clearing one module

void ModuleScanner::clearModule(int i, std::function<void(bool, const QString &)> callback)
{
    if (m_running || i < 0 || i >= m_results.size() || m_results[i].state != ModuleResult::State::Answered) {
        callback(false, tr("Scan the modules first."));
        return;
    }
    m_running = true;

    // Same safety rule as the OBD-II clear: never with the engine running.
    // Engine speed is read with the normal addressing (restored after every scan).
    m_client->readPids({0x0C}, [this, i, callback](const QList<ObdClient::PidReading> &readings) {
        for (const auto &rd : readings) {
            double rpm = 0;
            const PidDefinition *def = PidTable::find(0x0C);
            if (def && def->value(rd.raw, &rpm) && rpm > 0) {
                m_running = false;
                callback(false, tr("The engine is running (%1 rpm). Switch it off, leave the ignition ON, and try again.")
                                    .arg(qRound(rpm)));
                return;
            }
        }

        address(m_results[i].module, [this, i, callback](bool ok) {
            if (!ok) {
                restoreAddressing([this, callback] {
                    m_running = false;
                    callback(false, tr("The adapter didn't accept the module address."));
                });
                return;
            }
            const bool uds = m_results[i].protocol == DiagProtocol::Uds;
            // UDS: group FFFFFF = all codes. KWP2000: group FF00 = all codes.
            const QByteArray cmd = uds ? QByteArray("14FFFFFF") : QByteArray("14FF00");
            m_client->request(cmd, [this, i, uds, callback](const ObdClient::Response &r) {
                const bool cleared = r.ok();
                const QString message = cleared ? QString() : r.errorText();
                // Re-read what's left: a fault that is still present comes straight back.
                readDtcs(i, uds ? DiagProtocol::Uds : DiagProtocol::Kwp, true, [this, i, cleared, message, callback] {
                    emit moduleUpdated(i);
                    restoreAddressing([this, cleared, message, callback] {
                        m_running = false;
                        callback(cleared, message);
                    });
                });
            }, ElmSession::Options{.timeoutMs = 5000, .userConfirmedClear = true});
        });
    });
}

} // namespace obdaryn
