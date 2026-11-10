// SPDX-License-Identifier: GPL-3.0-or-later

#include "obd/VehicleInfo.h"

#include <QCoreApplication>
#include <QDate>
#include <QHash>

namespace obdaryn {

namespace {

// World Manufacturer Identifiers → brand. Not exhaustive: covers the brands
// common in Europe and North America. Two-character keys act as a fallback.
const QHash<QString, QString> &wmiTable()
{
    static const QHash<QString, QString> t = {
        // Hyundai / Kia / Genesis
        {"KMH", "Hyundai"}, {"KMF", "Hyundai"}, {"KMJ", "Hyundai"}, {"TMA", "Hyundai"},
        {"NLH", "Hyundai"}, {"MAL", "Hyundai"}, {"5NP", "Hyundai"}, {"5NM", "Hyundai"},
        {"KNA", "Kia"}, {"KND", "Kia"}, {"KNE", "Kia"}, {"KNM", "Renault Samsung"},
        {"U5Y", "Kia"}, {"U6Y", "Kia"}, {"5XY", "Kia"}, {"5XX", "Kia"}, {"KMT", "Genesis"},
        // Volkswagen group
        {"WVW", "Volkswagen"}, {"WVG", "Volkswagen"}, {"WV1", "Volkswagen"}, {"WV2", "Volkswagen"},
        {"3VW", "Volkswagen"}, {"1VW", "Volkswagen"}, {"WAU", "Audi"}, {"WUA", "Audi"},
        {"TMB", "Škoda"}, {"VSS", "SEAT"}, {"VSE", "SEAT"}, {"WP0", "Porsche"}, {"WP1", "Porsche"},
        // Other German
        {"WBA", "BMW"}, {"WBS", "BMW M"}, {"WBY", "BMW i"}, {"WMW", "MINI"},
        {"WDB", "Mercedes-Benz"}, {"WDD", "Mercedes-Benz"}, {"W1K", "Mercedes-Benz"},
        {"W1N", "Mercedes-Benz"}, {"WDC", "Mercedes-Benz"}, {"WME", "smart"},
        {"W0L", "Opel"}, {"W0V", "Opel"}, {"WF0", "Ford"},
        // France / Romania / Italy / Spain
        {"VF1", "Renault"}, {"VF3", "Peugeot"}, {"VF7", "Citroën"}, {"VR3", "Peugeot"},
        {"VR7", "Citroën"}, {"UU1", "Dacia"}, {"UU2", "Dacia"}, {"VGA", "Alpine"},
        {"ZFA", "Fiat"}, {"ZAR", "Alfa Romeo"}, {"ZLA", "Lancia"}, {"ZFF", "Ferrari"},
        {"ZHW", "Lamborghini"}, {"ZAM", "Maserati"}, {"ZCF", "Iveco"},
        // Japan
        {"JT", "Toyota"}, {"SB1", "Toyota"}, {"VNK", "Toyota"}, {"NMT", "Toyota"},
        {"JTH", "Lexus"}, {"JHM", "Honda"}, {"SHH", "Honda"}, {"JN1", "Nissan"},
        {"SJN", "Nissan"}, {"VSK", "Nissan"}, {"JM1", "Mazda"}, {"JMZ", "Mazda"},
        {"JS2", "Suzuki"}, {"JSA", "Suzuki"}, {"TSM", "Suzuki"}, {"JMB", "Mitsubishi"},
        {"JA3", "Mitsubishi"}, {"JF1", "Subaru"}, {"JF2", "Subaru"},
        // Sweden / UK / Czech
        {"YV1", "Volvo"}, {"YV4", "Volvo"}, {"LYV", "Volvo"}, {"SAL", "Land Rover"},
        {"SAJ", "Jaguar"}, {"SCC", "Lotus"}, {"SCB", "Bentley"}, {"SAR", "Rover"},
        // North America
        {"1FA", "Ford"}, {"1FT", "Ford"}, {"1FM", "Ford"}, {"3FA", "Ford"}, {"1G1", "Chevrolet"},
        {"1GC", "Chevrolet"}, {"2G1", "Chevrolet"}, {"KL1", "Chevrolet"}, {"1GT", "GMC"},
        {"1G6", "Cadillac"}, {"1C3", "Chrysler"}, {"1C4", "Jeep"}, {"1J4", "Jeep"},
        {"2C3", "Dodge"}, {"1D7", "Ram"}, {"5YJ", "Tesla"}, {"7SA", "Tesla"},
        // China
        {"LRW", "Tesla"}, {"XP7", "Tesla"}, {"LGX", "BYD"}, {"LC0", "BYD"}, {"LVV", "Chery"},
        {"LSJ", "MG"}, {"SDP", "MG"}, {"LB3", "Geely"}, {"L6T", "Geely"},
        // Others
        {"XTA", "Lada"}, {"SUP", "Daewoo / FSO"}, {"KPT", "SsangYong"},
    };
    return t;
}

QString regionOf(QChar c)
{
    auto t = [](const char *s) { return QCoreApplication::translate("VinRegion", s); };
    const char ch = c.toLatin1();
    switch (ch) {
    case '1':
    case '4':
    case '5': return t(QT_TRANSLATE_NOOP("VinRegion", "United States"));
    case '2': return t(QT_TRANSLATE_NOOP("VinRegion", "Canada"));
    case '3': return t(QT_TRANSLATE_NOOP("VinRegion", "Mexico"));
    case '6': return t(QT_TRANSLATE_NOOP("VinRegion", "Australia"));
    case '7': return t(QT_TRANSLATE_NOOP("VinRegion", "New Zealand"));
    case '8':
    case '9': return t(QT_TRANSLATE_NOOP("VinRegion", "South America"));
    case 'J': return t(QT_TRANSLATE_NOOP("VinRegion", "Japan"));
    case 'K': return t(QT_TRANSLATE_NOOP("VinRegion", "South Korea"));
    case 'L': return t(QT_TRANSLATE_NOOP("VinRegion", "China"));
    case 'M': return t(QT_TRANSLATE_NOOP("VinRegion", "India / Southeast Asia"));
    case 'N': return t(QT_TRANSLATE_NOOP("VinRegion", "Turkey / Central Asia"));
    case 'S': return t(QT_TRANSLATE_NOOP("VinRegion", "United Kingdom"));
    case 'T': return t(QT_TRANSLATE_NOOP("VinRegion", "Central Europe (CZ, CH, HU, ...)"));
    case 'U': return t(QT_TRANSLATE_NOOP("VinRegion", "Eastern Europe (RO, ...)"));
    case 'V': return t(QT_TRANSLATE_NOOP("VinRegion", "France / Spain"));
    case 'W': return t(QT_TRANSLATE_NOOP("VinRegion", "Germany"));
    case 'X': return t(QT_TRANSLATE_NOOP("VinRegion", "Russia / Eastern Europe"));
    case 'Y': return t(QT_TRANSLATE_NOOP("VinRegion", "Nordic countries"));
    case 'Z': return t(QT_TRANSLATE_NOOP("VinRegion", "Italy"));
    default:  return t(QT_TRANSLATE_NOOP("VinRegion", "Africa / other"));
    }
}

int vinValue(QChar c)
{
    // ISO 3779 transliteration used by the check digit.
    static const QString letters = QStringLiteral("ABCDEFGHJKLMNPRSTUVWXYZ");
    static const int values[] = {1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 7, 9, 2, 3, 4, 5, 6, 7, 8, 9};
    if (c.isDigit())
        return c.digitValue();
    const qsizetype i = letters.indexOf(c);
    return i >= 0 ? values[i] : -1;
}

/// Cleans ASCII payload bytes: drops NUL padding and non-printable bytes.
QString asciiOf(const QByteArray &bytes)
{
    QString s;
    for (char ch : bytes) {
        if (ch >= 0x20 && ch < 0x7F)
            s.append(QLatin1Char(ch));
    }
    return s.trimmed();
}

/// Payload of a service 09 answer without "49 <pid> [count]". On older
/// protocols every frame repeats "49 <pid> <sequence>" before 4 data bytes.
QByteArray infoPayload(const EcuMessage &m, bool can, bool hasCountByte)
{
    if (can)
        return m.data.mid(hasCountByte ? 3 : 2);
    QByteArray out;
    for (const QByteArray &f : m.frames)
        out += f.mid(3);
    return out;
}

} // namespace

VinInfo decodeVin(const QString &raw)
{
    VinInfo info;
    info.vin = raw.trimmed().toUpper();
    const QString &v = info.vin;
    info.valid = v.size() == 17 && !v.contains(QLatin1Char('I')) && !v.contains(QLatin1Char('O'))
                 && !v.contains(QLatin1Char('Q'));
    if (v.size() < 3)
        return info;

    info.wmi = v.left(3);
    info.manufacturer = wmiTable().value(info.wmi, wmiTable().value(v.left(2)));
    info.region = regionOf(v[0]);
    if (!info.valid)
        return info;

    // Check digit (position 9).
    static const int weights[] = {8, 7, 6, 5, 4, 3, 2, 10, 0, 9, 8, 7, 6, 5, 4, 3, 2};
    int sum = 0;
    bool ok = true;
    for (int i = 0; i < 17; ++i) {
        const int value = vinValue(v[i]);
        if (value < 0) { ok = false; break; }
        sum += value * weights[i];
    }
    const int rem = sum % 11;
    const QChar expected = rem == 10 ? QLatin1Char('X') : QChar(u'0' + rem);
    info.checkDigitValid = ok && v[8] == expected;

    // Model year (position 10). The code repeats every 30 years.
    static const QString yearCodes = QStringLiteral("ABCDEFGHJKLMNPRSTVWXY123456789");
    const qsizetype idx = yearCodes.indexOf(v[9]);
    if (idx >= 0) {
        const int maxYear = QDate::currentDate().year() + 1;
        int year = 1980 + int(idx);
        while (year + 30 <= maxYear)
            year += 30;
        info.modelYear = year;
    }
    return info;
}

namespace VehicleInfoCodec {

QString vin(const EcuMessage &m, bool can)
{
    // CAN: 49 02 01 <17 ASCII>. Some ECUs pad with zeros or leading bytes; keep the last 17.
    QString s = asciiOf(infoPayload(m, can, true));
    if (s.size() > 17)
        s = s.right(17);
    return s;
}

QStringList calibrationIds(const EcuMessage &m, bool can)
{
    const QByteArray p = infoPayload(m, can, true);
    QStringList ids;
    for (qsizetype i = 0; i + 16 <= p.size(); i += 16) {
        const QString id = asciiOf(p.mid(i, 16));
        if (!id.isEmpty())
            ids << id;
    }
    return ids;
}

QStringList cvns(const EcuMessage &m, bool can)
{
    const QByteArray p = infoPayload(m, can, true);
    QStringList out;
    for (qsizetype i = 0; i + 4 <= p.size(); i += 4)
        out << QString::fromLatin1(p.mid(i, 4).toHex().toUpper());
    return out;
}

QString ecuName(const EcuMessage &m, bool can)
{
    // 20 bytes: 4-char acronym (NUL padded), '-', 15-char name.
    return asciiOf(infoPayload(m, can, true));
}

} // namespace VehicleInfoCodec

} // namespace obdaryn
