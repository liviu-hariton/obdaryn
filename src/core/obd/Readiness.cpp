// SPDX-License-Identifier: GPL-3.0-or-later

#include "obd/Readiness.h"

#include <QCoreApplication>

namespace obdaryn {

QString Monitor::translatedName() const
{
    return QCoreApplication::translate("Monitor", name);
}

int MonitorStatus::incompleteCount() const
{
    int n = 0;
    for (const Monitor &m : monitors)
        n += m.complete ? 0 : 1;
    return n;
}

bool MonitorStatus::likelyReadyForInspection() const
{
    return valid && !milOn && incompleteCount() <= 1;
}

MonitorStatus decodeMonitorStatus(const QByteArray &abcd)
{
    MonitorStatus s;
    if (abcd.size() < 4)
        return s;
    const quint8 a = quint8(abcd[0]);
    const quint8 b = quint8(abcd[1]);
    const quint8 c = quint8(abcd[2]); // non-continuous monitors: available
    const quint8 d = quint8(abcd[3]); // non-continuous monitors: 1 = NOT complete

    s.valid = true;
    s.milOn = (a & 0x80) != 0;
    s.storedDtcCount = a & 0x7F;
    s.compressionIgnition = (b & 0x08) != 0;

    // Byte B: continuous monitors. Low bits = available, bits 4–6 = not complete.
    static const char *continuous[] = {
        QT_TRANSLATE_NOOP("Monitor", "Misfire"),
        QT_TRANSLATE_NOOP("Monitor", "Fuel system"),
        QT_TRANSLATE_NOOP("Monitor", "Comprehensive components"),
    };
    for (int bit = 0; bit < 3; ++bit) {
        if (b & (1 << bit))
            s.monitors << Monitor{continuous[bit], true, true, (b & (1 << (bit + 4))) == 0};
    }

    // Bytes C/D: the meaning of each bit depends on spark vs. compression ignition.
    static const char *spark[] = {
        QT_TRANSLATE_NOOP("Monitor", "Catalyst"),
        QT_TRANSLATE_NOOP("Monitor", "Heated catalyst"),
        QT_TRANSLATE_NOOP("Monitor", "Evaporative system (EVAP)"),
        QT_TRANSLATE_NOOP("Monitor", "Secondary air system"),
        QT_TRANSLATE_NOOP("Monitor", "A/C refrigerant"),
        QT_TRANSLATE_NOOP("Monitor", "Oxygen sensor"),
        QT_TRANSLATE_NOOP("Monitor", "Oxygen sensor heater"),
        QT_TRANSLATE_NOOP("Monitor", "EGR / VVT system"),
    };
    static const char *diesel[] = {
        QT_TRANSLATE_NOOP("Monitor", "NMHC catalyst"),
        QT_TRANSLATE_NOOP("Monitor", "NOx / SCR aftertreatment"),
        nullptr,
        QT_TRANSLATE_NOOP("Monitor", "Boost pressure"),
        nullptr,
        QT_TRANSLATE_NOOP("Monitor", "Exhaust gas sensor"),
        QT_TRANSLATE_NOOP("Monitor", "Particulate filter (DPF)"),
        QT_TRANSLATE_NOOP("Monitor", "EGR / VVT system"),
    };
    const char *const *names = s.compressionIgnition ? diesel : spark;
    for (int bit = 0; bit < 8; ++bit) {
        if (!names[bit] || !(c & (1 << bit)))
            continue;
        s.monitors << Monitor{names[bit], false, true, (d & (1 << bit)) == 0};
    }
    return s;
}

} // namespace obdaryn
