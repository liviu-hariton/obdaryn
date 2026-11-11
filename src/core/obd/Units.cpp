// SPDX-License-Identifier: GPL-3.0-or-later

#include "obd/Units.h"

#include <QCoreApplication>

#include <cstring>

namespace obdaryn::Units {

namespace {

System g_system = System::Metric;

constexpr double kMilesPerKm = 0.621371192;
constexpr double kPsiPerKpa = 0.145037738;
constexpr double kLitersPerUsGallon = 3.785411784;
constexpr double kLitersPerUkGallon = 4.54609;
// mpg = k / (L/100 km)
constexpr double kUsMpgFactor = 235.214583;
constexpr double kUkMpgFactor = 282.480936;

QString unitText(const char *unit) { return QCoreApplication::translate("Unit", unit); }

bool is(const char *a, const char *b) { return std::strcmp(a, b) == 0; }

} // namespace

System system() { return g_system; }
void setSystem(System s) { g_system = s; }

System fromKey(const QString &k)
{
    if (k == QLatin1String("us"))
        return System::US;
    if (k == QLatin1String("uk"))
        return System::UK;
    return System::Metric;
}

QString key(System s)
{
    switch (s) {
    case System::US: return QStringLiteral("us");
    case System::UK: return QStringLiteral("uk");
    case System::Metric: break;
    }
    return QStringLiteral("metric");
}

Shown show(double value, const char *metricUnit, int decimals)
{
    const System s = g_system;
    if (!metricUnit)
        return {value, {}, decimals};
    if (s != System::Metric) {
        const double gallon = s == System::US ? kLitersPerUsGallon : kLitersPerUkGallon;
        if (is(metricUnit, "km/h"))
            return {value * kMilesPerKm, unitText(QT_TRANSLATE_NOOP("Unit", "mph")), decimals};
        if (is(metricUnit, "km"))
            return {value * kMilesPerKm, unitText(QT_TRANSLATE_NOOP("Unit", "mi")), decimals};
        if (is(metricUnit, "kPa"))
            return {value * kPsiPerKpa, unitText(QT_TRANSLATE_NOOP("Unit", "psi")), decimals + 1};
        if (is(metricUnit, "°C") && s == System::US)
            return {value * 9.0 / 5.0 + 32.0, unitText(QT_TRANSLATE_NOOP("Unit", "°F")), decimals};
        if (is(metricUnit, "L"))
            return {value / gallon, unitText(QT_TRANSLATE_NOOP("Unit", "gal")), decimals};
        if (is(metricUnit, "L/h"))
            return {value / gallon, unitText(QT_TRANSLATE_NOOP("Unit", "gal/h")), decimals + 1};
        if (is(metricUnit, "L/100 km"))
            return {value > 0 ? (s == System::US ? kUsMpgFactor : kUkMpgFactor) / value : 0,
                    unitText(QT_TRANSLATE_NOOP("Unit", "mpg")), decimals};
    }
    return {value, metricUnit[0] ? unitText(metricUnit) : QString(), decimals};
}

QString unitFor(const char *metricUnit)
{
    return show(1.0, metricUnit, 0).unit;
}

double litersFromShownVolume(double shown)
{
    switch (g_system) {
    case System::US: return shown * kLitersPerUsGallon;
    case System::UK: return shown * kLitersPerUkGallon;
    case System::Metric: break;
    }
    return shown;
}

} // namespace obdaryn::Units
