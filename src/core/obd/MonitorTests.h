// SPDX-License-Identifier: GPL-3.0-or-later
//
// On-board monitor test results (OBD-II service 06): the numbers behind the
// readiness monitors, each with the limits the car judges them against.

#pragma once

#include "obd/ResponseParser.h"

#include <QList>
#include <QString>

namespace obdaryn {

/**
 * @brief One test result from service 06 (CAN format, SAE J1979).
 *
 * Every emission monitor (catalyst, oxygen sensors, EVAP, misfire...) runs
 * tests and keeps its latest result next to a minimum and a maximum. A value
 * between them passes. Seeing how close a value sits to its limit tells you a
 * part is wearing out before the check-engine light comes on.
 *
 * On CAN each record is 9 bytes: monitor ID (MID), test ID (TID), unit and
 * scaling ID (UASID), then value, minimum and maximum (2 bytes each).
 */
struct MonitorTest {
    QString ecu;         ///< which module reported it, e.g. "7E8"
    quint8 mid = 0;
    quint8 tid = 0;
    quint8 uasid = 0;
    int rawValue = 0, rawMin = 0, rawMax = 0; ///< signed when uasid >= 0x80

    double value = 0, minimum = 0, maximum = 0; ///< scaled to `unit`
    QString unit;        ///< "mV", "ms", "°C"... empty for plain numbers
    int decimals = 0;
    bool scalingKnown = false; ///< false: shown raw (pass/fail still holds)
    bool hasMin = false, hasMax = false; ///< which limits are used (an open end sits at the range end)

    enum class Verdict { Passed, Close, Failed, NotRun };
    Verdict verdict = Verdict::NotRun;

    QString monitorName() const; ///< translated, e.g. "Catalyst, bank 1"
    QString testName() const;    ///< translated, e.g. "Rich to lean switch time", or "Manufacturer test 0x85"
};

namespace MonitorTestCodec {

/// Decodes a CAN service 06 answer ("46 <MID> <records...>").
QList<MonitorTest> fromResponse(const EcuMessage &message);

/// Applies the unit and scaling table and judges the result (exposed for tests).
void scale(MonitorTest &test);

/// Name of a monitor ID, or an empty string if it's not a standard one.
QString monitorName(quint8 mid);

} // namespace MonitorTestCodec

} // namespace obdaryn
