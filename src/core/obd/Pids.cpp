// SPDX-License-Identifier: GPL-3.0-or-later

#include "obd/Pids.h"

#include <QCoreApplication>
#include <QHash>

namespace obdaryn {

namespace {

// Small helpers so the table below reads like the J1979 formulas.
inline double AB(const quint8 *d) { return d[0] * 256.0 + d[1]; }
inline double CD(const quint8 *d) { return d[2] * 256.0 + d[3]; }
inline double pct(quint8 a) { return a * 100.0 / 255.0; }
inline double trim(quint8 a) { return (a - 128.0) * 100.0 / 128.0; }

// pid, bytes, name, unit, min, max, decimals, formula
// (QT_TRANSLATE_NOOP is spelled out on every line: lupdate does not expand macros.)
const QList<PidDefinition> kPids = {
    {0x01, 4, QT_TRANSLATE_NOOP("Pid", "Monitor status since codes cleared"), "", 0, 0, 0, nullptr},
    {0x02, 2, QT_TRANSLATE_NOOP("Pid", "DTC that caused the freeze frame"), "", 0, 0, 0, nullptr},
    {0x03, 2, QT_TRANSLATE_NOOP("Pid", "Fuel system status"), "", 0, 0, 0, nullptr},
    {0x04, 1, QT_TRANSLATE_NOOP("Pid", "Calculated engine load"), "%", 0, 100, 1, [](const quint8 *d) { return pct(d[0]); }},
    {0x05, 1, QT_TRANSLATE_NOOP("Pid", "Coolant temperature"), "°C", -40, 130, 0, [](const quint8 *d) { return d[0] - 40.0; }},
    {0x06, 1, QT_TRANSLATE_NOOP("Pid", "Short-term fuel trim, bank 1"), "%", -25, 25, 1, [](const quint8 *d) { return trim(d[0]); }},
    {0x07, 1, QT_TRANSLATE_NOOP("Pid", "Long-term fuel trim, bank 1"), "%", -25, 25, 1, [](const quint8 *d) { return trim(d[0]); }},
    {0x08, 1, QT_TRANSLATE_NOOP("Pid", "Short-term fuel trim, bank 2"), "%", -25, 25, 1, [](const quint8 *d) { return trim(d[0]); }},
    {0x09, 1, QT_TRANSLATE_NOOP("Pid", "Long-term fuel trim, bank 2"), "%", -25, 25, 1, [](const quint8 *d) { return trim(d[0]); }},
    {0x0A, 1, QT_TRANSLATE_NOOP("Pid", "Fuel pressure (gauge)"), QT_TRANSLATE_NOOP("Unit", "kPa"), 0, 765, 0, [](const quint8 *d) { return d[0] * 3.0; }},
    {0x0B, 1, QT_TRANSLATE_NOOP("Pid", "Intake manifold pressure"), QT_TRANSLATE_NOOP("Unit", "kPa"), 0, 255, 0, [](const quint8 *d) { return double(d[0]); }},
    {0x0C, 2, QT_TRANSLATE_NOOP("Pid", "Engine speed"), QT_TRANSLATE_NOOP("Unit", "rpm"), 0, 7000, 0, [](const quint8 *d) { return AB(d) / 4.0; }},
    {0x0D, 1, QT_TRANSLATE_NOOP("Pid", "Vehicle speed"), QT_TRANSLATE_NOOP("Unit", "km/h"), 0, 220, 0, [](const quint8 *d) { return double(d[0]); }},
    {0x0E, 1, QT_TRANSLATE_NOOP("Pid", "Ignition timing advance"), "°", -64, 64, 1, [](const quint8 *d) { return d[0] / 2.0 - 64.0; }},
    {0x0F, 1, QT_TRANSLATE_NOOP("Pid", "Intake air temperature"), "°C", -40, 80, 0, [](const quint8 *d) { return d[0] - 40.0; }},
    {0x10, 2, QT_TRANSLATE_NOOP("Pid", "Mass air flow"), QT_TRANSLATE_NOOP("Unit", "g/s"), 0, 300, 2, [](const quint8 *d) { return AB(d) / 100.0; }},
    {0x11, 1, QT_TRANSLATE_NOOP("Pid", "Throttle position"), "%", 0, 100, 1, [](const quint8 *d) { return pct(d[0]); }},
    {0x12, 1, QT_TRANSLATE_NOOP("Pid", "Commanded secondary air status"), "", 0, 0, 0, nullptr},
    {0x13, 1, QT_TRANSLATE_NOOP("Pid", "Oxygen sensors present"), "", 0, 0, 0, nullptr},
    {0x14, 2, QT_TRANSLATE_NOOP("Pid", "O2 sensor voltage, bank 1 sensor 1"), QT_TRANSLATE_NOOP("Unit", "V"), 0, 1.275, 3, [](const quint8 *d) { return d[0] / 200.0; }},
    {0x15, 2, QT_TRANSLATE_NOOP("Pid", "O2 sensor voltage, bank 1 sensor 2"), QT_TRANSLATE_NOOP("Unit", "V"), 0, 1.275, 3, [](const quint8 *d) { return d[0] / 200.0; }},
    {0x16, 2, QT_TRANSLATE_NOOP("Pid", "O2 sensor voltage, bank 1 sensor 3"), QT_TRANSLATE_NOOP("Unit", "V"), 0, 1.275, 3, [](const quint8 *d) { return d[0] / 200.0; }},
    {0x17, 2, QT_TRANSLATE_NOOP("Pid", "O2 sensor voltage, bank 1 sensor 4"), QT_TRANSLATE_NOOP("Unit", "V"), 0, 1.275, 3, [](const quint8 *d) { return d[0] / 200.0; }},
    {0x18, 2, QT_TRANSLATE_NOOP("Pid", "O2 sensor voltage, bank 2 sensor 1"), QT_TRANSLATE_NOOP("Unit", "V"), 0, 1.275, 3, [](const quint8 *d) { return d[0] / 200.0; }},
    {0x19, 2, QT_TRANSLATE_NOOP("Pid", "O2 sensor voltage, bank 2 sensor 2"), QT_TRANSLATE_NOOP("Unit", "V"), 0, 1.275, 3, [](const quint8 *d) { return d[0] / 200.0; }},
    {0x1A, 2, QT_TRANSLATE_NOOP("Pid", "O2 sensor voltage, bank 2 sensor 3"), QT_TRANSLATE_NOOP("Unit", "V"), 0, 1.275, 3, [](const quint8 *d) { return d[0] / 200.0; }},
    {0x1B, 2, QT_TRANSLATE_NOOP("Pid", "O2 sensor voltage, bank 2 sensor 4"), QT_TRANSLATE_NOOP("Unit", "V"), 0, 1.275, 3, [](const quint8 *d) { return d[0] / 200.0; }},
    {0x1C, 1, QT_TRANSLATE_NOOP("Pid", "OBD standard"), "", 0, 0, 0, nullptr},
    {0x1D, 1, QT_TRANSLATE_NOOP("Pid", "Oxygen sensors present (4 banks)"), "", 0, 0, 0, nullptr},
    {0x1E, 1, QT_TRANSLATE_NOOP("Pid", "Auxiliary input status"), "", 0, 0, 0, nullptr},
    {0x1F, 2, QT_TRANSLATE_NOOP("Pid", "Run time since engine start"), QT_TRANSLATE_NOOP("Unit", "s"), 0, 65535, 0, [](const quint8 *d) { return AB(d); }},
    {0x21, 2, QT_TRANSLATE_NOOP("Pid", "Distance with MIL on"), QT_TRANSLATE_NOOP("Unit", "km"), 0, 65535, 0, [](const quint8 *d) { return AB(d); }},
    {0x22, 2, QT_TRANSLATE_NOOP("Pid", "Fuel rail pressure (relative)"), QT_TRANSLATE_NOOP("Unit", "kPa"), 0, 5177, 1, [](const quint8 *d) { return AB(d) * 0.079; }},
    {0x23, 2, QT_TRANSLATE_NOOP("Pid", "Fuel rail pressure (gauge)"), QT_TRANSLATE_NOOP("Unit", "kPa"), 0, 655350, 0, [](const quint8 *d) { return AB(d) * 10.0; }},
    {0x24, 4, QT_TRANSLATE_NOOP("Pid", "O2 sensor 1 equivalence ratio"), "λ", 0, 2, 3, [](const quint8 *d) { return AB(d) * 2.0 / 65536.0; }},
    {0x25, 4, QT_TRANSLATE_NOOP("Pid", "O2 sensor 2 equivalence ratio"), "λ", 0, 2, 3, [](const quint8 *d) { return AB(d) * 2.0 / 65536.0; }},
    {0x26, 4, QT_TRANSLATE_NOOP("Pid", "O2 sensor 3 equivalence ratio"), "λ", 0, 2, 3, [](const quint8 *d) { return AB(d) * 2.0 / 65536.0; }},
    {0x27, 4, QT_TRANSLATE_NOOP("Pid", "O2 sensor 4 equivalence ratio"), "λ", 0, 2, 3, [](const quint8 *d) { return AB(d) * 2.0 / 65536.0; }},
    {0x28, 4, QT_TRANSLATE_NOOP("Pid", "O2 sensor 5 equivalence ratio"), "λ", 0, 2, 3, [](const quint8 *d) { return AB(d) * 2.0 / 65536.0; }},
    {0x29, 4, QT_TRANSLATE_NOOP("Pid", "O2 sensor 6 equivalence ratio"), "λ", 0, 2, 3, [](const quint8 *d) { return AB(d) * 2.0 / 65536.0; }},
    {0x2A, 4, QT_TRANSLATE_NOOP("Pid", "O2 sensor 7 equivalence ratio"), "λ", 0, 2, 3, [](const quint8 *d) { return AB(d) * 2.0 / 65536.0; }},
    {0x2B, 4, QT_TRANSLATE_NOOP("Pid", "O2 sensor 8 equivalence ratio"), "λ", 0, 2, 3, [](const quint8 *d) { return AB(d) * 2.0 / 65536.0; }},
    {0x2C, 1, QT_TRANSLATE_NOOP("Pid", "Commanded EGR"), "%", 0, 100, 1, [](const quint8 *d) { return pct(d[0]); }},
    {0x2D, 1, QT_TRANSLATE_NOOP("Pid", "EGR error"), "%", -100, 100, 1, [](const quint8 *d) { return trim(d[0]); }},
    {0x2E, 1, QT_TRANSLATE_NOOP("Pid", "Commanded evaporative purge"), "%", 0, 100, 1, [](const quint8 *d) { return pct(d[0]); }},
    {0x2F, 1, QT_TRANSLATE_NOOP("Pid", "Fuel tank level"), "%", 0, 100, 1, [](const quint8 *d) { return pct(d[0]); }},
    {0x30, 1, QT_TRANSLATE_NOOP("Pid", "Warm-ups since codes cleared"), "", 0, 255, 0, [](const quint8 *d) { return double(d[0]); }},
    {0x31, 2, QT_TRANSLATE_NOOP("Pid", "Distance since codes cleared"), QT_TRANSLATE_NOOP("Unit", "km"), 0, 65535, 0, [](const quint8 *d) { return AB(d); }},
    {0x32, 2, QT_TRANSLATE_NOOP("Pid", "Evap system vapor pressure"), QT_TRANSLATE_NOOP("Unit", "Pa"), -8192, 8192, 2,
     [](const quint8 *d) { return double(qint16(quint16(d[0] << 8 | d[1]))) / 4.0; }},
    {0x33, 1, QT_TRANSLATE_NOOP("Pid", "Barometric pressure"), QT_TRANSLATE_NOOP("Unit", "kPa"), 0, 255, 0, [](const quint8 *d) { return double(d[0]); }},
    {0x34, 4, QT_TRANSLATE_NOOP("Pid", "O2 sensor 1 current"), QT_TRANSLATE_NOOP("Unit", "mA"), -128, 128, 2, [](const quint8 *d) { return CD(d) / 256.0 - 128.0; }},
    {0x35, 4, QT_TRANSLATE_NOOP("Pid", "O2 sensor 2 current"), QT_TRANSLATE_NOOP("Unit", "mA"), -128, 128, 2, [](const quint8 *d) { return CD(d) / 256.0 - 128.0; }},
    {0x36, 4, QT_TRANSLATE_NOOP("Pid", "O2 sensor 3 current"), QT_TRANSLATE_NOOP("Unit", "mA"), -128, 128, 2, [](const quint8 *d) { return CD(d) / 256.0 - 128.0; }},
    {0x37, 4, QT_TRANSLATE_NOOP("Pid", "O2 sensor 4 current"), QT_TRANSLATE_NOOP("Unit", "mA"), -128, 128, 2, [](const quint8 *d) { return CD(d) / 256.0 - 128.0; }},
    {0x38, 4, QT_TRANSLATE_NOOP("Pid", "O2 sensor 5 current"), QT_TRANSLATE_NOOP("Unit", "mA"), -128, 128, 2, [](const quint8 *d) { return CD(d) / 256.0 - 128.0; }},
    {0x39, 4, QT_TRANSLATE_NOOP("Pid", "O2 sensor 6 current"), QT_TRANSLATE_NOOP("Unit", "mA"), -128, 128, 2, [](const quint8 *d) { return CD(d) / 256.0 - 128.0; }},
    {0x3A, 4, QT_TRANSLATE_NOOP("Pid", "O2 sensor 7 current"), QT_TRANSLATE_NOOP("Unit", "mA"), -128, 128, 2, [](const quint8 *d) { return CD(d) / 256.0 - 128.0; }},
    {0x3B, 4, QT_TRANSLATE_NOOP("Pid", "O2 sensor 8 current"), QT_TRANSLATE_NOOP("Unit", "mA"), -128, 128, 2, [](const quint8 *d) { return CD(d) / 256.0 - 128.0; }},
    {0x3C, 2, QT_TRANSLATE_NOOP("Pid", "Catalyst temperature, bank 1 sensor 1"), "°C", -40, 1000, 0, [](const quint8 *d) { return AB(d) / 10.0 - 40.0; }},
    {0x3D, 2, QT_TRANSLATE_NOOP("Pid", "Catalyst temperature, bank 2 sensor 1"), "°C", -40, 1000, 0, [](const quint8 *d) { return AB(d) / 10.0 - 40.0; }},
    {0x3E, 2, QT_TRANSLATE_NOOP("Pid", "Catalyst temperature, bank 1 sensor 2"), "°C", -40, 1000, 0, [](const quint8 *d) { return AB(d) / 10.0 - 40.0; }},
    {0x3F, 2, QT_TRANSLATE_NOOP("Pid", "Catalyst temperature, bank 2 sensor 2"), "°C", -40, 1000, 0, [](const quint8 *d) { return AB(d) / 10.0 - 40.0; }},
    {0x41, 4, QT_TRANSLATE_NOOP("Pid", "Monitor status this drive cycle"), "", 0, 0, 0, nullptr},
    {0x42, 2, QT_TRANSLATE_NOOP("Pid", "Control module voltage"), QT_TRANSLATE_NOOP("Unit", "V"), 0, 16, 2, [](const quint8 *d) { return AB(d) / 1000.0; }},
    {0x43, 2, QT_TRANSLATE_NOOP("Pid", "Absolute load"), "%", 0, 400, 1, [](const quint8 *d) { return AB(d) * 100.0 / 255.0; }},
    {0x44, 2, QT_TRANSLATE_NOOP("Pid", "Commanded equivalence ratio"), "λ", 0, 2, 3, [](const quint8 *d) { return AB(d) * 2.0 / 65536.0; }},
    {0x45, 1, QT_TRANSLATE_NOOP("Pid", "Relative throttle position"), "%", 0, 100, 1, [](const quint8 *d) { return pct(d[0]); }},
    {0x46, 1, QT_TRANSLATE_NOOP("Pid", "Ambient air temperature"), "°C", -40, 60, 0, [](const quint8 *d) { return d[0] - 40.0; }},
    {0x47, 1, QT_TRANSLATE_NOOP("Pid", "Absolute throttle position B"), "%", 0, 100, 1, [](const quint8 *d) { return pct(d[0]); }},
    {0x48, 1, QT_TRANSLATE_NOOP("Pid", "Absolute throttle position C"), "%", 0, 100, 1, [](const quint8 *d) { return pct(d[0]); }},
    {0x49, 1, QT_TRANSLATE_NOOP("Pid", "Accelerator pedal position D"), "%", 0, 100, 1, [](const quint8 *d) { return pct(d[0]); }},
    {0x4A, 1, QT_TRANSLATE_NOOP("Pid", "Accelerator pedal position E"), "%", 0, 100, 1, [](const quint8 *d) { return pct(d[0]); }},
    {0x4B, 1, QT_TRANSLATE_NOOP("Pid", "Accelerator pedal position F"), "%", 0, 100, 1, [](const quint8 *d) { return pct(d[0]); }},
    {0x4C, 1, QT_TRANSLATE_NOOP("Pid", "Commanded throttle actuator"), "%", 0, 100, 1, [](const quint8 *d) { return pct(d[0]); }},
    {0x4D, 2, QT_TRANSLATE_NOOP("Pid", "Time run with MIL on"), QT_TRANSLATE_NOOP("Unit", "min"), 0, 65535, 0, [](const quint8 *d) { return AB(d); }},
    {0x4E, 2, QT_TRANSLATE_NOOP("Pid", "Time since codes cleared"), QT_TRANSLATE_NOOP("Unit", "min"), 0, 65535, 0, [](const quint8 *d) { return AB(d); }},
    {0x51, 1, QT_TRANSLATE_NOOP("Pid", "Fuel type"), "", 0, 0, 0, nullptr},
    {0x52, 1, QT_TRANSLATE_NOOP("Pid", "Ethanol fuel"), "%", 0, 100, 1, [](const quint8 *d) { return pct(d[0]); }},
    {0x59, 2, QT_TRANSLATE_NOOP("Pid", "Fuel rail absolute pressure"), QT_TRANSLATE_NOOP("Unit", "kPa"), 0, 655350, 0, [](const quint8 *d) { return AB(d) * 10.0; }},
    {0x5A, 1, QT_TRANSLATE_NOOP("Pid", "Relative accelerator pedal position"), "%", 0, 100, 1, [](const quint8 *d) { return pct(d[0]); }},
    {0x5B, 1, QT_TRANSLATE_NOOP("Pid", "Hybrid battery remaining life"), "%", 0, 100, 1, [](const quint8 *d) { return pct(d[0]); }},
    {0x5C, 1, QT_TRANSLATE_NOOP("Pid", "Engine oil temperature"), "°C", -40, 160, 0, [](const quint8 *d) { return d[0] - 40.0; }},
    {0x5D, 2, QT_TRANSLATE_NOOP("Pid", "Fuel injection timing"), "°", -210, 302, 2, [](const quint8 *d) { return AB(d) / 128.0 - 210.0; }},
    {0x5E, 2, QT_TRANSLATE_NOOP("Pid", "Engine fuel rate"), QT_TRANSLATE_NOOP("Unit", "L/h"), 0, 3277, 2, [](const quint8 *d) { return AB(d) / 20.0; }},
    {0x61, 1, QT_TRANSLATE_NOOP("Pid", "Driver's demand engine torque"), "%", -125, 130, 0, [](const quint8 *d) { return d[0] - 125.0; }},
    {0x62, 1, QT_TRANSLATE_NOOP("Pid", "Actual engine torque"), "%", -125, 130, 0, [](const quint8 *d) { return d[0] - 125.0; }},
    {0x63, 2, QT_TRANSLATE_NOOP("Pid", "Engine reference torque"), QT_TRANSLATE_NOOP("Unit", "Nm"), 0, 65535, 0, [](const quint8 *d) { return AB(d); }},
    {0xA6, 4, QT_TRANSLATE_NOOP("Pid", "Odometer"), QT_TRANSLATE_NOOP("Unit", "km"), 0, 429496729, 1,
     [](const quint8 *d) { return (double(d[0]) * 16777216.0 + d[1] * 65536.0 + d[2] * 256.0 + d[3]) / 10.0; }},
};

const QHash<quint8, int> &indexByPid()
{
    static const QHash<quint8, int> index = [] {
        QHash<quint8, int> h;
        for (int i = 0; i < kPids.size(); ++i)
            h.insert(kPids[i].pid, i);
        return h;
    }();
    return index;
}

} // namespace

QString PidDefinition::translatedName() const
{
    return QCoreApplication::translate("Pid", name);
}

QString PidDefinition::translatedUnit() const
{
    return QCoreApplication::translate("Unit", unit);
}

bool PidDefinition::value(const QByteArray &raw, double *out) const
{
    if (!decode || raw.size() < dataBytes)
        return false;
    *out = decode(reinterpret_cast<const quint8 *>(raw.constData()));
    return true;
}

const QList<PidDefinition> &PidTable::all()
{
    return kPids;
}

const PidDefinition *PidTable::find(quint8 pid)
{
    const auto it = indexByPid().constFind(pid);
    return it == indexByPid().constEnd() ? nullptr : &kPids[*it];
}

int PidTable::dataLength(quint8 pid)
{
    if (isSupportBitmap(pid))
        return 4;
    const PidDefinition *def = find(pid);
    return def ? def->dataBytes : -1;
}

QString PidTable::describeStatus(quint8 pid, const QByteArray &raw)
{
    if (raw.isEmpty())
        return {};
    const quint8 a = quint8(raw[0]);
    auto t = [](const char *s) { return QCoreApplication::translate("PidStatus", s); };

    switch (pid) {
    case 0x03: // fuel system status (bank 1 in A)
        switch (a) {
        case 0x00: return t(QT_TRANSLATE_NOOP("PidStatus", "Engine off"));
        case 0x01: return t(QT_TRANSLATE_NOOP("PidStatus", "Open loop (engine warming up)"));
        case 0x02: return t(QT_TRANSLATE_NOOP("PidStatus", "Closed loop (using O2 sensors)"));
        case 0x04: return t(QT_TRANSLATE_NOOP("PidStatus", "Open loop (engine load or fuel cut)"));
        case 0x08: return t(QT_TRANSLATE_NOOP("PidStatus", "Open loop (system fault)"));
        case 0x10: return t(QT_TRANSLATE_NOOP("PidStatus", "Closed loop (feedback fault)"));
        default:   return QStringLiteral("0x%1").arg(a, 2, 16, QLatin1Char('0'));
        }
    case 0x1C: { // which OBD standard the car was certified to
        switch (a) {
        case 1:  return QStringLiteral("OBD-II (CARB)");
        case 2:  return QStringLiteral("OBD (EPA)");
        case 3:  return QStringLiteral("OBD + OBD-II");
        case 4:  return QStringLiteral("OBD-I");
        case 5:  return t(QT_TRANSLATE_NOOP("PidStatus", "Not OBD compliant"));
        case 6:  return QStringLiteral("EOBD (Europe)");
        case 7:  return QStringLiteral("EOBD + OBD-II");
        case 8:  return QStringLiteral("EOBD + OBD");
        case 9:  return QStringLiteral("EOBD + OBD + OBD-II");
        case 10: return QStringLiteral("JOBD (Japan)");
        case 11: return QStringLiteral("JOBD + OBD-II");
        case 12: return QStringLiteral("JOBD + EOBD");
        case 13: return QStringLiteral("JOBD + EOBD + OBD-II");
        case 17: return QStringLiteral("EMD");
        case 18: return QStringLiteral("EMD+");
        case 19: return QStringLiteral("HD OBD-C");
        case 20: return QStringLiteral("HD OBD");
        case 21: return QStringLiteral("WWH OBD");
        case 23: return QStringLiteral("HD EOBD-I");
        case 24: return QStringLiteral("HD EOBD-I N");
        case 25: return QStringLiteral("HD EOBD-II");
        case 26: return QStringLiteral("HD EOBD-II N");
        case 28: return QStringLiteral("OBDBr-1 (Brazil)");
        case 29: return QStringLiteral("OBDBr-2 (Brazil)");
        case 30: return QStringLiteral("KOBD (Korea)");
        case 31: return QStringLiteral("IOBD I (India)");
        case 32: return QStringLiteral("IOBD II (India)");
        case 33: return QStringLiteral("HD EOBD-VI");
        default: return QStringLiteral("%1").arg(a);
        }
    }
    case 0x51: { // fuel type
        static const char *types[] = {
            QT_TRANSLATE_NOOP("PidStatus", "Not available"),
            QT_TRANSLATE_NOOP("PidStatus", "Gasoline"),
            QT_TRANSLATE_NOOP("PidStatus", "Methanol"),
            QT_TRANSLATE_NOOP("PidStatus", "Ethanol"),
            QT_TRANSLATE_NOOP("PidStatus", "Diesel"),
            QT_TRANSLATE_NOOP("PidStatus", "LPG"),
            QT_TRANSLATE_NOOP("PidStatus", "CNG"),
            QT_TRANSLATE_NOOP("PidStatus", "Propane"),
            QT_TRANSLATE_NOOP("PidStatus", "Electric"),
        };
        if (a < sizeof(types) / sizeof(types[0]))
            return t(types[a]);
        return t(QT_TRANSLATE_NOOP("PidStatus", "Hybrid / bi-fuel"));
    }
    default:
        return QString::fromLatin1(raw.toHex(' ').toUpper());
    }
}

} // namespace obdaryn
