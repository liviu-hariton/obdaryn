// SPDX-License-Identifier: GPL-3.0-or-later
//
// The adapter console: every command sent and every answer received.

#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QtQml/qqmlregistration.h>

namespace obdaryn {

/**
 * @brief A capped, append-only log of adapter traffic.
 *
 * Useful when an adapter or car behaves oddly: the raw lines show exactly
 * what was asked and answered, and can be copied into a bug report.
 */
class TrafficLogModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by AppController")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Role { TimeRole = Qt::UserRole + 1, DirectionRole, TextRole };
    static constexpr int kMaxEntries = 3000;

    explicit TrafficLogModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void append(const QString &direction, const QString &text);
    Q_INVOKABLE void clear();
    /// Whole log as plain text, for the clipboard.
    Q_INVOKABLE QString toText() const;

signals:
    void countChanged();

private:
    struct Entry {
        QDateTime time;
        QString direction;
        QString text;
    };
    QList<Entry> m_entries;
};

} // namespace obdaryn
