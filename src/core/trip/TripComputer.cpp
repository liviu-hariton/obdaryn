// SPDX-License-Identifier: GPL-3.0-or-later

#include "trip/TripComputer.h"

#include <algorithm>
#include <cmath>

namespace obdaryn {

namespace {

// PIDs used here (SAE J1979 service 01).
constexpr quint8 kFuelStatus = 0x03, kLoad = 0x04, kMap = 0x0B, kRpm = 0x0C, kSpeed = 0x0D,
                 kIntakeTemp = 0x0F, kMaf = 0x10, kLambda = 0x44, kFuelRate = 0x5E;

constexpr double kPetrolAfr = 14.7;        ///< stoichiometric air-fuel ratio, g air per g fuel
constexpr double kPetrolDensity = 745.0;   ///< g/L (EN 228 petrol: 720–775)
constexpr double kAirGasConstant = 287.05; ///< J/(kg·K), dry air
constexpr double kVolumetricEfficiency = 0.85; ///< typical for a modern petrol engine; calibration corrects it

} // namespace

QSet<quint8> TripComputer::inputPids(const QSet<quint8> &supported, bool diesel)
{
    QSet<quint8> wanted{kSpeed, kRpm};
    if (supported.contains(kFuelRate))
        wanted << kFuelRate;
    else if (!diesel && supported.contains(kMaf))
        wanted << kMaf << kLambda << kFuelStatus << kLoad;
    else if (!diesel)
        wanted << kMap << kIntakeTemp << kLambda << kFuelStatus << kLoad;
    return wanted & supported;
}

TripComputer::FuelMethod TripComputer::methodFor(const QSet<quint8> &supported, const Settings &settings)
{
    if (supported.contains(kFuelRate))
        return FuelMethod::FuelRate;
    if (settings.diesel)
        return FuelMethod::None;
    if (supported.contains(kMaf))
        return FuelMethod::MassAirFlow;
    if (supported.contains(kMap) && supported.contains(kRpm) && supported.contains(kIntakeTemp))
        return settings.engineLiters > 0 ? FuelMethod::SpeedDensity : FuelMethod::NeedsEngineSize;
    return FuelMethod::None;
}

void TripComputer::setSettings(const Settings &settings)
{
    m_settings = settings;
    m_method = methodFor(m_supported, m_settings);
}

void TripComputer::setSupported(const QSet<quint8> &supported)
{
    m_supported = supported;
    m_method = methodFor(m_supported, m_settings);
}

void TripComputer::reset()
{
    m_stats = {};
    m_lastMs = -1;
    m_lastSpeed = 0;
    m_lastFlow = 0;
}

bool TripComputer::fuelCut() const
{
    // Status 0x04 means "open loop due to driving conditions": either full
    // load enrichment or deceleration fuel cut. Low load tells them apart.
    if (!has(kFuelStatus) || !has(kLoad) || !has(kRpm))
        return false;
    return (int(value(kFuelStatus)) & 0x04) && value(kLoad) < 20.0 && value(kRpm) > 1000;
}

double TripComputer::fuelFlowLph() const
{
    if (has(kRpm) && value(kRpm) <= 0)
        return 0; // engine off
    switch (m_method) {
    case FuelMethod::FuelRate:
        return has(kFuelRate) ? value(kFuelRate) : -1;
    case FuelMethod::MassAirFlow:
    case FuelMethod::SpeedDensity: {
        if (fuelCut())
            return 0;
        double airGs = 0;
        if (m_method == FuelMethod::MassAirFlow) {
            if (!has(kMaf))
                return -1;
            airGs = value(kMaf);
        } else {
            if (!has(kMap) || !has(kIntakeTemp) || !has(kRpm))
                return -1;
            // Air density in the manifold (ideal gas), times the volume the
            // engine draws: half its displacement per revolution (4-stroke).
            const double densityKgM3 = value(kMap) * 1000.0 / (kAirGasConstant * (value(kIntakeTemp) + 273.15));
            const double m3PerSecond = value(kRpm) / 120.0 * m_settings.engineLiters / 1000.0 * kVolumetricEfficiency;
            airGs = densityKgM3 * m3PerSecond * 1000.0;
        }
        // A commanded ratio below 1 means rich: more fuel for the same air.
        const double lambda = has(kLambda) && value(kLambda) > 0.5 && value(kLambda) < 1.5 ? value(kLambda) : 1.0;
        const double fuelGs = airGs / (kPetrolAfr * lambda);
        return fuelGs * 3600.0 / kPetrolDensity;
    }
    case FuelMethod::None:
    case FuelMethod::NeedsEngineSize:
        return -1;
    }
    return -1;
}

TripComputer::Calibration TripComputer::learnCalibration(double oldFactor, double oldLiters, double pumpLiters,
                                                        double estimatedRawLiters)
{
    Calibration c;
    c.factor = oldFactor;
    c.liters = oldLiters;
    if (estimatedRawLiters < 5.0 || pumpLiters <= 0) {
        c.result = Calibration::Result::TooLittleFuel;
        return c;
    }
    c.measuredFactor = pumpLiters / estimatedRawLiters;
    if (c.measuredFactor < 0.5 || c.measuredFactor > 2.0) {
        c.result = Calibration::Result::TooFarOff;
        return c;
    }
    c.liters = oldLiters + pumpLiters;
    c.factor = (oldFactor * oldLiters + c.measuredFactor * pumpLiters) / c.liters;
    return c;
}

void TripComputer::addSample(qint64 ms, const QHash<quint8, double> &values)
{
    for (auto it = values.cbegin(); it != values.cend(); ++it)
        m_last.insert(it.key(), it.value());

    const double speed = has(kSpeed) ? std::max(0.0, value(kSpeed)) : 0;
    const double flow = fuelFlowLph();
    const bool engineOn = has(kRpm) && value(kRpm) > 0;

    if (m_lastMs >= 0 && ms > m_lastMs && ms - m_lastMs <= kMaxGapMs) {
        const qint64 dt = ms - m_lastMs;
        const double hours = dt / 3600000.0;
        // Trapezoids: the average of the previous and current value over the interval.
        m_stats.distanceKm += (m_lastSpeed + speed) / 2.0 * hours;
        if (flow >= 0) {
            const double liters = (std::max(0.0, m_lastFlow) + flow) / 2.0 * hours;
            m_stats.rawFuelLiters += liters;
        }
        m_stats.durationMs += dt;
        if (engineOn)
            m_stats.engineMs += dt;
        if (speed >= 2)
            m_stats.movingMs += dt;
        else if (engineOn)
            m_stats.idleMs += dt;
    }
    m_lastMs = ms;
    m_lastSpeed = speed;
    m_lastFlow = flow;

    m_stats.fuelKnown = flow >= 0;
    m_stats.fuelLiters = m_stats.rawFuelLiters * m_settings.calibration;
    m_stats.maxSpeedKmh = std::max(m_stats.maxSpeedKmh, speed);
    m_stats.instantSpeedKmh = speed;
    m_stats.instantLph = flow >= 0 ? flow * m_settings.calibration : 0;
}

} // namespace obdaryn
