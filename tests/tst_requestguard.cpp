// SPDX-License-Identifier: GPL-3.0-or-later
//
// The safety guard is the most important code in Obdaryn: these tests pin
// down exactly what may and may not reach the car.

#include "safety/RequestGuard.h"

#include <QTest>

using obdaryn::RequestGuard;

class TestRequestGuard : public QObject
{
    Q_OBJECT
private slots:
    void allowsReadServices_data();
    void allowsReadServices();
    void blocksWriteAndControlServices_data();
    void blocksWriteAndControlServices();
    void clearingNeedsConfirmation();
    void adapterCommands();
    void malformedInput();
};

void TestRequestGuard::allowsReadServices_data()
{
    QTest::addColumn<QByteArray>("cmd");
    QTest::newRow("rpm") << QByteArray("010C");
    QTest::newRow("rpm with spaces") << QByteArray("01 0c");
    QTest::newRow("rpm with response count") << QByteArray("010C1");
    QTest::newRow("multi pid") << QByteArray("010C0D05110B04");
    QTest::newRow("freeze frame") << QByteArray("020C00");
    QTest::newRow("stored dtc") << QByteArray("03");
    QTest::newRow("monitor tests") << QByteArray("0600");
    QTest::newRow("pending dtc") << QByteArray("07");
    QTest::newRow("vin") << QByteArray("0902");
    QTest::newRow("permanent dtc") << QByteArray("0A");
    QTest::newRow("uds read dtc") << QByteArray("1902FF");
    QTest::newRow("uds read did") << QByteArray("22F190");
    QTest::newRow("uds tester present") << QByteArray("3E00");
    QTest::newRow("uds tester present no reply") << QByteArray("3E80");
    QTest::newRow("kwp read dtc") << QByteArray("1802FF00");
    QTest::newRow("kwp ecu id") << QByteArray("1A87");
    QTest::newRow("kwp local id (Hyundai 2101)") << QByteArray("2101");
}

void TestRequestGuard::allowsReadServices()
{
    QFETCH(QByteArray, cmd);
    const auto v = RequestGuard::check(cmd);
    QVERIFY2(v.allowed, qPrintable(v.reason));
}

void TestRequestGuard::blocksWriteAndControlServices_data()
{
    QTest::addColumn<QByteArray>("cmd");
    QTest::newRow("obd on-board control") << QByteArray("0801");
    QTest::newRow("session control extended") << QByteArray("1003");
    QTest::newRow("session control programming") << QByteArray("1002");
    QTest::newRow("ecu reset") << QByteArray("1101");
    QTest::newRow("read memory by address") << QByteArray("2312345601");
    QTest::newRow("security access") << QByteArray("2701");
    QTest::newRow("communication control") << QByteArray("280301");
    QTest::newRow("write did") << QByteArray("2EF19001");
    QTest::newRow("io control uds") << QByteArray("2F123403");
    QTest::newRow("io control kwp") << QByteArray("300107");
    QTest::newRow("routine control") << QByteArray("3101FF00");
    QTest::newRow("request download") << QByteArray("3400");
    QTest::newRow("transfer data") << QByteArray("3601");
    QTest::newRow("kwp write local id") << QByteArray("3B0101");
    QTest::newRow("control dtc setting") << QByteArray("8502");
    QTest::newRow("tester present odd subfunction") << QByteArray("3E01");
    QTest::newRow("too long for one frame") << QByteArray("0100000000000000");
}

void TestRequestGuard::blocksWriteAndControlServices()
{
    QFETCH(QByteArray, cmd);
    QVERIFY(!RequestGuard::check(cmd).allowed);
    // Not even a "confirmed" flag can unlock a non-clear service.
    QVERIFY(!RequestGuard::check(cmd, true).allowed);
}

void TestRequestGuard::clearingNeedsConfirmation()
{
    QVERIFY(!RequestGuard::check("04").allowed);
    QVERIFY(RequestGuard::check("04", true).allowed);
    QVERIFY(!RequestGuard::check("14FFFFFF").allowed);
    QVERIFY(RequestGuard::check("14FFFFFF", true).allowed);  // UDS: all groups
    QVERIFY(RequestGuard::check("14FF00", true).allowed);    // KWP: all groups
    QVERIFY(!RequestGuard::check("14", true).allowed);       // malformed
}

void TestRequestGuard::adapterCommands()
{
    for (const char *ok : {"ATZ", "AT Z", "ate0", "ATH1", "ATSP0", "ATSPA6", "ATSH7E0",
                           "AT SH 18 DA 10 F1", "ATCRA7E8", "ATCRA", "ATST32", "ATRV",
                           "ATDPN", "AT@1", "ATI", "ATAT2"}) {
        QVERIFY2(RequestGuard::check(ok).allowed, ok);
    }
    // These could transmit arbitrary bytes or permanently change the adapter.
    for (const char *bad : {"ATWM8210F13E", "ATFCSD300000", "ATRTR", "AT@3ABCDEFGHIJKL",
                            "ATPP0ESV7A", "ATPP0EON", "ATBRD23", "ATMA", "ATLP", "ATSH7E", "ATCFC0"}) {
        QVERIFY2(!RequestGuard::check(bad).allowed, bad);
    }
}

void TestRequestGuard::malformedInput()
{
    // A bare CR makes the ELM327 repeat the previous command (maybe "04").
    QVERIFY(!RequestGuard::check("").allowed);
    QVERIFY(!RequestGuard::check("   ").allowed);
    // Two commands smuggled into one.
    QVERIFY(!RequestGuard::check("0100\r04").allowed);
    QVERIFY(!RequestGuard::check("ATZ\n04").allowed);
    // STN-only or garbage.
    QVERIFY(!RequestGuard::check("STDI").allowed);
    QVERIFY(!RequestGuard::check("HELLO").allowed);
}

QTEST_GUILESS_MAIN(TestRequestGuard)
#include "tst_requestguard.moc"
