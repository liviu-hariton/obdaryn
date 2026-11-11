// SPDX-License-Identifier: GPL-3.0-or-later
//
// Metric, US and UK units for display. Everything is stored and computed in
// metric; values are converted only when shown.

#pragma once

#include <QString>

namespace obdaryn::Units {

enum class System {
    Metric, ///< km, km/h, °C, kPa, L, L/100 km
    US,     ///< mi, mph, °F, psi, US gal, mpg (US)
    UK,     ///< mi, mph, °C, psi, imperial gal, mpg (UK)
};

/// The system the app shows (set by the app from the user's setting).
System system();
void setSystem(System system);
/// "metric" / "us" / "uk" ⇄ System, for settings.
System fromKey(const QString &key);
QString key(System system);

/// A value ready to show.
struct Shown {
    double value = 0;
    QString unit;      ///< translated, e.g. "mph"
    int decimals = 0;
};

/**
 * @brief Converts a metric value for display in the current system.
 * @param metricUnit the untranslated metric symbol ("km/h", "°C", "kPa", "km",
 *        "L", "L/h", "L/100 km"); anything else is returned unchanged
 *        (with its unit translated).
 *
 * Consumption is special: L/100 km becomes miles per gallon, which is the
 * inverse (lower L/100 km = higher mpg); 0 stays 0 ("not known").
 */
Shown show(double value, const char *metricUnit, int decimals);

/// The translated unit a metric unit is shown in (for column headers).
QString unitFor(const char *metricUnit);

/// Converts a value back from the current system to metric (for fields the
/// user types in, such as a fuel price per gallon).
double litersFromShownVolume(double shown);

} // namespace obdaryn::Units
