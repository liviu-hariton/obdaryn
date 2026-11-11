// SPDX-License-Identifier: GPL-3.0-or-later
//
// Charts of several signals over time: live, or from one or two recordings.

#pragma once

#include "chart/SignalSeries.h"
#include "obd/ObdClient.h"

#include <QHash>
#include <QObject>
#include <QSet>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

namespace obdaryn {

/**
 * @brief The data behind the Charts screen.
 *
 * One strip per signal, all on the same time axis: different units never
 * share a y-axis. Two sources:
 *  - **Live**: the last 1–10 minutes of the connected car. The selected
 *    signals are read on every poll cycle while the screen is open.
 *  - **Recordings**: a whole recording (A), optionally with a second one (B)
 *    drawn over it, aligned at their starts, to compare two drives.
 *
 * QML tells the model the plot size (setPlotSize); the model returns each
 * strip's lines already in pixels, reduced to two points per pixel column.
 * Values are stored in metric and converted for display (Units).
 */
class ChartModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by AppController")

    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int windowSeconds READ windowSeconds WRITE setWindowSeconds NOTIFY sourceChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    /// Signals that can be charted now: [{pid, name, selected}].
    Q_PROPERTY(QVariantList available READ available NOTIFY selectionChanged)
    /// One entry per strip: {pid, name, unit, latest, lowText, highText, topLabel, bottomLabel, lineA, lineB}.
    Q_PROPERTY(QVariantList strips READ strips NOTIFY stripsChanged)
    /// Time labels under the strips: [{position (0..1), text}].
    Q_PROPERTY(QVariantList timeTicks READ timeTicks NOTIFY stripsChanged)
    /// Built-in and saved presets that apply here: [{name, saved, available}].
    Q_PROPERTY(QVariantList presets READ presets NOTIFY presetsChanged)
    Q_PROPERTY(QString recordingA READ recordingA NOTIFY sourceChanged)
    Q_PROPERTY(QString recordingB READ recordingB NOTIFY sourceChanged)
    /// File paths of the loaded recordings (to show their friendly names).
    Q_PROPERTY(QString recordingAPath READ recordingAPath NOTIFY sourceChanged)
    Q_PROPERTY(QString recordingBPath READ recordingBPath NOTIFY sourceChanged)
    Q_PROPERTY(int maxSignals READ maxSignals CONSTANT)

public:
    explicit ChartModel(QObject *parent = nullptr);

    static constexpr int kMaxSignals = 6;
    int maxSignals() const { return kMaxSignals; }

    QString source() const { return m_source; }
    void setSource(const QString &source);       ///< "live" or "recording"
    int windowSeconds() const { return m_windowSeconds; }
    void setWindowSeconds(int seconds);
    bool active() const { return m_active; }
    void setActive(bool active);

    QVariantList available() const;
    QVariantList strips() const { return m_strips; }
    QVariantList timeTicks() const { return m_timeTicks; }
    QVariantList presets() const;
    QString recordingA() const { return m_nameA; }
    QString recordingB() const { return m_nameB; }
    QString recordingAPath() const { return m_pathA; }
    QString recordingBPath() const { return m_pathB; }

    Q_INVOKABLE void toggleSignal(int pid);
    Q_INVOKABLE void applyPreset(const QString &name);
    Q_INVOKABLE void savePreset(const QString &name);
    Q_INVOKABLE void deletePreset(const QString &name);
    /// Plot size of one strip, in pixels (all strips are the same size).
    Q_INVOKABLE void setPlotSize(qreal width, qreal height);
    /// Loads a recording as A (`slot` 0) or B (1); an empty path removes B.
    Q_INVOKABLE void loadRecording(int slot, const QString &pathOrUrl);
    /// The values under the crosshair at `fraction` (0..1) of the time axis:
    /// {time, rows: [{name, a, b}]}.
    Q_INVOKABLE QVariantMap readout(qreal fraction) const;
    /// The charted window as CSV text (one column per signal), for the clipboard.
    Q_INVOKABLE QString toCsv() const;

    // ---- Fed by AppController ------------------------------------------------
    void setSupported(const QSet<quint8> &pids);
    void addReadings(qint64 ms, const QList<ObdClient::PidReading> &readings);
    /// The PIDs live charts need on every poll cycle (empty when not shown).
    QSet<quint8> requiredPids() const;
    void retranslate() { rebuild(); emit selectionChanged(); emit presetsChanged(); }

signals:
    void sourceChanged();
    void activeChanged();
    void selectionChanged();
    void stripsChanged();
    void presetsChanged();
    /// The live inputs changed: AppController updates the poll list.
    void requiredPidsChanged();
    void notice(bool success, const QString &message);

private:
    struct Preset {
        QString name;
        QList<quint8> pids;
        bool saved = false; ///< the user's own (can be deleted)
    };
    QList<Preset> allPresets() const;
    QSet<quint8> availablePids() const;
    /// Time span of the chart in ms: [from, to].
    void window(qint64 *from, qint64 *to) const;
    void rebuild();

    QString m_source = QStringLiteral("live");
    int m_windowSeconds = 120;
    bool m_active = false;
    QList<quint8> m_selected;
    QSet<quint8> m_supported;
    qreal m_plotWidth = 600, m_plotHeight = 80;

    QHash<quint8, SignalSeries> m_live;
    qint64 m_liveNow = 0;
    QHash<quint8, SignalSeries> m_a, m_b; ///< recordings, time from their start
    qint64 m_durationA = 0, m_durationB = 0;
    QString m_nameA, m_nameB;
    QString m_pathA, m_pathB;

    QVariantList m_strips;
    QVariantList m_timeTicks;
    QTimer m_refresh; ///< live charts redraw a few times a second, not on every sample
};

} // namespace obdaryn
