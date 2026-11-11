// SPDX-License-Identifier: GPL-3.0-or-later
//
// Manufacturer values: what a brand's modules report beyond generic OBD-II,
// from the OBDb community signal sets (CC BY-SA 4.0, shipped as data files).

#pragma once

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

namespace obdaryn {

/// Model years a command applies to (OBDb "filter"). Empty means every year.
struct YearRange {
    int from = 0, to = 0;   ///< 0: open end
    QList<int> years;       ///< listed years, in addition to from..to
    bool empty() const { return from == 0 && to == 0 && years.isEmpty(); }
    bool contains(int year) const;
};

/**
 * @brief One value inside a manufacturer answer, e.g. "Engine oil temperature".
 *
 * The bit position counts from the first byte after the answer's service byte
 * and identifier ("61 01 <data...>" or "62 E0 10 <data...>"), most significant
 * bit first, as in OBDb's signal sets. Multi-byte values are big-endian unless
 * `littleEndian` ("blsb" in OBDb).
 */
struct ManufacturerSignal {
    QString id;            ///< OBDb id, e.g. "ELANTRA_ENGINE_OIL_T"
    QString name;          ///< English, from OBDb
    QString group;         ///< OBDb "path", e.g. "Engine", "Tires"
    QString unit;          ///< OBDb unit name, e.g. "celsius", "psi", "noyes"
    int bit = 0, length = 8;
    bool littleEndian = false, isSigned = false;
    double mul = 1, div = 1, add = 0;
    bool hasNullMin = false, hasNullMax = false;
    double nullMin = 0, nullMax = 0;  ///< values outside this range mean "not available"

    /// The name in the UI language: a translated name where Obdaryn has one
    /// (the common values, see ManufacturerNames), OBDb's English name otherwise.
    QString displayName() const;

    /// Decodes the value (in OBDb's unit) from the bytes after the identifier.
    /// False when the answer is too short or the value means "not available".
    bool decode(const QByteArray &data, double *value) const;
    /// A yes/no or on/off state rather than a number.
    bool isState() const { return unit == QLatin1String("noyes") || unit == QLatin1String("offon"); }

    /// The metric unit the app stores and converts ("°C", "kPa", "km/h", "V"...);
    /// empty for plain numbers and states.
    const char *metricUnit() const;
    /// Converts a decoded value into metricUnit() (psi to kPa, °F to °C, mph to km/h...).
    double toMetric(double value) const;
    /// Sensible decimals for display, from the value's resolution.
    int decimals() const;
};

/// One request to one module, and the values in its answer.
struct ManufacturerCommand {
    quint16 header = 0;        ///< request ID, e.g. 0x7E0
    quint16 receiver = 0;      ///< answer ID, e.g. 0x7E8
    quint8 service = 0;        ///< 0x21 or 0x22 (read-only)
    QByteArray identifier;     ///< 1 byte for 0x21, 2 bytes for 0x22
    double intervalSeconds = 1;///< how often it's worth reading (OBDb "freq")
    bool flowControlHeader = false; ///< needs the explicit flow-control frame (OBDb "fcm1")
    YearRange years;           ///< model years it applies to
    YearRange debugYears;      ///< years for which OBDb hasn't verified it
    QList<ManufacturerSignal> signalList;

    /// The request as sent: "2101", "22E010".
    QByteArray request() const;
    /// A stable key: "7E0:2101".
    QString key() const;
    bool appliesTo(int year) const;
};

/// One car model's signal set.
struct ManufacturerModel {
    QString make;              ///< "Hyundai"
    QString model;             ///< "Elantra"
    QString repository;        ///< "OBDb/Hyundai-Elantra"
    QString commit;            ///< the OBDb commit it was imported from
    QList<ManufacturerCommand> commands;

    /// The commands for a model year. Year 0 (unknown) keeps only the commands
    /// that apply to every year.
    QList<ManufacturerCommand> commandsFor(int year) const;
};

/**
 * @brief The manufacturer data packs compiled into the app (the JSON files in data/obdb).
 *
 * Every request in them passes the safety guard: tools/import_obdb.py keeps
 * only services 0x21 and 0x22, and the guard checks each request again when
 * it is sent.
 */
class ManufacturerData
{
public:
    static const ManufacturerData &instance();

    /// Parses one pack (the JSON written by tools/import_obdb.py). Exposed for tests.
    static QList<ManufacturerModel> parse(const QByteArray &json, QString *attribution = nullptr);

    const QList<ManufacturerModel> &models() const { return m_models; }
    /// Models of a make ("Hyundai"), sorted by name.
    QStringList modelsOf(const QString &make) const;
    const ManufacturerModel *find(const QString &make, const QString &model) const;
    /// An OBDb group ("Engine", "Battery.Cells") in the UI language, segments
    /// joined with " · " ("Baterie · Celule").
    static QString groupName(const QString &path);
    /// Credits to show wherever these values appear.
    QString attribution() const { return m_attribution; }

private:
    ManufacturerData();
    QList<ManufacturerModel> m_models;
    QString m_attribution;
};

} // namespace obdaryn
