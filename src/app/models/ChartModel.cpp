// SPDX-License-Identifier: GPL-3.0-or-later

#include "models/ChartModel.h"

#include "AppController.h"

#include "obd/Pids.h"
#include "obd/Units.h"
#include "recording/Recording.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QSettings>
#include <QUrl>

#include <algorithm>
#include <cmath>

namespace obdaryn {

namespace {

constexpr qint64 kLiveKeepMs = 10 * 60 * 1000; // the longest live window
const QString kSelectedKey = QStringLiteral("charts/selected");
const QString kWindowKey = QStringLiteral("charts/windowSeconds");
const QString kPresetsKey = QStringLiteral("charts/presets");

/// "1:05", "-0:30".
QString clock(qint64 ms, bool negative = false)
{
    const qint64 s = std::llabs(ms) / 1000;
    return QStringLiteral("%1%2:%3").arg(negative && s > 0 ? QStringLiteral("−") : QString()).arg(s / 60).arg(s % 60, 2, 10, QLatin1Char('0'));
}

/// The displayed value of a metric reading of `pid`.
Units::Shown shown(quint8 pid, double metric)
{
    const PidDefinition *def = PidTable::find(pid);
    return def ? Units::show(metric, def->unit, def->decimals) : Units::Shown{metric, {}, 1};
}

QString text(quint8 pid, double metric, bool withUnit = true)
{
    const Units::Shown s = shown(pid, metric);
    const QString number = QLocale().toString(s.value, 'f', s.decimals);
    return withUnit && !s.unit.isEmpty() ? QStringLiteral("%1 %2").arg(number, s.unit) : number;
}

QString localPath(const QString &pathOrUrl)
{
    const QUrl url(pathOrUrl);
    return url.isLocalFile() ? url.toLocalFile() : pathOrUrl;
}

/// Numeric samples per PID from a recording, from the ECU that reported each one most.
QHash<quint8, SignalSeries> seriesFrom(const Recording &rec)
{
    const QList<Recording::PidSample> samples = rec.pidSamples();
    QHash<quint8, QHash<QString, int>> counts;
    for (const auto &s : samples)
        ++counts[s.pid][s.ecu];
    QHash<quint8, QString> best;
    for (auto it = counts.cbegin(); it != counts.cend(); ++it) {
        int n = -1;
        for (auto e = it.value().cbegin(); e != it.value().cend(); ++e) {
            if (e.value() > n) {
                n = e.value();
                best[it.key()] = e.key();
            }
        }
    }
    QHash<quint8, SignalSeries> out;
    for (const auto &s : samples) {
        const PidDefinition *def = PidTable::find(s.pid);
        double v = 0;
        if (!def || !def->isNumeric() || best.value(s.pid) != s.ecu || !def->value(s.raw, &v) || !std::isfinite(v))
            continue;
        out[s.pid].add(s.t, v);
    }
    return out;
}

} // namespace

ChartModel::ChartModel(QObject *parent) : QObject(parent)
{
    QSettings settings;
    for (const QVariant &v : settings.value(kSelectedKey).toList())
        m_selected << quint8(v.toInt());
    m_windowSeconds = settings.value(kWindowKey, 120).toInt();

    m_refresh.setInterval(250);
    connect(&m_refresh, &QTimer::timeout, this, &ChartModel::rebuild);
}

void ChartModel::setSource(const QString &source)
{
    const QString s = source == QLatin1String("recording") ? source : QStringLiteral("live");
    if (s == m_source)
        return;
    m_source = s;
    emit sourceChanged();
    emit selectionChanged();
    emit presetsChanged();
    emit requiredPidsChanged();
    rebuild();
    setActive(m_active); // restarts or stops the live refresh
}

void ChartModel::setWindowSeconds(int seconds)
{
    seconds = std::clamp(seconds, 30, int(kLiveKeepMs / 1000));
    if (seconds == m_windowSeconds)
        return;
    m_windowSeconds = seconds;
    QSettings().setValue(kWindowKey, seconds);
    emit sourceChanged();
    rebuild();
}

void ChartModel::setActive(bool active)
{
    const bool changed = active != m_active;
    m_active = active;
    if (m_active && m_source == QLatin1String("live"))
        m_refresh.start();
    else
        m_refresh.stop();
    if (changed) {
        emit activeChanged();
        emit requiredPidsChanged();
        rebuild();
    }
}

QSet<quint8> ChartModel::availablePids() const
{
    QSet<quint8> pids;
    if (m_source == QLatin1String("live")) {
        for (quint8 pid : m_supported) {
            const PidDefinition *def = PidTable::find(pid);
            if (def && def->isNumeric())
                pids << pid;
        }
    } else {
        for (auto it = m_a.cbegin(); it != m_a.cend(); ++it)
            pids << it.key();
        for (auto it = m_b.cbegin(); it != m_b.cend(); ++it)
            pids << it.key();
    }
    return pids;
}

QVariantList ChartModel::available() const
{
    QList<quint8> pids = availablePids().values();
    std::sort(pids.begin(), pids.end());
    QVariantList out;
    for (quint8 pid : pids)
        out << QVariantMap{{QStringLiteral("pid"), pid},
                           {QStringLiteral("name"), PidTable::find(pid)->translatedName()},
                           {QStringLiteral("selected"), m_selected.contains(pid)}};
    return out;
}

void ChartModel::toggleSignal(int pid)
{
    const quint8 p = quint8(pid);
    if (m_selected.contains(p))
        m_selected.removeAll(p);
    else if (m_selected.size() < kMaxSignals)
        m_selected << p;
    else
        return;
    QVariantList list;
    for (quint8 s : std::as_const(m_selected))
        list << s;
    QSettings().setValue(kSelectedKey, list);
    emit selectionChanged();
    emit requiredPidsChanged();
    rebuild();
}

QList<ChartModel::Preset> ChartModel::allPresets() const
{
    // Built-in sets for the questions owners ask most; names are translated.
    QList<Preset> presets = {
        {tr("Fuel mixture"), {0x0C, 0x06, 0x07, 0x14, 0x44}},
        {tr("Engine load"), {0x0C, 0x04, 0x0B, 0x11}},
        {tr("Temperatures"), {0x05, 0x0F, 0x3C, 0x46}},
        {tr("Charging"), {0x42, 0x0C}},
        {tr("Speed and throttle"), {0x0D, 0x0C, 0x11}},
    };
    const QJsonArray saved = QJsonDocument::fromJson(QSettings().value(kPresetsKey).toByteArray()).array();
    for (const QJsonValue &v : saved) {
        Preset p;
        p.name = v.toObject().value(QStringLiteral("name")).toString();
        p.saved = true;
        for (const QJsonValue &pid : v.toObject().value(QStringLiteral("pids")).toArray())
            p.pids << quint8(pid.toInt());
        if (!p.name.isEmpty())
            presets << p;
    }
    return presets;
}

QVariantList ChartModel::presets() const
{
    const QSet<quint8> available = availablePids();
    QVariantList out;
    for (const Preset &p : allPresets()) {
        int count = 0;
        for (quint8 pid : p.pids)
            count += available.contains(pid);
        out << QVariantMap{{QStringLiteral("name"), p.name}, {QStringLiteral("saved"), p.saved},
                           {QStringLiteral("available"), count > 0}};
    }
    return out;
}

void ChartModel::applyPreset(const QString &name)
{
    const QSet<quint8> available = availablePids();
    for (const Preset &p : allPresets()) {
        if (p.name != name)
            continue;
        m_selected.clear();
        for (quint8 pid : p.pids) {
            if (available.contains(pid) && m_selected.size() < kMaxSignals)
                m_selected << pid;
        }
        QVariantList list;
        for (quint8 s : std::as_const(m_selected))
            list << s;
        QSettings().setValue(kSelectedKey, list);
        emit selectionChanged();
        emit requiredPidsChanged();
        rebuild();
        return;
    }
}

void ChartModel::savePreset(const QString &name)
{
    const QString n = name.trimmed();
    if (n.isEmpty() || m_selected.isEmpty())
        return;
    QJsonArray saved = QJsonDocument::fromJson(QSettings().value(kPresetsKey).toByteArray()).array();
    for (qsizetype i = saved.size() - 1; i >= 0; --i) {
        if (saved[i].toObject().value(QStringLiteral("name")).toString() == n)
            saved.removeAt(i); // saving under the same name replaces it
    }
    QJsonArray pids;
    for (quint8 p : std::as_const(m_selected))
        pids << int(p);
    saved << QJsonObject{{QStringLiteral("name"), n}, {QStringLiteral("pids"), pids}};
    QSettings().setValue(kPresetsKey, QJsonDocument(saved).toJson(QJsonDocument::Compact));
    emit presetsChanged();
    emit notice(true, tr("Saved the preset \"%1\".").arg(n));
}

void ChartModel::deletePreset(const QString &name)
{
    QJsonArray saved = QJsonDocument::fromJson(QSettings().value(kPresetsKey).toByteArray()).array();
    for (qsizetype i = saved.size() - 1; i >= 0; --i) {
        if (saved[i].toObject().value(QStringLiteral("name")).toString() == name)
            saved.removeAt(i);
    }
    QSettings().setValue(kPresetsKey, QJsonDocument(saved).toJson(QJsonDocument::Compact));
    emit presetsChanged();
}

void ChartModel::setPlotSize(qreal width, qreal height)
{
    if (width < 10 || height < 10 || (qFuzzyCompare(width, m_plotWidth) && qFuzzyCompare(height, m_plotHeight)))
        return;
    m_plotWidth = width;
    m_plotHeight = height;
    rebuild();
}

void ChartModel::setSupported(const QSet<quint8> &pids)
{
    m_supported = pids;
    // A first-time user gets a useful start rather than an empty screen.
    if (m_selected.isEmpty()) {
        for (quint8 pid : {quint8(0x0C), quint8(0x0D), quint8(0x04)}) {
            if (pids.contains(pid))
                m_selected << pid;
        }
    }
    emit selectionChanged();
    emit presetsChanged();
    emit requiredPidsChanged();
}

void ChartModel::addReadings(qint64 ms, const QList<ObdClient::PidReading> &readings)
{
    QSet<quint8> seen;
    for (const auto &r : readings) {
        if (seen.contains(r.pid))
            continue; // the first ECU (the engine) wins, as on the live data screen
        seen.insert(r.pid);
        const PidDefinition *def = PidTable::find(r.pid);
        double v = 0;
        if (def && def->isNumeric() && def->value(r.raw, &v) && std::isfinite(v))
            m_live[r.pid].add(ms, v);
    }
    m_liveNow = ms;
    // Keep ten minutes, the longest window; trim now and then, not on every reading.
    if (ms % 5000 < 300) {
        for (auto it = m_live.begin(); it != m_live.end(); ++it)
            it->dropBefore(ms - kLiveKeepMs);
    }
}

QSet<quint8> ChartModel::requiredPids() const
{
    if (!m_active || m_source != QLatin1String("live"))
        return {};
    return QSet<quint8>(m_selected.cbegin(), m_selected.cend()) & m_supported;
}

void ChartModel::loadRecording(int slot, const QString &pathOrUrl)
{
    const QString path = localPath(pathOrUrl);
    if (slot == 1 && path.isEmpty()) {
        m_b.clear();
        m_durationB = 0;
        m_nameB.clear();
        m_pathB.clear();
        emit sourceChanged();
        emit selectionChanged();
        rebuild();
        return;
    }
    Recording rec;
    QString error;
    if (!rec.load(path, &error)) {
        emit notice(false, tr("Couldn't open %1. %2").arg(QFileInfo(path).fileName(), error));
        return;
    }
    QHash<quint8, SignalSeries> series = seriesFrom(rec);
    if (series.isEmpty()) {
        emit notice(false, tr("%1 has no live data to chart.").arg(QFileInfo(path).fileName()));
        return;
    }
    // Named like in the recordings list, wherever the file came from: date first,
    // so two drives of the same car tell apart.
    const auto [car, when] = AppController::recordingLabel(rec.header, QFileInfo(path));
    const QString name = when + QStringLiteral(" · ") + car;
    if (slot == 1 && !m_a.isEmpty()) {
        m_b = std::move(series);
        m_durationB = rec.durationMs;
        m_nameB = name;
        m_pathB = QFileInfo(path).absoluteFilePath();
    } else {
        m_a = std::move(series);
        m_durationA = rec.durationMs;
        m_nameA = name;
        m_pathA = QFileInfo(path).absoluteFilePath();
    }
    m_source = QStringLiteral("recording");
    m_refresh.stop();
    // Nothing chosen yet, or nothing chosen that this recording has: start with
    // useful values rather than an empty screen.
    const QSet<quint8> available = availablePids();
    bool anyAvailable = false;
    for (quint8 pid : std::as_const(m_selected))
        anyAvailable |= available.contains(pid);
    if (!anyAvailable) {
        m_selected.clear();
        for (quint8 pid : {quint8(0x0C), quint8(0x0D), quint8(0x04)}) {
            if (available.contains(pid))
                m_selected << pid;
        }
    }
    emit sourceChanged();
    emit selectionChanged();
    emit presetsChanged();
    emit requiredPidsChanged();
    rebuild();
}

void ChartModel::window(qint64 *from, qint64 *to) const
{
    if (m_source == QLatin1String("live")) {
        *to = m_liveNow;
        *from = m_liveNow - qint64(m_windowSeconds) * 1000;
    } else {
        *from = 0;
        *to = std::max<qint64>(1000, std::max(m_durationA, m_durationB));
    }
}

void ChartModel::rebuild()
{
    qint64 from = 0, to = 0;
    window(&from, &to);
    const bool live = m_source == QLatin1String("live");
    const QSet<quint8> available = availablePids();
    const int columns = std::max(10, int(m_plotWidth));
    const double span = double(to - from);

    QVariantList strips;
    for (quint8 pid : std::as_const(m_selected)) {
        if (!available.contains(pid))
            continue;
        const PidDefinition *def = PidTable::find(pid);
        const SignalSeries a = live ? m_live.value(pid) : m_a.value(pid);
        const SignalSeries b = live ? SignalSeries() : m_b.value(pid);

        // One y-scale per strip, from what is visible, in the shown unit.
        double lo = 0, hi = 0, blo = 0, bhi = 0;
        bool any = a.range(from, to, &lo, &hi);
        if (b.range(from, to, &blo, &bhi)) {
            lo = any ? std::min(lo, blo) : blo;
            hi = any ? std::max(hi, bhi) : bhi;
            any = true;
        }
        double slo = any ? shown(pid, lo).value : 0, shi = any ? shown(pid, hi).value : 1;
        if (shi - slo < 1e-6) { // a flat line sits in the middle
            slo -= 1;
            shi += 1;
        }
        const double pad = (shi - slo) * 0.08;
        slo -= pad;
        shi += pad;

        auto line = [&](const SignalSeries &s) {
            QVariantList points;
            for (const QPointF &p : s.envelope(from, to, columns)) {
                const double x = (p.x() - from) / span * m_plotWidth;
                const double y = m_plotHeight - (shown(pid, p.y()).value - slo) / (shi - slo) * m_plotHeight;
                points << QPointF(x, y);
            }
            return points;
        };

        double latest = 0;
        const bool hasLatest = live && a.latest(&latest);
        const int decimals = shown(pid, 0).decimals;
        strips << QVariantMap{
            {QStringLiteral("pid"), pid},
            {QStringLiteral("name"), def->translatedName()},
            {QStringLiteral("unit"), shown(pid, 0).unit},
            {QStringLiteral("latest"), hasLatest ? text(pid, latest) : QString()},
            {QStringLiteral("rangeText"), any ? tr("%1 to %2").arg(text(pid, lo, false), text(pid, hi)) : QString()},
            {QStringLiteral("topLabel"), QLocale().toString(shi - pad, 'f', decimals)},
            {QStringLiteral("bottomLabel"), QLocale().toString(slo + pad, 'f', decimals)},
            {QStringLiteral("lineA"), line(a)},
            {QStringLiteral("lineB"), line(b)},
            {QStringLiteral("empty"), !any},
        };
    }
    m_strips = strips;

    // Five time labels under the strips.
    QVariantList ticks;
    for (int i = 0; i <= 4; ++i) {
        const double f = i / 4.0;
        const qint64 t = from + qint64(f * span);
        ticks << QVariantMap{{QStringLiteral("position"), f},
                             {QStringLiteral("text"), live ? (i == 4 ? tr("now") : clock(t - to, true)) : clock(t)}};
    }
    m_timeTicks = ticks;
    emit stripsChanged();
}

QVariantMap ChartModel::readout(qreal fraction) const
{
    qint64 from = 0, to = 0;
    window(&from, &to);
    const bool live = m_source == QLatin1String("live");
    const qint64 t = from + qint64(std::clamp(fraction, 0.0, 1.0) * double(to - from));
    // A value counts if it was read close to that moment (rotation can leave gaps).
    const qint64 tolerance = std::max<qint64>(2000, (to - from) / 150);
    // Comparing two drives of different length: past the end of the shorter one
    // there is nothing to show for it. Say so once, instead of a dash per row.
    const bool comparing = !live && !m_b.isEmpty();
    const bool endedA = comparing && t > m_durationA + tolerance;
    const bool endedB = comparing && t > m_durationB + tolerance;
    QString note;
    if (endedA)
        //: Charts readout, comparing recordings A and B: A is shorter than the moment pointed at. %1 = time, e.g. "12:05"
        note = tr("Recording A ended at %1").arg(clock(m_durationA));
    else if (endedB)
        //: Charts readout, comparing recordings A and B: B is shorter than the moment pointed at. %1 = time, e.g. "12:05"
        note = tr("Recording B ended at %1").arg(clock(m_durationB));
    QVariantList rows;
    for (const QVariant &v : m_strips) {
        const quint8 pid = quint8(v.toMap().value(QStringLiteral("pid")).toInt());
        double a = 0, b = 0;
        const bool hasA = (live ? m_live.value(pid) : m_a.value(pid)).valueAt(t, tolerance, &a);
        const bool hasB = comparing && m_b.value(pid).valueAt(t, tolerance, &b);
        // "—": a gap in the data (values are read in rotation); empty: that recording has ended.
        rows << QVariantMap{{QStringLiteral("name"), v.toMap().value(QStringLiteral("name"))},
                            {QStringLiteral("a"), hasA ? text(pid, a) : endedA ? QString() : QStringLiteral("—")},
                            {QStringLiteral("b"), hasB ? text(pid, b) : endedB ? QString() : QStringLiteral("—")}};
    }
    return {{QStringLiteral("time"), live ? (to - t < 500 ? tr("now") : clock(t - to, true)) : clock(t)},
            {QStringLiteral("note"), note},
            {QStringLiteral("rows"), rows}};
}

QString ChartModel::toCsv() const
{
    qint64 from = 0, to = 0;
    window(&from, &to);
    const bool live = m_source == QLatin1String("live");
    const bool withB = !live && !m_b.isEmpty();
    QStringList head{QStringLiteral("\"%1\"").arg(tr("Time (s)"))};
    QList<quint8> pids;
    for (const QVariant &v : m_strips) {
        const quint8 pid = quint8(v.toMap().value(QStringLiteral("pid")).toInt());
        pids << pid;
        // Exports stay metric, like the recording export.
        const PidDefinition *def = PidTable::find(pid);
        const QString name = QStringLiteral("%1 (%2)").arg(def->translatedName(), def->translatedUnit());
        head << QStringLiteral("\"%1\"").arg(withB ? QStringLiteral("A: ") + name : name);
        if (withB)
            head << QStringLiteral("\"B: %1\"").arg(name);
    }
    QString csv = head.join(QLatin1Char(',')) + QLatin1Char('\n');
    // A regular half-second grid, as a bounded row count (never a loop over time
    // values a file could inflate): 200,000 rows is over a day of data.
    const qint64 rows = std::min<qint64>((to - from) / 500 + 1, 200000);
    for (qint64 row = 0; row < rows; ++row) {
        const qint64 t = from + row * 500;
        QStringList cells{QString::number((t - from) / 1000.0, 'f', 1)};
        for (quint8 pid : std::as_const(pids)) {
            const PidDefinition *def = PidTable::find(pid);
            double v = 0;
            cells << ((live ? m_live.value(pid) : m_a.value(pid)).valueAt(t, 1000, &v) ? QString::number(v, 'f', def->decimals) : QString());
            if (withB)
                cells << (m_b.value(pid).valueAt(t, 1000, &v) ? QString::number(v, 'f', def->decimals) : QString());
        }
        csv += cells.join(QLatin1Char(',')) + QLatin1Char('\n');
    }
    return csv;
}

} // namespace obdaryn
