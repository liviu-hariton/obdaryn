// SPDX-License-Identifier: GPL-3.0-or-later
//
// Replays of real cars. Each fixture in tests/data is a trimmed Obdaryn
// recording (VIN and calibration IDs removed) that once showed a bug; the
// test pins the fix against the car's own answers.
//
//  - hyundai-elantra-2015-eu.obdrec: 2015 Hyundai Elantra (EU, petrol), Vgate
//    iCar Pro Wi-Fi, engine idling. No PID 51; ABS, airbag, power steering
//    and TPMS refuse code requests in different ways; the cluster answers
//    TesterPresent only.

#include "obd/ModuleScanner.h"
#include "obd/ObdClient.h"
#include "recording/Recording.h"
#include "transport/ReplayTransport.h"
#include "vehicles/ModuleCatalog.h"

#include <QSignalSpy>
#include <QTest>

using namespace obdaryn;

class TestRealCars : public QObject
{
    Q_OBJECT
private slots:
    void init();
    void cleanup();
    void unsupportedPidKeepsCombinedRequests();
    void explainsModuleRefusals();

private:
    const ModuleResult *find(const ModuleScanner &s, const QString &category) const;
    ObdClient *m_client = nullptr;
};

void TestRealCars::init()
{
    Recording rec;
    QString error;
    QVERIFY2(rec.load(QStringLiteral(OBDARYN_TEST_DATA "/hyundai-elantra-2015-eu.obdrec"), &error), qPrintable(error));
    m_client = new ObdClient;
    auto *replay = new ReplayTransport(rec, QStringLiteral("elantra"));
    replay->setInstant(true);
    m_client->session()->setTransport(replay);
    m_client->session()->open();
    QVERIFY(QTest::qWaitFor([this] { return m_client->session()->state() == ElmSession::State::Ready; }, 5000));
}

void TestRealCars::cleanup()
{
    delete m_client;
    m_client = nullptr;
}

const ModuleResult *TestRealCars::find(const ModuleScanner &s, const QString &category) const
{
    for (const ModuleResult &r : s.results()) {
        if (r.module.category == category)
            return &r;
    }
    return nullptr;
}

void TestRealCars::unsupportedPidKeepsCombinedRequests()
{
    // The app asked for PIDs 1C + 51 together. This car has no PID 51, so it
    // answered 1C only, which used to look like "this ECU ignores combined
    // requests" and slowed live data down to one value per request for the
    // whole drive.
    QList<quint8> supported;
    bool done = false;
    m_client->readSupportedPids(0x01, [&](const QList<quint8> &p) { supported = p; done = true; });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 5000));
    QVERIFY(supported.contains(0x1C));
    QVERIFY(!supported.contains(0x51));
    QVERIFY(!m_client->pidAdvertised(0x51));

    QList<ObdClient::PidReading> readings;
    done = false;
    m_client->readPids({0x1C, 0x51}, [&](const QList<ObdClient::PidReading> &r) { readings = r; done = true; });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 5000));
    QCOMPARE(readings.size(), 1);
    QCOMPARE(readings[0].pid, quint8(0x1C));
    QVERIFY(m_client->multiPidSupported());

    // And combined requests still work afterwards.
    done = false;
    m_client->readPids({0x0C, 0x0D, 0x05, 0x04, 0x42, 0x11}, [&](const QList<ObdClient::PidReading> &r) { readings = r; done = true; });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 5000));
    QCOMPARE(readings.size(), 6);
    QVERIFY(m_client->multiPidSupported());
}

void TestRealCars::explainsModuleRefusals()
{
    ModuleScanner scanner(m_client);
    QSignalSpy finished(&scanner, &ModuleScanner::finished);
    scanner.scan(*ModuleCatalog::find(QStringLiteral("hyundai-kia")));
    QVERIFY(finished.wait(30000));

    // Engine: refuses UDS and KWP code requests but answers OBD-II, whose
    // codes the generic scan reads. Not an error.
    QCOMPARE(find(scanner, "engine")->state, ModuleResult::State::ObdOnly);

    // ABS: "conditions not correct" with the engine idling: the advice is to
    // switch the engine off, not to turn the ignition on.
    const ModuleResult *abs = find(scanner, "abs");
    QCOMPARE(abs->state, ModuleResult::State::Error);
    QVERIFY2(abs->error.contains(QStringLiteral("engine is running")), qPrintable(abs->error));

    // Power steering: "busy" to UDS (asked again, still busy), then
    // "conditions not correct" to KWP: same advice as the ABS.
    const ModuleResult *steering = find(scanner, "steering");
    QCOMPARE(steering->state, ModuleResult::State::Error);
    QVERIFY2(steering->error.contains(QStringLiteral("engine is running")), qPrintable(steering->error));

    // Airbag: UDS says "needs a diagnostic session" (0x7F), KWP "not
    // supported" (0x11). The session refusal is the one that explains it.
    const ModuleResult *airbag = find(scanner, "airbag");
    QCOMPARE(airbag->state, ModuleResult::State::Error);
    QVERIFY2(airbag->error.contains(QStringLiteral("diagnostic session")), qPrintable(airbag->error));

    // TPMS: KWP2000's own "not in this session" code (0x80).
    QVERIFY2(find(scanner, "tpms")->error.contains(QStringLiteral("diagnostic session")),
             qPrintable(find(scanner, "tpms")->error));

    // Cluster: answers TesterPresent, silent to code requests. Its ignition is
    // obviously on, so no "turn the ignition on" advice.
    const ModuleResult *cluster = find(scanner, "cluster");
    QCOMPARE(cluster->state, ModuleResult::State::Error);
    QVERIFY(!cluster->error.contains(QStringLiteral("ignition")));

    QCOMPARE(find(scanner, "transmission")->state, ModuleResult::State::Absent);
}

QTEST_GUILESS_MAIN(TestRealCars)
#include "tst_realcars.moc"
