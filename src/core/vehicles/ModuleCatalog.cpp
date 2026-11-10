// SPDX-License-Identifier: GPL-3.0-or-later

#include "vehicles/ModuleCatalog.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>

namespace obdaryn {

QString ModuleDef::displayName() const
{
    return ModuleCatalog::categoryName(category);
}

QString ModuleDef::addressText() const
{
    return QStringLiteral("%1 → %2")
        .arg(tx, 3, 16, QLatin1Char('0'))
        .arg(rx, 3, 16, QLatin1Char('0'))
        .toUpper();
}

QString ModuleCatalog::categoryName(const QString &category)
{
    auto t = [](const char *s) { return QCoreApplication::translate("Module", s); };
    if (category == QLatin1String("engine"))       return t(QT_TRANSLATE_NOOP("Module", "Engine"));
    if (category == QLatin1String("transmission")) return t(QT_TRANSLATE_NOOP("Module", "Transmission"));
    if (category == QLatin1String("hybrid"))       return t(QT_TRANSLATE_NOOP("Module", "Hybrid system"));
    if (category == QLatin1String("abs"))          return t(QT_TRANSLATE_NOOP("Module", "ABS / stability control"));
    if (category == QLatin1String("airbag"))       return t(QT_TRANSLATE_NOOP("Module", "Airbag (SRS)"));
    if (category == QLatin1String("steering"))     return t(QT_TRANSLATE_NOOP("Module", "Power steering"));
    if (category == QLatin1String("tpms"))         return t(QT_TRANSLATE_NOOP("Module", "Tyre pressure (TPMS)"));
    if (category == QLatin1String("body"))         return t(QT_TRANSLATE_NOOP("Module", "Body control"));
    if (category == QLatin1String("cluster"))      return t(QT_TRANSLATE_NOOP("Module", "Instrument cluster"));
    if (category == QLatin1String("climate"))      return t(QT_TRANSLATE_NOOP("Module", "Climate control"));
    if (category == QLatin1String("gateway"))      return t(QT_TRANSLATE_NOOP("Module", "Gateway"));
    if (category == QLatin1String("infotainment")) return t(QT_TRANSLATE_NOOP("Module", "Infotainment"));
    return t(QT_TRANSLATE_NOOP("Module", "Other module"));
}

VehicleProfile ModuleCatalog::parse(const QByteArray &json)
{
    VehicleProfile p;
    const QJsonObject o = QJsonDocument::fromJson(json).object();
    if (o.isEmpty())
        return p;
    p.id = o.value(QStringLiteral("id")).toString();
    p.label = o.value(QStringLiteral("label")).toString();
    p.notes = o.value(QStringLiteral("notes")).toString();
    for (auto it = o.begin(); it != o.end(); ++it) {
        if (it.key().startsWith(QLatin1String("notes_")))
            p.localizedNotes.insert(it.key().mid(6), it.value().toString());
    }
    for (const QJsonValue &v : o.value(QStringLiteral("makes")).toArray())
        p.makes << v.toString();
    for (const QJsonValue &v : o.value(QStringLiteral("wmi")).toArray())
        p.wmi << v.toString().toUpper();

    for (const QJsonValue &v : o.value(QStringLiteral("modules")).toArray()) {
        const QJsonObject m = v.toObject();
        ModuleDef d;
        d.id = m.value(QStringLiteral("id")).toString();
        d.category = m.value(QStringLiteral("category")).toString();
        bool okTx = false, okRx = false;
        d.tx = quint16(m.value(QStringLiteral("tx")).toString().toUInt(&okTx, 16));
        d.rx = quint16(m.value(QStringLiteral("rx")).toString().toUInt(&okRx, 16));
        const QString dtc = m.value(QStringLiteral("dtc")).toString();
        d.dtc = dtc == QLatin1String("uds") ? DiagProtocol::Uds
              : dtc == QLatin1String("kwp") ? DiagProtocol::Kwp : DiagProtocol::Auto;
        d.verified = m.value(QStringLiteral("verified")).toBool();
        d.source = m.value(QStringLiteral("source")).toString();
        // Only 11-bit CAN addresses are supported for now.
        if (okTx && okRx && d.tx <= 0x7FF && d.rx <= 0x7FF && !d.id.isEmpty())
            p.modules << d;
    }
    return p;
}

const QList<VehicleProfile> &ModuleCatalog::profiles()
{
    static const QList<VehicleProfile> all = [] {
        QList<VehicleProfile> list;
        const QDir dir(QStringLiteral(":/obdaryn/data/vehicles"));
        for (const QString &name : dir.entryList({QStringLiteral("*.json")}, QDir::Files, QDir::Name)) {
            QFile f(dir.filePath(name));
            if (!f.open(QIODevice::ReadOnly))
                continue;
            VehicleProfile p = parse(f.readAll());
            if (!p.id.isEmpty())
                list << p;
        }
        // Generic last: it's the fallback, not a brand.
        std::stable_sort(list.begin(), list.end(), [](const VehicleProfile &a, const VehicleProfile &b) {
            const bool ga = a.id == QLatin1String("generic"), gb = b.id == QLatin1String("generic");
            return ga != gb ? gb : a.label < b.label;
        });
        return list;
    }();
    return all;
}

const VehicleProfile *ModuleCatalog::find(const QString &id)
{
    for (const VehicleProfile &p : profiles()) {
        if (p.id == id)
            return &p;
    }
    return nullptr;
}

const VehicleProfile &ModuleCatalog::forVehicle(const QString &manufacturer, const QString &vin)
{
    const QString v = vin.trimmed().toUpper();
    for (const VehicleProfile &p : profiles()) {
        for (const QString &prefix : p.wmi) {
            if (!v.isEmpty() && v.startsWith(prefix))
                return p;
        }
    }
    for (const VehicleProfile &p : profiles()) {
        if (!manufacturer.isEmpty() && p.makes.contains(manufacturer, Qt::CaseInsensitive))
            return p;
    }
    static const VehicleProfile empty;
    const VehicleProfile *generic = find(QStringLiteral("generic"));
    return generic ? *generic : empty;
}

} // namespace obdaryn
