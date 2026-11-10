// SPDX-License-Identifier: GPL-3.0-or-later
//
// Session recording and replay: record the demo car, replay the file, and
// check that every layer sees the same car.

#include "obd/ModuleScanner.h"
#include "obd/ObdClient.h"
#include "recording/Recording.h"
#include "recording/SessionRecorder.h"
#include "transport/ReplayTransport.h"
#include "transport/SimulatedTransport.h"
#include "vehicles/ModuleCatalog.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace obdaryn;

class TestRecording : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void adapterTarget();
    void fileContents();
    void replaysCodesAndVin();
    void rebuildsLiveData();
    void replaysModuleScan();
    void neverReplaysAClear();
    void exportsCsv();
    void rejectsOtherFiles();

private:
    static bool waitReady(ObdClient &client);
    static ModuleScanner *scan(ObdClient &client);
    ObdClient *replayClient();

    QTemporaryDir m_dir;
    QString m_path;
    Recording m_recording;
    QStringList m_recordedCodes;
    QString m_recordedVin;
};

bool TestRecording::waitReady(ObdClient &client)
{
    client.session()->open();
    return QTest::qWaitFor([&client] { return client.session()->state() == ElmSession::State::Ready; }, 5000);
}

ModuleScanner *TestRecording::scan(ObdClient &client)
{
    auto *scanner = new ModuleScanner(&client, &client);
    QSignalSpy done(scanner, &ModuleScanner::finished);
    scanner->scan(*ModuleCatalog::find(QStringLiteral("hyundai-kia")));
    done.wait(20000);
    return scanner;
}

ObdClient *TestRecording::replayClient()
{
    auto *client = new ObdClient(this);
    auto *replay = new ReplayTransport(m_recording, QStringLiteral("test"));
    replay->setInstant(true);
    client->session()->setTransport(replay);
    return client;
}

void TestRecording::initTestCase()
{
    QVERIFY(m_dir.isValid());
    m_path = m_dir.filePath(QStringLiteral("demo.obdrec"));

    // Record: connect first, then press "Record", like a user would.
    ObdClient client;
    auto *sim = new SimulatedTransport;
    sim->setLatencyMs(0);
    client.session()->setTransport(sim);
    SessionRecorder recorder(client.session());
    QVERIFY(waitReady(client));
    QVERIFY(recorder.start(m_path, {{QStringLiteral("app"), QStringLiteral("test")}}));

    bool done = false;
    client.readAllDtcs([&](const QList<Dtc> &dtcs, const QString &) {
        for (const Dtc &d : dtcs)
            m_recordedCodes << d.code;
        client.readVehicleInfo([&](const ObdClient::VehicleInformation &info) {
            m_recordedVin = info.vin;
            client.readPids({0x0C, 0x0D, 0x05, 0x42}, [&](const QList<ObdClient::PidReading> &) { done = true; });
        });
    });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 5000));
    delete scan(client);
    recorder.stop();
    QVERIFY(!recorder.isRecording());
    QVERIFY(recorder.exchangeCount() > 20);

    QString error;
    QVERIFY2(m_recording.load(m_path, &error), qPrintable(error));
}

void TestRecording::adapterTarget()
{
    AdapterTarget t;
    t.apply("ATSH7D1");
    QCOMPARE(t.header, QStringLiteral("7D1"));
    t.apply("1902FF");
    QCOMPARE(t.header, QStringLiteral("7D1"));
    t.apply("ATSH7DF");
    QVERIFY(t.header.isEmpty());
    t.apply("ATSH7D2");
    t.apply("ATZ");
    QVERIFY(t.header.isEmpty());
}

void TestRecording::fileContents()
{
    QCOMPARE(m_recording.header.value(QStringLiteral("app")).toString(), QStringLiteral("test"));
    QVERIFY(isCan(m_recording.protocol()));
    QVERIFY(m_recording.headersOn());

    // The handshake from before "Record" is there as snapshot entries.
    bool sawReset = false, sawAbsRequest = false;
    for (const RecordedExchange &e : std::as_const(m_recording.exchanges)) {
        sawReset |= e.snapshot && e.command == "ATZ" && e.raw.contains("ELM327");
        sawAbsRequest |= !e.snapshot && e.target == QLatin1String("7D1") && e.command.startsWith("18");
    }
    QVERIFY(sawReset);
    QVERIFY(sawAbsRequest); // module requests remember which module they went to

    QJsonObject header;
    qint64 duration = -1;
    QVERIFY(Recording::peek(m_path, &header, &duration));
    QCOMPARE(header.value(QStringLiteral("format")).toString(), QStringLiteral("obdaryn-recording"));
    QCOMPARE(duration, m_recording.durationMs);
}

void TestRecording::replaysCodesAndVin()
{
    ObdClient *client = replayClient();
    QVERIFY(waitReady(*client));
    QStringList codes;
    QString vin;
    bool done = false;
    client->readAllDtcs([&](const QList<Dtc> &dtcs, const QString &) {
        for (const Dtc &d : dtcs)
            codes << d.code;
        client->readVehicleInfo([&](const ObdClient::VehicleInformation &info) {
            vin = info.vin;
            done = true;
        });
    });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 5000));
    QCOMPARE(codes, m_recordedCodes);
    QCOMPARE(vin, m_recordedVin);
    QVERIFY(codes.contains(QStringLiteral("P0420")));
    delete client;
}

void TestRecording::rebuildsLiveData()
{
    ObdClient *client = replayClient();
    QVERIFY(waitReady(*client));
    // A different combination and order than recorded: still answered.
    QList<ObdClient::PidReading> readings;
    bool done = false;
    client->readPids({0x42, 0x0C}, [&](const QList<ObdClient::PidReading> &r) { readings = r; done = true; });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 5000));
    QSet<quint8> pids;
    for (const auto &r : readings) {
        if (r.ecu == QLatin1String("7E8"))
            pids.insert(r.pid);
    }
    QCOMPARE(pids, (QSet<quint8>{0x42, 0x0C}));

    // A value that was never recorded is simply missing, as on a car that doesn't support it.
    done = false;
    client->readPids({0x5C}, [&](const QList<ObdClient::PidReading> &r) { readings = r; done = true; });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 5000));
    QVERIFY(readings.isEmpty());
    delete client;
}

void TestRecording::replaysModuleScan()
{
    ObdClient *client = replayClient();
    QVERIFY(waitReady(*client));
    ModuleScanner *scanner = scan(*client);
    QStringList found;
    for (const ModuleResult &r : scanner->results()) {
        for (const ModuleDtc &d : r.dtcs)
            found << r.module.category + QLatin1Char(':') + d.code;
    }
    QVERIFY2(found.contains(QStringLiteral("abs:C1203")), qPrintable(found.join(QLatin1Char(' '))));
    QVERIFY2(found.contains(QStringLiteral("airbag:B1346")), qPrintable(found.join(QLatin1Char(' '))));
    delete client;
}

void TestRecording::neverReplaysAClear()
{
    ObdClient *client = replayClient();
    QVERIFY(waitReady(*client));
    ElmSession::Reply reply;
    bool done = false;
    client->session()->send("04", [&](const ElmSession::Reply &r) { reply = r; done = true; },
                            ElmSession::Options{.userConfirmedClear = true});
    QVERIFY(QTest::qWaitFor([&] { return done; }, 5000));
    QCOMPARE(reply.status, ElmSession::Reply::Status::NoData);
    delete client;
}

void TestRecording::exportsCsv()
{
    const QStringList lines = m_recording.toCsv().split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    QVERIFY(lines.size() >= 2);
    QVERIFY2(lines.first().startsWith(QStringLiteral("\"Time (s)\"")), qPrintable(lines.first()));
    QVERIFY2(lines.first().contains(QStringLiteral("rpm")), qPrintable(lines.first()));
    // Every row has as many cells as the header.
    const qsizetype columns = lines.first().count(QLatin1Char(','));
    for (const QString &line : lines)
        QCOMPARE(line.count(QLatin1Char(',')), columns);
}

void TestRecording::rejectsOtherFiles()
{
    const QString path = m_dir.filePath(QStringLiteral("other.obdrec"));
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("{\"hello\":1}\n");
    f.close();
    Recording r;
    QString error;
    QVERIFY(!r.load(path, &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(!r.load(m_dir.filePath(QStringLiteral("missing.obdrec")), &error));
}

QTEST_GUILESS_MAIN(TestRecording)
#include "tst_recording.moc"
