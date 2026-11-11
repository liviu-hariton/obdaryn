// SPDX-License-Identifier: GPL-3.0-or-later
//
// Adapter health: reconnecting after a dropped link or a sleeping adapter,
// waiting for the car after the ignition was turned off, response counts,
// and the adapter check.

#include "elm327/AdapterCheck.h"
#include "elm327/ConnectionKeeper.h"
#include "obd/ObdClient.h"
#include "transport/SimulatedTransport.h"

#include <QSignalSpy>
#include <QTest>

using namespace obdaryn;

class TestHealth : public QObject
{
    Q_OBJECT
private slots:
    void init();
    void cleanup();
    void reconnectsAfterDrop();
    void reconnectsSleepingAdapter();
    void waitsForTheCar();
    void noReconnectAfterUserDisconnect();
    void responseCounts();
    void responseCountsFallBack();
    void adapterCheck();
    void verdicts();
    void healthStats();

private:
    bool ready() { return QTest::qWaitFor([this] { return m_client->session()->state() == ElmSession::State::Ready; }, 8000); }
    ObdClient *m_client = nullptr;
    SimulatedTransport *m_sim = nullptr;
    ConnectionKeeper *m_keeper = nullptr;
};

void TestHealth::init()
{
    m_client = new ObdClient;
    m_sim = new SimulatedTransport;
    m_sim->setLatencyMs(0);
    m_client->session()->setTransport(m_sim);
    m_keeper = new ConnectionKeeper(m_client->session(), m_client);
    m_keeper->setDelays({20, 20, 20}, 50);
    m_keeper->start();
    m_client->session()->open();
}

void TestHealth::cleanup()
{
    delete m_client;
    m_client = nullptr;
}

void TestHealth::reconnectsAfterDrop()
{
    QVERIFY(ready());
    m_sim->close(); // the Wi-Fi link drops
    QVERIFY(QTest::qWaitFor([this] { return m_keeper->reconnecting(); }, 2000));
    QVERIFY(ready());
    // The keeper reacts on the next event-loop turn (its connections are queued).
    QVERIFY(QTest::qWaitFor([this] { return !m_keeper->reconnecting(); }, 1000));
    QCOMPARE(m_keeper->attempt(), 0);
}

void TestHealth::reconnectsSleepingAdapter()
{
    QVERIFY(ready());
    QSignalSpy unresponsive(m_client->session(), &ElmSession::adapterUnresponsive);
    m_sim->setAsleep(true);
    for (int i = 0; i < ElmSession::kUnresponsiveTimeouts; ++i)
        m_client->session()->send("0100", [](const ElmSession::Reply &) {}, ElmSession::Options{.timeoutMs = 50});
    QVERIFY(unresponsive.wait(10000));
    m_sim->setAsleep(false); // it wakes up on the reconnect
    QVERIFY(QTest::qWaitFor([this] { return m_client->session()->state() != ElmSession::State::Ready; }, 2000));
    QVERIFY(ready());
}

void TestHealth::waitsForTheCar()
{
    QVERIFY(ready());
    m_client->session()->setSilenceThresholdMs(100);
    m_sim->setCarAnswering(false); // ignition off
    QVERIFY(QTest::qWaitFor([this] {
        m_client->session()->send("010C", [](const ElmSession::Reply &) {});
        return m_client->session()->state() == ElmSession::State::NoVehicle;
    }, 5000));
    QVERIFY(QTest::qWaitFor([this] { return m_keeper->waitingForVehicle(); }, 1000));
    m_sim->setCarAnswering(true); // ignition on again: picks up by itself
    QVERIFY(ready());
}

void TestHealth::noReconnectAfterUserDisconnect()
{
    QVERIFY(ready());
    m_keeper->stop();
    m_client->session()->close();
    QTest::qWait(200);
    QCOMPARE(m_client->session()->state(), ElmSession::State::Disconnected);
    QVERIFY(!m_keeper->reconnecting());
}

void TestHealth::responseCounts()
{
    QVERIFY(ready());
    QStringList sent;
    connect(m_client->session(), &ElmSession::exchanged, this, [&sent](const QByteArray &c) { sent << QString::fromLatin1(c); });
    bool done = false;
    m_client->readSupportedPids(0x01, [&](const QList<quint8> &) { done = true; });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 3000));

    QList<ObdClient::PidReading> readings;
    done = false;
    m_client->readPids({0x0C, 0x0D}, [&](const QList<ObdClient::PidReading> &r) { readings = r; done = true; });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 3000));
    QVERIFY2(sent.contains(QStringLiteral("010C0D1")), qPrintable(sent.join(QLatin1Char(' ')))); // only the engine knows them
    QCOMPARE(m_client->responseCounts(), ObdClient::ResponseCounts::Working);
    QCOMPARE(readings.size(), 2);

    // Answers longer than one CAN frame get no count.
    done = false;
    m_client->readPids({0x0C, 0x0D, 0x05, 0x04}, [&](const QList<ObdClient::PidReading> &) { done = true; });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 3000));
    QVERIFY(sent.contains(QStringLiteral("010C0D0504")));
}

void TestHealth::responseCountsFallBack()
{
    QVERIFY(ready());
    m_sim->setRejectsResponseCounts(true);
    bool done = false;
    m_client->readSupportedPids(0x01, [&](const QList<quint8> &) { done = true; });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 3000));
    QList<ObdClient::PidReading> readings;
    done = false;
    m_client->readPids({0x0C}, [&](const QList<ObdClient::PidReading> &r) { readings = r; done = true; });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 3000));
    QCOMPARE(m_client->responseCounts(), ObdClient::ResponseCounts::NotSupported);
    QCOMPARE(readings.size(), 1); // still read, without the digit
}

void TestHealth::adapterCheck()
{
    QVERIFY(ready());
    AdapterCheck::Result result;
    bool done = false;
    AdapterCheck::run(m_client->session(), [&](const AdapterCheck::Result &r) { result = r; done = true; });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 3000));
    QCOMPARE(result.version, QStringLiteral("1.5"));
    QCOMPARE(result.verdict, AdapterCheck::Verdict::Compatible);
    QVERIFY(result.flowControl);
    QCOMPARE(result.items.size(), 5);
    QVERIFY(result.items[0].supported);  // AT IGN
    QVERIFY(!result.items[2].supported); // AT PPS: "?"
    QVERIFY(!result.summary().isEmpty());
}

void TestHealth::verdicts()
{
    using V = AdapterCheck::Verdict;
    auto items = [](bool all) {
        QList<AdapterCheck::Item> list;
        for (const char *c : {"AT IGN", "AT CS", "AT PPS"})
            list << AdapterCheck::Item{QString::fromLatin1(c), {}, all, {}, {}};
        return list;
    };
    QCOMPARE(AdapterCheck::verdictFor(QStringLiteral("2.1"), items(true)), V::Genuine);
    QCOMPARE(AdapterCheck::verdictFor(QStringLiteral("2.1"), items(false)), V::Compatible);
    QCOMPARE(AdapterCheck::verdictFor(QStringLiteral("1.5"), items(true)), V::Compatible);
    QCOMPARE(AdapterCheck::verdictFor(QStringLiteral("1.3a"), items(false)), V::Genuine); // older versions lack them legitimately
    QCOMPARE(AdapterCheck::verdictFor(QString(), items(true)), V::Unknown);
}

void TestHealth::healthStats()
{
    QVERIFY(ready());
    const ElmSession::Health h = m_client->session()->health();
    QVERIFY(h.commands >= 10); // the handshake
    QCOMPARE(h.timeouts, 0);
    QVERIFY(h.averageMs >= 0);
}

QTEST_GUILESS_MAIN(TestHealth)
#include "tst_health.moc"
