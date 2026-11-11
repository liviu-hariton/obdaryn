// SPDX-License-Identifier: GPL-3.0-or-later

#include "obd/Dtc.h"
#include "obd/DtcDatabase.h"
#include "obd/Pids.h"
#include "obd/Protocol.h"
#include "obd/Readiness.h"
#include "obd/Units.h"
#include "obd/VehicleInfo.h"

#include <QTest>

using namespace obdaryn;

class TestDecoders : public QObject
{
    Q_OBJECT
private slots:
    void pidFormulas();
    void dtcDecoding();
    void dtcFromCanAndLegacy();
    void dtcDatabase();
    void manufacturerSpecific();
    void brandPack();
    void readiness();
    void vin();
    void protocolNumbers();
    void units();
};

void TestDecoders::pidFormulas()
{
    auto decode = [](quint8 pid, const char *hex) {
        double v = 0;
        const bool ok = PidTable::find(pid)->value(QByteArray::fromHex(hex), &v);
        return ok ? v : -9999.0;
    };
    QCOMPARE(decode(0x0C, "1AF8"), 1726.0);     // (26*256 + 248) / 4
    QCOMPARE(decode(0x05, "7B"), 83.0);         // 123 - 40
    QCOMPARE(decode(0x0D, "32"), 50.0);
    QCOMPARE(decode(0x42, "3746"), 14.150);     // 14150 mV
    QCOMPARE(decode(0x06, "80"), 0.0);          // trim: 128 = 0 %
    QVERIFY(qAbs(decode(0x04, "FF") - 100.0) < 1e-9);
    QCOMPARE(decode(0x32, "FFFC"), -1.0);       // signed vapor pressure
    QCOMPARE(PidTable::dataLength(0x0C), 2);
    QCOMPARE(PidTable::dataLength(0x40), 4);
    QCOMPARE(PidTable::dataLength(0x99), -1);
}

void TestDecoders::dtcDecoding()
{
    QCOMPARE(DtcCodec::decode(0x04, 0x20), QStringLiteral("P0420"));
    QCOMPARE(DtcCodec::decode(0x01, 0x33), QStringLiteral("P0133"));
    QCOMPARE(DtcCodec::decode(0x41, 0x23), QStringLiteral("C0123"));
    QCOMPARE(DtcCodec::decode(0x92, 0x34), QStringLiteral("B1234"));
    QCOMPARE(DtcCodec::decode(0xC1, 0x00), QStringLiteral("U0100"));
    QCOMPARE(DtcCodec::decode(0x2A, 0xBC), QStringLiteral("P2ABC"));
}

void TestDecoders::dtcFromCanAndLegacy()
{
    EcuMessage can{"7E8", QByteArray::fromHex("4302013304200000"), {}};
    QCOMPARE(DtcCodec::fromResponse(can, true), (QStringList{"P0133", "P0420"}));

    EcuMessage legacy;
    legacy.frames << QByteArray::fromHex("43013304200171") << QByteArray::fromHex("43030000000000");
    QCOMPARE(DtcCodec::fromResponse(legacy, false), (QStringList{"P0133", "P0420", "P0171", "P0300"}));
}

void TestDecoders::dtcDatabase()
{
    const DtcInfo info = DtcDatabase::instance().lookup("p0420");
    QVERIFY(info.known);
    QVERIFY(info.title.contains(QStringLiteral("Catalyst")));
    QVERIFY(!info.causes.isEmpty());
    QVERIFY(info.emissionsRelevant);
    QVERIFY(DtcDatabase::instance().size() > 9000);

    const DtcInfo unknown = DtcDatabase::instance().lookup("P1A2B");
    QVERIFY(!unknown.known);
    QVERIFY(unknown.manufacturerSpecific);
    QVERIFY(!unknown.system.isEmpty());
}

void TestDecoders::brandPack()
{
    const DtcDatabase &db = DtcDatabase::instance();
    QVERIFY(db.brandSize("hyundai-kia") > 150);

    // Same code, different module, different meaning.
    const DtcInfo esc = db.lookup("C1604", "hyundai-kia", "en", "abs");
    const DtcInfo mdps = db.lookup("C1604", "hyundai-kia", "en", "steering");
    QVERIFY(esc.known && mdps.known);
    QVERIFY(esc.title.contains(QStringLiteral("ABS/ESC")));
    QVERIFY(mdps.title.contains(QStringLiteral("Power steering")));
    QCOMPARE(esc.confidence, QStringLiteral("official"));
    QVERIFY(!esc.references.isEmpty());

    // A brand entry for another module is never used.
    QVERIFY(!db.lookup("C1513", "hyundai-kia", "en", "body").known);
    // Without a brand, a manufacturer code stays unexplained.
    QVERIFY(!db.lookup("C1513").known);

    // Romanian text, and English fallback for the generic layer.
    const DtcInfo ro = db.lookup("B1346", "hyundai-kia", "ro", "airbag");
    QCOMPARE(ro.language, QStringLiteral("ro"));
    QVERIFY(ro.title.contains(QStringLiteral("Airbag frontal șofer")));
    QCOMPARE(db.lookup("P0420", "hyundai-kia", "ro").source, QStringLiteral("generic"));
    // Every vehicle profile with a brand pack loads, with Romanian text.
    for (const char *brand : {"toyota-lexus", "ford", "vag", "renault-dacia"})
        QVERIFY2(db.brandSize(brand) > 20, brand);
    QVERIFY(db.lookup("C1201", "toyota-lexus", "en", "abs").known);
    QCOMPARE(db.lookup("B1000", "vag", "ro", "airbag").language, QStringLiteral("ro"));

    // Romanian layer over the generic codes: every title is translated.
    QVERIFY(db.lookup("P0420", "", "ro").title.contains(QStringLiteral("catalizator")));
    QCOMPARE(db.lookup("P0420", "", "en").title, QStringLiteral("Catalyst System Efficiency Below Threshold Bank 1"));

    // Standard code with a Hyundai/Kia override (6-speed automatic solenoid names).
    const DtcInfo a6 = db.lookup("P0753", "hyundai-kia", "en", "transmission");
    QCOMPARE(a6.source, QStringLiteral("hyundai-kia"));
    QVERIFY(a6.title.contains(QStringLiteral("UD brake")));
    QCOMPARE(db.lookup("P0753", "", "en").source, QStringLiteral("generic"));
}

void TestDecoders::manufacturerSpecific()
{
    QVERIFY(DtcCodec::isManufacturerSpecific("P1326"));  // Hyundai/Kia knock sensor detect
    QVERIFY(DtcCodec::isManufacturerSpecific("P3000"));
    QVERIFY(!DtcCodec::isManufacturerSpecific("P3400"));
    QVERIFY(!DtcCodec::isManufacturerSpecific("P0420"));
    QVERIFY(DtcCodec::isManufacturerSpecific("B1234"));
    QVERIFY(!DtcCodec::isManufacturerSpecific("U0100"));
}

void TestDecoders::readiness()
{
    // MIL on, 2 codes; continuous monitors all complete; catalyst + EVAP incomplete.
    const MonitorStatus s = decodeMonitorStatus(QByteArray::fromHex("8207E505"));
    QVERIFY(s.valid);
    QVERIFY(s.milOn);
    QCOMPARE(s.storedDtcCount, 2);
    QVERIFY(!s.compressionIgnition);
    QCOMPARE(s.monitors.size(), 3 + 5);
    QCOMPARE(s.incompleteCount(), 2);
    QVERIFY(!s.likelyReadyForInspection());

    const MonitorStatus ok = decodeMonitorStatus(QByteArray::fromHex("0007E500"));
    QCOMPARE(ok.incompleteCount(), 0);
    QVERIFY(ok.likelyReadyForInspection());
}

void TestDecoders::vin()
{
    const VinInfo hy = decodeVin("KMHDH41EBEU000001");
    QVERIFY(hy.valid);
    QCOMPARE(hy.manufacturer, QStringLiteral("Hyundai"));
    QCOMPARE(hy.modelYear, 2014);

    // A real-format North American VIN with a valid check digit.
    const VinInfo us = decodeVin("1M8GDM9AXKP042788");
    QVERIFY(us.valid);
    QVERIFY(us.checkDigitValid);

    QVERIFY(!decodeVin("KMHDH41EBEU00000O").valid); // letter O is never used

    EcuMessage m{"7E8", QByteArray("\x49\x02\x01", 3) + "KMHDH41EBEU000001", {}};
    QCOMPARE(VehicleInfoCodec::vin(m, true), QStringLiteral("KMHDH41EBEU000001"));
}

void TestDecoders::protocolNumbers()
{
    QCOMPARE(protocolFromDpn("A6"), Protocol::Can11_500);
    QCOMPARE(protocolFromDpn("7"), Protocol::Can29_500);
    QCOMPARE(protocolFromDpn("3"), Protocol::Iso9141);
    QCOMPARE(protocolFromDpn("AA"), Protocol::J1939);
    QCOMPARE(protocolFromDpn("?"), Protocol::Unknown);
    QVERIFY(isCan(Protocol::Can11_250));
    QVERIFY(!isCan(Protocol::KwpFast));
}

void TestDecoders::units()
{
    using namespace Units;
    setSystem(System::Metric);
    QCOMPARE(show(100, "km/h", 0).value, 100.0);
    QCOMPARE(show(90, "°C", 0).unit, QStringLiteral("°C"));

    setSystem(System::US);
    QVERIFY(qAbs(show(100, "km/h", 0).value - 62.137) < 0.001);
    QCOMPARE(show(100, "km/h", 0).unit, QStringLiteral("mph"));
    QCOMPARE(show(100, "°C", 0).value, 212.0);
    QVERIFY(qAbs(show(100, "kPa", 0).value - 14.5038) < 0.001);
    QVERIFY(qAbs(show(10, "L/100 km", 1).value - 23.52) < 0.01); // US mpg
    QCOMPARE(show(0, "L/100 km", 1).value, 0.0);                 // unknown stays unknown
    QVERIFY(qAbs(litersFromShownVolume(1) - 3.785) < 0.001);
    QCOMPARE(show(14.2, "V", 1).value, 14.2);                     // not converted

    setSystem(System::UK);
    QCOMPARE(show(90, "°C", 0).unit, QStringLiteral("°C"));       // the UK keeps Celsius
    QVERIFY(qAbs(show(10, "L/100 km", 1).value - 28.25) < 0.01); // imperial mpg
    QVERIFY(qAbs(show(4.54609, "L", 2).value - 1.0) < 1e-9);
    setSystem(System::Metric);
}

QTEST_GUILESS_MAIN(TestDecoders)
#include "tst_decoders.moc"
