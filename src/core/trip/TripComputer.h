// SPDX-License-Identifier: GPL-3.0-or-later
//
// Trip computer: distance, time and fuel from live data.

#pragma once

#include <QHash>
#include <QList>
#include <QSet>
#include <QString>

namespace obdaryn {

/**
 * @brief Integrates live values into trip statistics.
 *
 * Generic OBD-II has no "fuel used" or "distance" counter that works on every
 * car, so they are computed from what every car does report:
 *
 *  - **Distance** = vehicle speed (PID 0D) integrated over time.
 *  - **Fuel**, from the best source the car offers:
 *    1. *Fuel rate* (PID 5E, L/h), measured by the engine computer.
 *    2. *Mass air flow* (PID 10, g/s): fuel = air / air-fuel ratio. Petrol
 *       burns at about 14.7 g of air per g of fuel; the commanded equivalence
 *       ratio (PID 44) corrects for rich or lean running when available.
 *    3. *Speed-density*, for cars without an air-flow sensor (many Hyundai,
 *       Kia, Toyota): the air mass is estimated from manifold pressure (0B),
 *       intake temperature (0F), engine speed (0C), the engine size and a
 *       typical volumetric efficiency of 85 %. Needs the engine size.
 *    Diesel engines run lean at a varying ratio, so only the fuel rate works for them.
 *  - **Fuel cut**: when the car coasts with the throttle closed, petrol engines
 *    stop injecting. Fuel system status (03) reports "open loop" and the load
 *    (04) is low; that time counts as zero fuel.
 *
 * Estimates are typically within 10–20 %. A calibration factor, learned from
 * fill-ups (liters at the pump ÷ liters estimated), corrects the systematic error.
 *
 * Feed it one poll cycle at a time with addSample(); it keeps the last value of
 * each PID, because slow values (intake temperature) aren't read every cycle.
 */
class TripComputer
{
public:
    enum class FuelMethod {
        None,          ///< the car reports nothing to estimate fuel from
        FuelRate,      ///< PID 5E
        MassAirFlow,   ///< PID 10
        SpeedDensity,  ///< PIDs 0B + 0F + 0C and the engine size
        NeedsEngineSize, ///< speed-density possible, but the engine size isn't known
    };

    struct Settings {
        bool diesel = false;
        double engineLiters = 0;     ///< displacement, for speed-density; 0 = unknown
        double calibration = 1.0;    ///< multiplies the estimated fuel (from fill-ups)
    };

    struct Stats {
        double distanceKm = 0;
        double fuelLiters = 0;       ///< calibrated
        double rawFuelLiters = 0;    ///< before calibration (what fill-ups are compared with)
        qint64 durationMs = 0;       ///< time covered by samples (gaps excluded)
        qint64 engineMs = 0;         ///< engine running
        qint64 movingMs = 0;
        qint64 idleMs = 0;           ///< engine running, standing still
        double maxSpeedKmh = 0;
        double instantLph = 0;       ///< current fuel flow
        double instantSpeedKmh = 0;
        bool fuelKnown = false;      ///< fuel numbers are meaningful

        double averageSpeedKmh() const { return movingMs > 0 ? distanceKm / (movingMs / 3600000.0) : 0; }
        /// Average consumption, L/100 km; 0 until the trip has some distance.
        double litersPer100Km() const { return distanceKm >= 0.2 ? fuelLiters / distanceKm * 100.0 : 0; }
        /// Instant consumption: L/100 km while moving, else 0 (show instantLph instead).
        double instantLitersPer100Km() const { return instantSpeedKmh >= 3 ? instantLph / instantSpeedKmh * 100.0 : 0; }
    };

    /// The PIDs worth polling every cycle, given what the car supports.
    static QSet<quint8> inputPids(const QSet<quint8> &supported, bool diesel);
    /// The method that will be used with these supported PIDs and settings.
    static FuelMethod methodFor(const QSet<quint8> &supported, const Settings &settings);

    void setSettings(const Settings &settings);
    const Settings &settings() const { return m_settings; }
    void setSupported(const QSet<quint8> &supported);
    FuelMethod method() const { return m_method; }

    /**
     * @brief Adds one poll cycle.
     * @param ms     monotonic time of the cycle, in milliseconds
     * @param values decoded values by PID (only those read this cycle)
     */
    void addSample(qint64 ms, const QHash<quint8, double> &values);
    /// Starts a new trip (keeps settings and supported PIDs).
    void reset();

    const Stats &stats() const { return m_stats; }
    /// Fuel flow (L/h) for the current values, before calibration; -1 if unknown.
    double fuelFlowLph() const;

    /// What a fill-up taught about the estimate.
    struct Calibration {
        enum class Result { Learned, TooLittleFuel, TooFarOff };
        Result result = Result::Learned;
        double factor = 1.0;          ///< the new factor (unchanged unless Learned)
        double measuredFactor = 1.0;  ///< this fill-up alone: pump liters / estimated liters
        double liters = 0;            ///< total pump liters behind the new factor
    };
    /**
     * @brief Learns from a fill-up: the tank was filled to the click, twice,
     *        and every drive in between was followed by Obdaryn.
     *
     * The new factor is the liter-weighted average of the old one and this
     * fill-up's measurement, so one odd fill-up can't throw it far off. A
     * fill-up is refused when too little fuel was estimated to compare (under
     * 5 L) or when the ratio is beyond 2x either way, which almost always
     * means some drives weren't recorded.
     */
    static Calibration learnCalibration(double oldFactor, double oldLiters, double pumpLiters, double estimatedRawLiters);

    /// Above this gap between samples, the time in between isn't counted
    /// (module scan, adapter hiccup): guessing would invent kilometers.
    static constexpr qint64 kMaxGapMs = 5000;

private:
    bool has(quint8 pid) const { return m_last.contains(pid); }
    double value(quint8 pid) const { return m_last.value(pid); }
    bool fuelCut() const;

    Settings m_settings;
    QSet<quint8> m_supported;
    FuelMethod m_method = FuelMethod::None;
    QHash<quint8, double> m_last;
    qint64 m_lastMs = -1;
    double m_lastSpeed = 0;
    double m_lastFlow = 0;
    Stats m_stats;
};

} // namespace obdaryn
