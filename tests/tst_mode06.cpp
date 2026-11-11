// SPDX-License-Identifier: GPL-3.0-or-later
//
// On-board monitor test results (service 06): decoding, units, verdicts,
// and reading them from the demo car.

#include "obd/MonitorTests.h"
#include "obd/ObdClient.h"
#include "transport/SimulatedTransport.h"

#include <QTest>

using namespace obdaryn;

class TestMode06 : public QObject
{
    Q_OBJECT
private slots:
    void decodesRecords();
    void verdicts();
    void signedValues();
    void names();
    void readsDemoCar();
};

void TestMode06::decodesRecords()
{
    // Two records for MID 01: threshold voltage 0.45 V (0.4–0.5) and switch time 72 ms (≤ 100).
    EcuMessage m{"7E8", QByteArray::fromHex("46" "01010B01C2019001F4" "011010004800000064"), {}};
    const QList<MonitorTest> tests = MonitorTestCodec::fromResponse(m);
    QCOMPARE(tests.size(), 2);
    QCOMPARE(tests[0].mid, quint8(0x01));
    QCOMPARE(tests[0].tid, quint8(0x01));
    QVERIFY(qAbs(tests[0].value - 0.45) < 1e-9);
    QCOMPARE(tests[0].unit, QStringLiteral("V"));
    QCOMPARE(tests[1].value, 72.0);
    QCOMPARE(tests[1].unit, QStringLiteral("ms"));
    QCOMPARE(tests[1].verdict, MonitorTest::Verdict::Passed);
    QCOMPARE(tests[0].ecu, QStringLiteral("7E8"));
}

void TestMode06::verdicts()
{
    auto judge = [](int value, int min, int max, quint8 uasid = 0x01) {
        MonitorTest t;
        t.uasid = uasid;
        t.rawValue = value;
        t.rawMin = min;
        t.rawMax = max;
        MonitorTestCodec::scale(t);
        return t.verdict;
    };
    using V = MonitorTest::Verdict;
    QCOMPARE(judge(0, 0, 0), V::NotRun);
    QCOMPARE(judge(105, 0, 100), V::Failed);
    QCOMPARE(judge(50, 0, 100), V::Passed);
    QCOMPARE(judge(95, 0, 100), V::Close);       // only an upper limit, 5 % away
    QCOMPARE(judge(405, 400, 500), V::Close);    // both limits, near the lower one
    QCOMPARE(judge(450, 400, 500), V::Passed);
    QCOMPARE(judge(20, 18, 0xFFFF), V::Passed);  // only a lower limit, 11 % above it
    QCOMPARE(judge(19, 18, 0xFFFF), V::Close);
    QCOMPARE(judge(17, 18, 0xFFFF), V::Failed);
}

void TestMode06::signedValues()
{
    // UASID 0x8A: signed, 0.122 mV per bit. -100 raw, limits -500..500.
    EcuMessage m{"7E8", QByteArray::fromHex("46" "01808A" "FF9C" "FE0C" "01F4"), {}};
    const QList<MonitorTest> tests = MonitorTestCodec::fromResponse(m);
    QCOMPARE(tests.size(), 1);
    QCOMPARE(tests[0].rawValue, -100);
    QCOMPARE(tests[0].rawMin, -500);
    QVERIFY(qAbs(tests[0].value - (-12.2)) < 1e-9);
    QCOMPARE(tests[0].verdict, MonitorTest::Verdict::Passed);

    // An unknown unit is shown raw, but still judged.
    MonitorTest t;
    t.uasid = 0x7E;
    t.rawValue = 5;
    t.rawMax = 4;
    MonitorTestCodec::scale(t);
    QVERIFY(!t.scalingKnown);
    QCOMPARE(t.verdict, MonitorTest::Verdict::Failed);
}

void TestMode06::names()
{
    QCOMPARE(MonitorTestCodec::monitorName(0x21), QStringLiteral("Catalyst, bank 1"));
    QCOMPARE(MonitorTestCodec::monitorName(0x06), QStringLiteral("Oxygen sensor, bank 2 sensor 2"));
    QCOMPARE(MonitorTestCodec::monitorName(0xA4), QStringLiteral("Misfire, cylinder 3"));
    MonitorTest t;
    t.mid = 0xEE;
    t.tid = 0x85;
    QCOMPARE(t.monitorName(), QStringLiteral("Monitor 0xEE"));
    QCOMPARE(t.testName(), QStringLiteral("Manufacturer test 0x85"));
}

void TestMode06::readsDemoCar()
{
    ObdClient client;
    auto *sim = new SimulatedTransport;
    sim->setLatencyMs(0);
    client.session()->setTransport(sim);
    client.session()->open();
    QVERIFY(QTest::qWaitFor([&] { return client.session()->state() == ElmSession::State::Ready; }, 3000));

    QList<MonitorTest> tests;
    bool done = false;
    client.readMonitorTests([&](const QList<MonitorTest> &t, const QString &error) {
        QVERIFY(error.isEmpty());
        tests = t;
        done = true;
    });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 5000));
    QVERIFY(tests.size() >= 10);
    auto find = [&](quint8 mid, quint8 tid) {
        for (const MonitorTest &t : tests)
            if (t.mid == mid && t.tid == tid)
                return t;
        return MonitorTest{};
    };
    QCOMPARE(find(0x21, 0x80).verdict, MonitorTest::Verdict::Failed); // matches the stored P0420
    QCOMPARE(find(0x3C, 0x80).verdict, MonitorTest::Verdict::NotRun);
    QCOMPARE(find(0xA4, 0x0B).verdict, MonitorTest::Verdict::Close);  // cylinder 3
}

QTEST_GUILESS_MAIN(TestMode06)
#include "tst_mode06.moc"
