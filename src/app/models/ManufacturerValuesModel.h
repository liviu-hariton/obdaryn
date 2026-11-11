// SPDX-License-Identifier: GPL-3.0-or-later
//
// Manufacturer values (OBDb data packs) for the Live data screen.

#pragma once

#include "obd/ManufacturerData.h"
#include "obd/ManufacturerReader.h"

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QtQml/qqmlregistration.h>

namespace obdaryn {

/**
 * @brief One row per value the car's modules answered, grouped like OBDb
 *        groups them ("Engine", "Tires", ...).
 *
 * AppController finds which commands the car answers, hands them over with
 * setCommands(), and feeds every answer back with applyResult(). Values are
 * stored in metric and converted for display (Units), like generic live data.
 */
class ManufacturerValuesModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by AppController")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        GroupRole,      ///< OBDb group, e.g. "Engine"
        ModuleRole,     ///< the module that answers, e.g. "Engine (ECM)"
        RequestRole,    ///< "7E0 21 01", for power users
        ValueTextRole,  ///< formatted in the chosen units, or "Yes"/"No"
        UnitRole,
        HasValueRole,
    };

    explicit ManufacturerValuesModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    /// Replaces the rows with every value of these commands (the ones the car answers).
    /// `moduleNames` maps a request ID (0x7E0) to a readable module name.
    void setCommands(const QList<ManufacturerCommand> &commands, const QHash<quint16, QString> &moduleNames);
    void clear();
    /// Stores the values of one answer.
    void applyResult(const ManufacturerCommand &command, const ManufacturerReader::Result &result);
    /// Units or language changed: every text is rebuilt.
    void refreshTexts();

signals:
    void countChanged();

private:
    struct Row {
        QString commandKey;
        ManufacturerSignal signal;
        QString module;
        QString request;
        double metric = 0;
        bool hasValue = false;
    };
    QList<Row> m_rows;
    QHash<QString, QList<int>> m_rowsOfCommand; ///< command key -> row indices
};

} // namespace obdaryn
