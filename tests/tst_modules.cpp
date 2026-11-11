// SPDX-License-Identifier: GPL-3.0-or-later
//
// Full-vehicle module scan: profiles, UDS / KWP decoding, and an end-to-end
// scan of the simulated Hyundai.

#include "obd/ModuleScanner.h"
#include "obd/ObdClient.h"
#include "obd/Pids.h"
#include "transport/SimulatedTransport.h"
#include "vehicles/ModuleCatalog.h"

#include <QSignalSpy>
#include <QTest>

using namespace obdaryn;

class TestModules : public QObject
{
    Q_OBJECT
private slots:
    void init();
    void cleanup();
    void catalogLoadsBrands();
    void profileMatching();
    void flowControlRule();
    void decodesUds();
    void decodesKwp();
    void scansDemoCar();
    void restoresGenericAddressing();
    void clearsOneModule();

private:
    bool connectDemo();
    bool runScan(ModuleScanner &scanner);
    const ModuleResult *find(const ModuleScanner &s, const QString &category) const;
    ObdClient *m_client = nullptr;
    SimulatedTransport *m_sim = nullptr;
};

void TestModules::init()
{
    m_client = new ObdClient;
    m_sim = new SimulatedTransport;
    m_sim->setLatencyMs(0);
    m_client->session()->setTransport(m_sim);
}

void TestModules::cleanup()
{
    delete m_client;
    m_client = nullptr;
}

bool TestModules::connectDemo()
{
    m_client->session()->open();
    return QTest::qWaitFor([this] { return m_client->session()->state() == ElmSession::State::Ready; }, 3000);
}

bool TestModules::runScan(ModuleScanner &scanner)
{
    QSignalSpy done(&scanner, &ModuleScanner::finished);
    scanner.scan(*ModuleCatalog::find(QStringLiteral("hyundai-kia")));
    return done.wait(20000);
}

const ModuleResult *TestModules::find(const ModuleScanner &s, const QString &category) const
{
    for (const ModuleResult &r : s.results()) {
        if (r.module.category == category)
            return &r;
    }
    return nullptr;
}

void TestModules::catalogLoadsBrands()
{
    QStringList ids;
    for (const VehicleProfile &p : ModuleCatalog::profiles())
        ids << p.id;
    for (const char *id : {"generic", "hyundai-kia", "toyota-lexus", "ford", "vag", "renault-dacia"})
        QVERIFY2(ids.contains(QLatin1String(id)), id);
    QCOMPARE(ids.last(), QStringLiteral("generic")); // fallback listed last
    for (const VehicleProfile &p : ModuleCatalog::profiles())
        QVERIFY2(!p.modules.isEmpty(), qPrintable(p.id));
}

void TestModules::profileMatching()
{
    QCOMPARE(ModuleCatalog::forVehicle("Hyundai", "KMHDH41EBEU000001").id, QStringLiteral("hyundai-kia"));
    QCOMPARE(ModuleCatalog::forVehicle("Dacia", "UU1ABC").id, QStringLiteral("renault-dacia"));
    QCOMPARE(ModuleCatalog::forVehicle("Škoda", "").id, QStringLiteral("vag"));      // by name, no VIN
    QCOMPARE(ModuleCatalog::forVehicle("", "JTDBR32E").id, QStringLiteral("toyota-lexus")); // 2-char WMI
    QCOMPARE(ModuleCatalog::forVehicle("Lada", "XTA21").id, QStringLiteral("generic"));
    // Suzuki: Japanese, Hungarian (also the Fiat Sedici) and Spanish (Santana) VINs.
    QCOMPARE(ModuleCatalog::forVehicle("", "JSAJTD54V00100001").id, QStringLiteral("suzuki"));
    QCOMPARE(ModuleCatalog::forVehicle("", "TSMLYD21S00100001").id, QStringLiteral("suzuki"));
    QCOMPARE(ModuleCatalog::forVehicle("", "VSEFJ13V000100001").id, QStringLiteral("suzuki")); // not SEAT (VSS)
    QCOMPARE(ModuleCatalog::forVehicle("Suzuki", "").id, QStringLiteral("suzuki"));
    // The Toyota-built Suzuki Across carries Toyota's electronics.
    QCOMPARE(ModuleCatalog::forVehicle("", "JSTGBRFV00D085679").id, QStringLiteral("toyota-lexus"));
}

void TestModules::flowControlRule()
{
    const VehicleProfile *vag = ModuleCatalog::find("vag");
    const VehicleProfile *hk = ModuleCatalog::find("hyundai-kia");
    for (const ModuleDef &m : hk->modules)
        QVERIFY2(!m.needsFlowControlHeader(), qPrintable(m.id)); // Hyundai: always +8
    bool vagNeedsFc = false;
    for (const ModuleDef &m : vag->modules)
        vagNeedsFc |= m.needsFlowControlHeader();
    QVERIFY(vagNeedsFc); // 713 → 77D
}

void TestModules::decodesUds()
{
    // Two records: U0100 confirmed + lamp; C0035-1A failing now, pending; one with status 0 (ignored).
    const auto dtcs = ModuleDtcCodec::fromUds(QByteArray::fromHex("5902FF" "C1000088" "40351A05" "01330000"));
    QCOMPARE(dtcs.size(), 2);
    QCOMPARE(dtcs[0].displayCode(), QStringLiteral("U0100"));
    QCOMPARE(dtcs[0].kind, DtcKind::Stored);
    QVERIFY(dtcs[0].warningLamp);
    QVERIFY(!dtcs[0].activeNow);
    QCOMPARE(dtcs[1].displayCode(), QStringLiteral("C0035-1A"));
    QCOMPARE(dtcs[1].kind, DtcKind::Pending);
    QVERIFY(dtcs[1].activeNow);
}

void TestModules::decodesKwp()
{
    // C0035 present now + lamp, P0133 stored not present, P0171 maturing.
    const auto dtcs = ModuleDtcCodec::fromKwp(QByteArray::fromHex("5803" "4035E0" "013320" "017140"));
    QCOMPARE(dtcs.size(), 3);
    QCOMPARE(dtcs[0].code, QStringLiteral("C0035"));
    QVERIFY(dtcs[0].activeNow && dtcs[0].warningLamp);
    QCOMPARE(dtcs[1].kind, DtcKind::Stored);
    QVERIFY(!dtcs[1].activeNow);
    QCOMPARE(dtcs[2].kind, DtcKind::Pending);
    QCOMPARE(ModuleDtcCodec::fromKwp(QByteArray::fromHex("5800")).size(), 0);
}

void TestModules::scansDemoCar()
{
    QVERIFY(connectDemo());
    ModuleScanner scanner(m_client);
    QVERIFY(runScan(scanner));

    const ModuleResult *abs = find(scanner, "abs");
    QVERIFY(abs);
    QCOMPARE(abs->state, ModuleResult::State::Answered);
    QCOMPARE(abs->protocol, DiagProtocol::Kwp);            // UDS refused, fell back to KWP
    QCOMPARE(abs->dtcs.size(), 1);
    QCOMPARE(abs->dtcs[0].code, QStringLiteral("C1203"));
    QVERIFY(abs->dtcs[0].activeNow);
    QCOMPARE(abs->partNumber, QStringLiteral("58920-3X000"));

    const ModuleResult *airbag = find(scanner, "airbag");
    QCOMPARE(airbag->state, ModuleResult::State::Answered);
    QCOMPARE(airbag->protocol, DiagProtocol::Uds);
    QCOMPARE(airbag->dtcs.size(), 1);
    QCOMPARE(airbag->dtcs[0].code, QStringLiteral("B1346"));
    QCOMPARE(airbag->softwareVersion, QStringLiteral("1.04"));

    const ModuleResult *engine = find(scanner, "engine");
    QCOMPARE(engine->dtcs.size(), 3);                      // P0133 + P0420 stored, P0171 pending

    QCOMPARE(find(scanner, "body")->dtcs.size(), 0);
    QCOMPARE(find(scanner, "cluster")->state, ModuleResult::State::Answered);
    QCOMPARE(find(scanner, "steering")->state, ModuleResult::State::Absent);
    QCOMPARE(find(scanner, "climate")->state, ModuleResult::State::Absent);
    // TPMS answers TesterPresent but not code requests: present, codes unreadable.
    QCOMPARE(find(scanner, "tpms")->state, ModuleResult::State::Error);
    QVERIFY(!find(scanner, "tpms")->error.isEmpty());
}

void TestModules::restoresGenericAddressing()
{
    QVERIFY(connectDemo());
    ModuleScanner scanner(m_client);
    QVERIFY(runScan(scanner));

    // After the scan, a functional request must reach the engine and the
    // transmission again (both answer 0100).
    ObdClient::Response resp;
    bool done = false;
    m_client->request("0100", [&](const ObdClient::Response &r) { resp = r; done = true; });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 3000));
    QCOMPARE(resp.positive.size(), 2);
}

void TestModules::clearsOneModule()
{
    QVERIFY(connectDemo());
    ModuleScanner scanner(m_client);
    QVERIFY(runScan(scanner));
    int airbagIndex = -1;
    for (int i = 0; i < scanner.results().size(); ++i)
        if (scanner.results()[i].module.category == QLatin1String("airbag"))
            airbagIndex = i;
    QVERIFY(airbagIndex >= 0);

    // Engine running: refused before anything is sent to the module.
    m_sim->setEngineRunning(true);
    bool ok = true, done = false;
    scanner.clearModule(airbagIndex, [&](bool success, const QString &) { ok = success; done = true; });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 3000));
    QVERIFY(!ok);
    QCOMPARE(scanner.results()[airbagIndex].dtcs.size(), 1);

    // Engine off: cleared, and the list is re-read.
    m_sim->setEngineRunning(false);
    done = false;
    scanner.clearModule(airbagIndex, [&](bool success, const QString &) { ok = success; done = true; });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 5000));
    QVERIFY(ok);
    QCOMPARE(scanner.results()[airbagIndex].dtcs.size(), 0);
}

QTEST_GUILESS_MAIN(TestModules)
#include "tst_modules.moc"
