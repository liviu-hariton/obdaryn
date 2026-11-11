// SPDX-License-Identifier: GPL-3.0-or-later

#include "recording/Recording.h"

#include "obd/Pids.h"
#include "obd/ResponseParser.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QMap>
#include <QSet>
#include <QStringList>

#include <algorithm>

namespace obdaryn {

namespace {

// lupdate files these tr() calls under the enclosing namespace, so that is the context.
QString tr(const char *text) { return QCoreApplication::translate("obdaryn::Recording", text); }

/// The data lines of a raw reply: hex bytes only (drops the echo, "SEARCHING...",
/// "NO DATA", errors), which is what ResponseParser expects.
QStringList dataLines(const QByteArray &raw)
{
    QStringList lines;
    QByteArray text = raw;
    text.replace('\n', '\r');
    for (const QByteArray &part : text.split('\r')) {
        const QString line = QString::fromLatin1(part).trimmed();
        if (line.isEmpty())
            continue;
        const bool hexOnly = std::all_of(line.begin(), line.end(), [](QChar c) {
            return c.isDigit() || (c.toUpper() >= QLatin1Char('A') && c.toUpper() <= QLatin1Char('F'))
                   || c == QLatin1Char(' ') || c == QLatin1Char(':');
        });
        if (hexOnly)
            lines << line;
    }
    return lines;
}

RecordedExchange exchangeFromJson(const QJsonObject &o)
{
    RecordedExchange e;
    e.t = qint64(o.value(QStringLiteral("t")).toDouble());
    e.target = o.value(QStringLiteral("h")).toString();
    e.command = o.value(QStringLiteral("c")).toString().toLatin1();
    e.raw = o.value(QStringLiteral("r")).toString().toLatin1();
    e.ms = o.value(QStringLiteral("ms")).toInt();
    e.timedOut = o.value(QStringLiteral("x")).toBool();
    e.snapshot = o.value(QStringLiteral("s")).toBool();
    return e;
}

} // namespace

void AdapterTarget::apply(const QByteArray &command)
{
    if (command.startsWith("ATSH")) {
        const QString h = QString::fromLatin1(command.mid(4));
        // 7DF (11-bit) and 18DB33F1 (29-bit) are the "all OBD ECUs" addresses.
        header = (h == QLatin1String("7DF") || h == QLatin1String("18DB33F1")) ? QString() : h;
    } else if (command == "ATZ" || command == "ATWS" || command == "ATD") {
        header.clear(); // resets restore the default address
    }
}

bool Recording::load(const QString &path, QString *error)
{
    auto fail = [error](const QString &message) {
        if (error)
            *error = message;
        return false;
    };
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return fail(tr("Couldn't open the file: %1").arg(file.errorString()));

    header = {};
    exchanges.clear();
    durationMs = 0;
    double end = -1;

    bool first = true;
    while (!file.atEnd()) {
        const QByteArray line = file.readLine().trimmed();
        if (line.isEmpty())
            continue;
        const QJsonObject o = QJsonDocument::fromJson(line).object();
        if (first) {
            first = false;
            if (o.value(QStringLiteral("format")).toString() != QLatin1String(kFormatName))
                return fail(tr("This isn't an Obdaryn recording."));
            if (o.value(QStringLiteral("version")).toInt() > kFormatVersion)
                return fail(tr("This recording was made by a newer Obdaryn. Update the app to open it."));
            header = o;
            continue;
        }
        if (o.contains(QStringLiteral("end"))) {
            end = o.value(QStringLiteral("end")).toDouble();
            continue;
        }
        if (!o.contains(QStringLiteral("c")))
            continue; // a line cut short by a crash, or something newer we don't know
        const RecordedExchange e = exchangeFromJson(o);
        if (e.t < 0 || e.t > kMaxDurationMs)
            continue; // impossible time: damaged or crafted
        durationMs = qMax(durationMs, e.t);
        exchanges << e;
    }
    if (first)
        return fail(tr("The file is empty."));

    // The "end" line marks when Record was stopped, shortly after the last
    // exchange. Trust it only within a minute of that: a crafted file could
    // otherwise claim a duration of millennia.
    if (end >= durationMs && end <= durationMs + 60000)
        durationMs = qint64(end);

    // A recording started before the adapter finished connecting (the probe
    // does that) has no protocol in its header; the handshake itself is in the
    // recording, so take it from there.
    if (protocol() == Protocol::Unknown || !header.contains(QStringLiteral("headers"))) {
        for (const RecordedExchange &e : std::as_const(exchanges)) {
            if (e.timedOut)
                continue;
            if (e.command == "ATDPN") {
                for (const QByteArray &line : QByteArray(e.raw).replace('\n', '\r').split('\r')) {
                    const QString text = QString::fromLatin1(line).trimmed();
                    if (!text.isEmpty() && text != QLatin1String("ATDPN"))
                        header.insert(QStringLiteral("protocol"), int(protocolFromDpn(text)));
                }
            } else if (e.command == "ATH1" && e.raw.contains("OK")) {
                header.insert(QStringLiteral("headers"), true);
            }
        }
    }
    return true;
}

bool Recording::peek(const QString &path, QJsonObject *header, qint64 *durationMs)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    const QJsonObject h = QJsonDocument::fromJson(file.readLine().trimmed()).object();
    if (h.value(QStringLiteral("format")).toString() != QLatin1String(kFormatName))
        return false;
    if (header)
        *header = h;
    if (durationMs) {
        // The duration is in the last line (or the last exchange's time when the
        // recording was cut short); read only the end of the file.
        *durationMs = 0;
        const qint64 tail = qMin<qint64>(file.size(), 4096);
        file.seek(file.size() - tail);
        const QList<QByteArray> lines = file.read(tail).split('\n');
        for (auto it = lines.crbegin(); it != lines.crend(); ++it) {
            const QJsonObject o = QJsonDocument::fromJson(it->trimmed()).object();
            if (o.contains(QStringLiteral("end"))) {
                *durationMs = qint64(o.value(QStringLiteral("end")).toDouble());
                break;
            }
            if (o.contains(QStringLiteral("t"))) {
                *durationMs = qint64(o.value(QStringLiteral("t")).toDouble());
                break;
            }
        }
    }
    if (durationMs && (*durationMs < 0 || *durationMs > kMaxDurationMs))
        *durationMs = 0; // damaged or crafted: show no length rather than an absurd one
    return true;
}

QList<Recording::PidSample> Recording::pidSamples() const
{
    QList<PidSample> samples;
    const Protocol p = protocol();
    for (const RecordedExchange &e : exchanges) {
        if (e.timedOut || !e.target.isEmpty() || !e.command.startsWith("01") || e.command.size() < 4)
            continue;
        for (const EcuMessage &m : ResponseParser::parse(dataLines(e.raw), p, headersOn())) {
            if (m.data.isEmpty() || quint8(m.data[0]) != 0x41)
                continue;
            // Walk "41 <pid> <data...> <pid> <data...>", like ObdClient::readPids.
            qsizetype i = 1;
            while (i < m.data.size()) {
                const quint8 pid = quint8(m.data[i]);
                const int len = PidTable::dataLength(pid);
                if (len < 0 || i + 1 + len > m.data.size())
                    break;
                samples << PidSample{e.t, m.ecu, pid, m.data.mid(i + 1, len), e.ms};
                i += 1 + len;
            }
        }
    }
    return samples;
}

QString Recording::toCsv() const
{
    const QList<PidSample> samples = pidSamples();

    // Several ECUs may report the same value (both engine and gearbox know the
    // vehicle speed). Use the one that reported it most often.
    QHash<quint8, QHash<QString, int>> counts;
    for (const PidSample &s : samples)
        ++counts[s.pid][s.ecu];
    QMap<quint8, QString> sourceEcu; // sorted by PID → stable column order
    for (auto it = counts.cbegin(); it != counts.cend(); ++it) {
        const PidDefinition *def = PidTable::find(it.key());
        if (!def || !def->isNumeric())
            continue;
        QString best;
        int bestCount = -1;
        for (auto e = it.value().cbegin(); e != it.value().cend(); ++e) {
            if (e.value() > bestCount || (e.value() == bestCount && e.key() < best)) {
                best = e.key();
                bestCount = e.value();
            }
        }
        sourceEcu.insert(it.key(), best);
    }

    auto quoted = [](QString text) { return QLatin1Char('"') + text.replace(QLatin1Char('"'), QLatin1String("\"\"")) + QLatin1Char('"'); };
    const QList<quint8> pids = sourceEcu.keys();
    QStringList head{quoted(tr("Time (s)"))};
    for (quint8 pid : pids) {
        const PidDefinition *def = PidTable::find(pid);
        const QString unit = def->translatedUnit();
        head << quoted(unit.isEmpty() ? def->translatedName() : QStringLiteral("%1 (%2)").arg(def->translatedName(), unit));
    }

    // One row per moment that has at least one value.
    QMap<qint64, QHash<quint8, QString>> rows;
    for (const PidSample &s : samples) {
        if (sourceEcu.value(s.pid) != s.ecu)
            continue;
        const PidDefinition *def = PidTable::find(s.pid);
        double v = 0;
        if (def->value(s.raw, &v) && std::isfinite(v))
            rows[s.t].insert(s.pid, QString::number(v, 'f', def->decimals));
    }

    QString csv = head.join(QLatin1Char(',')) + QLatin1Char('\n');
    for (auto it = rows.cbegin(); it != rows.cend(); ++it) {
        QStringList cells{QString::number(it.key() / 1000.0, 'f', 3)};
        for (quint8 pid : pids)
            cells << it.value().value(pid);
        csv += cells.join(QLatin1Char(',')) + QLatin1Char('\n');
    }
    return csv;
}

} // namespace obdaryn
