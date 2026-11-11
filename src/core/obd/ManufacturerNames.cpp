// SPDX-License-Identifier: GPL-3.0-or-later
//
// Translations for the most common manufacturer value names (OBDb names are
// English). One name covers every model that uses it; numbered variants are
// written once as templates ("Misfires, cylinder %1"). Names not listed here
// are shown as OBDb has them.

#include "obd/ManufacturerData.h"

#include <QCoreApplication>
#include <QRegularExpression>

namespace obdaryn {

namespace {

constexpr const char *kContext = "obdaryn::ManufacturerNames";

// Only listed so lupdate finds them; lookups go through QCoreApplication::translate().
[[maybe_unused]] constexpr const char *kNames[] = {
    // Movement
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Vehicle speed"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Vehicle speed, average"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Vehicle speed, v%1"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Front left wheel speed"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Front right wheel speed"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Rear left wheel speed"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Rear right wheel speed"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Steering wheel angle"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Steering angle"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Brake pressure"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Brake pedal"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Brake switch"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Brake light"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Handbrake engaged?"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Reverse gear selected"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Motor-driven power steering current"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Vehicle traction control switch (raw)"),
    // Doors and seatbelts
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Front left door open?"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Front right door open?"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Rear left door open?"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Rear right door open?"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Trunk open?"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Hood latched?"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Front left door locked?"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Front right door locked?"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Driver seatbelt latched?"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Passenger seatbelt latched?"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Rear left seatbelt latched?"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Rear middle seatbelt latched?"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Rear right seatbelt latched?"),
    // Lights
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Headlight high beam"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Headlight low beam"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Headlight switch"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Hazard switch"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Left turn signal on?"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Right turn signal on?"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Defog light"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Defogging?"),
    // Tyres
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Front left tire pressure"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Front right tire pressure"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Rear left tire pressure"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Rear right tire pressure"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Tire pressure front left"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Tire pressure front right"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Tire pressure rear left"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Tire pressure rear right"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Front left tire temperature"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Front right tire temperature"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Rear left tire temperature"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Rear right tire temperature"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Tire temperature front left"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Tire temperature front right"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Tire temperature rear left"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Tire temperature rear right"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Sensor %1 odometer event %2"),
    // Engine
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Engine RPM"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Commanded engine idle RPM"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Engine coolant temperature"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Engine oil temperature"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Calculated oil temperature"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Intake air temperature"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Intake manifold absolute pressure"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Barometric pressure"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "MAP sensor voltage"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "IAT sensor voltage"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Coolant temperature sensor voltage"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Oxygen sensor %1 bank %2"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Short term fuel % trim, bank %1"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Short term fuel % trim, bank %1 idle"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Commanded evaporative purge"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Commanded throttle actuator"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Throttle position #%1"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Throttle position sensor voltage #%1"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Relative accelerator pedal position"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Requested torque"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Timing advance #%1"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Knock retard cylinder %1, alternate"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Knock retard, cylinder %1"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Knock detected"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Fuel cut"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Intake camshaft position"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Exhaust camshaft position"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "intake camshaft position actual"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "intake camshaft position commanded"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Engine run time, total"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Coolant fan PWM"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Air conditioning pressure"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Overheat protection, active"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Fuel injection timing #%1"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Injection duration cylinder %1"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Direct injection time %1"),
    // Misfires
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Misfires, cylinder %1"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Misfires, current, cylinder %1"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Misfires current, cylinder %1"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Misfires current total"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Misfires harmful for catalyst cylinder %1"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Misfires harmful for catalyst, cylinder %1"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Emission relevant misfires total, cylinder %1"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Emission relevant misfires total all cylinders"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Catalyst damaging misfires total, cylinder %1"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Catalyst damaging misfires total all cylinders"),
    // Transmission
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Automatic transmission fluid temperature"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Current gear"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Commanded gear"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Turbine speed"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Output speed"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Torque converter slip"),
    // Electrical, fuel, cabin
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Battery voltage"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Battery voltage after ignition"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "%1V battery voltage"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Cluster voltage"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Odometer (metric)"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Odometer, metric"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Odometer (imperial)"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Odometer, imperial"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Fuel level (volume)"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Fuel level, volume"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Interior temperature, passenger vent"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Interior temperature, passenger legs vent"),
    // Groups (each segment of an OBDb path)
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Engine"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Battery"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Movement"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Tires"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Cells"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Trips"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Charging"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Lights"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Doors"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Transmission"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Climate"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Control"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Seatbelts"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Fuel"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Misfires"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Orientation"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Electrical"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "ECU"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Modules"),
    QT_TRANSLATE_NOOP("obdaryn::ManufacturerNames", "Unknown"),
};

QString translated(const QString &text)
{
    return QCoreApplication::translate(kContext, text.toUtf8().constData());
}

} // namespace

QString ManufacturerSignal::displayName() const
{
    const QString exact = translated(name);
    if (exact != name)
        return exact;
    // Numbered variants: "Misfires, cylinder 3" looks up "Misfires, cylinder %1".
    static const QRegularExpression digits(QStringLiteral("\\d+"));
    QString pattern;
    QStringList numbers;
    qsizetype last = 0;
    for (auto it = digits.globalMatch(name); it.hasNext();) {
        const QRegularExpressionMatch m = it.next();
        numbers << m.captured();
        pattern += name.mid(last, m.capturedStart() - last) + QLatin1Char('%') + QString::number(numbers.size());
        last = m.capturedEnd();
    }
    if (numbers.isEmpty() || numbers.size() > 9)
        return name;
    pattern += name.mid(last);
    QString t = translated(pattern);
    if (t == pattern)
        return name;
    for (const QString &n : std::as_const(numbers))
        t = t.arg(n);
    return t;
}

QString ManufacturerData::groupName(const QString &path)
{
    QStringList parts;
    for (const QString &segment : path.split(QLatin1Char('.'), Qt::SkipEmptyParts))
        parts << translated(segment);
    return parts.join(QStringLiteral(" · "));
}

} // namespace obdaryn
