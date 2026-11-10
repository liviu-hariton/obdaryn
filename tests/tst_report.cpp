// SPDX-License-Identifier: GPL-3.0-or-later
//
// The shareable report: what it says, and that nothing from the car or a
// recording can inject markup into it.

#include "report/VehicleReport.h"
#include "vehicles/ModuleCatalog.h"

#include <QTest>

using namespace obdaryn;

class TestReport : public QObject
{
    Q_OBJECT
private slots:
    void summaryAndCodes();
    void saysWhereTheDataCameFrom();
    void recentlyCleared();
    void escapesEverything();
    void targets();
    void fileName();

private:
    static ReportInput sample();
};

ReportInput TestReport::sample()
{
    ReportInput in;
    in.source = ReportInput::Source::Car;
    in.sourceDetail = QStringLiteral("192.168.0.10:35000");
    in.appVersion = QStringLiteral("0.0-test");
    in.createdAt = QDateTime(QDate(2026, 11, 11), QTime(14, 3));
    in.brand = QStringLiteral("hyundai-kia");
    in.language = QStringLiteral("en");
    in.info.vin = QStringLiteral("KMHDH41EBEU000001");
    in.info.ecuNames.insert(QStringLiteral("7E8"), QStringLiteral("ECM-EngineControl"));
    in.readiness = decodeMonitorStatus(QByteArray::fromHex("8207E505")); // MIL on, 2 incomplete
    in.dtcs = {{QStringLiteral("P0420"), QStringLiteral("7E8"), DtcKind::Stored},
               {QStringLiteral("P0420"), QStringLiteral("7E8"), DtcKind::Permanent}};
    in.batteryVolts = 12.6;
    return in;
}

void TestReport::summaryAndCodes()
{
    ReportInput in = sample();
    const QString html = VehicleReport::html(in);
    QVERIFY(html.contains(QStringLiteral("<h1>Hyundai 2014</h1>")));
    QVERIFY(html.contains(QStringLiteral("KMHDH41EBEU000001")));
    QVERIFY(html.contains(QStringLiteral("Catalyst System Efficiency Below Threshold Bank 1")));
    QCOMPARE(html.count(QStringLiteral(">P0420<")), 1); // one entry, listing both memories
    QVERIFY(html.contains(QStringLiteral("Permanent")));
    QVERIFY(html.contains(QStringLiteral("Not scanned"))); // no module scan yet: said plainly
    QVERIFY(html.contains(QStringLiteral("Not ready")));

    // With a module scan, codes from other modules get the brand explanation.
    const VehicleProfile *hk = ModuleCatalog::find(QStringLiteral("hyundai-kia"));
    QVERIFY(hk);
    ModuleResult abs;
    for (const ModuleDef &m : hk->modules) {
        if (m.category == QLatin1String("abs"))
            abs.module = m;
    }
    abs.state = ModuleResult::State::Answered;
    ModuleDtc c1203;
    c1203.code = QStringLiteral("C1203");
    abs.dtcs << c1203;
    in.modulesScanned = true;
    in.modules = {abs};
    const QString scanned = VehicleReport::html(in);
    QVERIFY(scanned.contains(QStringLiteral("Wheel-speed sensor front right")));
    QVERIFY(scanned.contains(QStringLiteral("Codes in other modules")));
}

void TestReport::saysWhereTheDataCameFrom()
{
    ReportInput in = sample();
    in.source = ReportInput::Source::Demo;
    QVERIFY(VehicleReport::html(in).contains(QStringLiteral("simulated data, not a real vehicle")));
    in.source = ReportInput::Source::Replay;
    in.sourceDetail = QStringLiteral("Hyundai 2014, 11/11/26");
    QVERIFY(VehicleReport::html(in).contains(QStringLiteral("Made from a recording")));
}

void TestReport::recentlyCleared()
{
    ReportInput in = sample();
    in.pids.insert(0x31, QByteArray::fromHex("0014")); // 20 km since codes were cleared
    in.pids.insert(0x30, QByteArray::fromHex("03"));   // 3 warm-ups
    QVERIFY(VehicleReport::html(in).contains(QStringLiteral(">Recently<")));
    in.pids.insert(0x31, QByteArray::fromHex("05AA")); // 1450 km: not recent, whatever the warm-ups
    QVERIFY(VehicleReport::html(in).contains(QStringLiteral(">Not recently<")));
    in.pids.clear();
    QVERIFY(VehicleReport::html(in).contains(QStringLiteral("doesn't report when its codes were cleared")));
}

void TestReport::escapesEverything()
{
    ReportInput in = sample();
    in.source = ReportInput::Source::Replay;
    in.sourceDetail = QStringLiteral("<img src=http://example.invalid/x>");
    in.info.ecuNames.insert(QStringLiteral("7E9"), QStringLiteral("<script>alert(1)</script>"));
    in.info.calibrationIds << QStringLiteral("<b>cal</b>");
    in.adapter = QStringLiteral("ELM327 <i>v1.5</i>");
    const QString html = VehicleReport::html(in);
    QVERIFY(!html.contains(QStringLiteral("<img")));
    QVERIFY(!html.contains(QStringLiteral("<script")));
    QVERIFY(!html.contains(QStringLiteral("<b>cal")));
    QVERIFY(!html.contains(QStringLiteral("<i>v1.5")));
    QVERIFY(html.contains(QStringLiteral("&lt;script&gt;")));
}

void TestReport::targets()
{
    const ReportInput in = sample();
    const QString web = VehicleReport::html(in, VehicleReport::Target::Web);
    const QString pdf = VehicleReport::html(in, VehicleReport::Target::Pdf);
    QVERIFY(web.contains(QStringLiteral("@media screen")));
    QVERIFY(web.contains(QStringLiteral("Inter")));
    // Qt's PDF path would apply screen rules, and can't use the variable Inter.
    QVERIFY(!pdf.contains(QStringLiteral("@media")));
    QVERIFY(!pdf.contains(QStringLiteral("Inter,")));
}

void TestReport::fileName()
{
    ReportInput in = sample();
    QCOMPARE(VehicleReport::suggestedBaseName(in), QStringLiteral("Obdaryn report 2026-11-11 Hyundai 2014"));
    in.source = ReportInput::Source::Demo;
    QCOMPARE(VehicleReport::suggestedBaseName(in), QStringLiteral("Obdaryn report 2026-11-11 demo car"));
}

QTEST_GUILESS_MAIN(TestReport)
#include "tst_report.moc"
