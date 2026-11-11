// SPDX-License-Identifier: GPL-3.0-or-later
//
// The bridge between the QML user interface and the diagnostics core.

#pragma once

#include "models/ChartModel.h"
#include "models/DtcModel.h"
#include "models/LiveDataModel.h"
#include "models/ManufacturerValuesModel.h"
#include "models/ModuleScanModel.h"
#include "models/TrafficLogModel.h"
#include "UpdateChecker.h"
#include "obd/ObdClient.h"
#include "trip/TripComputer.h"

#include <QElapsedTimer>
#include <QDateTime>
#include <QJsonObject>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QTranslator>
#include <QUrl>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

class QFileInfo;

namespace obdaryn {

class SimulatedTransport;
class ConnectionKeeper;
class ReplayTransport;
class ModuleDiscovery;
class SessionRecorder;

/**
 * @brief Everything the screens need, exposed as one QML singleton "App".
 *
 * QML reads properties (App.connectionState, App.vin, App.dtcs, ...) and calls
 * the invokable methods (App.connectToAdapter(), App.clearCodes(), ...).
 * The controller sequences the work on the car:
 *
 *  1. connect → the ELM327 initializes → the car answers "0100"
 *  2. a first scan: vehicle info, readiness, trouble codes, freeze frame
 *  3. live data polling runs continuously in between, at low priority
 */
class AppController : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(App)
    QML_SINGLETON

    // Connection
    Q_PROPERTY(ConnectionState connectionState READ connectionState NOTIFY connectionChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY connectionChanged)
    Q_PROPERTY(bool demoMode READ demoMode NOTIFY connectionChanged)
    Q_PROPERTY(bool demoEngineRunning READ demoEngineRunning WRITE setDemoEngineRunning NOTIFY demoEngineRunningChanged)
    /// Demo car: ignition ON. Off, the car stops answering, and Obdaryn waits for it.
    Q_PROPERTY(bool demoIgnitionOn READ demoIgnitionOn WRITE setDemoIgnitionOn NOTIFY demoEngineRunningChanged)
    Q_PROPERTY(QString adapterHost READ adapterHost NOTIFY settingsChanged)
    Q_PROPERTY(int adapterPort READ adapterPort NOTIFY settingsChanged)
    Q_PROPERTY(QString adapterVersion READ adapterVersion NOTIFY adapterInfoChanged)
    Q_PROPERTY(QString adapterDescription READ adapterDescription NOTIFY adapterInfoChanged)
    Q_PROPERTY(QString transportDescription READ transportDescription NOTIFY adapterInfoChanged)
    Q_PROPERTY(QString protocolName READ protocolName NOTIFY adapterInfoChanged)
    Q_PROPERTY(double batteryVoltage READ batteryVoltage NOTIFY batteryVoltageChanged)

    // Scan progress
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(QString scanStep READ scanStep NOTIFY scanningChanged)
    Q_PROPERTY(QString lastScanTime READ lastScanTime NOTIFY scanFinished)

    // Vehicle
    Q_PROPERTY(QString vin READ vin NOTIFY vehicleChanged)
    Q_PROPERTY(QString manufacturer READ manufacturer NOTIFY vehicleChanged)
    Q_PROPERTY(int modelYear READ modelYear NOTIFY vehicleChanged)
    Q_PROPERTY(QString vinRegion READ vinRegion NOTIFY vehicleChanged)
    Q_PROPERTY(bool vinCheckDigitValid READ vinCheckDigitValid NOTIFY vehicleChanged)
    Q_PROPERTY(QStringList calibrationIds READ calibrationIds NOTIFY vehicleChanged)
    Q_PROPERTY(QStringList cvns READ cvns NOTIFY vehicleChanged)
    Q_PROPERTY(QVariantList modules READ modules NOTIFY vehicleChanged)
    Q_PROPERTY(QString obdStandard READ obdStandard NOTIFY vehicleChanged)
    Q_PROPERTY(QString fuelType READ fuelType NOTIFY vehicleChanged)

    // Readiness
    Q_PROPERTY(bool readinessValid READ readinessValid NOTIFY readinessChanged)
    Q_PROPERTY(bool milOn READ milOn NOTIFY readinessChanged)
    Q_PROPERTY(QVariantList monitors READ monitors NOTIFY readinessChanged)
    Q_PROPERTY(int incompleteMonitors READ incompleteMonitors NOTIFY readinessChanged)
    Q_PROPERTY(bool likelyReadyForInspection READ likelyReadyForInspection NOTIFY readinessChanged)
    Q_PROPERTY(bool dieselEngine READ dieselEngine NOTIFY readinessChanged)

    // On-board monitor test results (service 06)
    /// Grouped per monitor: [{monitor, verdict, tests: [{name, value, limits, verdict,
    /// position, minPosition, maxPosition}]}]; verdict is "passed"/"close"/"failed"/"notRun".
    Q_PROPERTY(QVariantList monitorTests READ monitorTests NOTIFY monitorTestsChanged)
    Q_PROPERTY(bool monitorTestsRead READ monitorTestsRead NOTIFY monitorTestsChanged)
    Q_PROPERTY(bool readingMonitorTests READ readingMonitorTests NOTIFY monitorTestsChanged)
    Q_PROPERTY(QString monitorTestsError READ monitorTestsError NOTIFY monitorTestsChanged)

    // Freeze frame
    Q_PROPERTY(bool freezeFrameAvailable READ freezeFrameAvailable NOTIFY freezeFrameChanged)
    Q_PROPERTY(QString freezeFrameDtc READ freezeFrameDtc NOTIFY freezeFrameChanged)
    Q_PROPERTY(QVariantList freezeFrameValues READ freezeFrameValues NOTIFY freezeFrameChanged)

    // Models
    Q_PROPERTY(obdaryn::LiveDataModel *liveData READ liveData CONSTANT)
    Q_PROPERTY(obdaryn::DtcModel *dtcs READ dtcs CONSTANT)
    Q_PROPERTY(obdaryn::TrafficLogModel *trafficLog READ trafficLog CONSTANT)
    Q_PROPERTY(obdaryn::ModuleScanModel *moduleScan READ moduleScan CONSTANT)
    Q_PROPERTY(obdaryn::ChartModel *charts READ charts CONSTANT)
    Q_PROPERTY(obdaryn::ManufacturerValuesModel *manufacturerValues READ manufacturerValues CONSTANT)

    // Manufacturer values (OBDb data packs): which model, and what the car answers.
    /// {available, make, models, model, year, yearFromVin, state, progress, total,
    ///  answered, attribution}. state: "choose" (no model yet), "ready" (model
    ///  chosen, not asked yet), "finding", "found", "none" (nothing answered).
    Q_PROPERTY(QVariantMap manufacturerSetup READ manufacturerSetup NOTIFY manufacturerSetupChanged)
    /// True while the manufacturer values are on screen: only then are they polled.
    Q_PROPERTY(bool manufacturerActive READ manufacturerActive WRITE setManufacturerActive NOTIFY manufacturerActiveChanged)

    // Full-vehicle module scan
    Q_PROPERTY(QVariantList vehicleProfiles READ vehicleProfiles CONSTANT)
    /// What Obdaryn knows per brand, for Help: [{brand, lines: [{label, text}]}],
    /// built from the shipped data (module lists, code packs, manufacturer values).
    Q_PROPERTY(QVariantList supportedCars READ supportedCars NOTIFY languageChanged)
    Q_PROPERTY(QString profileId READ profileId WRITE setProfileId NOTIFY profileChanged)
    Q_PROPERTY(QString profileNotes READ profileNotes NOTIFY profileChanged)
    Q_PROPERTY(bool profileChosenByUser READ profileChosenByUser NOTIFY profileChanged)
    Q_PROPERTY(bool moduleScanRunning READ moduleScanRunning NOTIFY moduleScanChanged)
    Q_PROPERTY(bool moduleScanDone READ moduleScanDone NOTIFY moduleScanChanged)
    Q_PROPERTY(QString moduleScanTime READ moduleScanTime NOTIFY moduleScanChanged)
    /// Profile the current module results came from (its brand pack explains their codes).
    Q_PROPERTY(QString moduleScanProfile READ moduleScanProfile NOTIFY moduleScanChanged)
    /// The current module results come from searching every address, not a brand list.
    Q_PROPERTY(bool moduleScanSearched READ moduleScanSearched NOTIFY moduleScanChanged)

    // Searching every address for modules no list knows (ModuleDiscovery)
    Q_PROPERTY(bool discoveryRunning READ discoveryRunning NOTIFY discoveryChanged)
    Q_PROPERTY(int discoveryDone READ discoveryDone NOTIFY discoveryChanged)
    Q_PROPERTY(int discoveryTotal READ discoveryTotal NOTIFY discoveryChanged)
    /// The address being asked now, "7A4" (empty when not searching).
    Q_PROPERTY(QString discoveryAddress READ discoveryAddress NOTIFY discoveryChanged)
    /// Addresses that answered so far, ["7D1 → 7D9", ...].
    Q_PROPERTY(QStringList discoveryFound READ discoveryFound NOTIFY discoveryChanged)

    // Recording and replay
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    /// Length of the running recording, "1:23".
    Q_PROPERTY(QString recordingTime READ recordingTime NOTIFY recordingTick)
    Q_PROPERTY(bool replayMode READ replayMode NOTIFY connectionChanged)
    /// Where the replay is, "1:23 of 4:10".
    Q_PROPERTY(QString replayPosition READ replayPosition NOTIFY recordingTick)
    /// Saved recordings, newest first: {path, title, detail}.
    Q_PROPERTY(QVariantList recordings READ recordings NOTIFY recordingsChanged)
    /// The folder they are saved in, for display ("~/Documents/Obdaryn/Recordings").
    Q_PROPERTY(QString recordingsFolder READ recordingsFolderDisplay CONSTANT)

    // Adapter health
    /// What the adapter supports (filled after the first scan): identification, version,
    /// description, deviceId, verdict ("genuine"/"compatible"/"unknown"), summary,
    /// items [{command, purpose, supported, answer}], done.
    Q_PROPERTY(QVariantMap adapterCheck READ adapterCheck NOTIFY adapterCheckChanged)
    /// Live connection numbers: commands, timeouts, errors, averageMs, p95Ms,
    /// multiPid, responseCounts ("unknown"/"working"/"notSupported"/"off").
    Q_PROPERTY(QVariantMap adapterHealth READ adapterHealth NOTIFY adapterHealthChanged)
    /// Why the last connection attempt failed, when the UI can help: "unreachable"
    /// (wrong Wi-Fi or macOS Local Network permission), "timeout", "refused", or "".
    Q_PROPERTY(QString connectionHint READ connectionHint NOTIFY connectionChanged)

    // Trip computer
    /// The running trip: distanceKm, fuelLiters, averageL100, instantL100, instantLph,
    /// averageSpeed, maxSpeed, duration, drivingTime, idleTime, startedAt, fuelKnown,
    /// method, engineLiters, calibration, calibrationFillUps, fillUpStarted,
    /// sinceFillUpLiters, saved (trips of this car are kept), active.
    Q_PROPERTY(QVariantMap trip READ trip NOTIFY tripChanged)
    /// This car's last trips, newest first: {when, distanceKm, fuelLiters, averageL100, duration, fuelKnown}.
    Q_PROPERTY(QVariantList tripHistory READ tripHistory NOTIFY tripHistoryChanged)

    /// "metric", "us" or "uk": how distances, speeds, temperatures, pressures and fuel are shown.
    Q_PROPERTY(QString units READ units WRITE setUnits NOTIFY unitsChanged)

    // App
    Q_PROPERTY(QString language READ language NOTIFY languageChanged)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    /// The opt-in check for a newer version (Settings, and Obdaryn → Check for Updates).
    Q_PROPERTY(obdaryn::UpdateChecker *updates READ updates CONSTANT)
    /// "Qt 6.12.0 · macOS 27.0": for the About window and bug reports.
    Q_PROPERTY(QString buildInfo READ buildInfo CONSTANT)
    /// The copyright years: "2026", or "2026–2028" for a build made in 2028.
    Q_PROPERTY(QString copyrightYears READ copyrightYears CONSTANT)
    Q_PROPERTY(QString dtcAttribution READ dtcAttribution CONSTANT)
    Q_PROPERTY(int dtcDatabaseSize READ dtcDatabaseSize CONSTANT)

public:
    /// The session's states, plus Reconnecting while the keeper brings a lost link back.
    enum class ConnectionState { Disconnected, Connecting, Initializing, NoVehicle, Ready, Failed, Reconnecting };
    Q_ENUM(ConnectionState)

    explicit AppController(QQmlEngine *engine, QObject *parent = nullptr);
    ~AppController() override;

    /// Factory used by the QML engine for the singleton.
    static AppController *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    /// How a recording is named on screen, from its header: the car ("Hyundai 2014",
    /// "Demo car", or the file name) and when it was recorded, in the user's locale.
    static QPair<QString, QString> recordingLabel(const QJsonObject &header, const QFileInfo &file);

    // ---- Actions callable from QML -------------------------------------------
    Q_INVOKABLE void connectToAdapter(const QString &host, int port);
    Q_INVOKABLE void connectDemo();
    Q_INVOKABLE void disconnectAdapter();
    Q_INVOKABLE void retryVehicle();
    Q_INVOKABLE void runFullScan();
    Q_INVOKABLE void refreshCodes();
    Q_INVOKABLE void refreshReadiness();
    Q_INVOKABLE void refreshFreezeFrame();
    /// Reads the monitor test results (service 06) on request.
    Q_INVOKABLE void readMonitorTests();
    /// Clears codes. QML must have shown the confirmation first; the result
    /// arrives through clearFinished().
    Q_INVOKABLE void clearCodes();
    /// Everything known about one code, for the explanation panel. Uses the
    /// current vehicle profile's brand pack, unless `brand` names another one.
    Q_INVOKABLE QVariantMap explainCode(const QString &code, const QString &brand = QString(),
                                        const QString &module = QString()) const;
    /// Number of codes described by the current profile's brand pack.
    Q_INVOKABLE int brandCodeCount() const;
    Q_INVOKABLE void setLanguage(const QString &code);
    Q_INVOKABLE void copyToClipboard(const QString &text) const;

    /// Scans every module of the current profile (ABS, airbag, body...).
    Q_INVOKABLE void scanModules();
    /**
     * Searches every 11-bit diagnostic address for a module (about a minute),
     * then scans the codes of whatever answered. For modules the brand's list
     * doesn't know. Sends only AT SH and TesterPresent (3E 00) while searching.
     */
    Q_INVOKABLE void discoverModules();
    /// Stops the module scan or the address search.
    Q_INVOKABLE void cancelModuleScan();
    /// Clears one module's codes after the user confirmed; result via moduleClearFinished().
    Q_INVOKABLE void clearModuleCodes(int row);

    /// Starts recording the connected session into the recordings folder.
    Q_INVOKABLE void startRecording();
    Q_INVOKABLE void stopRecording();
    /// Connects to a recording instead of a car. Takes a path or a file:// URL.
    Q_INVOKABLE void replayRecording(const QString &pathOrUrl);
    /// Writes the recording's live data as CSV next to it.
    Q_INVOKABLE void exportRecordingCsv(const QString &pathOrUrl);
    Q_INVOKABLE void showRecordingsFolder() const;
    Q_INVOKABLE void refreshRecordings();

    /// Opens macOS System Settings at Privacy & Security → Local Network.
    Q_INVOKABLE void openLocalNetworkSettings() const;

    /// Saves the current trip to the history and starts a new one.
    Q_INVOKABLE void newTrip();
    /// The tank was just filled to the click with this amount (liters, or gallons
    /// in US/UK units); learns the calibration (see TripComputer).
    Q_INVOKABLE void recordFillUp(double amount);
    Q_INVOKABLE void resetFuelCalibration();
    /// Engine size in liters, needed by cars without an air-flow sensor.
    Q_INVOKABLE void setEngineLiters(double liters);
    /// Fuel price in the shown volume unit (per liter, or per gallon in US/UK units),
    /// and the currency text to show next to costs. 0 removes the price.
    Q_INVOKABLE void setFuelPrice(double pricePerShownUnit, const QString &currency);

    /// Where the save dialog starts: Documents/Obdaryn/Reports/<date> <car>.pdf
    Q_INVOKABLE QUrl suggestedReportUrl() const;
    /**
     * @brief Saves the shareable report: PDF, or a web page when the name ends in .html.
     *
     * Reads the "since codes were cleared" counters fresh, then writes the
     * report from the last scan (and module scan, if one ran). Opens the file
     * afterwards unless `open` is false. The result arrives through notice().
     */
    Q_INVOKABLE void saveReport(const QString &pathOrUrl, bool open = true);

    // ---- Property getters ------------------------------------------------------
    ConnectionState connectionState() const;
    QString statusMessage() const { return m_statusMessage; }
    bool demoMode() const { return m_demo; }
    bool demoEngineRunning() const;
    void setDemoEngineRunning(bool running);
    bool demoIgnitionOn() const { return m_demoIgnitionOn; }
    void setDemoIgnitionOn(bool on);
    /// Developer capture mode: the demo adapter's link drops, to show reconnecting.
    void simulateLinkDrop();
    QString adapterHost() const;
    int adapterPort() const;
    QString adapterVersion() const { return m_client->session()->adapterVersion(); }
    QString adapterDescription() const { return m_client->session()->adapterDescription(); }
    QString transportDescription() const;
    QString protocolName() const;
    double batteryVoltage() const { return m_batteryVoltage; }

    bool scanning() const { return !m_scanStep.isEmpty(); }
    QString scanStep() const { return m_scanStep; }
    QString lastScanTime() const { return m_lastScanTime; }

    QString vin() const { return m_vin; }
    QString manufacturer() const { return m_manufacturer; }
    int modelYear() const { return m_modelYear; }
    QString vinRegion() const { return m_vinRegion; }
    bool vinCheckDigitValid() const { return m_vinCheckDigitValid; }
    QStringList calibrationIds() const { return m_info.calibrationIds; }
    QStringList cvns() const { return m_info.cvns; }
    QVariantList modules() const;
    QString obdStandard() const { return m_obdStandard; }
    QString fuelType() const { return m_fuelType; }

    bool readinessValid() const { return m_readiness.valid; }
    bool milOn() const { return m_readiness.milOn; }
    QVariantList monitors() const;
    int incompleteMonitors() const { return m_readiness.incompleteCount(); }
    bool likelyReadyForInspection() const { return m_readiness.likelyReadyForInspection(); }
    bool dieselEngine() const { return m_readiness.compressionIgnition; }

    QVariantList monitorTests() const;
    bool monitorTestsRead() const { return m_monitorTestsRead; }
    bool readingMonitorTests() const { return m_readingMonitorTests; }
    QString monitorTestsError() const { return m_monitorTestsError; }
    const QList<MonitorTest> &monitorTestList() const { return m_monitorTests; }

    bool freezeFrameAvailable() const { return m_freeze.available; }
    QString freezeFrameDtc() const { return m_freeze.dtc; }
    QVariantList freezeFrameValues() const;

    LiveDataModel *liveData() const { return m_liveData; }
    DtcModel *dtcs() const { return m_dtcs; }
    TrafficLogModel *trafficLog() const { return m_trafficLog; }
    ModuleScanModel *moduleScan() const { return m_moduleModel; }
    ChartModel *charts() const { return m_charts; }
    ManufacturerValuesModel *manufacturerValues() const { return m_manufacturerValues; }
    QVariantMap manufacturerSetup() const;
    bool manufacturerActive() const { return m_manufacturerActive; }
    void setManufacturerActive(bool active);
    /// Chooses the car's model for manufacturer values (remembered per VIN).
    Q_INVOKABLE void setManufacturerModel(const QString &model);
    /// Sets the model year when the VIN doesn't tell it.
    Q_INVOKABLE void setManufacturerYear(int year);
    /// Asks every module once which of the model's values it answers.
    Q_INVOKABLE void findManufacturerValues();

    QVariantList vehicleProfiles() const;
    QVariantList supportedCars() const;
    QString profileId() const { return m_profileId; }
    void setProfileId(const QString &id);
    QString profileNotes() const;
    bool profileChosenByUser() const { return m_profileChosenByUser; }
    bool moduleScanRunning() const { return busyWithModules(); }
    bool moduleScanDone() const { return !m_moduleScanTime.isEmpty(); }
    QString moduleScanTime() const { return m_moduleScanTime; }
    QString moduleScanProfile() const { return m_moduleScanner->profileId(); }
    bool moduleScanSearched() const { return m_moduleScanSearched; }
    bool discoveryRunning() const;
    int discoveryDone() const;
    int discoveryTotal() const;
    QString discoveryAddress() const;
    QStringList discoveryFound() const;

    bool recording() const;
    QString recordingTime() const;
    bool replayMode() const { return !m_replay.isNull(); }
    QString replayPosition() const;
    QVariantList recordings() const { return m_recordings; }
    QString recordingsFolderDisplay() const;
    QVariantMap trip() const { return m_tripCache; }
    QVariantMap adapterCheck() const { return m_adapterCheck; }
    QVariantMap adapterHealth() const;
    QString connectionHint() const { return m_connectionHint; }
    QVariantList tripHistory() const { return m_tripHistory; }

    QString units() const;
    void setUnits(const QString &units);
    QString language() const { return m_language; }
    QString appVersion() const;
    UpdateChecker *updates() const { return m_updates; }
    QString buildInfo() const;
    QString copyrightYears() const;
    QString dtcAttribution() const;
    int dtcDatabaseSize() const;

signals:
    void connectionChanged();
    void demoEngineRunningChanged();
    void settingsChanged();
    void adapterInfoChanged();
    void batteryVoltageChanged();
    void scanningChanged();
    void scanFinished();
    void vehicleChanged();
    void readinessChanged();
    void freezeFrameChanged();
    void monitorTestsChanged();
    void languageChanged();
    void unitsChanged();
    /// Result of clearCodes(): success flag and a message for the user.
    void clearFinished(bool success, const QString &message);
    void moduleClearFinished(bool success, const QString &message);
    void profileChanged();
    void moduleScanChanged();
    void discoveryChanged();
    void recordingChanged();
    void recordingTick();
    void recordingsChanged();
    void tripChanged();
    void adapterCheckChanged();
    void adapterHealthChanged();
    void tripHistoryChanged();
    void manufacturerSetupChanged();
    void manufacturerActiveChanged();
    /// A short message for the toast (recording saved, export written, ...).
    void notice(bool success, const QString &message);

private:
    void startSession(Transport *transport, bool demo);
    void onKeeperChanged();
    void runAdapterCheck();
    void onSessionStateChanged();
    void setScanStep(const QString &step);
    void schedulePoll();
    void pollLiveData();
    void resetVehicleData();
    /// Loads the remembered manufacturer setup for this car (model, year, values it answers).
    void loadManufacturerSetup();
    /// The chosen model's commands for the car's year, or empty.
    QList<ManufacturerCommand> manufacturerCommands() const;
    /// Readable names of the modules, from the brand's module list ("Engine", "ABS / ESC").
    QHash<quint16, QString> manufacturerModuleNames() const;
    /// Reads the answered commands that are due (OBDb's interval); false if none is.
    bool pollManufacturerValues();
    QString vehicleSettingsGroup() const;
    void readStatusPids();
    void readVoltage();
    void pickProfileForVehicle();
    /// Runs `action` once no live-data poll is in flight. Module work changes the
    /// adapter's target address, so it must never interleave with a poll.
    void whenPollIdle(std::function<void()> action);
    bool busyWithModules() const { return m_moduleScanner->running() || discoveryRunning() || bool(m_afterPoll); }
    /// Live polling restarts once module work is over (if the car is still there).
    void resumePolling();
    /// The adapter is pointed at one module (module work or a manufacturer read):
    /// requests meant for all OBD-II ECUs must wait.
    bool adapterBusy() const { return busyWithModules() || m_manufacturerReading; }
    QString recordingsFolder() const;
    QString reportsFolder() const;
    // Trip computer
    /// Live data polls the trip's and the charts' inputs on every cycle.
    void updateRequiredPids();
    void startTrip();
    void endTrip();
    void feedTrip(const QList<ObdClient::PidReading> &readings);
    void bankTripFuel();
    /// Trips are saved (history, fill-up calibration) only for a real car.
    /// OBDARYN_DEMO_TRIPS=1 (developer capture mode) saves the demo car's too, under its own key.
    bool tripSaved() const
    {
        if (m_replay || m_vin.isEmpty())
            return false;
        return !m_demo || qEnvironmentVariableIntValue("OBDARYN_DEMO_TRIPS") == 1;
    }
    QString tripKey() const;
    void loadTripHistory();
    QVariantMap buildTripMap() const;
    void refreshTrip();
    void updateTicker();

    QQmlEngine *m_engine;
    ObdClient *m_client;
    LiveDataModel *m_liveData;
    DtcModel *m_dtcs;
    TrafficLogModel *m_trafficLog;
    QPointer<SimulatedTransport> m_simulator;
    QTranslator m_appTranslator;
    QTranslator m_qtTranslator;

    bool m_demo = false;
    bool m_demoIgnitionOn = true;
    QString m_statusMessage;
    double m_batteryVoltage = 0;
    QString m_scanStep;
    QString m_lastScanTime;

    QString m_vin, m_manufacturer, m_vinRegion, m_obdStandard, m_fuelType;
    int m_modelYear = 0;
    bool m_vinCheckDigitValid = false;
    ObdClient::VehicleInformation m_info;
    MonitorStatus m_readiness;
    ObdClient::FreezeFrame m_freeze;
    QList<MonitorTest> m_monitorTests;
    bool m_monitorTestsRead = false;
    bool m_readingMonitorTests = false;
    QString m_monitorTestsError;

    QTimer m_pollTimer;
    QTimer m_voltageTimer;
    bool m_pollInFlight = false;
    bool m_pollingEnabled = false;
    int m_samplesThisSecond = 0;
    QElapsedTimer m_rateClock;

    QString m_language;

    ModuleScanner *m_moduleScanner;
    ModuleScanModel *m_moduleModel;
    ModuleDiscovery *m_discovery;
    UpdateChecker *m_updates = nullptr;
    bool m_moduleScanSearched = false;
    QString m_profileId = QStringLiteral("generic");
    bool m_profileChosenByUser = false;
    QString m_moduleScanTime;
    std::function<void()> m_afterPoll;

    SessionRecorder *m_recorder;
    QPointer<ReplayTransport> m_replay;
    QTimer m_ticker; ///< refreshes the recording / replay clocks once a second
    QVariantList m_recordings;

    ChartModel *m_charts;
    QElapsedTimer m_chartClock;
    ManufacturerValuesModel *m_manufacturerValues;
    QString m_manufacturerModel;
    int m_manufacturerYear = 0;               ///< set by the user when the VIN has no year
    QList<ManufacturerCommand> m_manufacturerAnswered; ///< the commands this car answers
    QHash<QString, qint64> m_manufacturerLastRead;    ///< command key -> m_chartClock ms
    QString m_manufacturerState = QStringLiteral("choose");
    int m_manufacturerProgress = 0, m_manufacturerTotal = 0;
    bool m_manufacturerActive = false;
    bool m_manufacturerReading = false;          ///< a ManufacturerReader pass is running
    std::function<void()> m_afterManufacturerRead; ///< one short request waiting for it (Save report)
    QSet<quint8> m_tripInputs;
    TripComputer m_trip;
    bool m_tripActive = false;
    QElapsedTimer m_tripClock;
    QDateTime m_tripStarted;
    double m_tripBanked = 0;      ///< raw liters of this trip already added to "since fill-up"
    qint64 m_tripLastBank = 0;
    qint64 m_tripLastEmit = 0;
    int m_fuelTypeCode = 0;       ///< PID 51 raw value
    ConnectionKeeper *m_keeper;
    QVariantMap m_adapterCheck;
    QString m_connectionHint;

    QVariantList m_tripHistory;
    QVariantMap m_tripCache;
};

} // namespace obdaryn
