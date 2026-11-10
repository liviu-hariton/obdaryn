// SPDX-License-Identifier: GPL-3.0-or-later

#include "obd/Dtc.h"

#include <QCoreApplication>

namespace obdaryn::DtcCodec {

QString decode(quint8 a, quint8 b)
{
    static const char letters[] = {'P', 'C', 'B', 'U'};
    const char letter = letters[(a >> 6) & 0x03];
    const int firstDigit = (a >> 4) & 0x03;
    return QStringLiteral("%1%2%3%4")
        .arg(QLatin1Char(letter))
        .arg(firstDigit)
        .arg(a & 0x0F, 1, 16)
        .arg(b, 2, 16, QLatin1Char('0'))
        .toUpper();
}

QStringList fromResponse(const EcuMessage &message, bool can)
{
    QStringList codes;
    auto addPairs = [&codes](const QByteArray &bytes, qsizetype from) {
        for (qsizetype i = from; i + 1 < bytes.size(); i += 2) {
            const quint8 a = quint8(bytes[i]);
            const quint8 b = quint8(bytes[i + 1]);
            if (a == 0 && b == 0)
                continue; // padding, "P0000" is not a real code
            codes << decode(a, b);
        }
    };

    if (can) {
        if (message.data.size() < 2)
            return codes;
        // Byte 1 is the number of codes; trust the bytes we actually received.
        addPairs(message.data, 2);
    } else {
        for (const QByteArray &frame : message.frames)
            addPairs(frame, 1);
    }
    codes.removeDuplicates();
    return codes;
}

bool isManufacturerSpecific(const QString &code)
{
    if (code.size() != 5)
        return false;
    const QChar letter = code[0];
    const QChar d1 = code[1];
    if (letter == QLatin1Char('P')) {
        if (d1 == QLatin1Char('1'))
            return true;
        if (d1 == QLatin1Char('3'))
            return code[2] < QLatin1Char('4'); // P30xx–P33xx manufacturer, P34xx–P39xx SAE
        return false;
    }
    return d1 == QLatin1Char('1') || d1 == QLatin1Char('2');
}

QString systemName(const QString &code)
{
    auto t = [](const char *s) { return QCoreApplication::translate("DtcSystem", s); };
    if (code.size() != 5)
        return {};

    switch (code[0].toLatin1()) {
    case 'B': return t(QT_TRANSLATE_NOOP("DtcSystem", "Body (airbags, comfort, lighting, ...)"));
    case 'C': return t(QT_TRANSLATE_NOOP("DtcSystem", "Chassis (ABS, stability control, steering, ...)"));
    case 'U': return t(QT_TRANSLATE_NOOP("DtcSystem", "Network communication between modules"));
    default: break;
    }

    // Manufacturer powertrain codes don't follow the SAE subsystem layout, so
    // guessing a subsystem from their digits would be wrong (P1A2B isn't
    // necessarily hybrid).
    if (isManufacturerSpecific(code))
        return t(QT_TRANSLATE_NOOP("DtcSystem", "Powertrain (manufacturer-specific)"));

    // Generic powertrain: the third character names the subsystem (SAE J2012 layout).
    switch (code[2].toLatin1()) {
    case '0': return t(QT_TRANSLATE_NOOP("DtcSystem", "Fuel, air metering and emission controls"));
    case '1':
    case '2': return t(QT_TRANSLATE_NOOP("DtcSystem", "Fuel and air metering"));
    case '3': return t(QT_TRANSLATE_NOOP("DtcSystem", "Ignition system or misfire"));
    case '4': return t(QT_TRANSLATE_NOOP("DtcSystem", "Auxiliary emission controls"));
    case '5': return t(QT_TRANSLATE_NOOP("DtcSystem", "Vehicle speed, idle control and auxiliary inputs"));
    case '6': return t(QT_TRANSLATE_NOOP("DtcSystem", "Engine computer and output circuits"));
    case '7':
    case '8':
    case '9': return t(QT_TRANSLATE_NOOP("DtcSystem", "Transmission"));
    case 'A':
    case 'B':
    case 'C': return t(QT_TRANSLATE_NOOP("DtcSystem", "Hybrid / electric propulsion"));
    default:  return t(QT_TRANSLATE_NOOP("DtcSystem", "Powertrain"));
    }
}

} // namespace obdaryn::DtcCodec
