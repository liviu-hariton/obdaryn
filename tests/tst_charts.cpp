// SPDX-License-Identifier: GPL-3.0-or-later
//
// Chart series: downsampling keeps every extreme, lookups, trimming.

#include "chart/SignalSeries.h"

#include <QTest>

using namespace obdaryn;

class TestCharts : public QObject
{
    Q_OBJECT
private slots:
    void envelopeKeepsSpikes();
    void envelopeIsBounded();
    void lookups();
    void outOfOrderAndTrim();
};

void TestCharts::envelopeKeepsSpikes()
{
    SignalSeries s;
    for (int t = 0; t < 10000; t += 10)
        s.add(t, t == 5000 ? 100.0 : 1.0); // one sample spike in 1000
    // 50 columns for 1000 samples: the spike must still be drawn.
    const QList<QPointF> points = s.envelope(0, 10000, 50);
    bool spike = false;
    for (const QPointF &p : points)
        spike |= p.y() == 100.0;
    QVERIFY(spike);
}

void TestCharts::envelopeIsBounded()
{
    SignalSeries s;
    for (int t = 0; t < 600000; t += 50)
        s.add(t, std::sin(t / 1000.0));
    const QList<QPointF> points = s.envelope(0, 600000, 800);
    QVERIFY(points.size() <= 2 * 800); // at most two points per column
    QVERIFY(points.size() >= 800);
    // In time order, so the line never doubles back.
    for (qsizetype i = 1; i < points.size(); ++i)
        QVERIFY(points[i].x() >= points[i - 1].x());
}

void TestCharts::lookups()
{
    SignalSeries s;
    s.add(1000, 10);
    s.add(2000, 20);
    double v = 0;
    QVERIFY(s.valueAt(1500, 1000, &v));
    QCOMPARE(v, 10.0);
    QVERIFY(!s.valueAt(4000, 1000, &v)); // too old: a gap, not a value
    QVERIFY(!s.valueAt(500, 1000, &v));  // before the first sample
    double lo = 0, hi = 0;
    QVERIFY(s.range(0, 3000, &lo, &hi));
    QCOMPARE(lo, 10.0);
    QCOMPARE(hi, 20.0);
    QVERIFY(!s.range(3000, 4000, &lo, &hi));
    QVERIFY(s.latest(&v));
    QCOMPARE(v, 20.0);
}

void TestCharts::outOfOrderAndTrim()
{
    SignalSeries s;
    s.add(3000, 3);
    s.add(1000, 1); // a late reading lands where it belongs
    s.add(2000, 2);
    QCOMPARE(s.samples().first().t, 1000);
    QCOMPARE(s.samples().last().t, 3000);
    s.dropBefore(2000);
    QCOMPARE(s.samples().size(), 2);
    QCOMPARE(s.samples().first().v, 2.0);
}

QTEST_GUILESS_MAIN(TestCharts)
#include "tst_charts.moc"
