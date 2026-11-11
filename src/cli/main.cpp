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
//     --report-html FILE  also save the shareable report (the same web page the
//                    app saves with "Save report..."); implies --report
//     --model NAME   also read the manufacturer values of this model (OBDb data
//                    pack, e.g. "Elantra" for a Hyundai): which modules answer
//                    which requests, with the decoded values
//
// It uses exactly the same core library as the app, so it tells "adapter or
// car problem" from "UI problem". Like the app, it only reads: every command
// still passes the safety guard, and nothing here clears codes.

#include "elm327/AdapterCheck.h"
#include "obd/DtcDatabase.h"
#include "obd/ManufacturerData.h"
#include "obd/ManufacturerReader.h"
#include "obd/ModuleAddressing.h"
#include "obd/ModuleScanner.h"
#include "obd/ObdClient.h"
#include "obd/Pids.h"
#include "obd/Readiness.h"
#include "obd/VehicleInfo.h"
#include "report/VehicleReport.h"
#include "recording/SessionRecorder.h"
#include "transport/ReplayTransport.h"
#include "transport/SimulatedTransport.h"
#include "transport/TcpTransport.h"
#include "vehicles/ModuleCatalog.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
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
    QCoreApplication::setApplicationVersion(QStringLiteral(OBDARYN_VERSION));
    QTextStream out(stdout);
    QStringList args = app.arguments().mid(1);
    // "--record FILE" / "--replay FILE": take the option and its value out of the list.
    auto takeValue = [&args](const QString &option) {
        const qsizetype i = args.indexOf(option);
        if (i < 0 || i + 1 >= args.size())
            return QString();
        const QString value = args.takeAt(i + 1);
        args.removeAt(i);
        return value;
    };
    const QString reportHtmlPath = takeValue(QStringLiteral("--report-html"));
    const QString brandModel = takeValue(QStringLiteral("--model"));
    const bool report = args.removeAll(QStringLiteral("--report")) > 0 || !reportHtmlPath.isEmpty();
    const bool modules = args.removeAll(QStringLiteral("--modules")) > 0 || report;
    const bool verbose = args.removeAll(QStringLiteral("--verbose")) > 0;
    const bool demo = args.removeAll(QStringLiteral("--demo")) > 0;
    const QString recordPath = takeValue(QStringLiteral("--record"));
    const QString replayPath = takeValue(QStringLiteral("--replay"));

    ObdClient client;
    SessionRecorder recorder(client.session());
    // What the shareable report needs, filled in by the steps below as they read.
    ReportInput rep;
    rep.appVersion = QCoreApplication::applicationVersion();
    rep.language = QStringLiteral("en"); // the probe has no translations loaded
    if (!replayPath.isEmpty()) {
        Recording recording;
        QString error;
        if (!recording.load(replayPath, &error)) {
            out << "Can't replay " << replayPath << ": " << error << Qt::endl;
            return 1;
        }
        client.session()->setTransport(new ReplayTransport(std::move(recording), QFileInfo(replayPath).fileName()));
        rep.source = ReportInput::Source::Replay;
        rep.sourceDetail = QFileInfo(replayPath).fileName();
    } else if (demo) {
        client.session()->setTransport(new SimulatedTransport);
        rep.source = ReportInput::Source::Demo;
    } else {
        const QString host = args.value(0, QStringLiteral("192.168.0.10"));
        const quint16 port = quint16(args.value(1, QStringLiteral("35000")).toUInt());
        client.session()->setTransport(new TcpTransport(host, port));
        rep.source = ReportInput::Source::Car;
        rep.sourceDetail = QStringLiteral("%1:%2").arg(host).arg(port);
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
        rep.adapter = client.session()->adapterVersion();
        rep.protocol = protocolDisplayName(client.session()->protocol());
        client.readVehicleInfo([&, next](const ObdClient::VehicleInformation &info) {
            rep.info = info;
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
            rep.readiness = st;
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
            rep.dtcs = dtcs;
            rep.scanTime = QTime::currentTime().toString(QStringLiteral("HH:mm"));
            rep.brand = ModuleCatalog::forVehicle(make, vin).id;
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
                        // For the report: OBD standard and fuel type (text values), and the
                        // "since codes were cleared" counters. The engine (first ECU) wins.
                        if (r.pid == 0x1C && rep.obdStandard.isEmpty())
                            rep.obdStandard = PidTable::describeStatus(0x1C, r.raw);
                        if (r.pid == 0x51 && rep.fuelType.isEmpty())
                            rep.fuelType = PidTable::describeStatus(0x51, r.raw);
                        if (QList<quint8>{0x21, 0x30, 0x31, 0x4D, 0x4E}.contains(r.pid) && !rep.pids.contains(r.pid))
                            rep.pids.insert(r.pid, r.raw);
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
                rep.freezeFrame = ff;
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

        // ---- Self-test results (service 06): the numbers behind the readiness monitors
        steps.add([&](Steps::Next next) {
            section("Self-test results (Mode 06)");
            client.readMonitorTests([&, next](const QList<MonitorTest> &tests, const QString &error) {
                rep.monitorTestsRead = error.isEmpty();
                rep.monitorTests = tests;
                if (!error.isEmpty())
                    out << "  " << error << Qt::endl;
                else if (tests.isEmpty())
                    out << "  none reported" << Qt::endl;
                for (const MonitorTest &t : tests) {
                    const char *verdict = t.verdict == MonitorTest::Verdict::Passed ? "passed"
                                        : t.verdict == MonitorTest::Verdict::Close ? "CLOSE to its limit"
                                        : t.verdict == MonitorTest::Verdict::Failed ? "FAILED" : "not run";
                    out << "  " << t.ecu << "  MID " << pidHex(t.mid) << " TID " << pidHex(t.tid) << "  "
                        << (t.monitorName() + QStringLiteral(": ") + t.testName()).leftJustified(62) << "  "
                        << QString::number(t.value, 'f', t.decimals) << ' ' << t.unit
                        << "  (min " << QString::number(t.minimum, 'f', t.decimals)
                        << ", max " << QString::number(t.maximum, 'f', t.decimals) << ")  " << verdict
                        << "  raw " << t.rawValue << '/' << t.rawMin << '/' << t.rawMax << Qt::endl;
                }
                next();
            });
        });

        // ---- Battery voltage at the OBD socket (measured by the adapter)
        steps.add([&](Steps::Next next) {
            client.readBatteryVoltage([&, next](double volts) {
                rep.batteryVolts = volts;
                next();
            });
        });

        // ---- Polling benchmark: values per second the dashboard will get, and
        // what response counts ("010C0D1") gain on this adapter and car.
        auto benchmark = [&](const QString &label, const QList<quint8> &pids, bool responseCounts, Steps::Next next) {
            client.setResponseCountsEnabled(responseCounts);
            auto clock = std::make_shared<QElapsedTimer>();
            auto count = std::make_shared<int>(0);
            auto rounds = std::make_shared<int>(0);
            auto loop = std::make_shared<std::function<void()>>();
            clock->start();
            *loop = [&, label, pids, clock, count, rounds, loop, next] {
                if (++*rounds > 20) {
                    const double secs = clock->elapsed() / 1000.0;
                    out << "  " << label.leftJustified(46) << *count << " values in " << QString::number(secs, 'f', 2)
                        << " s = " << QString::number(*count / secs, 'f', 1) << " values/s, "
                        << QString::number(20 / secs, 'f', 1) << " cycles/s" << Qt::endl;
                    QTimer::singleShot(0, [loop] { *loop = nullptr; }); // break the self-reference later
                    next();
                    return;
                }
                client.readPids(pids, [count, loop](const QList<ObdClient::PidReading> &rs) {
                    *count += int(rs.size());
                    QTimer::singleShot(0, [loop] { if (*loop) (*loop)(); });
                });
            };
            (*loop)();
        };
        steps.add([&](Steps::Next next) {
            section("Polling speed");
            // The supported-PID map tells which ECUs answer what (needed for response counts).
            client.readSupportedPids(0x01, [&, next](const QList<quint8> &) {
                benchmark(QStringLiteral("dashboard, 6 values per request:"), {0x0C, 0x0D, 0x05, 0x04, 0x11, 0x0B}, true, next);
            });
        });
        steps.add([&](Steps::Next next) {
            benchmark(QStringLiteral("2 values per request, response counts on:"), {0x0C, 0x0D}, true, next);
        });
        steps.add([&](Steps::Next next) {
            benchmark(QStringLiteral("2 values per request, response counts off:"), {0x0C, 0x0D}, false, [&, next] {
                const char *state = client.responseCounts() == ObdClient::ResponseCounts::NotSupported ? "not supported" : "supported";
                out << "  response counts on this adapter: " << state << Qt::endl;
                client.setResponseCountsEnabled(true);
                next();
            });
        });

        // ---- What the adapter really supports (read-only adapter commands only)
        steps.add([&](Steps::Next next) {
            section("Adapter check");
            AdapterCheck::run(client.session(), [&, next](const AdapterCheck::Result &r) {
                out << "  " << r.identification << " | " << r.description
                    << (r.deviceId.isEmpty() ? QString() : QStringLiteral(" | ") + r.deviceId) << Qt::endl;
                out << "  " << r.summary() << Qt::endl;
                for (const AdapterCheck::Item &i : r.items)
                    out << "  " << i.command.leftJustified(12) << (i.supported ? "works        " : "not supported") << "  " << i.answer << Qt::endl;
                const ElmSession::Health h = client.session()->health();
                out << "  answers: " << QString::number(h.averageMs, 'f', 0) << " ms average, 95 % within "
                    << QString::number(h.p95Ms, 'f', 0) << " ms; " << h.timeouts << " timeouts and " << h.errors
                    << " errors in " << h.commands << " commands" << Qt::endl;
                next();
            });
        });
    }

    // ---- Manufacturer values (OBDb data pack for the given model)
    if (!brandModel.isEmpty()) {
        steps.add([&](Steps::Next next) {
            section("Manufacturer values");
            const ManufacturerModel *model = ManufacturerData::instance().find(make, brandModel);
            if (!model) {
                out << "  No data pack for " << make << ' ' << brandModel << ". Models: "
                    << ManufacturerData::instance().modelsOf(make).join(QStringLiteral(", ")) << Qt::endl;
                next();
                return;
            }
            const int year = decodeVin(vin).modelYear;
            const QList<ManufacturerCommand> commands = model->commandsFor(year);
            out << "  " << model->make << ' ' << model->model << ", model year " << (year > 0 ? QString::number(year) : QStringLiteral("unknown"))
                << ": " << commands.size() << " requests (" << model->repository << " @ " << model->commit.left(10) << ")" << Qt::endl;
            ManufacturerReader::read(&client, commands, [&out, commands, next](const QList<ManufacturerReader::Result> &results) {
                int answered = 0;
                for (int i = 0; i < results.size(); ++i) {
                    const ManufacturerCommand &c = commands[i];
                    const ManufacturerReader::Result &r = results[i];
                    answered += r.answered ? 1 : 0;
                    out << "  " << c.key().leftJustified(12)
                        << (r.answered ? "answered" : r.refused ? "refused (not supported)" : "no answer") << Qt::endl;
                    for (const ManufacturerReader::Value &v : r.values) {
                        const ManufacturerSignal &sig = c.signalList[v.signal];
                        const QString unit = QString::fromUtf8(sig.metricUnit());
                        out << "      " << sig.name.leftJustified(52)
                            << QString::number(sig.toMetric(v.value), 'f', sig.decimals()) << (unit.isEmpty() ? QString() : QLatin1Char(' ') + unit)
                            << "   [" << sig.id << ']' << Qt::endl;
                    }
                }
                out << "  " << answered << " of " << results.size() << " requests answered" << Qt::endl;
                next();
            });
        });
    }

    // ---- Module scan
    if (modules) {
        steps.add([&](Steps::Next next) {
            const VehicleProfile profile = ModuleCatalog::forVehicle(make, vin);
            section("All modules");
            out << "  Module list: " << profile.label << Qt::endl;
            auto *scanner = new ModuleScanner(&client, &app);
            QObject::connect(scanner, &ModuleScanner::finished, [&out, &rep, scanner, next, profile] {
                rep.modulesScanned = true;
                rep.modules = scanner->results();
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
            if (!reportHtmlPath.isEmpty()) {
                // The same web page the app saves; it says where the data came from.
                rep.createdAt = QDateTime::currentDateTime();
                QFile file(reportHtmlPath);
                if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)
                    && file.write(VehicleReport::html(rep, VehicleReport::Target::Web).toUtf8()) >= 0 && file.flush())
                    out << Qt::endl << "Report saved to " << reportHtmlPath << Qt::endl;
                else
                    out << Qt::endl << "Couldn't save the report to " << reportHtmlPath << ": " << file.errorString() << Qt::endl;
            }
            out << Qt::endl << "Done." << Qt::endl;
            client.session()->close();
            app.exit(0);
        });
    });

    client.session()->open();
    return QCoreApplication::exec();
}
