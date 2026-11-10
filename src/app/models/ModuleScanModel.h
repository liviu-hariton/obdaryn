// SPDX-License-Identifier: GPL-3.0-or-later
//
// The results of a full-vehicle module scan, one row per module.

#pragma once

#include "obd/ModuleScanner.h"

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>

namespace obdaryn {

/**
 * @brief Exposes ModuleScanner results to QML.
 *
 * Rows update one by one while the scan runs, so the list fills in live.
 */
class ModuleScanModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by AppController")
    Q_PROPERTY(int count READ rowCount NOTIFY summaryChanged)
    Q_PROPERTY(int answeredCount READ answeredCount NOTIFY summaryChanged)
    Q_PROPERTY(int codeCount READ codeCount NOTIFY summaryChanged)
    /// Codes in modules generic OBD-II can't reach (everything except engine and transmission).
    Q_PROPERTY(int extraCodeCount READ extraCodeCount NOTIFY summaryChanged)
    Q_PROPERTY(int modulesWithCodes READ modulesWithCodes NOTIFY summaryChanged)
    /// Stored (confirmed) codes outside engine and transmission: the badge turns red when > 0.
    Q_PROPERTY(int extraStoredCount READ extraStoredCount NOTIFY summaryChanged)
    Q_PROPERTY(int doneCount READ doneCount NOTIFY summaryChanged)

public:
    enum Role {
        ModuleIdRole = Qt::UserRole + 1,
        NameRole,
        CategoryRole,
        AddressRole,
        StateRole,        ///< "waiting" | "scanning" | "absent" | "answered" | "obdonly" | "error"
        ProtocolRole,     ///< "UDS" | "KWP2000" | ""
        VerifiedRole,
        SourceRole,
        DtcsRole,         ///< list of { code, displayCode, title, kind, kindLabel, activeNow, warningLamp, status }
        CodeCountRole,
        PartNumberRole,
        SoftwareRole,
        ErrorRole,
    };

    explicit ModuleScanModel(ModuleScanner *scanner, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int answeredCount() const;
    int codeCount() const;
    int extraCodeCount() const;
    int modulesWithCodes() const;
    int extraStoredCount() const;
    int doneCount() const;

    void retranslate();

signals:
    void summaryChanged();

private:
    ModuleScanner *m_scanner;
};

} // namespace obdaryn
