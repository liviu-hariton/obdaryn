// SPDX-License-Identifier: GPL-3.0-or-later
//
// Live sensor values for the dashboard and the live data screen.

#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QSet>
#include <QtQml/qqmlregistration.h>

namespace obdaryn {

/**
 * @brief One row per numeric PID the car supports, with its latest value and a
 *        short history for sparklines.
 *
 * The model doesn't talk to the car itself: AppController asks it which PIDs
 * to poll next (nextPollBatch()) and feeds the answers back (applyReading()).
 * "Pinned" PIDs are shown on the dashboard and polled on every cycle; the
 * others are polled in rotation, a few per cycle, so the dashboard stays fast.
 */
class LiveDataModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by AppController")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(double samplesPerSecond READ samplesPerSecond NOTIFY samplesPerSecondChanged)

public:
    enum Role {
        PidRole = Qt::UserRole + 1,
        PidHexRole,
        NameRole,
        UnitRole,
        ValueRole,      ///< double, NaN until the first reading
        ValueTextRole,  ///< formatted with the PID's decimals
        RawRole,        ///< raw bytes as hex, for power users
        MinimumRole,
        MaximumRole,
        HistoryRole,    ///< QVariantList of the last samples (oldest first)
        PinnedRole,
        HasValueRole,
    };

    static constexpr int kHistoryLength = 120;

    explicit LiveDataModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    /// Replaces the rows with the numeric PIDs from this list.
    void setSupportedPids(const QList<quint8> &pids);
    void clear();

    /// PIDs to request in the next polling cycle.
    QList<quint8> nextPollBatch();
    void applyReading(quint8 pid, const QByteArray &raw);

    Q_INVOKABLE void setPinned(int pid, bool pinned);
    Q_INVOKABLE bool isPinned(int pid) const { return m_pinned.contains(quint8(pid)); }
    /// Rows to show on the dashboard, in a stable order.
    Q_INVOKABLE QVariantList pinnedRows() const;
    /// Values as CSV text (name, value, unit) for "copy to clipboard".
    Q_INVOKABLE QString toCsv() const;

    double samplesPerSecond() const { return m_samplesPerSecond; }
    void setSamplesPerSecond(double v);

    /// Re-emits names and units after a language switch.
    void retranslate();

signals:
    void countChanged();
    void samplesPerSecondChanged();
    void pinnedChanged();

private:
    struct Row {
        quint8 pid = 0;
        double value = 0;
        bool hasValue = false;
        QByteArray raw;
        QList<double> history;
    };
    int rowOf(quint8 pid) const;
    void savePinned() const;

    QList<Row> m_rows;
    QSet<quint8> m_pinned;
    int m_rotation = 0;
    double m_samplesPerSecond = 0;
};

} // namespace obdaryn
