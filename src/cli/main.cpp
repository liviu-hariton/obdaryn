// SPDX-License-Identifier: GPL-3.0-or-later
//
// obdaryn-probe: command-line check of an adapter and a car.
//
//   obdaryn-probe                         # 192.168.0.10:35000 (Vgate iCar Pro Wi-Fi), short summary
//   obdaryn-probe 127.0.0.1 35000         # Python ELM327-emulator
//   obdaryn-probe --demo                  # built-in simulated car
//   options:
//     --modules   also scan every module of the car's brand (ABS, airbag, ...)
//     --report    full first-contact report: adapter fingerprint, every live
//                 value with raw bytes, readiness, freeze frame, vehicle info,
//                 codes, module scan and a polling-speed benchmark
//     --verbose   print the raw adapter traffic too
//     --record FILE  save the whole session as a recording (.obdrec) that the
//                    app can replay without the car
//     --replay FILE  run against a recording instead of an adapter
//
// It uses exactly the same core library as the app, so it tells "adapter or
// car problem" from "UI problem". Like the app, it only reads: every command
// still passes the safety guard, and nothing here clears codes.

#include "obd/DtcDatabase.h"
#include "obd/ModuleScanner.h"
#include "obd/ObdClient.h"
#include "obd/Pids.h"
#include "obd/Readiness.h"
#include "obd/VehicleInfo.h"
#include "recording/SessionRecorder.h"
#include "transport/ReplayTransport.h"
#include "transport/SimulatedTransport.h"
#include "transport/TcpTransport.h"
#include "vehicles/ModuleCatalog.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QTextStream>
#include <QTimer>

#include <functional>
#include <memory>

using namespace obdaryn;

namespace {

/// Runs asynchronous steps one after another.
class Steps
{
public:
    using Next = std::function<void()>;
    using Step = std::function<void(Next next)>;
    void add(Step s) { m_steps << std::move(s); }
    void run(Next done)
    {
        m_done = std::move(done);
        advance();
    }

private:
    void advance()
    {
        if (m_index >= m_steps.size()) {
            m_done();
            return;
        }
        // Run each step from the event loop, so deep callback chains don't nest.
        QTimer::singleShot(0, [this] { m_steps[m_index++]([this] { advance(); }); });
    }
    QList<Step> m_steps;
    qsizetype m_index = 0;
    Next m_done;
};

QString hex(const QByteArray &b)
{
    return QString::fromLatin1(b.toHex(' ').toUpper());
}

QString pidHex(quint8 p)
{
    return QStringLiteral("%1").arg(p, 2, 16, QLatin1Char('0')).toUpper();
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    QStringList args = app.arguments().mid(1);
    const bool report = args.removeAll(QStringLiteral("--report")) > 0;
    const bool modules = args.removeAll(QStringLiteral("--modules")) > 0 || report;
    const bool verbose = args.removeAll(QStringLiteral("--verbose")) > 0;
    const bool demo = args.removeAll(QStringLiteral("--demo")) > 0;
    // "--record FILE" / "--replay FILE": take the option and its value out of the list.
    auto takeValue = [&args](const QString &option) {
        const qsizetype i = args.indexOf(option);
        if (i < 0 || i + 1 >= args.size())
            return QString();
        const QString value = args.takeAt(i + 1);
        args.removeAt(i);
        return value;
    };
    const QString recordPath = takeValue(QStringLiteral("--record"));
    const QString replayPath = takeValue(QStringLiteral("--replay"));

    ObdClient client;
    SessionRecorder recorder(client.session());
    if (!replayPath.isEmpty()) {
        Recording recording;
        QString error;
        if (!recording.load(replayPath, &error)) {
            out << "Can't replay " << replayPath << ": " << error << Qt::endl;
            return 1;
        }
        client.session()->setTransport(new ReplayTransport(std::move(recording), QFileInfo(replayPath).fileName()));
    } else if (demo) {
        client.session()->setTransport(new SimulatedTransport);
    } else {
        const QString host = args.value(0, QStringLiteral("192.168.0.10"));
        const quint16 port = quint16(args.value(1, QStringLiteral("35000")).toUInt());
        client.session()->setTransport(new TcpTransport(host, port));
    }
    if (verbose) {
        QObject::connect(client.session(), &ElmSession::trafficLogged, [&out](const QString &d, const QString &t) {
            out << "    " << d << ' ' << t << Qt::endl;
        });
    }

    // Recording starts before connecting, so the handshake is in the file too.
    if (!recordPath.isEmpty()) {
        QString error;
        if (!recorder.start(recordPath, {{QStringLiteral("app"), QStringLiteral("obdaryn-probe")},
                                         {QStringLiteral("demo"), demo}}, &error)) {
            out << "Can't record to " << recordPath << ": " << error << Qt::endl;
            return 1;
        }
        out << "Recording to " << recordPath << Qt::endl;
    }

    out << "Obdaryn probe, " << QDateTime::currentDateTime().toString(Qt::ISODate)
        << (report ? "  (full report)" : "") << Qt::endl;

    // Give up after 5 minutes whatever happens (a full report takes about a minute).
    QTimer::singleShot(300000, &app, [&] { out << "Timed out." << Qt::endl; app.exit(2); });

    QString make, vin;
    Steps steps;
    auto section = [&out](const char *title) { out << Qt::endl << "== " << title << " ==" << Qt::endl; };

    // ---- Adapter fingerprint (report only): which allowed AT commands it knows.
    if (report) {
        steps.add([&](Steps::Next next) {
            section("Adapter");
            const QList<QByteArray> cmds = {"ATI", "AT@1", "ATDP", "ATDPN", "ATRV", "ATST32", "ATAT2", "ATAT1",
                                            "ATCRA7E8", "ATCRA", "ATFCSH7E0", "ATFCSD300000", "ATFCSM1", "ATFCSM0", "ATH1"};
            // The session queue runs these in order; the last callback continues.
            for (qsizetype i = 0; i < cmds.size(); ++i) {
                const bool last = i == cmds.size() - 1;
                client.session()->send(cmds[i], [&out, cmd = cmds[i], last, next](const ElmSession::Reply &r) {
                    out << "  " << QString::fromLatin1(cmd).leftJustified(14)
                        << (r.ok() ? r.lines.join(QStringLiteral(" | ")) : r.error) << "   [" << r.elapsedMs << " ms]" << Qt::endl;
                    if (last)
                        next();
                });
            }
        });
    }

    // ---- Vehicle info
    steps.add([&](Steps::Next next) {
        section("Vehicle");
        out << "  Adapter:  " << client.session()->adapterVersion() << " / " << client.session()->adapterDescription() << Qt::endl;
        out << "  Protocol: " << protocolDisplayName(client.session()->protocol()) << Qt::endl;
        client.readVehicleInfo([&, next](const ObdClient::VehicleInformation &info) {
            const VinInfo v = decodeVin(info.vin);
            make = v.manufacturer;
            vin = v.vin;
            out << "  VIN:      " << (v.vin.isEmpty() ? QStringLiteral("(not reported)") : v.vin);
            if (!v.manufacturer.isEmpty())
                out << "  (" << v.manufacturer << ", " << v.modelYear << ", check digit "
                    << (v.checkDigitValid ? "valid" : "not valid") << ")";
            out << Qt::endl;
            for (auto it = info.ecuNames.begin(); it != info.ecuNames.end(); ++it)
                out << "  Module:   " << it.key() << "  " << it.value() << Qt::endl;
            for (const QString &c : info.calibrationIds)
                out << "  CALID:    " << c << Qt::endl;
            for (const QString &c : info.cvns)
                out << "  CVN:      " << c << Qt::endl;
            next();
        });
    });

    // ---- Readiness
    steps.add([&](Steps::Next next) {
        section("Readiness");
        client.readMonitorStatus([&, next](const MonitorStatus &st) {
            if (!st.valid) {
                out << "  (not reported)" << Qt::endl;
                next();
                return;
            }
            out << "  MIL " << (st.milOn ? "ON" : "off") << ", " << st.storedDtcCount << " stored, "
                << (st.compressionIgnition ? "diesel" : "petrol") << " monitors, incomplete "
                << st.incompleteCount() << '/' << st.monitors.size() << Qt::endl;
            for (const Monitor &m : st.monitors)
                out << "  " << QString::fromLatin1(m.name).leftJustified(30) << (m.complete ? "complete" : "NOT complete") << Qt::endl;
            next();
        });
    });

    // ---- Codes
    steps.add([&](Steps::Next next) {
        section("Trouble codes (generic OBD-II)");
        client.readAllDtcs([&, next](const QList<Dtc> &dtcs, const QString &err) {
            if (dtcs.isEmpty())
                out << "  none" << (err.isEmpty() ? QString() : "  (" + err + ")") << Qt::endl;
            const QString brand = ModuleCatalog::forVehicle(make, vin).id;
            for (const Dtc &d : dtcs) {
                const QString module = d.ecu == QLatin1String("7E9") ? QStringLiteral("transmission") : QStringLiteral("engine");
                const DtcInfo info = DtcDatabase::instance().lookup(d.code, brand, QStringLiteral("en"), module);
                const char *kind = d.kind == DtcKind::Stored ? "stored" : d.kind == DtcKind::Pending ? "pending" : "permanent";
                out << "  " << d.code << "  [" << kind << ", " << d.ecu << "]  " << (info.known ? info.title : info.system) << Qt::endl;
            }
            next();
        });
    });

    if (report) {
        // ---- Every supported live value, with raw bytes (to compare with Car Scanner)
        steps.add([&](Steps::Next next) {
            section("Live data (one reading each)");
            client.readSupportedPids(0x01, [&, next](const QList<quint8> &pids) {
                QStringList list;
                for (quint8 p : pids)
                    list << pidHex(p);
                out << "  Supported PIDs (" << pids.size() << "): " << list.join(QLatin1Char(' ')) << Qt::endl;
                client.readPids(pids, [&, next](const QList<ObdClient::PidReading> &rs) {
                    for (const auto &r : rs) {
                        const PidDefinition *def = PidTable::find(r.pid);
                        double v = 0;
                        const QString value = def && def->value(r.raw, &v)
                                                  ? QString::number(v, 'f', def->decimals) + ' ' + def->translatedUnit()
                                                  : PidTable::describeStatus(r.pid, r.raw);
                        out << "  " << pidHex(r.pid) << "  " << (def ? def->translatedName() : QStringLiteral("unknown")).leftJustified(42)
                            << value.leftJustified(20) << "raw " << hex(r.raw) << "  (" << r.ecu << ")" << Qt::endl;
                    }
                    next();
                });
            });
        });

        // ---- Freeze frame
        steps.add([&](Steps::Next next) {
            section("Freeze frame");
            client.readFreezeFrame([&, next](const ObdClient::FreezeFrame &ff) {
                if (!ff.available) {
                    out << "  none stored" << Qt::endl;
                    next();
                    return;
                }
                out << "  Triggered by " << ff.dtc << Qt::endl;
                for (const auto &r : ff.values) {
                    const PidDefinition *def = PidTable::find(r.pid);
                    double v = 0;
                    if (def && def->value(r.raw, &v))
                        out << "  " << def->translatedName().leftJustified(42) << v << ' ' << def->translatedUnit() << Qt::endl;
                }
                next();
            });
        });

        // ---- Polling benchmark: values per second the dashboard will get
        steps.add([&](Steps::Next next) {
            section("Polling speed");
            auto clock = std::make_shared<QElapsedTimer>();
            auto count = std::make_shared<int>(0);
            auto rounds = std::make_shared<int>(0);
            auto loop = std::make_shared<std::function<void()>>();
            clock->start();
            *loop = [&, clock, count, rounds, loop, next] {
                if (++*rounds > 20) {
                    const double secs = clock->elapsed() / 1000.0;
                    out << "  " << *count << " values in " << QString::number(secs, 'f', 2) << " s = "
                        << QString::number(*count / secs, 'f', 1) << " values/s" << Qt::endl;
                    QTimer::singleShot(0, [loop] { *loop = nullptr; }); // break the self-reference later
                    next();
                    return;
                }
                client.readPids({0x0C, 0x0D, 0x05, 0x04, 0x11, 0x0B}, [count, loop](const QList<ObdClient::PidReading> &rs) {
                    *count += int(rs.size());
                    QTimer::singleShot(0, [loop] { if (*loop) (*loop)(); });
                });
            };
            (*loop)();
        });
    }

    // ---- Module scan
    if (modules) {
        steps.add([&](Steps::Next next) {
            const VehicleProfile profile = ModuleCatalog::forVehicle(make, vin);
            section("All modules");
            out << "  Module list: " << profile.label << Qt::endl;
            auto *scanner = new ModuleScanner(&client, &app);
            QObject::connect(scanner, &ModuleScanner::finished, [&out, scanner, next, profile] {
                for (const ModuleResult &m : scanner->results()) {
                    const char *state = m.state == ModuleResult::State::Answered ? "answered"
                                      : m.state == ModuleResult::State::ObdOnly ? "generic OBD-II only"
                                      : m.state == ModuleResult::State::Absent ? "not found" : "error";
                    out << "  " << m.module.addressText() << "  " << m.module.displayName().leftJustified(26) << state;
                    if (m.state == ModuleResult::State::Answered)
                        out << (m.protocol == DiagProtocol::Uds ? " (UDS)" : " (KWP2000)");
                    if (!m.partNumber.isEmpty())
                        out << "  part " << m.partNumber;
                    if (!m.softwareVersion.isEmpty())
                        out << "  sw " << m.softwareVersion;
                    if (!m.module.verified)
                        out << "  [unconfirmed address]";
                    if (m.state == ModuleResult::State::Error)
                        out << "  " << m.error;
                    out << Qt::endl;
                    for (const ModuleDtc &d : m.dtcs) {
                        const DtcInfo info = DtcDatabase::instance().lookup(d.code, profile.id, QStringLiteral("en"), m.module.category);
                        out << "      " << d.displayCode() << "  status 0x" << QString::number(d.status, 16).toUpper()
                            << (d.activeNow ? "  [present now]" : "") << "  " << (info.known ? info.title : info.system) << Qt::endl;
                    }
                }
                next();
            });
            scanner->scan(profile);
        });
    }

    QObject::connect(client.session(), &ElmSession::stateChanged, [&](ElmSession::State s) {
        if (s == ElmSession::State::Failed || s == ElmSession::State::NoVehicle) {
            out << "Error: " << client.session()->lastError() << Qt::endl;
            app.exit(1);
            return;
        }
        if (s != ElmSession::State::Ready)
            return;
        steps.run([&] {
            out << Qt::endl << "Done." << Qt::endl;
            client.session()->close();
            app.exit(0);
        });
    });

    client.session()->open();
    return QCoreApplication::exec();
}
