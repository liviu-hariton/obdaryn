// SPDX-License-Identifier: GPL-3.0-or-later
//
// The shareable vehicle report: everything Obdaryn read, as one document.

#pragma once

#include "obd/ModuleScanner.h"
#include "obd/MonitorTests.h"
#include "obd/ObdClient.h"
#include "obd/Readiness.h"

#include <QDateTime>
#include <QHash>
#include <QList>
#include <QString>

namespace obdaryn {

/**
 * @brief What a report is made of. Filled by the app (or the probe) from the
 *        last scan; the builder only formats it.
 */
struct ReportInput
{
    /// Where the data came from. The report says so at the top, because a
    /// report made from the demo car or a replay must never pass for a live reading.
    enum class Source { Car, Demo, Replay };
    Source source = Source::Car;
    QString sourceDetail;      ///< Car: the adapter address; Replay: "Hyundai 2014, 11/11/26 14:03"

    QString appVersion;
    QDateTime createdAt;       ///< when the report was made
    QString scanTime;          ///< when the codes were read, localized ("14:03")
    QString brand;             ///< vehicle profile / brand pack id, e.g. "hyundai-kia"
    QString language;          ///< "en" / "ro": language of the code explanations

    QString adapter;           ///< "ELM327 v1.5"
    QString protocol;          ///< "ISO 15765-4 CAN (11 bit, 500 kbit/s)"
    double batteryVolts = 0;   ///< measured by the adapter at the OBD socket, 0 = unknown

    ObdClient::VehicleInformation info;
    QString obdStandard;
    QString fuelType;

    MonitorStatus readiness;
    QList<Dtc> dtcs;           ///< generic OBD-II codes (stored, pending, permanent)
    ObdClient::FreezeFrame freezeFrame;

    /// The trip running when the report was made (TripComputer::Stats, flattened
    /// so the report doesn't depend on the app). Shown when distanceKm >= 0.1.
    struct Trip {
        double distanceKm = 0;
        double fuelLiters = 0;
        bool fuelKnown = false;
        qint64 durationMs = 0, movingMs = 0, idleMs = 0;
        double averageSpeedKmh = 0, maxSpeedKmh = 0;
        QString method;        ///< "fuelRate", "maf", "speedDensity", ...
        int calibrationFillUps = 0;
        QString cost;          ///< "12.40 RON", or empty
    } trip;

    bool monitorTestsRead = false;  ///< service 06 results were read for this report
    QList<MonitorTest> monitorTests;

    bool modulesScanned = false;
    QList<ModuleResult> modules;

    /// Latest raw value of service 01 PIDs read for the report: the
    /// "since codes were cleared" counters (0x21, 0x30, 0x31, 0x4D, 0x4E).
    QHash<quint8, QByteArray> pids;
};

namespace VehicleReport {

/**
 * @brief The report as a standalone HTML document (UTF-8, no external files).
 *
 * The same HTML is saved as a web page and rendered to PDF by the app, so it
 * keeps to what both a browser and Qt's rich-text engine understand: tables,
 * paragraphs, headings and simple CSS. Every value from the car or a recording
 * is HTML-escaped.
 */
enum class Target {
    Web, ///< a web page: Inter where installed, wider layout on screens
    Pdf, ///< for the app's PDF renderer: static system fonts, no screen-only rules
};
QString html(const ReportInput &input, Target target = Target::Web);

/// A file name for the report, without extension: "Obdaryn report 2026-11-11 Hyundai 2014".
QString suggestedBaseName(const ReportInput &input);

/// The PDF target refers to the logo by this image name; the PDF writer
/// supplies the picture (Qt's rich-text engine can't draw SVG on its own).
/// The web target embeds the logo in the page instead.
inline constexpr const char *kLogoImageName = "obdaryn-logo";
/// The logo mark as SVG: the full drawing, or the simplified one for small sizes.
QByteArray logoSvg(bool small = false);

} // namespace VehicleReport

} // namespace obdaryn
