// SPDX-License-Identifier: GPL-3.0-or-later
//
// End-to-end: ObdClient → ElmSession → RequestGuard → SimulatedTransport.

#include "obd/ObdClient.h"
#include "obd/Pids.h"
#include "transport/SimulatedTransport.h"

#include <QSignalSpy>
#include <QTest>

using namespace obdaryn;

class TestSession : public QObject
{
    Q_OBJECT
private slots:
    void init();
    void cleanup();
    void connectsAndDetectsProtocol();
    void readsSupportedPidsAndValues();
    void readsDtcsFromAllEcus();
    void readsVehicleInfo();
    void readsFreezeFrame();
    void refusesClearWhileEngineRuns();
    void clearsWithEngineOff();
    void guardBlocksEvenRawSends();

private:
    bool waitReady();
    ObdClient *m_client = nullptr;
    SimulatedTransport *m_sim = nullptr;
};

void TestSession::init()
{
    m_client = new ObdClient;
    m_sim = new SimulatedTransport;
    m_sim->setLatencyMs(0);
    m_client->session()->setTransport(m_sim);
}

void TestSession::cleanup()
{
    delete m_client; // also deletes the transport
    m_client = nullptr;
    m_sim = nullptr;
}

bool TestSession::waitReady()
{
    m_client->session()->open();
    return QTest::qWaitFor([this] { return m_client->session()->state() == ElmSession::State::Ready; }, 3000);
}

void TestSession::connectsAndDetectsProtocol()
{
    QVERIFY(waitReady());
    QCOMPARE(m_client->session()->protocol(), Protocol::Can11_500);
    QVERIFY(m_client->session()->headersEnabled());
    QVERIFY(m_client->session()->adapterVersion().contains(QStringLiteral("ELM327")));
}

void TestSession::readsSupportedPidsAndValues()
{
    QVERIFY(waitReady());
    QList<quint8> supported;
    bool done = false;
    m_client->readSupportedPids(0x01, [&](const QList<quint8> &p) { supported = p; done = true; });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 3000));
    QVERIFY(supported.contains(0x0C));
    QVERIFY(supported.contains(0x42)); // needs the 0x20 and 0x40 ranges too

    QList<ObdClient::PidReading> readings;
    done = false;
    m_client->readPids({0x0C, 0x0D, 0x05, 0x04, 0x11, 0x0B, 0x42}, [&](const auto &r) {
        readings = r;
        done = true;
    });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 3000));
    QCOMPARE(readings.size(), 7);
    double rpm = 0;
    for (const auto &rd : readings) {
        if (rd.pid == 0x0C)
            QVERIFY(PidTable::find(0x0C)->value(rd.raw, &rpm));
    }
    QVERIFY(rpm > 500);
}

void TestSession::readsDtcsFromAllEcus()
{
    QVERIFY(waitReady());
    QList<Dtc> dtcs;
    bool done = false;
    m_client->readAllDtcs([&](const QList<Dtc> &d, const QString &) { dtcs = d; done = true; });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 3000));

    QStringList stored, pending, permanent;
    for (const Dtc &d : dtcs) {
        QCOMPARE(d.ecu, QStringLiteral("7E8"));
        (d.kind == DtcKind::Stored ? stored : d.kind == DtcKind::Pending ? pending : permanent) << d.code;
    }
    QCOMPARE(stored, (QStringList{"P0133", "P0420"}));
    QCOMPARE(pending, QStringList{"P0171"});
    QCOMPARE(permanent, QStringList{"P0420"});
}

void TestSession::readsVehicleInfo()
{
    QVERIFY(waitReady());
    ObdClient::VehicleInformation info;
    bool done = false;
    m_client->readVehicleInfo([&](const auto &i) { info = i; done = true; });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 3000));
    QCOMPARE(info.vin, QStringLiteral("KMHDH41EBEU000001"));
    QCOMPARE(info.calibrationIds, QStringList{"6AG1EU3B2KFA1000"});
    QCOMPARE(info.ecuNames.value("7E8"), QStringLiteral("ECM-EngineControl"));
    QVERIFY(info.ecuNames.contains("7E9"));
}

void TestSession::readsFreezeFrame()
{
    QVERIFY(waitReady());
    ObdClient::FreezeFrame ff;
    bool done = false;
    m_client->readFreezeFrame([&](const auto &f) { ff = f; done = true; });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 3000));
    QVERIFY(ff.available);
    QCOMPARE(ff.dtc, QStringLiteral("P0133"));
    QVERIFY(ff.values.size() >= 8);
}

void TestSession::refusesClearWhileEngineRuns()
{
    QVERIFY(waitReady());
    m_sim->setEngineRunning(true);
    ObdClient::ClearResult result = ObdClient::ClearResult::Cleared;
    bool done = false;
    m_client->clearDtcs([&](ObdClient::ClearResult r, const QString &) { result = r; done = true; });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 3000));
    QCOMPARE(result, ObdClient::ClearResult::EngineRunning);
}

void TestSession::clearsWithEngineOff()
{
    QVERIFY(waitReady());
    m_sim->setEngineRunning(false);
    ObdClient::ClearResult result = ObdClient::ClearResult::NoResponse;
    bool done = false;
    m_client->clearDtcs([&](ObdClient::ClearResult r, const QString &) { result = r; done = true; });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 3000));
    QCOMPARE(result, ObdClient::ClearResult::Cleared);

    // Stored and pending are gone, the permanent code stays, monitors reset.
    QList<Dtc> dtcs;
    done = false;
    m_client->readAllDtcs([&](const QList<Dtc> &d, const QString &) { dtcs = d; done = true; });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 3000));
    QCOMPARE(dtcs.size(), 1);
    QCOMPARE(dtcs[0].kind, DtcKind::Permanent);

    MonitorStatus status;
    done = false;
    m_client->readMonitorStatus([&](const MonitorStatus &s) { status = s; done = true; });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 3000));
    QVERIFY(!status.milOn);
    QCOMPARE(status.incompleteCount(), 5);
}

void TestSession::guardBlocksEvenRawSends()
{
    QVERIFY(waitReady());
    QSignalSpy log(m_client->session(), &ElmSession::trafficLogged);
    ElmSession::Reply reply;
    bool done = false;
    // Clear without confirmation, then a write: neither may reach the "car".
    m_client->session()->send("04", [&](const ElmSession::Reply &r) { reply = r; });
    m_client->session()->send("2EF19001", [&](const ElmSession::Reply &r) { reply = r; done = true; });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 3000));
    QCOMPARE(reply.status, ElmSession::Reply::Status::Blocked);
    for (const QList<QVariant> &entry : log)
        QVERIFY(entry.at(0).toString() != QStringLiteral(">")); // nothing was written
}

QTEST_GUILESS_MAIN(TestSession)
#include "tst_session.moc"
