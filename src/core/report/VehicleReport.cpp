// SPDX-License-Identifier: GPL-3.0-or-later

#include "report/VehicleReport.h"

#include "obd/DtcDatabase.h"
#include "obd/Pids.h"
#include "obd/VehicleInfo.h"
#include "vehicles/ModuleCatalog.h"

#include <QCoreApplication>
#include <QLocale>
#include <QMap>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>

#include <algorithm>
#include <cmath>

namespace obdaryn::VehicleReport {

namespace {

QString tr(const char *text, const char *disambiguation = nullptr, int n = -1)
{
    // lupdate files these tr() calls under the enclosing namespace, so that is the context.
    return QCoreApplication::translate("obdaryn::VehicleReport", text, disambiguation, n);
}

/// Everything from the car or a recording goes through here before it reaches the HTML.
QString esc(const QString &text) { return text.toHtmlEscaped(); }

// The palette of the app's light theme (DESIGN.md): ink colors that hold 4.5:1
// on white, so the PDF reads the same printed in black and white or in color.
// Color is only ever spent on state, and every colored word says what it means.
const char *kRed = "#B3302A";
const char *kAmber = "#9A5800";
const char *kGreen = "#17703F";
const char *kBlue = "#2A64CC";

QString colored(const char *color, const QString &text, bool bold = true)
{
    return QStringLiteral("<span style=\"color:%1;%2\">%3</span>")
        .arg(QLatin1String(color), bold ? QStringLiteral("font-weight:600") : QString(), text);
}

QString kindName(DtcKind kind)
{
    switch (kind) {
    case DtcKind::Stored: return tr("Stored");
    case DtcKind::Pending: return tr("Pending");
    case DtcKind::Permanent: return tr("Permanent");
    }
    return {};
}

const char *kindColor(DtcKind kind)
{
    return kind == DtcKind::Stored ? kRed : kind == DtcKind::Pending ? kAmber : kBlue;
}

QString likelihoodName(const QString &likelihood)
{
    if (likelihood == QLatin1String("high"))
        return tr("Likely");
    if (likelihood == QLatin1String("medium"))
        return tr("Possible");
    return tr("Rare");
}

/// Standard OBD-II addresses: 7E8 / 18DAF110 is the engine, 7E9 / 18DAF118 the transmission.
QString genericModule(const QString &ecu)
{
    if (ecu == QLatin1String("7E8") || ecu == QLatin1String("18DAF110"))
        return QStringLiteral("engine");
    if (ecu == QLatin1String("7E9") || ecu == QLatin1String("18DAF118"))
        return QStringLiteral("transmission");
    return {};
}

bool pidValue(const ReportInput &in, quint8 pid, double *value)
{
    const PidDefinition *def = PidTable::find(pid);
    const auto it = in.pids.constFind(pid);
    return def && it != in.pids.cend() && def->value(*it, value) && std::isfinite(*value);
}

QString number(double v, int decimals = 0)
{
    return QLocale().toString(v, 'f', decimals);
}

/// One "label | verdict | detail" row of the summary table.
QString summaryRow(const QString &label, const QString &verdict, const QString &detail)
{
    return QStringLiteral("<tr><td class=\"label\" width=\"27%\">%1</td><td class=\"verdict\" width=\"17%\">%2</td><td class=\"detail\">%3</td></tr>")
        .arg(label, verdict, detail);
}

QString keyValueRow(const QString &key, const QString &value, bool mono = false)
{
    if (value.isEmpty())
        return {};
    return QStringLiteral("<tr><td class=\"key\" width=\"30%\">%1</td><td%3>%2</td></tr>")
        .arg(key, value, mono ? QStringLiteral(" class=\"mono\"") : QString());
}

QString carName(const ReportInput &in)
{
    const VinInfo vin = decodeVin(in.info.vin);
    if (vin.manufacturer.isEmpty())
        return {};
    return vin.modelYear > 0 ? QStringLiteral("%1 %2").arg(vin.manufacturer).arg(vin.modelYear) : vin.manufacturer;
}

/// The explanation of one code: title, what it means, likely causes, where the text comes from.
QString explanation(const DtcInfo &info)
{
    QString out;
    out += QStringLiteral("<p class=\"title\">%1</p>")
               .arg(info.known ? esc(info.title) : tr("No description for this code yet."));
    out += QStringLiteral("<p class=\"faint\">%1</p>").arg(esc(info.system));
    if (!info.description.isEmpty())
        out += QStringLiteral("<p>%1</p>").arg(esc(info.description));
    if (!info.causes.isEmpty()) {
        QStringList items;
        for (const auto &c : info.causes)
            items << QStringLiteral("<li><span class=\"muted\">%1:</span> %2</li>").arg(likelihoodName(c.first), esc(c.second));
        out += QStringLiteral("<p class=\"sub\">%1</p><ul>%2</ul>").arg(tr("Causes"), items.join(QString()));
    }
    if (info.known) {
        const QString source = info.source == QLatin1String("generic") ? tr("obdex, a free generic code database")
                                                                        : tr("the Obdaryn %1 pack, written from public sources").arg(esc(info.brandName));
        out += QStringLiteral("<p class=\"faint\">%1</p>").arg(tr("Explanation from %1. Guidance only: confirm with a workshop before replacing parts.").arg(source));
    }
    return out;
}

// ---------------------------------------------------------------------------
// Sections

QString headerSection(const ReportInput &in)
{
    QString out;
    const QString car = carName(in);
    out += QStringLiteral("<h1>%1</h1>").arg(car.isEmpty() ? tr("Vehicle report") : esc(car));

    QStringList facts;
    if (!in.info.vin.isEmpty())
        facts << tr("VIN %1").arg(QStringLiteral("<span class=\"mono\">%1</span>").arg(esc(in.info.vin)));
    facts << tr("Obdaryn report of %1, %2").arg(QLocale().toString(in.createdAt.date(), QLocale::LongFormat),
                                                 QLocale().toString(in.createdAt.time(), QLocale::ShortFormat));
    out += QStringLiteral("<p class=\"muted\">%1</p>").arg(facts.join(QStringLiteral(" · ")));

    // Where the data came from, said plainly.
    QString source;
    switch (in.source) {
    case ReportInput::Source::Car:
        source = tr("Read from the car through the adapter at %1.").arg(esc(in.sourceDetail));
        if (!in.scanTime.isEmpty())
            source += QLatin1Char(' ') + tr("Codes read at %1.").arg(esc(in.scanTime));
        break;
    case ReportInput::Source::Demo:
        source = colored(kAmber, tr("Demo car: simulated data, not a real vehicle."));
        break;
    case ReportInput::Source::Replay:
        source = colored(kBlue, tr("Made from a recording (%1).").arg(esc(in.sourceDetail)))
                 + QLatin1Char(' ') + tr("The values are those the car gave when it was recorded.");
        break;
    }
    out += QStringLiteral("<p>%1</p>").arg(source);
    return out;
}

QString summarySection(const ReportInput &in)
{
    QString rows;

    // Check-engine light.
    if (in.readiness.valid) {
        QString detail = in.readiness.milOn ? tr("The engine computer has the warning light on.")
                                            : tr("The engine computer doesn't ask for the warning light.");
        double milKm = 0;
        if (in.readiness.milOn && pidValue(in, 0x21, &milKm) && milKm > 0)
            detail += QLatin1Char(' ') + tr("Driven %1 km with it on.").arg(number(milKm));
        rows += summaryRow(tr("Check-engine light"),
                           in.readiness.milOn ? colored(kRed, tr("On")) : colored(kGreen, tr("Off")), detail);
    }

    // Generic codes.
    int stored = 0, pending = 0, permanent = 0;
    QSet<QString> distinct;
    for (const Dtc &d : in.dtcs) {
        distinct.insert(d.code);
        (d.kind == DtcKind::Stored ? stored : d.kind == DtcKind::Pending ? pending : permanent)++;
    }
    if (distinct.isEmpty()) {
        rows += summaryRow(tr("Trouble codes"), colored(kGreen, tr("None")),
                           tr("No stored, pending or permanent code in the engine or transmission."));
    } else {
        QStringList parts;
        if (stored) parts << tr("%n stored", "", stored);
        if (pending) parts << tr("%n pending", "", pending);
        if (permanent) parts << tr("%n permanent", "", permanent);
        rows += summaryRow(tr("Trouble codes"), colored(stored ? kRed : kAmber, tr("%n code(s)", "", int(distinct.size()))),
                           parts.join(QStringLiteral(" · ")));
    }

    // Other modules.
    if (!in.modulesScanned) {
        rows += summaryRow(tr("Other modules"), colored(kAmber, tr("Not scanned")),
                           tr("ABS, airbag and the other modules weren't read for this report."));
    } else {
        int answered = 0, codes = 0;
        QStringList withCodes;
        for (const ModuleResult &m : in.modules) {
            if (m.state == ModuleResult::State::Answered) {
                ++answered;
                if (!m.dtcs.isEmpty() && m.module.category != QLatin1String("engine") && m.module.category != QLatin1String("transmission")) {
                    codes += int(m.dtcs.size());
                    withCodes << esc(m.module.displayName());
                }
            }
        }
        rows += summaryRow(tr("Other modules"),
                           codes ? colored(kRed, tr("%n code(s)", "", codes)) : colored(kGreen, tr("No codes")),
                           codes ? tr("In %1.").arg(withCodes.join(QStringLiteral(", ")))
                                 : tr("%n module(s) answered.", "", answered));
    }

    // Inspection readiness.
    if (in.readiness.valid) {
        const int incomplete = in.readiness.incompleteCount();
        rows += summaryRow(tr("Inspection readiness"),
                           in.readiness.likelyReadyForInspection() ? colored(kGreen, tr("Likely ready"))
                                                                   : colored(kAmber, tr("Not ready")),
                           incomplete ? tr("%1 of %2 self-tests haven't finished since the codes were last cleared.")
                                            .arg(incomplete).arg(in.readiness.monitors.size())
                                      : tr("Every self-test has finished."));
    }

    // Self-test results, when they were read.
    if (in.monitorTestsRead && !in.monitorTests.isEmpty()) {
        int failed = 0, close = 0;
        for (const MonitorTest &t : in.monitorTests) {
            failed += t.verdict == MonitorTest::Verdict::Failed;
            close += t.verdict == MonitorTest::Verdict::Close;
        }
        QStringList parts;
        if (failed) parts << tr("%n failed", "", failed);
        if (close) parts << tr("%n close to the limit", "", close);
        rows += summaryRow(tr("Self-test results"),
                           failed ? colored(kRed, tr("Failed")) : close ? colored(kAmber, tr("Close to a limit")) : colored(kGreen, tr("Passed")),
                           parts.isEmpty() ? tr("Every test that has run is within its limits.") : parts.join(QStringLiteral(" · ")) + QLatin1Char('.'));
    }

    // When were the codes last cleared? On a used car, "just now" deserves a question.
    double km = 0, warmups = 0, minutes = 0;
    const bool hasKm = pidValue(in, 0x31, &km), hasWarmups = pidValue(in, 0x30, &warmups), hasMinutes = pidValue(in, 0x4E, &minutes);
    if (hasKm || hasWarmups || hasMinutes) {
        QStringList parts;
        if (hasKm) parts << tr("%1 km").arg(number(km));
        if (hasWarmups) parts << tr("%n warm-up(s)", "", int(warmups));
        if (hasMinutes) parts << tr("%1 h of engine running").arg(number(minutes / 60.0, minutes < 600 ? 1 : 0));
        // Distance is the better signal when the car reports it; warm-ups otherwise.
        const bool recent = hasKm ? km < 160 : (hasWarmups && warmups < 15);
        rows += summaryRow(tr("Codes last cleared"),
                           recent ? colored(kAmber, tr("Recently")) : colored(kGreen, tr("Not recently")),
                           tr("%1 ago.").arg(parts.join(QStringLiteral(", ")))
                               + (recent ? QLatin1Char(' ') + tr("Normal after a repair. On a car for sale, ask why: clearing hides faults until the car checks itself again.")
                                         : QString()));
    } else {
        rows += summaryRow(tr("Codes last cleared"), QStringLiteral("<span class=\"faint\">%1</span>").arg(tr("Unknown")),
                           tr("This car doesn't report when its codes were cleared."));
    }

    // Battery.
    if (in.batteryVolts > 0) {
        const double v = in.batteryVolts;
        const QString detail = v >= 13.6 ? tr("Charging: the engine was running.")
                             : v >= 12.2 ? tr("Healthy with the engine off.")
                             : v >= 12.0 ? tr("Getting low.")
                                         : tr("Low.");
        rows += summaryRow(tr("Battery"), colored(v < 12.0 ? kRed : v < 12.2 ? kAmber : kGreen, tr("%1 V").arg(number(v, 1))),
                           detail + QLatin1Char(' ') + tr("Measured by the adapter at the OBD socket."));
    }

    return QStringLiteral("<h2>%1</h2><table class=\"rows\" width=\"100%\" cellspacing=\"0\" cellpadding=\"4\">%2</table>")
        .arg(tr("At a glance"), rows);
}

QString codesSection(const ReportInput &in)
{
    QString out = QStringLiteral("<h2>%1</h2>").arg(tr("Trouble codes: engine and transmission"));
    if (in.dtcs.isEmpty())
        return out + QStringLiteral("<p class=\"muted\">%1</p>").arg(tr("None. The emission-related modules report no stored, pending or permanent code."));

    // One entry per code, listing every memory it is in (P0420 can be stored and permanent).
    QMap<QString, QList<Dtc>> byCode;
    QStringList order;
    for (const Dtc &d : in.dtcs) {
        if (!byCode.contains(d.code))
            order << d.code;
        byCode[d.code] << d;
    }
    out += QStringLiteral("<table class=\"codes\" width=\"100%\" cellspacing=\"0\" cellpadding=\"5\">");
    for (const QString &code : std::as_const(order)) {
        const QList<Dtc> &list = byCode[code];
        QStringList kinds;
        QStringList ecus;
        for (const Dtc &d : list) {
            const QString k = colored(kindColor(d.kind), kindName(d.kind));
            if (!kinds.contains(k))
                kinds << k;
            const QString name = in.info.ecuNames.value(d.ecu);
            const QString ecu = name.isEmpty() ? esc(d.ecu) : QStringLiteral("%1 (%2)").arg(esc(name), esc(d.ecu));
            if (!d.ecu.isEmpty() && !ecus.contains(ecu))
                ecus << ecu;
        }
        const DtcInfo info = DtcDatabase::instance().lookup(code, in.brand, in.language, genericModule(list.first().ecu));
        out += QStringLiteral("<tr><td class=\"code mono\" width=\"14%\">%1</td><td>%2</td></tr>")
                   .arg(esc(code),
                        QStringLiteral("<p>%1%2</p>").arg(kinds.join(QStringLiteral(" · ")),
                                                          ecus.isEmpty() ? QString() : QStringLiteral(" <span class=\"muted\">· %1</span>").arg(ecus.join(QStringLiteral(", "))))
                            + explanation(info));
    }
    return out + QStringLiteral("</table>");
}

QString moduleState(const ModuleResult &m)
{
    switch (m.state) {
    case ModuleResult::State::Answered:
        return m.dtcs.isEmpty() ? colored(kGreen, tr("No codes")) : colored(kRed, tr("%n code(s)", "", int(m.dtcs.size())));
    case ModuleResult::State::Absent: return QStringLiteral("<span class=\"faint\">%1</span>").arg(tr("Not found"));
    case ModuleResult::State::ObdOnly: return QStringLiteral("<span class=\"faint\">%1</span>").arg(tr("Answers only generic OBD-II"));
    case ModuleResult::State::Error: return colored(kAmber, tr("Couldn't read codes"));
    case ModuleResult::State::Waiting:
    case ModuleResult::State::Scanning: return QStringLiteral("<span class=\"faint\">%1</span>").arg(tr("Not scanned"));
    }
    return {};
}

QString modulesSection(const ReportInput &in)
{
    QString out = QStringLiteral("<h2>%1</h2>").arg(tr("All modules"));
    if (!in.modulesScanned)
        return out + QStringLiteral("<p class=\"muted\">%1</p>").arg(tr("Not scanned. To include ABS, airbag, body and the other modules, run the module scan (All modules) before saving the report."));

    out += QStringLiteral("<table class=\"rows\" width=\"100%\" cellspacing=\"0\" cellpadding=\"4\">");
    for (const ModuleResult &m : in.modules) {
        QStringList ident;
        if (!m.partNumber.isEmpty())
            ident << tr("Part %1").arg(esc(m.partNumber));
        if (!m.softwareVersion.isEmpty())
            ident << tr("software %1").arg(esc(m.softwareVersion));
        out += QStringLiteral("<tr><td class=\"label\" width=\"34%\">%1<br><span class=\"faint mono\">%2</span></td><td class=\"verdict\" width=\"22%\">%3</td><td class=\"detail\">%4</td></tr>")
                   .arg(esc(m.module.displayName()), esc(m.module.addressText()), moduleState(m),
                        QStringLiteral("<span class=\"muted\">%1</span>").arg(ident.join(QStringLiteral(", "))));
    }
    out += QStringLiteral("</table>");

    // The codes themselves, with explanations.
    QString codes;
    for (const ModuleResult &m : in.modules) {
        if (m.state != ModuleResult::State::Answered || m.dtcs.isEmpty())
            continue;
        // Engine and transmission codes are already explained above.
        if (m.module.category == QLatin1String("engine") || m.module.category == QLatin1String("transmission"))
            continue;
        for (const ModuleDtc &d : m.dtcs) {
            QStringList flags{colored(kindColor(d.kind), kindName(d.kind))};
            if (d.activeNow)
                flags << colored(kRed, tr("Present now"));
            if (d.warningLamp)
                flags << tr("Warning light requested");
            const DtcInfo info = DtcDatabase::instance().lookup(d.code, in.brand, in.language, m.module.category);
            codes += QStringLiteral("<tr><td class=\"code mono\" width=\"14%\">%1</td><td>%2</td></tr>")
                         .arg(esc(d.displayCode()),
                              QStringLiteral("<p>%1 <span class=\"muted\">· %2</span></p>")
                                      .arg(flags.join(QStringLiteral(" · ")), esc(m.module.displayName()))
                                  + explanation(info));
        }
    }
    if (!codes.isEmpty())
        out += QStringLiteral("<h3>%1</h3><table class=\"codes\" width=\"100%\" cellspacing=\"0\" cellpadding=\"5\">%2</table>").arg(tr("Codes in other modules"), codes);
    return out;
}

QString readinessSection(const ReportInput &in)
{
    if (!in.readiness.valid)
        return {};
    QString rows;
    for (const Monitor &m : in.readiness.monitors) {
        rows += QStringLiteral("<tr><td class=\"label\" width=\"45%\">%1</td><td width=\"20%\">%2</td><td class=\"faint\">%3</td></tr>")
                    .arg(esc(m.translatedName()),
                         m.complete ? colored(kGreen, tr("Complete")) : colored(kAmber, tr("Incomplete")),
                         m.continuous ? tr("runs continuously") : tr("runs during certain drives"));
    }
    return QStringLiteral("<h2>%1</h2><p class=\"muted\">%2</p><table class=\"rows\" width=\"100%\" cellspacing=\"0\" cellpadding=\"4\">%3</table>")
        .arg(tr("Readiness monitors"),
             tr("The self-tests the car runs on its emission systems. They reset when codes are cleared and finish over a few days of normal driving. An emissions inspection (ITP) can fail a car with incomplete monitors; how many are allowed depends on the country."),
             rows);
}

QString monitorTestsSection(const ReportInput &in)
{
    if (!in.monitorTestsRead || in.monitorTests.isEmpty())
        return {};
    QList<MonitorTest> tests = in.monitorTests;
    std::stable_sort(tests.begin(), tests.end(), [](const MonitorTest &a, const MonitorTest &b) {
        return a.mid != b.mid ? a.mid < b.mid : a.tid < b.tid;
    });
    QString rows;
    for (const MonitorTest &t : std::as_const(tests)) {
        const QString unit = t.unit.isEmpty() ? QString() : QLatin1Char(' ') + esc(t.unit);
        QString value, limits;
        if (t.verdict != MonitorTest::Verdict::NotRun) {
            value = number(t.value, t.decimals) + unit;
            if (t.hasMin && t.hasMax)
                limits = tr("%1 to %2").arg(number(t.minimum, t.decimals), number(t.maximum, t.decimals)) + unit;
            else if (t.hasMax)
                limits = tr("at most %1").arg(number(t.maximum, t.decimals)) + unit;
            else if (t.hasMin)
                limits = tr("at least %1").arg(number(t.minimum, t.decimals)) + unit;
        }
        const QString verdict = t.verdict == MonitorTest::Verdict::Failed ? colored(kRed, tr("Failed"))
                              : t.verdict == MonitorTest::Verdict::Close ? colored(kAmber, tr("Close to the limit"))
                              : t.verdict == MonitorTest::Verdict::Passed ? tr("Passed")
                                                                          : QStringLiteral("<span class=\"faint\">%1</span>").arg(tr("Not run yet"));
        rows += QStringLiteral("<tr><td class=\"label\" width=\"42%\">%1<br><span class=\"faint\">%2</span></td>"
                               "<td width=\"16%\">%3</td><td class=\"faint\" width=\"22%\">%4</td><td>%5</td></tr>")
                    .arg(esc(t.monitorName()), esc(t.testName()), value, limits, verdict);
    }
    return QStringLiteral("<h2>%1</h2><p class=\"muted\">%2</p><table class=\"rows\" width=\"100%\" cellspacing=\"0\" cellpadding=\"4\">%3</table>")
        .arg(tr("Self-test results"),
             tr("The latest result of each monitor's tests, next to the limits the car judges it by. A result close to its limit points to a part that's wearing out."),
             rows);
}

QString freezeFrameSection(const ReportInput &in)
{
    if (!in.freezeFrame.available)
        return {};
    QString rows;
    for (const auto &r : in.freezeFrame.values) {
        const PidDefinition *def = PidTable::find(r.pid);
        double v = 0;
        if (!def || !def->value(r.raw, &v) || !std::isfinite(v))
            continue;
        rows += QStringLiteral("<tr><td class=\"label\" width=\"45%\">%1</td><td class=\"num\">%2 <span class=\"faint\">%3</span></td></tr>")
                    .arg(esc(def->translatedName()), number(v, def->decimals), esc(def->translatedUnit()));
    }
    if (rows.isEmpty())
        return {};
    return QStringLiteral("<h2>%1</h2><p class=\"muted\">%2</p><table class=\"rows\" width=\"100%\" cellspacing=\"0\" cellpadding=\"4\">%3</table>")
        .arg(tr("Freeze frame"),
             in.freezeFrame.dtc.isEmpty() ? tr("Sensor values saved when a fault was stored.")
                                          : tr("Sensor values saved when %1 was stored.").arg(QStringLiteral("<span class=\"mono\">%1</span>").arg(esc(in.freezeFrame.dtc))),
             rows);
}

QString vehicleSection(const ReportInput &in)
{
    const VinInfo vin = decodeVin(in.info.vin);
    QString rows;
    rows += keyValueRow(tr("VIN"), esc(in.info.vin), true);
    rows += keyValueRow(tr("Manufacturer"), esc(vin.manufacturer));
    rows += keyValueRow(tr("Model year"), vin.modelYear > 0 ? QString::number(vin.modelYear) : QString());
    rows += keyValueRow(tr("Made in"), esc(vin.region));
    if (vin.valid)
        rows += keyValueRow(tr("VIN check digit"), vin.checkDigitValid ? tr("Valid") : tr("Doesn't match (normal outside North America)"));
    rows += keyValueRow(tr("OBD standard"), esc(in.obdStandard));
    rows += keyValueRow(tr("Fuel"), esc(in.fuelType));
    rows += keyValueRow(tr("Protocol"), esc(in.protocol));
    rows += keyValueRow(tr("Adapter"), esc(in.adapter));

    QStringList addresses = in.info.ecuNames.keys();
    addresses.sort();
    QStringList ecus;
    for (const QString &a : std::as_const(addresses))
        ecus << QStringLiteral("<span class=\"mono\">%1</span> %2").arg(esc(a), esc(in.info.ecuNames.value(a)));
    rows += keyValueRow(tr("Modules (OBD-II)"), ecus.join(QStringLiteral("<br>")));
    QStringList cal;
    for (const QString &c : in.info.calibrationIds)
        cal << esc(c);
    rows += keyValueRow(tr("Calibration IDs"), cal.join(QStringLiteral("<br>")), true);
    QStringList cvn;
    for (const QString &c : in.info.cvns)
        cvn << esc(c);
    rows += keyValueRow(tr("CVNs"), cvn.join(QStringLiteral("<br>")), true);
    return QStringLiteral("<h2>%1</h2><table class=\"rows\" width=\"100%\" cellspacing=\"0\" cellpadding=\"4\">%2</table>").arg(tr("Vehicle"), rows);
}

QString aboutSection(const ReportInput &in)
{
    return QStringLiteral("<h2>%1</h2><p class=\"muted\">%2</p><p class=\"muted\">%3</p><p class=\"faint\">%4</p>")
        .arg(tr("About this report"),
             tr("This report shows what the car's computers told a standard OBD-II adapter. It can't show the mileage, accident history or faults the car hasn't detected, and it doesn't replace an inspection by a mechanic."),
             tr("Obdaryn only reads: it can't change anything in a car. Code explanations are guidance; confirm with a workshop before replacing parts."),
             tr("Made with Obdaryn %1, free and open-source software: %2").arg(esc(in.appVersion), QStringLiteral("github.com/liviu-hariton/obdaryn")));
}

// Styles: kept to what both browsers and Qt's rich-text engine understand.
// %1 = sans font stack, %2 = monospace font stack.
const char *kStyle = R"(
body { font-family: %1; font-size: 10pt; color: #15181C; }
h1 { font-size: 20pt; font-weight: 700; margin: 0 0 6px 0; }
h2 { font-size: 13pt; font-weight: 700; margin: 28px 0 8px 0; }
h3 { font-size: 11pt; font-weight: 700; margin: 20px 0 6px 0; }
p { margin: 0 0 5px 0; }
ul { margin: 2px 0 6px 0; }
.mono { font-family: %2; }
.muted { color: #545C67; }
.faint { color: #6B737E; font-size: 9pt; }
.title { font-weight: 700; margin-top: 2px; }
.sub { font-weight: 700; margin-top: 6px; }
table.rows, table.codes { border-collapse: collapse; }
table.rows td, table.codes td { border-bottom: 1px solid #DCE0E6; vertical-align: top; }
td.label { font-weight: 700; }
td.key { color: #545C67; }
td.verdict { white-space: nowrap; }
td.code { font-weight: 700; font-size: 11pt; }
)";

// Browsers only: Qt's engine would apply @media screen to the PDF too.
const char *kWebStyle = R"(
body { line-height: 1.45; }
@media screen { body { max-width: 760px; margin: 40px auto; padding: 0 20px; } }
table.rows td { padding: 6pt 12pt 6pt 0; }
table.codes td { padding: 10pt 14pt 10pt 0; }
@media print { h2 { page-break-after: avoid; } tr { page-break-inside: avoid; } }
)";

} // namespace

QString html(const ReportInput &in, Target target)
{
    const QString car = carName(in);
    QString title = tr("Obdaryn report");
    if (!car.isEmpty())
        title += QStringLiteral(" – ") + car;

    // The app ships Inter as a variable font, which PDF output can't use
    // (every weight would come out the same), so PDFs use static system faces.
    const bool pdf = target == Target::Pdf;
    QString style = QString::fromLatin1(kStyle)
                        .arg(pdf ? QStringLiteral("\"Helvetica Neue\", \"Segoe UI\", Arial, sans-serif")
                                 : QStringLiteral("Inter, -apple-system, \"Segoe UI\", Helvetica, Arial, sans-serif"),
                             pdf ? QStringLiteral("Menlo, Consolas, \"DejaVu Sans Mono\", monospace")
                                 : QStringLiteral("\"JetBrains Mono\", Menlo, Consolas, monospace"));
    if (!pdf)
        style += QLatin1String(kWebStyle);

    QString body;
    body += headerSection(in);
    body += summarySection(in);
    body += codesSection(in);
    body += modulesSection(in);
    body += readinessSection(in);
    body += monitorTestsSection(in);
    body += freezeFrameSection(in);
    body += vehicleSection(in);
    body += aboutSection(in);

    return QStringLiteral("<!DOCTYPE html>\n<html lang=\"%1\"><head><meta charset=\"utf-8\">"
                          "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                          "<title>%2</title><style>%3</style></head><body>%4</body></html>\n")
        .arg(in.language == QLatin1String("ro") ? QStringLiteral("ro") : QStringLiteral("en"), esc(title), style, body);
}

QString suggestedBaseName(const ReportInput &in)
{
    QString name = tr("Obdaryn report") + QLatin1Char(' ') + in.createdAt.toString(QStringLiteral("yyyy-MM-dd"));
    if (in.source == ReportInput::Source::Demo)
        name += QLatin1Char(' ') + tr("demo car");
    else if (const QString car = carName(in); !car.isEmpty())
        name += QLatin1Char(' ') + car;
    // Keep it a safe file name whatever the car reported.
    name.remove(QRegularExpression(QStringLiteral("[^\\w\\s.-]"), QRegularExpression::UseUnicodePropertiesOption));
    return name.simplified();
}

} // namespace obdaryn::VehicleReport
