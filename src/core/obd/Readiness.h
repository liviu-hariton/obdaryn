// SPDX-License-Identifier: GPL-3.0-or-later
//
// Readiness monitors (service 01, PID 01): is the car ready for an emissions test?

#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

namespace obdaryn {

/**
 * @brief One self-test the engine computer runs while driving.
 *
 * "Monitors" are built-in tests (catalyst, O2 sensors, EVAP leak test, ...).
 * After codes are cleared or the battery is disconnected they all reset to
 * "not complete" and need a mix of city / highway driving to run again.
 * Emissions inspections (e.g. the Romanian ITP) typically fail a car whose
 * monitors are not complete.
 */
struct Monitor {
    const char *name = nullptr; ///< English source text; translate in context "Monitor"
    bool continuous = false;    ///< runs all the time (misfire, fuel, components)
    bool available = false;     ///< this car has the monitor
    bool complete = false;      ///< the test has run since codes were cleared
    QString translatedName() const;
};

struct MonitorStatus {
    bool valid = false;
    bool milOn = false;              ///< check-engine light commanded on
    int storedDtcCount = 0;          ///< emission-related stored codes
    bool compressionIgnition = false; ///< diesel monitor set
    QList<Monitor> monitors;         ///< only the monitors the car supports

    int incompleteCount() const;
    /// No MIL and at most one incomplete monitor. Rules vary by country and model
    /// year; this is the common "at most one not ready" rule, shown as guidance only.
    bool likelyReadyForInspection() const;
};

/// Decodes the 4 data bytes (A, B, C, D) of PID 01 or PID 41.
MonitorStatus decodeMonitorStatus(const QByteArray &abcd);

} // namespace obdaryn
