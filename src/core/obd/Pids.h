// SPDX-License-Identifier: GPL-3.0-or-later
//
// Standard OBD-II service 01 / 02 parameters (SAE J1979) and how to decode them.

#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

namespace obdaryn {

/**
 * @brief Describes one standard PID (Parameter ID).
 *
 * A PID is a numbered value the engine computer can report, for example
 * 0x0C = engine speed. The car answers with 1–4 raw bytes, traditionally
 * called A, B, C, D, and SAE J1979 defines a formula for each PID, e.g.
 * rpm = (256·A + B) / 4.
 *
 * The formulas here are written from the public J1979 definitions.
 */
struct PidDefinition {
    quint8 pid = 0;
    int dataBytes = 0;          ///< number of raw bytes (A, B, C, D) the car sends
    const char *name = nullptr; ///< English name; translate with translatedName()
    const char *unit = "";      ///< unit symbol; translate with translatedUnit()
    double minimum = 0;         ///< typical lower bound (for gauges)
    double maximum = 100;       ///< typical upper bound (for gauges)
    int decimals = 0;           ///< how many decimals the UI should show
    /// Formula from raw bytes to a number; nullptr for status / bit-field PIDs.
    double (*decode)(const quint8 *d) = nullptr;

    bool isNumeric() const { return decode != nullptr; }
    QString translatedName() const;
    QString translatedUnit() const;
    /// Decodes raw bytes; returns false if there are not enough bytes.
    bool value(const QByteArray &raw, double *out) const;
};

class PidTable
{
public:
    /// All known service 01 PIDs, ordered by PID number.
    static const QList<PidDefinition> &all();
    /// The definition for a PID, or nullptr when Obdaryn doesn't know it.
    static const PidDefinition *find(quint8 pid);
    /// Number of data bytes for a PID, or -1 if unknown. Needed to split
    /// multi-PID answers such as "41 0C 1A F8 0D 32".
    static int dataLength(quint8 pid);
    /// True for 0x00, 0x20, 0x40, ...: the "which PIDs are supported" bitmaps.
    static bool isSupportBitmap(quint8 pid) { return pid % 0x20 == 0; }

    /// Text for status PIDs: 0x03 fuel system, 0x1C OBD standard, 0x51 fuel type.
    static QString describeStatus(quint8 pid, const QByteArray &raw);
};

} // namespace obdaryn
