// SPDX-License-Identifier: GPL-3.0-or-later
//
// Manufacturer values (OBDb data packs): the shipped data, the decoder checked
// against answers recorded from real Elantras, the safety guard, and the
// reader against the demo car.

#include "obd/ManufacturerData.h"
#include "obd/ManufacturerReader.h"
#include "obd/ModuleAddressing.h"
#include "obd/ObdClient.h"
#include "obd/ResponseParser.h"
#include "safety/RequestGuard.h"
#include "transport/SimulatedTransport.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include <cmath>

using namespace obdaryn;

class TestManufacturer : public QObject
{
    Q_OBJECT
private slots:
    void packLoads();
    void yearFilter();
    void everyRequestPassesTheGuard();
    void decodesRealAnswers();
    void decodesFields();
    void units();
    void readsTheDemoCar();

private:
    static const ManufacturerModel &elantra();
    static const ManufacturerCommand *find(const QList<ManufacturerCommand> &list, const QString &key);
};

const ManufacturerModel &TestManufacturer::elantra()
{
    const ManufacturerModel *m = ManufacturerData::instance().find(QStringLiteral("Hyundai"), QStringLiteral("Elantra"));
    Q_ASSERT(m);
    return *m;
}

const ManufacturerCommand *TestManufacturer::find(const QList<ManufacturerCommand> &list, const QString &key)
{
    for (const ManufacturerCommand &c : list) {
        if (c.key() == key)
            return &c;
    }
    return nullptr;
}

void TestManufacturer::packLoads()
{
    const ManufacturerData &data = ManufacturerData::instance();
    QVERIFY(data.models().size() > 20);
    QVERIFY(data.modelsOf(QStringLiteral("Hyundai")).contains(QStringLiteral("Elantra")));
    QVERIFY(data.modelsOf(QStringLiteral("kia")).contains(QStringLiteral("Sportage")));
    QVERIFY(data.attribution().contains(QStringLiteral("CC BY-SA 4.0")));
    QVERIFY(!elantra().commit.isEmpty());
    QCOMPARE(elantra().repository, QStringLiteral("OBDb/Hyundai-Elantra"));
}

void TestManufacturer::yearFilter()
{
    const QList<ManufacturerCommand> y2014 = elantra().commandsFor(2014);
    QVERIFY(find(y2014, QStringLiteral("7E0:2101")));  // up to 2020
    QVERIFY(!find(y2014, QStringLiteral("770:22BC03"))); // 2021 on
    const QList<ManufacturerCommand> y2022 = elantra().commandsFor(2022);
    QVERIFY(!find(y2022, QStringLiteral("7E0:2101")));
    // An unknown year keeps only what applies to every year.
    for (const ManufacturerCommand &c : elantra().commandsFor(0))
        QVERIFY(c.years.empty() && c.debugYears.empty());

    YearRange r;
    r.from = 2010; r.to = 2015;
    QVERIFY(r.contains(2014) && !r.contains(2016) && !r.contains(2009));
    YearRange listed;
    listed.years = {2019};
    QVERIFY(listed.contains(2019) && !listed.contains(2018));
}

void TestManufacturer::everyRequestPassesTheGuard()
{
    // Every request in every shipped pack, and the adapter commands that address
    // its module, must be allowed: the pack can never make Obdaryn write.
    int checked = 0;
    for (const ManufacturerModel &m : ManufacturerData::instance().models()) {
        for (const ManufacturerCommand &c : m.commands) {
            const GuardVerdict v = RequestGuard::check(c.request());
            QVERIFY2(v.allowed, qPrintable(m.model + QLatin1Char(' ') + c.key() + QStringLiteral(": ") + v.reason));
            QVERIFY(c.service == 0x21 || c.service == 0x22);
            for (const QByteArray &at : {"ATSH" + ModuleAddressing::hex3(c.header), "ATCRA" + ModuleAddressing::hex3(c.receiver),
                                         "ATFCSH" + ModuleAddressing::hex3(c.header)})
                QVERIFY2(RequestGuard::check(at).allowed, at.constData());
            ++checked;
        }
    }
    QVERIFY(checked > 500);
}

void TestManufacturer::decodesRealAnswers()
{
    // Answers recorded from 2013-2014 Elantras, with the values OBDb decodes from
    // them (OBDb Hyundai-Elantra test cases, CC BY-SA 4.0).
    QFile f(QStringLiteral(OBDARYN_TEST_DATA "/obdb_elantra_cases.json"));
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QJsonArray cases = QJsonDocument::fromJson(f.readAll()).object().value(QStringLiteral("cases")).toArray();
    QVERIFY(cases.size() > 20);
    int compared = 0, skipped = 0;
    for (const QJsonValue &cv : cases) {
        const QJsonObject c = cv.toObject();
        const int year = c.value(QStringLiteral("year")).toInt();
        const QString key = c.value(QStringLiteral("header")).toString() + QLatin1Char(':') + c.value(QStringLiteral("request")).toString();
        const QList<ManufacturerCommand> commands = elantra().commandsFor(year);
        const ManufacturerCommand *command = find(commands, key);
        if (!command && key == QLatin1String("7E0:2116")) { // every value in it is excluded (knock retard)
            ++skipped;
            continue;
        }
        QVERIFY2(command, qPrintable(key));
        QStringList lines;
        for (const QJsonValue &l : c.value(QStringLiteral("lines")).toArray())
            lines << l.toString();
        const QList<EcuMessage> messages = ResponseParser::parse(lines, Protocol::Can11_500, true);
        QCOMPARE(messages.size(), 1);
        QCOMPARE(messages.first().ecu, QString::fromLatin1(ModuleAddressing::hex3(command->receiver)));
        QList<ManufacturerReader::Value> values;
        QVERIFY(ManufacturerReader::decodeAnswer(*command, messages.first().data, &values));
        const QJsonObject expected = c.value(QStringLiteral("expected")).toObject();
        for (auto it = expected.begin(); it != expected.end(); ++it) {
            int index = -1;
            for (int i = 0; i < command->signalList.size(); ++i) {
                if (command->signalList[i].id == it.key())
                    index = i;
            }
            if (index < 0) { // left out on import (EXCLUDED in tools/import_obdb.py)
                ++skipped;
                continue;
            }
            bool found = false;
            for (const ManufacturerReader::Value &v : values) {
                if (v.signal != index)
                    continue;
                found = true;
                const double want = it.value().toDouble();
                QVERIFY2(std::abs(v.value - want) <= std::max(0.01, std::abs(want) * 1e-4),
                         qPrintable(QStringLiteral("%1 %2: %3, expected %4").arg(key, it.key()).arg(v.value).arg(want)));
                ++compared;
            }
            // A "not available" value (outside its null range) is simply missing.
            if (!found)
                QVERIFY(command->signalList[index].hasNullMin || command->signalList[index].hasNullMax);
        }
    }
    QVERIFY2(compared > 120, qPrintable(QString::number(compared)));
    QVERIFY(skipped > 0); // the excluded voltages are in the recordings
    // The wrong answer for a command is not decoded.
    const ManufacturerCommand *rpm = find(elantra().commandsFor(2014), QStringLiteral("7E0:2101"));
    QList<ManufacturerReader::Value> values;
    QVERIFY(!ManufacturerReader::decodeAnswer(*rpm, QByteArray::fromHex("610200"), &values));
    QVERIFY(!ManufacturerReader::decodeAnswer(*rpm, QByteArray::fromHex("7F2131"), &values));
}

void TestManufacturer::decodesFields()
{
    ManufacturerSignal s;
    // Bits across a byte boundary, most significant first.
    s.bit = 4; s.length = 8;
    double v = 0;
    QVERIFY(s.decode(QByteArray::fromHex("ABCD"), &v));
    QCOMPARE(v, double(0xBC));
    // Signed, scaled, offset.
    s.bit = 0; s.length = 8; s.isSigned = true; s.mul = 0.5; s.add = 10;
    QVERIFY(s.decode(QByteArray::fromHex("FE"), &v)); // -2 * 0.5 + 10
    QCOMPARE(v, 9.0);
    // Little-endian 16 bits.
    s = {};
    s.length = 16; s.littleEndian = true;
    QVERIFY(s.decode(QByteArray::fromHex("3412"), &v));
    QCOMPARE(v, double(0x1234));
    // Too short: no value. Outside the null range: no value.
    QVERIFY(!s.decode(QByteArray::fromHex("34"), &v));
    s = {};
    s.hasNullMax = true; s.nullMax = 200;
    QVERIFY(!s.decode(QByteArray::fromHex("FF"), &v));
    QVERIFY(s.decode(QByteArray::fromHex("C8"), &v));
}

void TestManufacturer::units()
{
    ManufacturerSignal s;
    s.unit = QStringLiteral("psi");
    QCOMPARE(QString::fromUtf8(s.metricUnit()), QStringLiteral("kPa"));
    QVERIFY(std::abs(s.toMetric(32) - 220.63) < 0.01);
    s.unit = QStringLiteral("fahrenheit");
    QCOMPARE(s.toMetric(212), 100.0);
    s.unit = QStringLiteral("noyes");
    QVERIFY(s.isState());
    QCOMPARE(s.decimals(), 0);
    s.unit = QStringLiteral("celsius");
    s.mul = 0.75;
    QCOMPARE(s.decimals(), 1);
}

void TestManufacturer::readsTheDemoCar()
{
    ObdClient client;
    auto *sim = new SimulatedTransport;
    sim->setLatencyMs(0);
    client.session()->setTransport(sim);
    client.session()->open();
    QVERIFY(QTest::qWaitFor([&] { return client.session()->state() == ElmSession::State::Ready; }, 5000));

    const QList<ManufacturerCommand> commands = elantra().commandsFor(2014);
    QList<ManufacturerReader::Result> results;
    bool done = false;
    ManufacturerReader::read(&client, commands, [&](const QList<ManufacturerReader::Result> &r) { results = r; done = true; });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 60000));
    QCOMPARE(results.size(), commands.size());

    QHash<QString, ManufacturerReader::Result> byKey;
    for (const auto &r : results)
        byKey.insert(r.key, r);
    // The engine answers its local data, with a running engine's rpm.
    QVERIFY(byKey.value(QStringLiteral("7E0:2101")).answered);
    const ManufacturerCommand *engine = find(commands, QStringLiteral("7E0:2101"));
    bool sawRpm = false;
    for (const auto &v : byKey.value(QStringLiteral("7E0:2101")).values) {
        if (engine->signalList[v.signal].id == QLatin1String("ELANTRA_ENGINE_RPM")) {
            sawRpm = true;
            QVERIFY(v.value > 700 && v.value < 3300);
        }
    }
    QVERIFY(sawRpm);
    QVERIFY(byKey.value(QStringLiteral("7D1:2101")).answered); // wheel speeds
    // Not fitted (no answer) and not supported (refused).
    QVERIFY(!byKey.value(QStringLiteral("7B3:220101")).answered);
    QVERIFY(!byKey.value(QStringLiteral("7B3:220101")).refused);
    QVERIFY(byKey.value(QStringLiteral("7E0:2104")).refused);

    // The standard addressing is back: generic live data still works.
    QList<ObdClient::PidReading> readings;
    done = false;
    client.readPids({0x0C}, [&](const QList<ObdClient::PidReading> &r) { readings = r; done = true; });
    QVERIFY(QTest::qWaitFor([&] { return done; }, 5000));
    QVERIFY(!readings.isEmpty());
}

QTEST_GUILESS_MAIN(TestManufacturer)
#include "tst_manufacturer.moc"
