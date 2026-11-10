// SPDX-License-Identifier: GPL-3.0-or-later

#include "obd/ResponseParser.h"

#include <QTest>

using namespace obdaryn;

class TestResponseParser : public QObject
{
    Q_OBJECT
private slots:
    void canSingleFrameWithPadding();
    void canTwoEcus();
    void canMultiFrameVin();
    void canNoSpaces();
    void canHeadersOffMultiFrame();
    void can29Bit();
    void legacyWithHeaders();
};

void TestResponseParser::canSingleFrameWithPadding()
{
    const auto msgs = ResponseParser::parse({"7E8 04 41 0C 1A F8 AA AA AA"}, Protocol::Can11_500, true);
    QCOMPARE(msgs.size(), 1);
    QCOMPARE(msgs[0].ecu, QStringLiteral("7E8"));
    QCOMPARE(msgs[0].data, QByteArray::fromHex("410C1AF8")); // PCI length trims the padding
}

void TestResponseParser::canTwoEcus()
{
    const auto msgs = ResponseParser::parse({"7E9 06 41 00 80 00 00 00 AA", "7E8 06 41 00 BE 3F A8 13 AA"},
                                            Protocol::Can11_500, true);
    QCOMPARE(msgs.size(), 2);
    QCOMPARE(msgs[0].ecu, QStringLiteral("7E8")); // sorted: engine first
    QCOMPARE(msgs[1].ecu, QStringLiteral("7E9"));
    QCOMPARE(msgs[0].data, QByteArray::fromHex("4100BE3FA813"));
}

void TestResponseParser::canMultiFrameVin()
{
    const QStringList lines = {
        "7E8 10 14 49 02 01 4B 4D 48",
        "7E8 21 44 48 34 31 45 42 45",
        "7E8 22 55 30 30 30 30 30 31",
    };
    const auto msgs = ResponseParser::parse(lines, Protocol::Can11_500, true);
    QCOMPARE(msgs.size(), 1);
    QCOMPARE(msgs[0].data.size(), 20);
    QCOMPARE(msgs[0].data.mid(3), QByteArray("KMHDH41EBEU000001"));
}

void TestResponseParser::canNoSpaces()
{
    const auto msgs = ResponseParser::parse({"7E804410C1AF8AAAAAA"}, Protocol::Can11_500, true);
    QCOMPARE(msgs.size(), 1);
    QCOMPARE(msgs[0].data, QByteArray::fromHex("410C1AF8"));
}

void TestResponseParser::canHeadersOffMultiFrame()
{
    const QStringList lines = {"014", "0: 49 02 01 4B 4D 48", "1: 44 48 34 31 45 42 45", "2: 55 30 30 30 30 30 31"};
    const auto msgs = ResponseParser::parse(lines, Protocol::Can11_500, false);
    QCOMPARE(msgs.size(), 1);
    QCOMPARE(msgs[0].data.mid(3), QByteArray("KMHDH41EBEU000001"));
}

void TestResponseParser::can29Bit()
{
    const auto msgs = ResponseParser::parse({"18 DA F1 10 03 41 0D 32 00 00 00 00"}, Protocol::Can29_500, true);
    QCOMPARE(msgs.size(), 1);
    QCOMPARE(msgs[0].ecu, QStringLiteral("18DAF110"));
    QCOMPARE(msgs[0].data, QByteArray::fromHex("410D32"));
}

void TestResponseParser::legacyWithHeaders()
{
    // ISO 9141-2: 3 header bytes, data, checksum.
    const auto msgs = ResponseParser::parse({"48 6B 10 41 0C 1A F8 C4"}, Protocol::Iso9141, true);
    QCOMPARE(msgs.size(), 1);
    QCOMPARE(msgs[0].ecu, QStringLiteral("10"));
    QCOMPARE(msgs[0].data, QByteArray::fromHex("410C1AF8"));
}

QTEST_GUILESS_MAIN(TestResponseParser)
#include "tst_responseparser.moc"
