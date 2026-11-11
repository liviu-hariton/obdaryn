// SPDX-License-Identifier: GPL-3.0-or-later

#include "obd/ManufacturerData.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>
#include <cmath>

namespace obdaryn {

namespace {

YearRange yearsFrom(const QJsonValue &v)
{
    YearRange r;
    const QJsonObject o = v.toObject();
    r.from = o.value(QStringLiteral("from")).toInt();
    r.to = o.value(QStringLiteral("to")).toInt();
    for (const QJsonValue &y : o.value(QStringLiteral("years")).toArray())
        r.years << y.toInt();
    return r;
}

// OBDb unit -> the metric unit Obdaryn stores, and the factor to get there.
struct UnitMap {
    const char *obdb;
    const char *metric;
    double factor;
};
constexpr UnitMap kUnits[] = {
    {"celsius", "°C", 1},           {"fahrenheit", "°C", 1}, // °F handled in toMetric()
    {"kilometersPerHour", "km/h", 1}, {"milesPerHour", "km/h", 1.609344},
    {"kilometers", "km", 1},        {"miles", "km", 1.609344},
    {"kilopascal", "kPa", 1},       {"psi", "kPa", 6.894757}, {"bars", "bar", 1},
    {"volts", "V", 1},              {"millivolts", "mV", 1},  {"amps", "A", 1},
    {"percent", "%", 1},            {"rpm", "rpm", 1},        {"degrees", "°", 1},
    {"milliseconds", "ms", 1},      {"seconds", "s", 1},      {"minutes", "min", 1},
    {"hours", "h", 1},              {"liters", "L", 1},       {"newtonMeters", "Nm", 1},
    {"hertz", "Hz", 1},             {"metersPerSecondSquared", "m/s²", 1}, {"gravity", "g", 1},
    {"kilowatts", "kW", 1},         {"watts", "W", 1},        {"kilowattHours", "kWh", 1},
    {"ampereHours", "Ah", 1},       {"kiloohms", "kΩ", 1},    {"millimeters", "mm", 1},
};

const UnitMap *unitMap(const QString &unit)
{
    for (const UnitMap &u : kUnits) {
        if (unit == QLatin1String(u.obdb))
            return &u;
    }
    return nullptr;
}

} // namespace

bool YearRange::contains(int year) const
{
    if (empty())
        return true;
    if (years.contains(year))
        return true;
    if (from == 0 && to == 0)
        return false; // only listed years
    return (from == 0 || year >= from) && (to == 0 || year <= to);
}

bool ManufacturerSignal::decode(const QByteArray &data, double *value) const
{
    if (length <= 0 || length > 32 || bit < 0)
        return false;
    quint64 raw = 0;
    if (littleEndian) {
        // Whole bytes, least significant first.
        const int first = bit / 8, count = (length + 7) / 8;
        if (first + count > data.size())
            return false;
        for (int i = count - 1; i >= 0; --i)
            raw = (raw << 8) | quint8(data[first + i]);
        raw &= (quint64(1) << length) - 1;
    } else {
        // Bits counted from the most significant bit of the first byte.
        if (bit + length > data.size() * 8)
            return false;
        for (int b = bit; b < bit + length; ++b)
            raw = (raw << 1) | ((quint8(data[b / 8]) >> (7 - b % 8)) & 1);
    }
    double v = double(raw);
    if (isSigned && (raw >> (length - 1)) & 1)
        v = double(qint64(raw) - (qint64(1) << length));
    v = v * mul / (div == 0 ? 1 : div) + add;
    if ((hasNullMin && v < nullMin) || (hasNullMax && v > nullMax))
        return false;
    *value = v;
    return true;
}

const char *ManufacturerSignal::metricUnit() const
{
    const UnitMap *u = unitMap(unit);
    return u ? u->metric : "";
}

double ManufacturerSignal::toMetric(double value) const
{
    if (unit == QLatin1String("fahrenheit"))
        return (value - 32) * 5.0 / 9.0;
    const UnitMap *u = unitMap(unit);
    return u ? value * u->factor : value;
}

int ManufacturerSignal::decimals() const
{
    if (isState() || unit == QLatin1String("rpm"))
        return 0; // engine speed in tenths of a revolution helps nobody
    double step = std::abs(mul / (div == 0 ? 1 : div));
    if (const UnitMap *u = unitMap(unit))
        step *= u->factor;
    if (step <= 0 || step >= 1)
        return 0;
    return std::clamp(int(std::ceil(-std::log10(step) - 1e-9)), 0, 2);
}

QByteArray ManufacturerCommand::request() const
{
    return QByteArray::number(service, 16).toUpper().rightJustified(2, '0') + identifier.toHex().toUpper();
}

QString ManufacturerCommand::key() const
{
    return QStringLiteral("%1:%2").arg(header, 3, 16, QLatin1Char('0')).toUpper().arg(QString::fromLatin1(request()));
}

bool ManufacturerCommand::appliesTo(int year) const
{
    if (year <= 0)
        return years.empty() && debugYears.empty();
    return years.contains(year) && (debugYears.empty() || !debugYears.contains(year));
}

QList<ManufacturerCommand> ManufacturerModel::commandsFor(int year) const
{
    QList<ManufacturerCommand> out;
    for (const ManufacturerCommand &c : commands) {
        if (c.appliesTo(year))
            out << c;
    }
    return out;
}

QList<ManufacturerModel> ManufacturerData::parse(const QByteArray &json, QString *attribution)
{
    QList<ManufacturerModel> models;
    const QJsonObject root = QJsonDocument::fromJson(json).object();
    if (root.value(QStringLiteral("format")).toString() != QLatin1String("obdaryn-obdb"))
        return models;
    if (attribution)
        *attribution = root.value(QStringLiteral("attribution")).toString();
    for (const QJsonValue &mv : root.value(QStringLiteral("models")).toArray()) {
        const QJsonObject mo = mv.toObject();
        ManufacturerModel m;
        m.make = mo.value(QStringLiteral("make")).toString();
        m.model = mo.value(QStringLiteral("model")).toString();
        m.repository = mo.value(QStringLiteral("repo")).toString();
        m.commit = mo.value(QStringLiteral("commit")).toString();
        for (const QJsonValue &cv : mo.value(QStringLiteral("commands")).toArray()) {
            const QJsonObject co = cv.toObject();
            ManufacturerCommand c;
            bool okHeader = false, okReceiver = false, okService = false;
            c.header = quint16(co.value(QStringLiteral("hdr")).toString().toUInt(&okHeader, 16));
            c.receiver = quint16(co.value(QStringLiteral("rax")).toString().toUInt(&okReceiver, 16));
            c.service = quint8(co.value(QStringLiteral("svc")).toString().toUInt(&okService, 16));
            c.identifier = QByteArray::fromHex(co.value(QStringLiteral("id")).toString().toLatin1());
            // Only the read-only services, with the identifier length they take.
            const bool shapeOk = (c.service == 0x21 && c.identifier.size() == 1)
                                 || (c.service == 0x22 && c.identifier.size() == 2);
            if (!okHeader || !okReceiver || !okService || !shapeOk || c.header > 0x7FF || c.receiver > 0x7FF)
                continue;
            c.intervalSeconds = std::max(0.25, co.value(QStringLiteral("freq")).toDouble(1));
            c.flowControlHeader = co.value(QStringLiteral("fcm1")).toBool();
            c.years = yearsFrom(co.value(QStringLiteral("years")));
            c.debugYears = yearsFrom(co.value(QStringLiteral("dbg")));
            for (const QJsonValue &sv : co.value(QStringLiteral("signals")).toArray()) {
                const QJsonObject so = sv.toObject();
                const QJsonObject f = so.value(QStringLiteral("fmt")).toObject();
                ManufacturerSignal s;
                s.id = so.value(QStringLiteral("id")).toString();
                s.name = so.value(QStringLiteral("name")).toString();
                s.group = so.value(QStringLiteral("path")).toString();
                s.unit = f.value(QStringLiteral("unit")).toString();
                s.bit = f.value(QStringLiteral("bix")).toInt();
                s.length = f.value(QStringLiteral("len")).toInt(8);
                s.littleEndian = f.value(QStringLiteral("blsb")).toBool();
                s.isSigned = f.value(QStringLiteral("sign")).toBool();
                s.mul = f.value(QStringLiteral("mul")).toDouble(1);
                s.div = f.value(QStringLiteral("div")).toDouble(1);
                s.add = f.value(QStringLiteral("add")).toDouble(0);
                s.hasNullMin = f.contains(QStringLiteral("nullmin"));
                s.hasNullMax = f.contains(QStringLiteral("nullmax"));
                s.nullMin = f.value(QStringLiteral("nullmin")).toDouble();
                s.nullMax = f.value(QStringLiteral("nullmax")).toDouble();
                if (!s.id.isEmpty() && s.length > 0 && s.length <= 32)
                    c.signalList << s;
            }
            if (!c.signalList.isEmpty())
                m.commands << c;
        }
        if (!m.make.isEmpty() && !m.model.isEmpty() && !m.commands.isEmpty())
            models << m;
    }
    return models;
}

ManufacturerData::ManufacturerData()
{
    const QDir dir(QStringLiteral(":/obdaryn/data/obdb"));
    for (const QString &name : dir.entryList({QStringLiteral("*.json")}, QDir::Files, QDir::Name)) {
        QFile f(dir.filePath(name));
        if (!f.open(QIODevice::ReadOnly))
            continue;
        QString attribution;
        m_models += parse(f.readAll(), &attribution);
        if (m_attribution.isEmpty())
            m_attribution = attribution;
    }
}

const ManufacturerData &ManufacturerData::instance()
{
    static const ManufacturerData data;
    return data;
}

QStringList ManufacturerData::modelsOf(const QString &make) const
{
    QStringList out;
    for (const ManufacturerModel &m : m_models) {
        if (m.make.compare(make, Qt::CaseInsensitive) == 0)
            out << m.model;
    }
    out.sort(Qt::CaseInsensitive);
    return out;
}

const ManufacturerModel *ManufacturerData::find(const QString &make, const QString &model) const
{
    for (const ManufacturerModel &m : m_models) {
        if (m.make.compare(make, Qt::CaseInsensitive) == 0 && m.model.compare(model, Qt::CaseInsensitive) == 0)
            return &m;
    }
    return nullptr;
}

} // namespace obdaryn
