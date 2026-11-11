// SPDX-License-Identifier: GPL-3.0-or-later

#include "elm327/AdapterCheck.h"

#include "elm327/ElmSession.h"

#include <QCoreApplication>
#include <QRegularExpression>

#include <memory>

namespace obdaryn {

namespace {

QString tr(const char *text)
{
    // lupdate files tr() calls in this file under the namespace.
    return QCoreApplication::translate("obdaryn::AdapterCheck", text);
}

struct Probe {
    const char *command;   ///< sent as is (allowlisted)
    const char *label;     ///< shown as "AT IGN"
    const char *name;      ///< a short name in plain words, translated in tr()
    const char *purpose;   ///< English source, translated in tr()
    bool versionCheck;     ///< a command every genuine v1.4+ knows
};

// The order matters only for the log; each is independent and read-only.
const Probe kProbes[] = {
    {"ATIGN", "AT IGN", QT_TRANSLATE_NOOP("obdaryn::AdapterCheck", "Ignition sensing"), QT_TRANSLATE_NOOP("obdaryn::AdapterCheck", "Reads the ignition input"), true},
    {"ATCS", "AT CS", QT_TRANSLATE_NOOP("obdaryn::AdapterCheck", "Network error counters"), QT_TRANSLATE_NOOP("obdaryn::AdapterCheck", "How many errors the adapter saw on the car's network (CAN)"), true},
    {"ATPPS", "AT PPS", QT_TRANSLATE_NOOP("obdaryn::AdapterCheck", "Settings list"), QT_TRANSLATE_NOOP("obdaryn::AdapterCheck", "Lists its settings"), true},
    {"ATFCSM0", "AT FC SM 0", QT_TRANSLATE_NOOP("obdaryn::AdapterCheck", "Module-scan flow control"), QT_TRANSLATE_NOOP("obdaryn::AdapterCheck", "Flow control, needed for module scans of VW-group, Renault/Dacia and GM cars"), false},
    {"ATRV", "AT RV", QT_TRANSLATE_NOOP("obdaryn::AdapterCheck", "Battery voltage"), QT_TRANSLATE_NOOP("obdaryn::AdapterCheck", "Battery voltage at the OBD socket"), false},
};

struct State {
    AdapterCheck::Result result;
    std::function<void(const AdapterCheck::Result &)> done;
    int pending = 0;
};

QString firstLine(const ElmSession::Reply &r)
{
    return r.lines.isEmpty() ? r.error.trimmed() : r.lines.first().trimmed();
}

} // namespace

AdapterCheck::Verdict AdapterCheck::verdictFor(const QString &version, const QList<Item> &items)
{
    if (version.isEmpty())
        return Verdict::Unknown;
    // Elm Electronics released 1.0–1.4b and 2.x; "1.5" and "1.6" exist only on clones.
    if (version.startsWith(QLatin1String("1.5")) || version.startsWith(QLatin1String("1.6")))
        return Verdict::Compatible;
    const bool claimsModern = version >= QLatin1String("1.4") || version.startsWith(QLatin1Char('2'));
    if (claimsModern) {
        for (const Item &i : items) {
            if (!i.supported && (i.command == QLatin1String("AT IGN") || i.command == QLatin1String("AT CS")
                                 || i.command == QLatin1String("AT PPS")))
                return Verdict::Compatible;
        }
    }
    return Verdict::Genuine;
}

QString AdapterCheck::Result::summary() const
{
    // lupdate files tr() calls in this member under the Result class: match it.
    auto tr = [](const char *text) { return QCoreApplication::translate("obdaryn::AdapterCheck::Result", text); };
    switch (verdict) {
    case Verdict::Genuine:
        return tr("Behaves like a genuine ELM327 %1: it knows every command Obdaryn tried.").arg(version);
    case Verdict::Compatible:
        return version.startsWith(QLatin1String("1.5"))
                   ? tr("An ELM327-compatible adapter. It reports v1.5, a version Elm Electronics never made, which is common and usually fine. What it supports is listed below.")
                   : tr("An ELM327-compatible adapter: it reports v%1 but lacks some commands of that version. That's common and usually fine. What it supports is listed below.").arg(version);
    case Verdict::Unknown:
        return tr("This adapter doesn't identify itself as an ELM327, so Obdaryn may not work fully with it.");
    }
    return {};
}

void AdapterCheck::run(ElmSession *session, std::function<void(const Result &)> done)
{
    auto st = std::make_shared<State>();
    st->done = std::move(done);

    auto finishOne = [st] {
        if (--st->pending > 0)
            return;
        Result &r = st->result;
        r.verdict = verdictFor(r.version, r.items);
        for (const Item &i : std::as_const(r.items)) {
            if (i.command == QLatin1String("AT FC SM 0"))
                r.flowControl = i.supported;
        }
        st->done(r);
    };

    // Identification first; the session queue keeps the order.
    st->pending = 3 + int(std::size(kProbes));
    session->send("ATI", [st, finishOne](const ElmSession::Reply &r) {
        st->result.identification = firstLine(r);
        const QRegularExpressionMatch m = QRegularExpression(QStringLiteral("ELM327\\s*v?(\\d+\\.\\d+\\w*)"),
                                                             QRegularExpression::CaseInsensitiveOption)
                                              .match(st->result.identification);
        if (m.hasMatch())
            st->result.version = m.captured(1);
        finishOne();
    });
    session->send("AT@1", [st, finishOne](const ElmSession::Reply &r) {
        if (r.ok())
            st->result.description = firstLine(r);
        finishOne();
    });
    session->send("AT@2", [st, finishOne](const ElmSession::Reply &r) {
        // Most adapters have no identifier stored and answer "?".
        if (r.ok() && firstLine(r) != QLatin1String("?"))
            st->result.deviceId = firstLine(r);
        finishOne();
    });
    for (const Probe &p : kProbes) {
        session->send(p.command, [st, finishOne, p](const ElmSession::Reply &r) {
            Item item;
            item.command = QString::fromLatin1(p.label);
            item.purpose = tr(p.purpose);
            item.name = tr(p.name);
            item.supported = r.ok() && !r.lines.isEmpty() && firstLine(r) != QLatin1String("?");
            item.answer = firstLine(r);
            st->result.items << item;
            finishOne();
        });
    }
}

} // namespace obdaryn
