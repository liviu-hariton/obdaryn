// SPDX-License-Identifier: GPL-3.0-or-later

#include "obd/Protocol.h"

#include <QCoreApplication>

namespace obdaryn {

bool isCan(Protocol p)
{
    return static_cast<int>(p) >= static_cast<int>(Protocol::Can11_500);
}

bool uses29BitIds(Protocol p)
{
    return p == Protocol::Can29_500 || p == Protocol::Can29_250 || p == Protocol::J1939;
}

Protocol protocolFromDpn(const QString &reply)
{
    QString s = reply.trimmed().toUpper();
    if (s.startsWith(QLatin1Char('A')) && s.size() == 2)
        s.remove(0, 1); // "A6" → "6": the leading A only means "found automatically"
    if (s.size() != 1)
        return Protocol::Unknown;

    bool ok = false;
    const int n = s.toInt(&ok, 16); // 1..9, A, B, C
    if (!ok || n < 0 || n > 12)
        return Protocol::Unknown;
    return static_cast<Protocol>(n);
}

QString protocolDisplayName(Protocol p)
{
    // QT_TRANSLATE_NOOP marks the strings for lupdate; translate() looks them up at runtime.
    const char *name = nullptr;
    switch (p) {
    case Protocol::Unknown:   name = QT_TRANSLATE_NOOP("Protocol", "Unknown"); break;
    case Protocol::Auto:      name = QT_TRANSLATE_NOOP("Protocol", "Automatic (searching)"); break;
    case Protocol::J1850Pwm:  name = "SAE J1850 PWM"; break;
    case Protocol::J1850Vpw:  name = "SAE J1850 VPW"; break;
    case Protocol::Iso9141:   name = "ISO 9141-2"; break;
    case Protocol::Kwp5Baud:  name = QT_TRANSLATE_NOOP("Protocol", "ISO 14230-4 KWP (5-baud init)"); break;
    case Protocol::KwpFast:   name = QT_TRANSLATE_NOOP("Protocol", "ISO 14230-4 KWP (fast init)"); break;
    case Protocol::Can11_500: name = "ISO 15765-4 CAN (11 bit, 500 kbit/s)"; break;
    case Protocol::Can29_500: name = "ISO 15765-4 CAN (29 bit, 500 kbit/s)"; break;
    case Protocol::Can11_250: name = "ISO 15765-4 CAN (11 bit, 250 kbit/s)"; break;
    case Protocol::Can29_250: name = "ISO 15765-4 CAN (29 bit, 250 kbit/s)"; break;
    case Protocol::J1939:     name = "SAE J1939 CAN"; break;
    case Protocol::UserCan1:  name = QT_TRANSLATE_NOOP("Protocol", "User-defined CAN 1"); break;
    case Protocol::UserCan2:  name = QT_TRANSLATE_NOOP("Protocol", "User-defined CAN 2"); break;
    }
    return QCoreApplication::translate("Protocol", name);
}

} // namespace obdaryn
