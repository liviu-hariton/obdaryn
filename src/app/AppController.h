// SPDX-License-Identifier: GPL-3.0-or-later
//
// The bridge between the QML user interface and the diagnostics core.

#pragma once

#include "models/DtcModel.h"
#include "models/LiveDataModel.h"
#include "models/ModuleScanModel.h"
#include "models/TrafficLogModel.h"
#include "obd/ObdClient.h"

#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QTranslator>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

namespace obdaryn {

class SimulatedTransport;

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

    // Freeze frame
    Q_PROPERTY(bool freezeFrameAvailable READ freezeFrameAvailable NOTIFY freezeFrameChanged)
    Q_PROPERTY(QString freezeFrameDtc READ freezeFrameDtc NOTIFY freezeFrameChanged)
    Q_PROPERTY(QVariantList freezeFrameValues READ freezeFrameValues NOTIFY freezeFrameChanged)

    // Models
    Q_PROPERTY(obdaryn::LiveDataModel *liveData READ liveData CONSTANT)
    Q_PROPERTY(obdaryn::DtcModel *dtcs READ dtcs CONSTANT)
    Q_PROPERTY(obdaryn::TrafficLogModel *trafficLog READ trafficLog CONSTANT)
    Q_PROPERTY(obdaryn::ModuleScanModel *moduleScan READ moduleScan CONSTANT)

    // Full-vehicle module scan
    Q_PROPERTY(QVariantList vehicleProfiles READ vehicleProfiles CONSTANT)
    Q_PROPERTY(QString profileId READ profileId WRITE setProfileId NOTIFY profileChanged)
    Q_PROPERTY(QString profileNotes READ profileNotes NOTIFY profileChanged)
    Q_PROPERTY(bool profileChosenByUser READ profileChosenByUser NOTIFY profileChanged)
    Q_PROPERTY(bool moduleScanRunning READ moduleScanRunning NOTIFY moduleScanChanged)
    Q_PROPERTY(bool moduleScanDone READ moduleScanDone NOTIFY moduleScanChanged)
    Q_PROPERTY(QString moduleScanTime READ moduleScanTime NOTIFY moduleScanChanged)
    /// Profile the current module results came from (its brand pack explains their codes).
    Q_PROPERTY(QString moduleScanProfile READ moduleScanProfile NOTIFY moduleScanChanged)

    // App
    Q_PROPERTY(QString language READ language NOTIFY languageChanged)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    Q_PROPERTY(QString dtcAttribution READ dtcAttribution CONSTANT)
    Q_PROPERTY(int dtcDatabaseSize READ dtcDatabaseSize CONSTANT)

public:
    enum class ConnectionState { Disconnected, Connecting, Initializing, NoVehicle, Ready, Failed };
    Q_ENUM(ConnectionState)

    explicit AppController(QQmlEngine *engine, QObject *parent = nullptr);
    ~AppController() override;

    /// Factory used by the QML engine for the singleton.
    static AppController *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    // ---- Actions callable from QML -------------------------------------------
    Q_INVOKABLE void connectToAdapter(const QString &host, int port);
    Q_INVOKABLE void connectDemo();
    Q_INVOKABLE void disconnectAdapter();
    Q_INVOKABLE void retryVehicle();
    Q_INVOKABLE void runFullScan();
    Q_INVOKABLE void refreshCodes();
    Q_INVOKABLE void refreshReadiness();
    Q_INVOKABLE void refreshFreezeFrame();
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
    Q_INVOKABLE void cancelModuleScan();
    /// Clears one module's codes after the user confirmed; result via moduleClearFinished().
    Q_INVOKABLE void clearModuleCodes(int row);

    // ---- Property getters ------------------------------------------------------
    ConnectionState connectionState() const;
    QString statusMessage() const { return m_statusMessage; }
    bool demoMode() const { return m_demo; }
    bool demoEngineRunning() const;
    void setDemoEngineRunning(bool running);
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

    bool freezeFrameAvailable() const { return m_freeze.available; }
    QString freezeFrameDtc() const { return m_freeze.dtc; }
    QVariantList freezeFrameValues() const;

    LiveDataModel *liveData() const { return m_liveData; }
    DtcModel *dtcs() const { return m_dtcs; }
    TrafficLogModel *trafficLog() const { return m_trafficLog; }
    ModuleScanModel *moduleScan() const { return m_moduleModel; }

    QVariantList vehicleProfiles() const;
    QString profileId() const { return m_profileId; }
    void setProfileId(const QString &id);
    QString profileNotes() const;
    bool profileChosenByUser() const { return m_profileChosenByUser; }
    bool moduleScanRunning() const { return busyWithModules(); }
    bool moduleScanDone() const { return !m_moduleScanTime.isEmpty(); }
    QString moduleScanTime() const { return m_moduleScanTime; }
    QString moduleScanProfile() const { return m_moduleScanner->profileId(); }

    QString language() const { return m_language; }
    QString appVersion() const;
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
    void languageChanged();
    /// Result of clearCodes(): success flag and a message for the user.
    void clearFinished(bool success, const QString &message);
    void moduleClearFinished(bool success, const QString &message);
    void profileChanged();
    void moduleScanChanged();

private:
    void startSession(Transport *transport, bool demo);
    void onSessionStateChanged();
    void setScanStep(const QString &step);
    void schedulePoll();
    void pollLiveData();
    void resetVehicleData();
    void readStatusPids();
    void readVoltage();
    void pickProfileForVehicle();
    /// Runs `action` once no live-data poll is in flight. Module work changes the
    /// adapter's target address, so it must never interleave with a poll.
    void whenPollIdle(std::function<void()> action);
    bool busyWithModules() const { return m_moduleScanner->running() || bool(m_afterPoll); }

    QQmlEngine *m_engine;
    ObdClient *m_client;
    LiveDataModel *m_liveData;
    DtcModel *m_dtcs;
    TrafficLogModel *m_trafficLog;
    QPointer<SimulatedTransport> m_simulator;
    QTranslator m_appTranslator;
    QTranslator m_qtTranslator;

    bool m_demo = false;
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

    QTimer m_pollTimer;
    QTimer m_voltageTimer;
    bool m_pollInFlight = false;
    bool m_pollingEnabled = false;
    int m_samplesThisSecond = 0;
    QElapsedTimer m_rateClock;

    QString m_language;

    ModuleScanner *m_moduleScanner;
    ModuleScanModel *m_moduleModel;
    QString m_profileId = QStringLiteral("generic");
    bool m_profileChosenByUser = false;
    QString m_moduleScanTime;
    std::function<void()> m_afterPoll;
};

} // namespace obdaryn
