// SPDX-License-Identifier: GPL-3.0-or-later
//
// obdaryn-probe: a tiny command-line check of an adapter and car.
//
//   obdaryn-probe                      # 192.168.0.10:35000 (Vgate iCar Pro Wi-Fi)
//   obdaryn-probe 127.0.0.1 35000      # Python ELM327-emulator
//   obdaryn-probe --demo               # built-in simulated car
//
// It connects, prints adapter / protocol / VIN / readiness / trouble codes and
// a few live values, then exits. It uses exactly the same core library as the
// app, so it is the quickest way to tell "adapter or car problem" from "UI
// problem" on the first day with real hardware. Like the app, it only reads.

#include "obd/DtcDatabase.h"
#include "obd/ObdClient.h"
#include "obd/Pids.h"
#include "obd/VehicleInfo.h"
#include "transport/SimulatedTransport.h"
#include "transport/TcpTransport.h"

#include <QCoreApplication>
#include <QTextStream>
#include <QTimer>

using namespace obdaryn;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    const QStringList args = app.arguments().mid(1);

    ObdClient client;
    if (args.contains(QStringLiteral("--demo"))) {
        client.session()->setTransport(new SimulatedTransport);
    } else {
        const QString host = args.value(0, QStringLiteral("192.168.0.10"));
        const quint16 port = quint16(args.value(1, QStringLiteral("35000")).toUInt());
        client.session()->setTransport(new TcpTransport(host, port));
    }
    if (args.contains(QStringLiteral("--verbose"))) {
        QObject::connect(client.session(), &ElmSession::trafficLogged, [&out](const QString &d, const QString &t) {
            out << "  " << d << ' ' << t << Qt::endl;
        });
    }

    // Give up after 60 s whatever happens.
    QTimer::singleShot(60000, &app, [&] { out << "Timed out." << Qt::endl; app.exit(2); });

    QObject::connect(client.session(), &ElmSession::stateChanged, [&](ElmSession::State s) {
        if (s == ElmSession::State::Failed || s == ElmSession::State::NoVehicle) {
            out << "Error: " << client.session()->lastError() << Qt::endl;
            app.exit(1);
            return;
        }
        if (s != ElmSession::State::Ready)
            return;

        out << "Adapter:  " << client.session()->adapterVersion() << " / "
            << client.session()->adapterDescription() << Qt::endl;
        out << "Protocol: " << protocolDisplayName(client.session()->protocol()) << Qt::endl;

        client.readVehicleInfo([&](const ObdClient::VehicleInformation &info) {
            const VinInfo vin = decodeVin(info.vin);
            out << "VIN:      " << (vin.vin.isEmpty() ? QStringLiteral("(not reported)") : vin.vin);
            if (!vin.manufacturer.isEmpty())
                out << "  (" << vin.manufacturer << ", " << vin.modelYear << ")";
            out << Qt::endl;
            for (auto it = info.ecuNames.begin(); it != info.ecuNames.end(); ++it)
                out << "Module:   " << it.key() << "  " << it.value() << Qt::endl;

            client.readMonitorStatus([&](const MonitorStatus &st) {
                out << "MIL:      " << (st.milOn ? "ON" : "off") << ", monitors incomplete: "
                    << st.incompleteCount() << '/' << st.monitors.size() << Qt::endl;

                client.readAllDtcs([&](const QList<Dtc> &dtcs, const QString &err) {
                    if (dtcs.isEmpty())
                        out << "Codes:    none" << (err.isEmpty() ? QString() : "  (" + err + ")") << Qt::endl;
                    for (const Dtc &d : dtcs) {
                        const DtcInfo info = DtcDatabase::instance().lookup(d.code);
                        const char *kind = d.kind == DtcKind::Stored ? "stored" : d.kind == DtcKind::Pending ? "pending" : "permanent";
                        out << "Code:     " << d.code << "  [" << kind << ", " << d.ecu << "]  "
                            << (info.known ? info.title : info.system) << Qt::endl;
                    }

                    client.readPids({0x0C, 0x0D, 0x05, 0x04, 0x42, 0x11}, [&](const QList<ObdClient::PidReading> &rs) {
                        for (const auto &r : rs) {
                            const PidDefinition *def = PidTable::find(r.pid);
                            double v = 0;
                            if (def && def->value(r.raw, &v))
                                out << "Live:     " << def->translatedName() << " = " << v << ' ' << def->translatedUnit()
                                    << "  (" << r.ecu << ")" << Qt::endl;
                        }
                        client.session()->close();
                        app.exit(0);
                    });
                });
            });
        });
    });

    client.session()->open();
    return QCoreApplication::exec();
}
