// SPDX-License-Identifier: GPL-3.0-or-later
//
// The list of trouble codes found in the car.

#pragma once

#include "obd/Dtc.h"

#include <QAbstractListModel>
#include <QHash>
#include <QtQml/qqmlregistration.h>

namespace obdaryn {

/**
 * @brief Trouble codes, one row per (code, memory, module).
 *
 * A code can appear several times: P0420 is often both "stored" and
 * "permanent". The UI groups them by code; each row knows its memory kind.
 */
class DtcModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by AppController")
    Q_PROPERTY(int count READ rowCount NOTIFY countsChanged)
    Q_PROPERTY(int storedCount READ storedCount NOTIFY countsChanged)
    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY countsChanged)
    Q_PROPERTY(int permanentCount READ permanentCount NOTIFY countsChanged)
    /// Distinct codes that are stored or pending (what the sidebar badge counts).
    Q_PROPERTY(int activeCodeCount READ activeCodeCount NOTIFY countsChanged)

public:
    enum Role {
        CodeRole = Qt::UserRole + 1,
        KindRole,        ///< "stored" | "pending" | "permanent"
        KindLabelRole,   ///< translated
        EcuRole,         ///< "7E8"
        EcuNameRole,     ///< "ECM-EngineControl" when known
        TitleRole,       ///< short meaning; falls back to the system name
        SystemRole,
        KnownRole,
        ManufacturerSpecificRole,
        EmissionsRole,   ///< matters for the emissions inspection
    };

    explicit DtcModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    /// Distinct codes in display order (a code can be stored and permanent).
    Q_INVOKABLE QStringList codes() const;
    /// One entry per code with every memory it was found in:
    /// [{ code, title, ecu, ecuName, kinds: [{ kind, label }] }]. For compact lists.
    Q_INVOKABLE QVariantList grouped() const;

    void setDtcs(const QList<Dtc> &dtcs);
    void setEcuNames(const QHash<QString, QString> &names);
    void clear() { setDtcs({}); }
    void retranslate();
    /// Brand pack (vehicle-profile id) and language used for code titles.
    void setContext(const QString &brand, const QString &language);

    int storedCount() const { return countOf(DtcKind::Stored); }
    int pendingCount() const { return countOf(DtcKind::Pending); }
    int permanentCount() const { return countOf(DtcKind::Permanent); }
    int activeCodeCount() const;

signals:
    void countsChanged();

private:
    int countOf(DtcKind kind) const;
    QList<Dtc> m_dtcs;
    QHash<QString, QString> m_ecuNames;
    QString m_brand;
    QString m_language = QStringLiteral("en");
};

} // namespace obdaryn
