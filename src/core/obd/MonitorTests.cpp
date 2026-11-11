// SPDX-License-Identifier: GPL-3.0-or-later

#include "obd/MonitorTests.h"

#include <QCoreApplication>

#include <cmath>

namespace obdaryn {

namespace {

QString t(const char *text) { return QCoreApplication::translate("MonitorTests", text); }

/// Unit and scaling of a UASID (SAE J1979 Appendix E): physical = raw × factor + offset.
struct Scaling {
    quint8 uasid;
    double factor;
    double offset;
    const char *unit; ///< translated in context "Unit"
    int decimals;
};

// The entries Obdaryn knows. IDs 0x01–0x7F are unsigned, 0x80–0xFE signed.
// A test with an unknown ID is shown as a raw number: the pass/fail verdict
// doesn't depend on the unit, because the value and its limits share it.
const Scaling kScalings[] = {
    {0x01, 1, 0, "", 0},
    {0x02, 0.1, 0, "", 1},
    {0x03, 0.01, 0, "", 2},
    {0x04, 0.001, 0, "", 3},
    {0x05, 0.0000305, 0, "", 5},
    {0x06, 0.000305, 0, "", 4},
    {0x07, 0.25, 0, QT_TRANSLATE_NOOP("Unit", "rpm"), 0},
    {0x08, 0.01, 0, QT_TRANSLATE_NOOP("Unit", "km/h"), 2},
    {0x09, 1, 0, QT_TRANSLATE_NOOP("Unit", "km/h"), 0},
    {0x0A, 0.122, 0, QT_TRANSLATE_NOOP("Unit", "mV"), 0},
    {0x0B, 0.001, 0, QT_TRANSLATE_NOOP("Unit", "V"), 3},
    {0x0C, 0.01, 0, QT_TRANSLATE_NOOP("Unit", "V"), 2},
    {0x0D, 0.00390625, 0, QT_TRANSLATE_NOOP("Unit", "mA"), 3},
    {0x0E, 0.001, 0, QT_TRANSLATE_NOOP("Unit", "A"), 3},
    {0x0F, 0.01, 0, QT_TRANSLATE_NOOP("Unit", "A"), 2},
    {0x10, 1, 0, QT_TRANSLATE_NOOP("Unit", "ms"), 0},
    {0x11, 100, 0, QT_TRANSLATE_NOOP("Unit", "ms"), 0},
    {0x12, 1, 0, QT_TRANSLATE_NOOP("Unit", "s"), 0},
    {0x13, 1, 0, QT_TRANSLATE_NOOP("Unit", "mΩ"), 0},
    {0x14, 1, 0, QT_TRANSLATE_NOOP("Unit", "Ω"), 0},
    {0x15, 1, 0, QT_TRANSLATE_NOOP("Unit", "kΩ"), 0},
    {0x16, 0.1, -40, QT_TRANSLATE_NOOP("Unit", "°C"), 1},
    {0x17, 0.01, 0, QT_TRANSLATE_NOOP("Unit", "kPa"), 2},
    {0x1A, 1, 0, QT_TRANSLATE_NOOP("Unit", "kPa"), 0},
    {0x1B, 10, 0, QT_TRANSLATE_NOOP("Unit", "kPa"), 0},
    {0x1C, 0.01, 0, QT_TRANSLATE_NOOP("Unit", "°"), 2},
    {0x1D, 0.5, 0, QT_TRANSLATE_NOOP("Unit", "°"), 1},
    {0x1E, 0.0000305, 0, "λ", 3},
    {0x22, 1, 0, QT_TRANSLATE_NOOP("Unit", "Hz"), 0},
    {0x24, 1, 0, QT_TRANSLATE_NOOP("Unit", "counts"), 0},
    {0x25, 1, 0, QT_TRANSLATE_NOOP("Unit", "km"), 0},
    {0x27, 0.01, 0, QT_TRANSLATE_NOOP("Unit", "g/s"), 2},
    {0x28, 1, 0, QT_TRANSLATE_NOOP("Unit", "g/s"), 0},
    {0x2F, 0.01, 0, QT_TRANSLATE_NOOP("Unit", "%"), 2},
    {0x34, 1, 0, QT_TRANSLATE_NOOP("Unit", "min"), 0},
    {0x35, 10, 0, QT_TRANSLATE_NOOP("Unit", "ms"), 0},
    {0x81, 1, 0, "", 0},
    {0x82, 0.1, 0, "", 1},
    {0x83, 0.01, 0, "", 2},
    {0x84, 0.001, 0, "", 3},
    {0x85, 0.0000305, 0, "", 5},
    {0x86, 0.000305, 0, "", 4},
    {0x8A, 0.122, 0, QT_TRANSLATE_NOOP("Unit", "mV"), 0},
    {0x8B, 0.001, 0, QT_TRANSLATE_NOOP("Unit", "V"), 3},
    {0x8C, 0.01, 0, QT_TRANSLATE_NOOP("Unit", "V"), 2},
    {0x8D, 0.00390625, 0, QT_TRANSLATE_NOOP("Unit", "mA"), 3},
    {0x8E, 0.001, 0, QT_TRANSLATE_NOOP("Unit", "A"), 3},
    {0x90, 1, 0, QT_TRANSLATE_NOOP("Unit", "ms"), 0},
    {0x96, 0.1, 0, QT_TRANSLATE_NOOP("Unit", "°C"), 1},
    {0x9C, 0.01, 0, QT_TRANSLATE_NOOP("Unit", "°"), 2},
    {0x9D, 0.5, 0, QT_TRANSLATE_NOOP("Unit", "°"), 1},
    {0xAF, 0.01, 0, QT_TRANSLATE_NOOP("Unit", "%"), 2},
};

const Scaling *findScaling(quint8 uasid)
{
    for (const Scaling &s : kScalings) {
        if (s.uasid == uasid)
            return &s;
    }
    return nullptr;
}

/// Bank / sensor naming shared by oxygen sensors and their heaters.
QString bankSensor(int index)
{
    return QCoreApplication::translate("MonitorTests", "bank %1 sensor %2").arg(index / 4 + 1).arg(index % 4 + 1);
}

QString bank(int n) { return QCoreApplication::translate("MonitorTests", "bank %1").arg(n); }

} // namespace

QString MonitorTestCodec::monitorName(quint8 mid)
{
    auto join = [](const char *name, const QString &where) { return QStringLiteral("%1, %2").arg(t(name), where); };
    if (mid >= 0x01 && mid <= 0x10)
        return join(QT_TRANSLATE_NOOP("MonitorTests", "Oxygen sensor"), bankSensor(mid - 0x01));
    if (mid >= 0x21 && mid <= 0x24)
        return join(QT_TRANSLATE_NOOP("MonitorTests", "Catalyst"), bank(mid - 0x20));
    if (mid >= 0x31 && mid <= 0x34)
        return join(QT_TRANSLATE_NOOP("MonitorTests", "EGR system"), bank(mid - 0x30));
    if (mid >= 0x35 && mid <= 0x38)
        return join(QT_TRANSLATE_NOOP("MonitorTests", "Variable valve timing"), bank(mid - 0x34));
    switch (mid) {
    case 0x39: return t(QT_TRANSLATE_NOOP("MonitorTests", "EVAP system, cap off or large leak (0.150\")"));
    case 0x3A: return t(QT_TRANSLATE_NOOP("MonitorTests", "EVAP system, 0.090\" leak"));
    case 0x3B: return t(QT_TRANSLATE_NOOP("MonitorTests", "EVAP system, 0.040\" leak"));
    case 0x3C: return t(QT_TRANSLATE_NOOP("MonitorTests", "EVAP system, 0.020\" leak"));
    case 0x3D: return t(QT_TRANSLATE_NOOP("MonitorTests", "EVAP purge flow"));
    case 0xA1: return t(QT_TRANSLATE_NOOP("MonitorTests", "Misfire, all cylinders"));
    default: break;
    }
    if (mid >= 0x41 && mid <= 0x50)
        return join(QT_TRANSLATE_NOOP("MonitorTests", "Oxygen sensor heater"), bankSensor(mid - 0x41));
    if (mid >= 0x61 && mid <= 0x64)
        return join(QT_TRANSLATE_NOOP("MonitorTests", "Heated catalyst"), bank(mid - 0x60));
    if (mid >= 0x71 && mid <= 0x74)
        return QCoreApplication::translate("MonitorTests", "Secondary air system %1").arg(mid - 0x70);
    if (mid >= 0x81 && mid <= 0x84)
        return join(QT_TRANSLATE_NOOP("MonitorTests", "Fuel system"), bank(mid - 0x80));
    if (mid >= 0x85 && mid <= 0x86)
        return join(QT_TRANSLATE_NOOP("MonitorTests", "Boost pressure control"), bank(mid - 0x84));
    if (mid >= 0x90 && mid <= 0x91)
        return join(QT_TRANSLATE_NOOP("MonitorTests", "NOx absorber"), bank(mid - 0x8F));
    if (mid >= 0x98 && mid <= 0x99)
        return join(QT_TRANSLATE_NOOP("MonitorTests", "NOx catalyst"), bank(mid - 0x97));
    if (mid >= 0xA2 && mid <= 0xAD)
        return QCoreApplication::translate("MonitorTests", "Misfire, cylinder %1").arg(mid - 0xA1);
    if (mid >= 0xB0 && mid <= 0xB1)
        return join(QT_TRANSLATE_NOOP("MonitorTests", "Particulate filter"), bank(mid - 0xAF));
    return {};
}

QString MonitorTest::monitorName() const
{
    const QString name = MonitorTestCodec::monitorName(mid);
    return name.isEmpty() ? QCoreApplication::translate("MonitorTests", "Monitor 0x%1")
                                .arg(QString::number(mid, 16).toUpper().rightJustified(2, QLatin1Char('0')))
                          : name;
}

QString MonitorTest::testName() const
{
    switch (tid) {
    case 0x01: return t(QT_TRANSLATE_NOOP("MonitorTests", "Rich to lean threshold voltage"));
    case 0x02: return t(QT_TRANSLATE_NOOP("MonitorTests", "Lean to rich threshold voltage"));
    case 0x03: return t(QT_TRANSLATE_NOOP("MonitorTests", "Low voltage for switch time"));
    case 0x04: return t(QT_TRANSLATE_NOOP("MonitorTests", "High voltage for switch time"));
    case 0x05: return t(QT_TRANSLATE_NOOP("MonitorTests", "Rich to lean switch time"));
    case 0x06: return t(QT_TRANSLATE_NOOP("MonitorTests", "Lean to rich switch time"));
    case 0x07: return t(QT_TRANSLATE_NOOP("MonitorTests", "Minimum sensor voltage"));
    case 0x08: return t(QT_TRANSLATE_NOOP("MonitorTests", "Maximum sensor voltage"));
    case 0x09: return t(QT_TRANSLATE_NOOP("MonitorTests", "Time between transitions"));
    case 0x0A: return t(QT_TRANSLATE_NOOP("MonitorTests", "Sensor period"));
    case 0x0B: return t(QT_TRANSLATE_NOOP("MonitorTests", "Misfires, average of the last 10 drives"));
    case 0x0C: return t(QT_TRANSLATE_NOOP("MonitorTests", "Misfires, last or current drive"));
    default: break;
    }
    // Test IDs from 0x80 are defined by each manufacturer.
    return t(QT_TRANSLATE_NOOP("MonitorTests", "Manufacturer-specific test"));
}

void MonitorTestCodec::scale(MonitorTest &test)
{
    const Scaling *s = findScaling(test.uasid);
    test.scalingKnown = s != nullptr;
    const double factor = s ? s->factor : 1;
    const double offset = s ? s->offset : 0;
    test.unit = s && s->unit[0] ? QCoreApplication::translate("Unit", s->unit) : QString();
    test.decimals = s ? s->decimals : 0;
    test.value = test.rawValue * factor + offset;
    test.minimum = test.rawMin * factor + offset;
    test.maximum = test.rawMax * factor + offset;

    // An open end sits at the end of the range (0 / 0xFFFF unsigned, -0x8000 / 0x7FFF signed).
    const bool isSigned = test.uasid >= 0x80;
    test.hasMin = isSigned ? test.rawMin != -0x8000 : test.rawMin != 0;
    test.hasMax = isSigned ? test.rawMax != 0x7FFF : test.rawMax != 0xFFFF;

    // Judge on the raw numbers: same scale for value and limits.
    if (test.rawValue == 0 && test.rawMin == 0 && test.rawMax == 0) {
        test.verdict = MonitorTest::Verdict::NotRun; // the car resets results to zero until the test runs
        return;
    }
    if (test.rawValue < test.rawMin || test.rawValue > test.rawMax) {
        test.verdict = MonitorTest::Verdict::Failed;
        return;
    }
    // "Close": within 10 % of a limit that is really used.
    const bool hasMin = test.hasMin, hasMax = test.hasMax;
    double margin = 1.0;
    if (hasMin && hasMax && test.rawMax > test.rawMin)
        margin = double(std::min(test.rawValue - test.rawMin, test.rawMax - test.rawValue)) / (test.rawMax - test.rawMin);
    else if (hasMax && test.rawMax != 0)
        margin = double(test.rawMax - test.rawValue) / std::abs(test.rawMax);
    else if (hasMin && test.rawMin != 0)
        margin = double(test.rawValue - test.rawMin) / std::abs(test.rawMin);
    test.verdict = margin <= 0.1 ? MonitorTest::Verdict::Close : MonitorTest::Verdict::Passed;
}

QList<MonitorTest> MonitorTestCodec::fromResponse(const EcuMessage &message)
{
    QList<MonitorTest> tests;
    const QByteArray &d = message.data;
    if (d.isEmpty() || quint8(d[0]) != 0x46)
        return tests;
    auto u16 = [&d](qsizetype i) { return (quint8(d[i]) << 8) | quint8(d[i + 1]); };
    auto s16 = [&u16](qsizetype i) { return int(qint16(quint16(u16(i)))); };
    // Records start right after the 0x46: MID TID UASID VAL VAL MIN MIN MAX MAX.
    for (qsizetype i = 1; i + 9 <= d.size(); i += 9) {
        MonitorTest test;
        test.ecu = message.ecu;
        test.mid = quint8(d[i]);
        test.tid = quint8(d[i + 1]);
        test.uasid = quint8(d[i + 2]);
        const bool isSigned = test.uasid >= 0x80;
        test.rawValue = isSigned ? s16(i + 3) : u16(i + 3);
        test.rawMin = isSigned ? s16(i + 5) : u16(i + 5);
        test.rawMax = isSigned ? s16(i + 7) : u16(i + 7);
        scale(test);
        tests << test;
    }
    return tests;
}

} // namespace obdaryn
