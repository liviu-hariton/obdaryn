// SPDX-License-Identifier: GPL-3.0-or-later
//
// Trip computer: distance and fuel integration, fuel sources, fuel cut,
// gaps, and fill-up calibration.

#include "trip/TripComputer.h"

#include <QTest>

using namespace obdaryn;

class TestTrip : public QObject
{
    Q_OBJECT
private slots:
    void distance();
    void gapsAreNotInvented();
    void fuelRate();
    void massAirFlow();
    void speedDensity();
    void fuelCut();
    void methods();
    void calibration();

private:
    /// Feeds `seconds` of constant values, one sample every 500 ms.
    static void drive(TripComputer &trip, qint64 &ms, double seconds, const QHash<quint8, double> &values)
    {
        for (double t = 0; t < seconds; t += 0.5) {
            trip.addSample(ms, values);
            ms += 500;
        }
    }
};

void TestTrip::distance()
{
    TripComputer trip;
    trip.setSupported({0x0C, 0x0D});
    qint64 ms = 0;
    drive(trip, ms, 3600, {{0x0D, 60}, {0x0C, 2000}});
    QVERIFY(qAbs(trip.stats().distanceKm - 60.0) < 0.05);
    QVERIFY(qAbs(trip.stats().averageSpeedKmh() - 60.0) < 0.1);
    drive(trip, ms, 600, {{0x0D, 0}, {0x0C, 800}});
    QVERIFY(trip.stats().idleMs >= 595000);
    QCOMPARE(trip.stats().maxSpeedKmh, 60.0);
    QVERIFY(!trip.stats().fuelKnown); // nothing to estimate fuel from
}

void TestTrip::gapsAreNotInvented()
{
    TripComputer trip;
    trip.setSupported({0x0D});
    trip.addSample(0, {{0x0D, 100}});
    trip.addSample(1000, {{0x0D, 100}});
    trip.addSample(61000, {{0x0D, 100}}); // a minute without data: not counted
    QVERIFY(qAbs(trip.stats().distanceKm - 100.0 / 3600.0) < 1e-6);
    QCOMPARE(trip.stats().durationMs, 1000);
}

void TestTrip::fuelRate()
{
    TripComputer trip;
    trip.setSupported({0x0C, 0x0D, 0x5E});
    QCOMPARE(trip.method(), TripComputer::FuelMethod::FuelRate);
    qint64 ms = 0;
    drive(trip, ms, 3600, {{0x0D, 60}, {0x0C, 2000}, {0x5E, 6.0}});
    QVERIFY(qAbs(trip.stats().fuelLiters - 6.0) < 0.01);
    QVERIFY(qAbs(trip.stats().litersPer100Km() - 10.0) < 0.05);
    QVERIFY(qAbs(trip.stats().instantLitersPer100Km() - 10.0) < 0.01);

    // The calibration factor scales what's shown, not the raw estimate.
    trip.setSettings({false, 0, 1.1});
    trip.addSample(ms, {});
    QVERIFY(qAbs(trip.stats().fuelLiters - 6.6) < 0.02);
    QVERIFY(qAbs(trip.stats().rawFuelLiters - 6.0) < 0.01);
}

void TestTrip::massAirFlow()
{
    TripComputer trip;
    trip.setSupported({0x0C, 0x0D, 0x10});
    QCOMPARE(trip.method(), TripComputer::FuelMethod::MassAirFlow);
    trip.addSample(0, {{0x0C, 2000}, {0x10, 10.0}});
    // 10 g/s air / 14.7 = 0.680 g/s fuel = 3.29 L/h at 745 g/L
    QVERIFY(qAbs(trip.fuelFlowLph() - 10.0 / 14.7 * 3600.0 / 745.0) < 1e-9);
    trip.addSample(500, {{0x44, 0.9}}); // running rich: more fuel for the same air
    QVERIFY(trip.fuelFlowLph() > 3.6);
}

void TestTrip::speedDensity()
{
    TripComputer trip;
    trip.setSupported({0x0B, 0x0C, 0x0D, 0x0F});
    QCOMPARE(trip.method(), TripComputer::FuelMethod::NeedsEngineSize);
    trip.setSettings({false, 1.8, 1.0});
    QCOMPARE(trip.method(), TripComputer::FuelMethod::SpeedDensity);
    // Idle of a 1.8 L petrol engine: 30 kPa, 27 °C, 780 rpm → about 1.1 L/h.
    trip.addSample(0, {{0x0B, 30}, {0x0F, 27}, {0x0C, 780}, {0x0D, 0}});
    QVERIFY2(trip.fuelFlowLph() > 0.8 && trip.fuelFlowLph() < 1.4, qPrintable(QString::number(trip.fuelFlowLph())));
    trip.addSample(500, {{0x0C, 0}}); // engine off: no fuel
    QCOMPARE(trip.fuelFlowLph(), 0.0);
}

void TestTrip::fuelCut()
{
    TripComputer trip;
    trip.setSupported({0x03, 0x04, 0x0C, 0x0D, 0x10});
    trip.addSample(0, {{0x0C, 2200}, {0x10, 6.0}, {0x03, 2}, {0x04, 35}});
    QVERIFY(trip.fuelFlowLph() > 0);
    trip.addSample(500, {{0x03, 4}, {0x04, 8}}); // coasting, injectors off
    QCOMPARE(trip.fuelFlowLph(), 0.0);
    trip.addSample(1000, {{0x03, 4}, {0x04, 90}}); // open loop at full load: enrichment, not a cut
    QVERIFY(trip.fuelFlowLph() > 0);
}

void TestTrip::methods()
{
    // Diesel: the air-fuel ratio varies too much for the air-flow methods.
    QCOMPARE(TripComputer::methodFor({0x10, 0x0C}, {true, 0, 1}), TripComputer::FuelMethod::None);
    QCOMPARE(TripComputer::methodFor({0x5E, 0x10}, {true, 0, 1}), TripComputer::FuelMethod::FuelRate);
    QCOMPARE(TripComputer::methodFor({0x0D}, {false, 1.6, 1}), TripComputer::FuelMethod::None);
    // Only the PIDs that matter are polled every cycle.
    QCOMPARE(TripComputer::inputPids({0x0C, 0x0D, 0x10, 0x0B, 0x0F, 0x42}, false),
             (QSet<quint8>{0x0C, 0x0D, 0x10}));
}

void TestTrip::calibration()
{
    using R = TripComputer::Calibration::Result;
    auto c = TripComputer::learnCalibration(1.0, 0, 44.0, 40.0);
    QCOMPARE(c.result, R::Learned);
    QVERIFY(qAbs(c.factor - 1.1) < 1e-9);
    // A second fill-up is averaged in, weighted by liters.
    c = TripComputer::learnCalibration(c.factor, c.liters, 40.0, 40.0);
    QVERIFY(qAbs(c.factor - (1.1 * 44 + 1.0 * 40) / 84) < 1e-9);
    QCOMPARE(TripComputer::learnCalibration(1.0, 0, 3.0, 2.0).result, R::TooLittleFuel);
    // Way off: drives were probably missed. Refused, factor unchanged.
    c = TripComputer::learnCalibration(1.05, 50, 45.0, 12.0);
    QCOMPARE(c.result, R::TooFarOff);
    QCOMPARE(c.factor, 1.05);
}

QTEST_GUILESS_MAIN(TestTrip)
#include "tst_trip.moc"
