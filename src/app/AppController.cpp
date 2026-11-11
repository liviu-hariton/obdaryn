// SPDX-License-Identifier: GPL-3.0-or-later

#include "AppController.h"

#include "obd/ManufacturerData.h"
#include "obd/ManufacturerReader.h"

#include "elm327/AdapterCheck.h"
#include "elm327/ConnectionKeeper.h"
#include "obd/DtcDatabase.h"
#include "vehicles/ModuleCatalog.h"
#include "obd/Pids.h"
#include "obd/Units.h"
#include "obd/VehicleInfo.h"
#include "recording/SessionRecorder.h"
#include "report/ReportWriter.h"
#include "report/VehicleReport.h"
#include "transport/ReplayTransport.h"
#include "transport/SimulatedTransport.h"
#include "transport/TcpTransport.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QSysInfo>
#include <QUrl>

#include <cmath>
#include <QGuiApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QQmlEngine>
#include <QSettings>

namespace obdaryn {

namespace {
const QString kHostKey = QStringLiteral("adapter/host");
const QString kPortKey = QStringLiteral("adapter/port");
const QString kLanguageKey = QStringLiteral("ui/language");
const QString kDefaultHost = QStringLiteral("192.168.0.10"); // Vgate iCar Pro Wi-Fi / most ELM327 Wi-Fi
constexpr int kDefaultPort = 35000;

// Gap between live-data cycles. The adapter itself is the bottleneck, so this
// only leaves room for other queued requests (scans, voltage).
constexpr int kPollGapMs = 30;

// How many recordings the connect screen lists.
constexpr int kRecordingsListed = 8;

/// "4:05", or "1:02:05" past an hour.
QString clockText(qint64 ms)
{
    const qint64 s = qMax<qint64>(0, ms / 1000);
    if (s >= 3600)
        return QStringLiteral("%1:%2:%3").arg(s / 3600).arg((s / 60) % 60, 2, 10, QLatin1Char('0')).arg(s % 60, 2, 10, QLatin1Char('0'));
    return QStringLiteral("%1:%2").arg(s / 60).arg(s % 60, 2, 10, QLatin1Char('0'));
}

/// Accepts "/path/x.obdrec" or "file:///path/x.obdrec" (what QML file dialogs give).
QString localPath(const QString &pathOrUrl)
{
    const QUrl url(pathOrUrl);
    return url.isLocalFile() ? url.toLocalFile() : pathOrUrl;
}
} // namespace

AppController *AppController::create(QQmlEngine *qmlEngine, QJSEngine *)
{
    return new AppController(qmlEngine);
}

QPair<QString, QString> AppController::recordingLabel(const QJsonObject &header, const QFileInfo &file)
{
    QString car;
    if (header.value(QStringLiteral("demo")).toBool())
        car = tr("Demo car");
    else if (const QString make = header.value(QStringLiteral("manufacturer")).toString(); !make.isEmpty()) {
        const int year = header.value(QStringLiteral("modelYear")).toInt();
        car = year > 0 ? QStringLiteral("%1 %2").arg(make).arg(year) : make;
    } else
        car = file.completeBaseName(); // e.g. a probe recording: its name says what it is
    const QDateTime created = QDateTime::fromString(header.value(QStringLiteral("created")).toString(), Qt::ISODate);
    const QString when = QLocale().toString((created.isValid() ? created : file.lastModified()).toLocalTime(), QLocale::ShortFormat);
    return {car, when};
}

AppController::AppController(QQmlEngine *engine, QObject *parent)
    : QObject(parent),
      m_engine(engine),
      m_client(new ObdClient(this)),
      m_liveData(new LiveDataModel(this)),
      m_dtcs(new DtcModel(this)),
      m_trafficLog(new TrafficLogModel(this)),
      m_moduleScanner(new ModuleScanner(m_client, this)),
      m_moduleModel(new ModuleScanModel(m_moduleScanner, this)),
      m_recorder(new SessionRecorder(m_client->session(), this)),
      m_keeper(new ConnectionKeeper(m_client->session(), this)),
      m_charts(new ChartModel(this)),
      m_manufacturerValues(new ManufacturerValuesModel(this))
{
    m_chartClock.start();
    connect(m_charts, &ChartModel::requiredPidsChanged, this, &AppController::updateRequiredPids);
    connect(m_charts, &ChartModel::notice, this, &AppController::notice);
    connect(m_keeper, &ConnectionKeeper::changed, this, &AppController::onKeeperChanged);
    // Code titles follow the car's brand pack (e.g. Hyundai/Kia for P1326).
    connect(this, &AppController::profileChanged, this, [this] { m_dtcs->setContext(m_profileId, m_language); });

    // While modules are addressed one by one, live polling (which uses the
    // shared "all modules" address) must pause; it resumes when the scan ends.
    connect(m_moduleScanner, &ModuleScanner::started, this, [this] {
        m_pollingEnabled = false;
        emit moduleScanChanged();
    });
    connect(m_moduleScanner, &ModuleScanner::finished, this, [this] {
        m_moduleScanTime = QLocale().toString(QTime::currentTime(), QLocale::ShortFormat);
        emit moduleScanChanged();
        if (m_client->session()->state() == ElmSession::State::Ready && !scanning()) {
            m_pollingEnabled = true;
            schedulePoll();
        }
    });

    ElmSession *session = m_client->session();
    connect(session, &ElmSession::stateChanged, this, &AppController::onSessionStateChanged);
    connect(session, &ElmSession::infoChanged, this, &AppController::adapterInfoChanged);
    connect(session, &ElmSession::trafficLogged, m_trafficLog, &TrafficLogModel::append);

    m_pollTimer.setSingleShot(true);
    connect(&m_pollTimer, &QTimer::timeout, this, &AppController::pollLiveData);

    m_voltageTimer.setInterval(5000);
    connect(&m_voltageTimer, &QTimer::timeout, this, &AppController::readVoltage);

    // Recording: report where the file went when it ends (by the user or by a disconnect).
    m_ticker.setInterval(1000);
    connect(&m_ticker, &QTimer::timeout, this, &AppController::recordingTick);
    connect(&m_ticker, &QTimer::timeout, this, &AppController::adapterHealthChanged);
    connect(m_recorder, &SessionRecorder::recordingChanged, this, [this] {
        if (!m_recorder->isRecording() && !m_recorder->path().isEmpty()) {
            qint64 duration = 0;
            Recording::peek(m_recorder->path(), nullptr, &duration);
            emit notice(true, tr("Recording saved: %1, %2 long.")
                                  .arg(QFileInfo(m_recorder->path()).fileName(), clockText(duration)));
            refreshRecordings();
        }
        updateTicker();
        emit recordingChanged();
        emit recordingTick();
    });

    Units::setSystem(Units::fromKey(QSettings().value(QStringLiteral("ui/units")).toString()));

    // Language: saved choice, else Romanian when the system is Romanian, else English.
    QSettings settings;
    QString lang = settings.value(kLanguageKey).toString();
    if (lang.isEmpty())
        lang = QLocale::system().language() == QLocale::Romanian ? QStringLiteral("ro") : QStringLiteral("en");
    setLanguage(lang);
}

AppController::~AppController()
{
    m_pollTimer.stop();
    m_voltageTimer.stop();
    endTrip(); // quitting while connected still saves the trip
}

// ---------------------------------------------------------------------------
// Connection

void AppController::connectToAdapter(const QString &host, int port)
{
    QSettings settings;
    settings.setValue(kHostKey, host.trimmed());
    settings.setValue(kPortKey, port);
    emit settingsChanged();
    m_replay = nullptr;
    m_keeper->start(true);
    startSession(new TcpTransport(host.trimmed(), quint16(port)), false);
}

void AppController::connectDemo()
{
    m_replay = nullptr;
    m_keeper->start(true);
    m_demoIgnitionOn = true;
    auto *sim = new SimulatedTransport;
    m_simulator = sim;
    startSession(sim, true);
    emit demoEngineRunningChanged();
}

void AppController::startSession(Transport *transport, bool demo)
{
    endTrip();
    m_afterPoll = nullptr;
    m_pollingEnabled = false;
    m_pollTimer.stop();
    m_voltageTimer.stop();
    m_demo = demo;
    resetVehicleData();
    m_trafficLog->append(QStringLiteral("!"), demo ? tr("Starting demo car") : tr("Opening %1").arg(transport->description()));
    m_adapterCheck.clear();
    emit adapterCheckChanged();
    m_connectionHint.clear();
    m_client->session()->setTransport(transport);
    m_client->session()->open();
    updateTicker();
    emit connectionChanged();
}

void AppController::disconnectAdapter()
{
    m_keeper->stop(); // the user asked: no reconnecting
    m_afterPoll = nullptr;
    m_moduleScanner->cancel();
    m_pollingEnabled = false;
    m_pollTimer.stop();
    m_voltageTimer.stop();
    m_client->session()->close();
    setScanStep({});
}

void AppController::retryVehicle()
{
    m_client->session()->detectVehicle();
}

void AppController::onSessionStateChanged()
{
    const ElmSession *s = m_client->session();
    switch (s->state()) {
    case ElmSession::State::Connecting:
        m_statusMessage = tr("Connecting to %1…").arg(transportDescription());
        break;
    case ElmSession::State::Initializing:
        m_statusMessage = tr("Adapter found. Setting it up…");
        break;
    case ElmSession::State::NoVehicle:
    case ElmSession::State::Failed:
        endTrip();
        m_statusMessage = s->lastError();
        m_connectionHint.clear();
        if (s->state() == ElmSession::State::Failed && s->transport() && !m_demo && !m_replay) {
            switch (s->transport()->lastFailure()) {
            case Transport::Failure::Unreachable: m_connectionHint = QStringLiteral("unreachable"); break;
            case Transport::Failure::Timeout: m_connectionHint = QStringLiteral("timeout"); break;
            case Transport::Failure::Refused: m_connectionHint = QStringLiteral("refused"); break;
            default: break;
            }
        }
        m_pollingEnabled = false;
        m_voltageTimer.stop();
        break;
    case ElmSession::State::Ready:
        m_statusMessage.clear();
        m_connectionHint.clear();
        m_voltageTimer.start();
        readVoltage(); // right away, then every 5 s
        runFullScan();
        break;
    case ElmSession::State::Disconnected:
        endTrip();
        m_statusMessage.clear();
        m_pollingEnabled = false;
        m_voltageTimer.stop();
        break;
    }
    if (m_keeper->reconnecting() && s->state() != ElmSession::State::Ready)
        onKeeperChanged(); // keep "Reconnecting, attempt n" instead of each attempt's error
    updateTicker();
    emit connectionChanged();
    emit adapterInfoChanged();
}

AppController::ConnectionState AppController::connectionState() const
{
    if (m_keeper->reconnecting() && m_client->session()->state() != ElmSession::State::Ready)
        return ConnectionState::Reconnecting;
    return static_cast<ConnectionState>(m_client->session()->state());
}

void AppController::onKeeperChanged()
{
    if (m_keeper->reconnecting()) {
        m_statusMessage = tr("Lost the adapter. Reconnecting, attempt %1 of %2…")
                              .arg(qMax(1, m_keeper->attempt())).arg(ConnectionKeeper::kMaxAttempts);
    } else if (m_keeper->gaveUp()) {
        m_statusMessage = tr("Couldn't reconnect to the adapter after %1 tries. Check that it has power and that "
                             "this computer is on its Wi-Fi, then connect again.").arg(ConnectionKeeper::kMaxAttempts);
    }
    emit connectionChanged();
}

void AppController::openLocalNetworkSettings() const
{
#ifdef Q_OS_MACOS
    QDesktopServices::openUrl(QUrl(QStringLiteral("x-apple.systempreferences:com.apple.preference.security?Privacy_LocalNetwork")));
#endif
}

void AppController::runAdapterCheck()
{
    AdapterCheck::run(m_client->session(), [this](const AdapterCheck::Result &r) {
        QVariantList items;
        for (const AdapterCheck::Item &i : r.items)
            items << QVariantMap{{QStringLiteral("command"), i.command}, {QStringLiteral("purpose"), i.purpose},
                                 {QStringLiteral("name"), i.name},
                                 {QStringLiteral("supported"), i.supported}, {QStringLiteral("answer"), i.answer}};
        const char *verdict = r.verdict == AdapterCheck::Verdict::Genuine ? "genuine"
                            : r.verdict == AdapterCheck::Verdict::Compatible ? "compatible" : "unknown";
        m_adapterCheck = {{QStringLiteral("done"), true},
                          {QStringLiteral("identification"), r.identification},
                          {QStringLiteral("version"), r.version},
                          {QStringLiteral("description"), r.description},
                          {QStringLiteral("deviceId"), r.deviceId},
                          {QStringLiteral("verdict"), QLatin1String(verdict)},
                          {QStringLiteral("summary"), r.summary()},
                          {QStringLiteral("items"), items}};
        emit adapterCheckChanged();
    });
}

QVariantMap AppController::adapterHealth() const
{
    const ElmSession::Health h = m_client->session()->health();
    const char *counts = "unknown";
    switch (m_client->responseCounts()) {
    case ObdClient::ResponseCounts::Working: counts = "working"; break;
    case ObdClient::ResponseCounts::NotSupported: counts = "notSupported"; break;
    case ObdClient::ResponseCounts::Off: counts = "off"; break;
    case ObdClient::ResponseCounts::Unknown: break;
    }
    return {{QStringLiteral("commands"), h.commands},
            {QStringLiteral("timeouts"), h.timeouts},
            {QStringLiteral("errors"), h.errors},
            {QStringLiteral("averageMs"), h.averageMs},
            {QStringLiteral("p95Ms"), h.p95Ms},
            {QStringLiteral("multiPid"), m_client->multiPidSupported()},
            {QStringLiteral("responseCounts"), QLatin1String(counts)}};
}

QString AppController::transportDescription() const
{
    const Transport *t = m_client->session()->transport();
    return t ? t->description() : QString();
}

QString AppController::protocolName() const
{
    return m_client->session()->state() == ElmSession::State::Ready
               ? protocolDisplayName(m_client->session()->protocol()) : QString();
}

QString AppController::adapterHost() const
{
    return QSettings().value(kHostKey, kDefaultHost).toString();
}

int AppController::adapterPort() const
{
    return QSettings().value(kPortKey, kDefaultPort).toInt();
}

bool AppController::demoEngineRunning() const
{
    return m_simulator ? m_simulator->engineRunning() : false;
}

void AppController::setDemoIgnitionOn(bool on)
{
    if (!m_simulator || m_demoIgnitionOn == on)
        return;
    m_demoIgnitionOn = on;
    m_simulator->setCarAnswering(on);
    emit demoEngineRunningChanged();
}

void AppController::simulateLinkDrop()
{
    if (m_simulator)
        m_simulator->close();
}

void AppController::setDemoEngineRunning(bool running)
{
    if (!m_simulator || m_simulator->engineRunning() == running)
        return;
    m_simulator->setEngineRunning(running);
    emit demoEngineRunningChanged();
}

// ---------------------------------------------------------------------------
// Scanning

void AppController::setScanStep(const QString &step)
{
    if (m_scanStep == step)
        return;
    m_scanStep = step;
    emit scanningChanged();
}

void AppController::resetVehicleData()
{
    m_manufacturerReading = false;
    m_afterManufacturerRead = nullptr;
    m_manufacturerModel.clear();
    m_manufacturerYear = 0;
    m_manufacturerAnswered.clear();
    m_manufacturerLastRead.clear();
    m_manufacturerState = QStringLiteral("choose");
    m_manufacturerValues->clear();
    emit manufacturerSetupChanged();
    m_vin.clear();
    m_manufacturer.clear();
    m_vinRegion.clear();
    m_obdStandard.clear();
    m_fuelType.clear();
    m_modelYear = 0;
    m_vinCheckDigitValid = false;
    m_info = {};
    m_readiness = {};
    m_freeze = {};
    m_monitorTests.clear();
    m_monitorTestsRead = false;
    m_monitorTestsError.clear();
    emit monitorTestsChanged();
    m_batteryVoltage = 0;
    m_lastScanTime.clear();
    m_moduleScanTime.clear();
    if (!m_profileChosenByUser)
        m_profileId = QStringLiteral("generic");
    emit profileChanged();
    emit moduleScanChanged();
    m_dtcs->clear();
    m_liveData->clear();
    emit vehicleChanged();
    emit readinessChanged();
    emit freezeFrameChanged();
    emit batteryVoltageChanged();
    emit scanFinished();
}

void AppController::runFullScan()
{
    if (m_client->session()->state() != ElmSession::State::Ready || scanning() || m_moduleScanner->running())
        return;

    // The scan is a chain: each step starts when the previous one answered.
    setScanStep(tr("Reading vehicle information"));
    m_client->readVehicleInfo([this](const ObdClient::VehicleInformation &info) {
        m_info = info;
        const VinInfo vin = decodeVin(info.vin);
        m_vin = vin.vin;
        m_manufacturer = vin.manufacturer;
        m_modelYear = vin.modelYear;
        m_vinRegion = vin.region;
        m_vinCheckDigitValid = vin.checkDigitValid;
        m_dtcs->setEcuNames(info.ecuNames);
        emit vehicleChanged();
        pickProfileForVehicle();
        loadManufacturerSetup();

        setScanStep(tr("Checking readiness monitors"));
        m_client->readMonitorStatus([this](const MonitorStatus &status) {
            m_readiness = status;
            emit readinessChanged();

            setScanStep(tr("Reading trouble codes"));
            m_client->readAllDtcs([this](const QList<Dtc> &dtcs, const QString &) {
                m_dtcs->setDtcs(dtcs);

                setScanStep(tr("Reading the freeze frame"));
                m_client->readFreezeFrame([this](const ObdClient::FreezeFrame &ff) {
                    m_freeze = ff;
                    emit freezeFrameChanged();

                    setScanStep(tr("Finding supported live data"));
                    m_client->readSupportedPids(0x01, [this](const QList<quint8> &pids) {
                        m_liveData->setSupportedPids(pids);
                        m_charts->setSupported(QSet<quint8>(pids.cbegin(), pids.cend()));
                        readStatusPids();
                        if (m_adapterCheck.isEmpty() && !m_replay) {
                            m_adapterCheck.insert(QStringLiteral("done"), false);
                            runAdapterCheck(); // once per connection, after the important reads
                        }
                        m_lastScanTime = QLocale().toString(QTime::currentTime(), QLocale::ShortFormat);
                        setScanStep({});
                        emit scanFinished();
                        m_pollingEnabled = true;
                        m_rateClock.start();
                        schedulePoll();
                    });
                });
            });
        });
    });
}

// ---------------------------------------------------------------------------
// Module scan

QVariantList AppController::vehicleProfiles() const
{
    QVariantList list;
    for (const VehicleProfile &p : ModuleCatalog::profiles()) {
        const QString label = p.id == QLatin1String("generic") ? tr("Any car (standard addresses only)") : p.label;
        list << QVariantMap{{QStringLiteral("value"), p.id}, {QStringLiteral("label"), label},
                            {QStringLiteral("moduleCount"), int(p.modules.size())}};
    }
    return list;
}

QVariantList AppController::supportedCars() const
{
    QVariantList list;
    for (const VehicleProfile &p : ModuleCatalog::profiles()) {
        if (p.id == QLatin1String("generic"))
            continue; // every car: said in the text above the list
        QVariantList lines;
        const int modules = int(p.modules.size());
        const int confirmed = int(std::count_if(p.modules.cbegin(), p.modules.cend(), [](const ModuleDef &m) { return m.verified; }));
        //: Help, supported cars: what the all-module scan covers for a brand
        lines << QVariantMap{{QStringLiteral("label"), tr("All modules")},
                             {QStringLiteral("text"), tr("%n module(s) at the brand's own addresses, %1 of them confirmed on real cars.", "", modules)
                                                          .arg(confirmed)}};
        if (const int codes = DtcDatabase::instance().brandSize(p.id); codes > 0)
            lines << QVariantMap{{QStringLiteral("label"), tr("Code explanations")},
                                 {QStringLiteral("text"), tr("%n manufacturer code(s), in English and Romanian.", "", codes)}};
        for (const QString &make : p.makes) {
            const QStringList models = ManufacturerData::instance().modelsOf(make);
            if (models.isEmpty())
                continue;
            //: Help, supported cars: "Hyundai values" (manufacturer values for that make)
            lines << QVariantMap{{QStringLiteral("label"), tr("%1 values").arg(make)},
                                 {QStringLiteral("text"), tr("%n model(s): %1.", "", int(models.size())).arg(models.join(QStringLiteral(", ")))}};
        }
        list << QVariantMap{{QStringLiteral("brand"), p.label}, {QStringLiteral("lines"), lines}};
    }
    return list;
}

void AppController::setProfileId(const QString &id)
{
    if (!ModuleCatalog::find(id) || (id == m_profileId && m_profileChosenByUser))
        return;
    m_profileId = id;
    m_profileChosenByUser = true;
    emit profileChanged();
}

QString AppController::profileNotes() const
{
    const VehicleProfile *p = ModuleCatalog::find(m_profileId);
    return p ? p->notesFor(m_language) : QString();
}

void AppController::pickProfileForVehicle()
{
    if (m_profileChosenByUser)
        return;
    const QString id = ModuleCatalog::forVehicle(m_manufacturer, m_vin).id;
    if (!id.isEmpty() && id != m_profileId) {
        m_profileId = id;
        emit profileChanged();
    }
}

void AppController::whenPollIdle(std::function<void()> action)
{
    m_pollingEnabled = false;
    m_pollTimer.stop();
    if (m_pollInFlight)
        m_afterPoll = std::move(action);
    else
        action();
}

void AppController::scanModules()
{
    if (m_replay && !m_replay->hasModuleRequests()) {
        emit notice(false, tr("This recording has no module scan to replay. Scan the modules while recording to include them."));
        return;
    }
    const VehicleProfile *profile = ModuleCatalog::find(m_profileId);
    if (!profile || m_client->session()->state() != ElmSession::State::Ready
        || scanning() || busyWithModules())
        return;
    const VehicleProfile copy = *profile;
    whenPollIdle([this, copy] { m_moduleScanner->scan(copy); });
    emit moduleScanChanged();
}

void AppController::cancelModuleScan()
{
    m_moduleScanner->cancel();
}

void AppController::clearModuleCodes(int row)
{
    if (replayMode()) {
        emit moduleClearFinished(false, tr("This is a recording, so there is no car to clear codes on."));
        return;
    }
    if (busyWithModules() || scanning()) {
        emit moduleClearFinished(false, tr("Wait for the current scan to finish."));
        return;
    }
    const QString name = row >= 0 && row < m_moduleScanner->results().size()
                             ? m_moduleScanner->results()[row].module.displayName() : QString();
    whenPollIdle([this, row, name] { m_moduleScanner->clearModule(row, [this, name](bool ok, const QString &message) {
        emit moduleScanChanged();
        emit moduleClearFinished(ok, ok ? tr("%1: codes cleared. Any code still listed is a fault that is present right now.").arg(name)
                                        : message);
        if (m_client->session()->state() == ElmSession::State::Ready) {
            m_pollingEnabled = true;
            schedulePoll();
        }
    }); });
    emit moduleScanChanged();
}

void AppController::readVoltage()
{
    m_client->readBatteryVoltage([this](double v) {
        if (v > 0 && !qFuzzyCompare(v, m_batteryVoltage)) {
            m_batteryVoltage = v;
            emit batteryVoltageChanged();
        }
    });
}

void AppController::readStatusPids()
{
    // OBD standard (PID 1C) and fuel type (PID 51) are text, not live values.
    m_client->readPids({0x1C, 0x51}, [this](const QList<ObdClient::PidReading> &readings) {
        for (const auto &r : readings) {
            if (r.pid == 0x1C && m_obdStandard.isEmpty())
                m_obdStandard = PidTable::describeStatus(0x1C, r.raw);
            if (r.pid == 0x51 && m_fuelType.isEmpty()) {
                m_fuelType = PidTable::describeStatus(0x51, r.raw);
                m_fuelTypeCode = r.raw.isEmpty() ? 0 : quint8(r.raw[0]);
            }
        }
        emit vehicleChanged();
        if (!m_tripActive)
            startTrip(); // the supported PIDs and the fuel type are known now
    });
}

void AppController::refreshCodes()
{
    if (scanning() || adapterBusy())
        return;
    setScanStep(tr("Reading trouble codes"));
    m_client->readMonitorStatus([this](const MonitorStatus &status) {
        m_readiness = status;
        emit readinessChanged();
        m_client->readAllDtcs([this](const QList<Dtc> &dtcs, const QString &) {
            m_dtcs->setDtcs(dtcs);
            setScanStep({});
            emit scanFinished();
        });
    });
}

void AppController::refreshReadiness()
{
    if (adapterBusy())
        return;
    m_client->readMonitorStatus([this](const MonitorStatus &status) {
        m_readiness = status;
        emit readinessChanged();
    });
}

void AppController::refreshFreezeFrame()
{
    if (adapterBusy())
        return;
    m_client->readFreezeFrame([this](const ObdClient::FreezeFrame &ff) {
        m_freeze = ff;
        emit freezeFrameChanged();
    });
}

void AppController::readMonitorTests()
{
    if (m_readingMonitorTests || adapterBusy() || m_client->session()->state() != ElmSession::State::Ready)
        return;
    m_readingMonitorTests = true;
    emit monitorTestsChanged();
    m_client->readMonitorTests([this](const QList<MonitorTest> &tests, const QString &error) {
        m_readingMonitorTests = false;
        m_monitorTestsRead = true;
        m_monitorTests = tests;
        m_monitorTestsError = error;
        emit monitorTestsChanged();
    });
}

namespace {
const char *verdictKey(MonitorTest::Verdict v)
{
    switch (v) {
    case MonitorTest::Verdict::Passed: return "passed";
    case MonitorTest::Verdict::Close: return "close";
    case MonitorTest::Verdict::Failed: return "failed";
    case MonitorTest::Verdict::NotRun: break;
    }
    return "notRun";
}
/// Worse first: a monitor shows its worst test.
int severity(MonitorTest::Verdict v)
{
    return v == MonitorTest::Verdict::Failed ? 3 : v == MonitorTest::Verdict::Close ? 2 : v == MonitorTest::Verdict::Passed ? 1 : 0;
}
} // namespace

QVariantList AppController::monitorTests() const
{
    QList<MonitorTest> sorted = m_monitorTests;
    std::stable_sort(sorted.begin(), sorted.end(), [](const MonitorTest &a, const MonitorTest &b) {
        return a.mid != b.mid ? a.mid < b.mid : a.tid < b.tid;
    });
    const QLocale locale;
    auto number = [&locale](double v, int decimals) { return locale.toString(v, 'f', decimals); };

    QVariantList groups;
    QVariantList tests;
    int currentMid = -1;
    MonitorTest::Verdict worst = MonitorTest::Verdict::NotRun;
    QString monitor;
    auto flush = [&] {
        if (currentMid >= 0)
            groups << QVariantMap{{QStringLiteral("monitor"), monitor},
                                  {QStringLiteral("verdict"), QLatin1String(verdictKey(worst))},
                                  {QStringLiteral("tests"), tests}};
        tests.clear();
        worst = MonitorTest::Verdict::NotRun;
    };
    for (const MonitorTest &t : std::as_const(sorted)) {
        if (t.mid != currentMid) {
            flush();
            currentMid = t.mid;
            monitor = t.monitorName();
        }
        if (severity(t.verdict) > severity(worst))
            worst = t.verdict;
        const QString unit = t.unit.isEmpty() ? QString() : QLatin1Char(' ') + t.unit;
        QString limits;
        if (t.verdict == MonitorTest::Verdict::NotRun)
            ; // a test that hasn't run has no limits yet (all zero)
        else if (t.hasMin && t.hasMax)
            limits = tr("%1 to %2").arg(number(t.minimum, t.decimals), number(t.maximum, t.decimals)) + unit;
        else if (t.hasMax)
            limits = tr("at most %1").arg(number(t.maximum, t.decimals)) + unit;
        else if (t.hasMin)
            limits = tr("at least %1").arg(number(t.minimum, t.decimals)) + unit;

        // Where the value sits, for the little range bar: a domain a bit wider
        // than the limits, so a value just outside still shows.
        double lo = t.hasMin ? t.minimum : 0, hi = t.hasMax ? t.maximum : (t.hasMin ? t.minimum * 2 : 1);
        if (!t.hasMin && t.hasMax && t.maximum < 0)
            lo = t.maximum * 2;
        const double pad = (hi - lo) * 0.15;
        lo -= t.hasMin ? pad : 0;
        hi += pad;
        auto pos = [lo, hi](double v) { return hi > lo ? qBound(0.0, (v - lo) / (hi - lo), 1.0) : 0.5; };
        tests << QVariantMap{{QStringLiteral("name"), t.testName()},
                             // Shown faint next to manufacturer tests, to tell them apart.
                             {QStringLiteral("tid"), t.tid >= 0x80 ? QStringLiteral("TID %1").arg(QString::number(t.tid, 16).toUpper()) : QString()},
                             {QStringLiteral("value"), t.verdict == MonitorTest::Verdict::NotRun ? QString() : number(t.value, t.decimals) + unit},
                             {QStringLiteral("limits"), limits},
                             {QStringLiteral("verdict"), QLatin1String(verdictKey(t.verdict))},
                             {QStringLiteral("position"), pos(t.value)},
                             {QStringLiteral("minPosition"), t.hasMin ? pos(t.minimum) : -1.0},
                             {QStringLiteral("maxPosition"), t.hasMax ? pos(t.maximum) : -1.0}};
    }
    flush();
    return groups;
}

void AppController::clearCodes()
{
    if (m_client->session()->state() != ElmSession::State::Ready) {
        emit clearFinished(false, tr("Not connected to a car."));
        return;
    }
    if (replayMode()) {
        emit clearFinished(false, tr("This is a recording, so there is no car to clear codes on."));
        return;
    }
    if (busyWithModules()) {
        emit clearFinished(false, tr("Wait for the module scan to finish."));
        return;
    }
    if (m_manufacturerReading) {
        // The adapter is pointed at one module: a clear now would reach only that one.
        emit clearFinished(false, tr("Wait a moment: Obdaryn is reading the car's modules. Try again when it's done."));
        return;
    }
    setScanStep(tr("Clearing trouble codes"));
    m_client->clearDtcs([this](ObdClient::ClearResult result, const QString &detail) {
        setScanStep({});
        switch (result) {
        case ObdClient::ClearResult::Cleared:
            emit clearFinished(true, tr("Codes cleared. Readiness monitors are reset; drive normally "
                                        "for a few days before an inspection."));
            refreshCodes();
            refreshFreezeFrame();
            break;
        case ObdClient::ClearResult::EngineRunning:
        case ObdClient::ClearResult::Rejected:
        case ObdClient::ClearResult::NoResponse:
            emit clearFinished(false, detail);
            break;
        }
    });
}

// ---------------------------------------------------------------------------
// Live data

void AppController::schedulePoll()
{
    if (m_pollingEnabled && !m_pollInFlight && !m_pollTimer.isActive())
        m_pollTimer.start(kPollGapMs);
}

void AppController::pollLiveData()
{
    if (!m_pollingEnabled || m_client->session()->state() != ElmSession::State::Ready)
        return;
    // Manufacturer values on screen and due: they take this turn.
    if (pollManufacturerValues())
        return;
    const QList<quint8> batch = m_liveData->nextPollBatch();
    if (batch.isEmpty())
        return;

    m_pollInFlight = true;
    m_client->readPids(batch, [this](const QList<ObdClient::PidReading> &readings) {
        m_pollInFlight = false;
        if (m_afterPoll) {
            // Module work was waiting for this poll to finish.
            auto action = std::move(m_afterPoll);
            m_afterPoll = nullptr;
            action();
            return;
        }
        QSet<quint8> seen;
        for (const auto &r : readings) {
            if (seen.contains(r.pid))
                continue; // first ECU (the engine) wins
            seen.insert(r.pid);
            m_liveData->applyReading(r.pid, r.raw);
        }
        feedTrip(readings);
        m_charts->addReadings(m_chartClock.elapsed(), readings);
        m_samplesThisSecond += int(seen.size());
        if (m_rateClock.elapsed() >= 1000) {
            m_liveData->setSamplesPerSecond(m_samplesThisSecond * 1000.0 / m_rateClock.restart());
            m_samplesThisSecond = 0;
        }
        schedulePoll();
    });
}

// ---------------------------------------------------------------------------
// Manufacturer values (OBDb data packs)

QString AppController::vehicleSettingsGroup() const
{
    // Per car: a model chosen for one VIN must not follow the user to the next car.
    return QStringLiteral("vehicles/") + (m_vin.isEmpty() ? QStringLiteral("unknown") : m_vin);
}

QVariantMap AppController::manufacturerSetup() const
{
    const QStringList models = ManufacturerData::instance().modelsOf(m_manufacturer);
    const int vinYear = decodeVin(m_vin).modelYear;
    return {{QStringLiteral("available"), !models.isEmpty()},
            {QStringLiteral("make"), m_manufacturer},
            {QStringLiteral("models"), models},
            {QStringLiteral("model"), m_manufacturerModel},
            {QStringLiteral("year"), vinYear > 0 ? vinYear : m_manufacturerYear},
            {QStringLiteral("yearFromVin"), vinYear > 0},
            {QStringLiteral("state"), m_manufacturerState},
            {QStringLiteral("progress"), m_manufacturerProgress},
            {QStringLiteral("total"), m_manufacturerTotal},
            {QStringLiteral("answered"), int(m_manufacturerAnswered.size())},
            {QStringLiteral("attribution"), ManufacturerData::instance().attribution()}};
}

QList<ManufacturerCommand> AppController::manufacturerCommands() const
{
    const ManufacturerModel *model = ManufacturerData::instance().find(m_manufacturer, m_manufacturerModel);
    if (!model)
        return {};
    const int vinYear = decodeVin(m_vin).modelYear;
    return model->commandsFor(vinYear > 0 ? vinYear : m_manufacturerYear);
}

QHash<quint16, QString> AppController::manufacturerModuleNames() const
{
    QHash<quint16, QString> names;
    if (const VehicleProfile *p = ModuleCatalog::find(ModuleCatalog::forVehicle(m_manufacturer, m_vin).id)) {
        for (const ModuleDef &m : p->modules)
            names.insert(m.tx, m.displayName());
    }
    return names;
}

void AppController::loadManufacturerSetup()
{
    QSettings settings;
    settings.beginGroup(vehicleSettingsGroup());
    m_manufacturerModel = settings.value(QStringLiteral("manufacturerModel")).toString();
    m_manufacturerYear = settings.value(QStringLiteral("manufacturerYear")).toInt();
    const QStringList answered = settings.value(QStringLiteral("manufacturerAnswered")).toStringList();
    const bool asked = settings.contains(QStringLiteral("manufacturerAnswered"));
    settings.endGroup();
    if (!ManufacturerData::instance().find(m_manufacturer, m_manufacturerModel))
        m_manufacturerModel.clear();
    // What this car answered last time: no need to ask every module again.
    m_manufacturerAnswered.clear();
    for (const ManufacturerCommand &c : manufacturerCommands()) {
        if (answered.contains(c.key()))
            m_manufacturerAnswered << c;
    }
    m_manufacturerLastRead.clear();
    m_manufacturerValues->setCommands(m_manufacturerAnswered, manufacturerModuleNames());
    m_manufacturerState = m_manufacturerModel.isEmpty() ? QStringLiteral("choose")
                        : !asked ? QStringLiteral("ready")
                        : m_manufacturerAnswered.isEmpty() ? QStringLiteral("none") : QStringLiteral("found");
    emit manufacturerSetupChanged();
}

void AppController::setManufacturerModel(const QString &model)
{
    if (model == m_manufacturerModel || !ManufacturerData::instance().find(m_manufacturer, model))
        return;
    QSettings settings;
    settings.beginGroup(vehicleSettingsGroup());
    settings.setValue(QStringLiteral("manufacturerModel"), model);
    settings.remove(QStringLiteral("manufacturerAnswered")); // another model: ask again
    settings.endGroup();
    loadManufacturerSetup();
}

void AppController::setManufacturerYear(int year)
{
    if (year < 1996 || year > QDate::currentDate().year() + 1 || year == m_manufacturerYear)
        return;
    QSettings settings;
    settings.beginGroup(vehicleSettingsGroup());
    settings.setValue(QStringLiteral("manufacturerYear"), year);
    settings.remove(QStringLiteral("manufacturerAnswered"));
    settings.endGroup();
    loadManufacturerSetup();
}

void AppController::setManufacturerActive(bool active)
{
    if (active == m_manufacturerActive)
        return;
    m_manufacturerActive = active;
    emit manufacturerActiveChanged();
    if (active)
        schedulePoll();
}

void AppController::findManufacturerValues()
{
    const QList<ManufacturerCommand> commands = manufacturerCommands();
    if (commands.isEmpty() || m_manufacturerState == QLatin1String("finding")
        || m_client->session()->state() != ElmSession::State::Ready || scanning() || busyWithModules())
        return;
    m_manufacturerState = QStringLiteral("finding");
    m_manufacturerProgress = 0;
    m_manufacturerTotal = int(commands.size());
    m_manufacturerReading = true;
    // Shown like any scan, so actions that need the standard addressing wait.
    //: Status while finding manufacturer values, e.g. "Asking the modules for Hyundai values"
    const QString step = tr("Asking the modules for %1 values").arg(m_manufacturer);
    setScanStep(step);
    emit manufacturerSetupChanged();
    // Module requests change the adapter's target address: never during a poll.
    const QString vin = m_vin;
    whenPollIdle([this, commands, vin, step] {
        ManufacturerReader::read(m_client, commands, [this, commands, vin, step](const QList<ManufacturerReader::Result> &results) {
            // Whatever happened, the standard addressing is back: resume.
            m_manufacturerReading = false;
            if (m_scanStep == step)
                setScanStep({}); // only our own status, never a scan that started since
            const auto resume = [this] {
                if (m_client->session()->state() == ElmSession::State::Ready) {
                    m_pollingEnabled = true;
                    schedulePoll();
                }
            };
            if (vin != m_vin || m_manufacturerState != QLatin1String("finding")) {
                resume(); // another car, model or year meanwhile: these answers aren't for it
                return;
            }
            if (ManufacturerReader::interrupted(results)) {
                // The connection dropped: nothing is known, so nothing is remembered.
                loadManufacturerSetup();
                resume();
                return;
            }
            m_manufacturerAnswered.clear();
            QStringList keys;
            for (int i = 0; i < results.size(); ++i) {
                if (results[i].answered) {
                    m_manufacturerAnswered << commands[i];
                    keys << commands[i].key();
                }
            }
            // A recording or the demo car says nothing about the user's car: remember only real ones.
            if (!m_demo && !m_replay) {
                QSettings settings;
                settings.beginGroup(vehicleSettingsGroup());
                settings.setValue(QStringLiteral("manufacturerAnswered"), keys);
                settings.endGroup();
            }
            m_manufacturerValues->setCommands(m_manufacturerAnswered, manufacturerModuleNames());
            m_manufacturerLastRead.clear();
            for (int i = 0; i < results.size(); ++i) {
                if (results[i].answered) {
                    m_manufacturerValues->applyResult(commands[i], results[i]);
                    m_manufacturerLastRead.insert(commands[i].key(), m_chartClock.elapsed());
                }
            }
            m_manufacturerState = m_manufacturerAnswered.isEmpty() ? QStringLiteral("none") : QStringLiteral("found");
            emit manufacturerSetupChanged();
            resume();
        }, [this](int done, int total) {
            m_manufacturerProgress = done;
            m_manufacturerTotal = total;
            emit manufacturerSetupChanged();
        });
    });
}

bool AppController::pollManufacturerValues()
{
    if (!m_manufacturerActive || m_manufacturerAnswered.isEmpty())
        return false;
    // The commands whose interval has passed, a few per turn so generic live
    // data (trip, pinned values) keeps flowing in between.
    constexpr int kPerTurn = 4;
    const qint64 now = m_chartClock.elapsed();
    QList<ManufacturerCommand> due;
    for (const ManufacturerCommand &c : std::as_const(m_manufacturerAnswered)) {
        const auto it = m_manufacturerLastRead.constFind(c.key());
        if (it == m_manufacturerLastRead.cend() || now - *it >= qint64(c.intervalSeconds * 1000)) {
            due << c;
            if (due.size() >= kPerTurn)
                break;
        }
    }
    if (due.isEmpty())
        return false;
    for (const ManufacturerCommand &c : std::as_const(due))
        m_manufacturerLastRead.insert(c.key(), now);
    m_pollInFlight = true;
    m_manufacturerReading = true;
    ManufacturerReader::read(m_client, due, [this, due](const QList<ManufacturerReader::Result> &results) {
        m_pollInFlight = false;
        m_manufacturerReading = false;
        if (m_afterManufacturerRead) {
            auto action = std::move(m_afterManufacturerRead);
            m_afterManufacturerRead = nullptr;
            action();
        }
        if (m_afterPoll) {
            // Module work was waiting for this poll to finish.
            auto action = std::move(m_afterPoll);
            m_afterPoll = nullptr;
            action();
            return;
        }
        for (int i = 0; i < results.size(); ++i)
            m_manufacturerValues->applyResult(due[i], results[i]);
        schedulePoll();
    });
    return true;
}

// ---------------------------------------------------------------------------
// Recording and replay

QString AppController::recordingsFolder() const
{
    // Developer capture mode and tests point this elsewhere.
    const QString overridden = qEnvironmentVariable("OBDARYN_RECORDINGS_DIR");
    if (!overridden.isEmpty())
        return overridden;
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QStringLiteral("/Obdaryn/Recordings");
}

QString AppController::recordingsFolderDisplay() const
{
    QString path = QDir::toNativeSeparators(recordingsFolder());
    const QString home = QDir::toNativeSeparators(QDir::homePath());
    if (path.startsWith(home))
        path.replace(0, home.size(), QStringLiteral("~"));
    return path;
}

bool AppController::recording() const
{
    return m_recorder->isRecording();
}

QString AppController::recordingTime() const
{
    return clockText(m_recorder->elapsedMs());
}

QString AppController::replayPosition() const
{
    if (!m_replay)
        return {};
    return tr("%1 of %2").arg(clockText(m_replay->positionMs()), clockText(m_replay->durationMs()));
}

void AppController::updateTicker()
{
    // Also refreshes the adapter's health numbers while connected.
    const bool needed = m_recorder->isRecording() || m_client->session()->state() != ElmSession::State::Disconnected;
    if (needed && !m_ticker.isActive())
        m_ticker.start();
    else if (!needed)
        m_ticker.stop();
}

void AppController::startRecording()
{
    if (m_client->session()->state() != ElmSession::State::Ready || m_recorder->isRecording())
        return;
    if (replayMode()) {
        emit notice(false, tr("This is already a recording."));
        return;
    }
    const QString folder = recordingsFolder();
    if (!QDir().mkpath(folder)) {
        emit notice(false, tr("Couldn't create the folder %1.").arg(QDir::toNativeSeparators(folder)));
        return;
    }
    // The file name says when and what, never the VIN: names show up everywhere.
    QString name = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH-mm-ss"));
    if (m_demo)
        name += QStringLiteral(" demo car");
    else if (!m_manufacturer.isEmpty())
        name += QLatin1Char(' ') + m_manufacturer + (m_modelYear > 0 ? QStringLiteral(" %1").arg(m_modelYear) : QString());
    const QString path = folder + QLatin1Char('/') + name + QLatin1Char('.') + QLatin1String(Recording::kSuffix);

    const QJsonObject metadata{
        {QStringLiteral("app"), QStringLiteral("Obdaryn %1").arg(appVersion())},
        {QStringLiteral("demo"), m_demo},
        {QStringLiteral("vin"), m_vin},
        {QStringLiteral("manufacturer"), m_manufacturer},
        {QStringLiteral("modelYear"), m_modelYear},
        {QStringLiteral("profile"), m_profileId},
    };
    QString error;
    if (!m_recorder->start(path, metadata, &error))
        emit notice(false, tr("Couldn't start recording: %1").arg(error));
}

void AppController::stopRecording()
{
    m_recorder->stop();
}

void AppController::replayRecording(const QString &pathOrUrl)
{
    const QString path = localPath(pathOrUrl);
    Recording rec;
    QString error;
    if (!rec.load(path, &error)) {
        emit notice(false, tr("Couldn't open %1. %2").arg(QFileInfo(path).fileName(), error));
        return;
    }
    if (m_recorder->isRecording())
        m_recorder->stop();
    const auto [car, when] = recordingLabel(rec.header, QFileInfo(path));
    //: Sidebar, while replaying: which car and when it was recorded, e.g. "Hyundai 2014, 11/11/26 14:03"
    auto *replay = new ReplayTransport(std::move(rec), tr("%1, %2").arg(car, when));
    m_replay = replay;
    m_keeper->start(false); // a recording has no link to lose
    startSession(replay, false);
}

void AppController::exportRecordingCsv(const QString &pathOrUrl)
{
    const QString path = localPath(pathOrUrl);
    Recording rec;
    QString error;
    if (!rec.load(path, &error)) {
        emit notice(false, tr("Couldn't open %1. %2").arg(QFileInfo(path).fileName(), error));
        return;
    }
    const QFileInfo info(path);
    const QString csvPath = info.absolutePath() + QLatin1Char('/') + info.completeBaseName() + QStringLiteral(".csv");
    QFile out(csvPath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit notice(false, tr("Couldn't write %1: %2").arg(QFileInfo(csvPath).fileName(), out.errorString()));
        return;
    }
    const QString csv = rec.toCsv();
    out.write(csv.toUtf8());
    const int rows = qMax(0, int(csv.count(QLatin1Char('\n'))) - 1);
    if (rows == 0)
        emit notice(false, tr("%1 has no live data, so the CSV file only has column names.").arg(info.fileName()));
    else
        emit notice(true, tr("Saved %1 next to the recording (%n rows).", "", rows).arg(QFileInfo(csvPath).fileName()));
}

void AppController::showRecordingsFolder() const
{
    QDir().mkpath(recordingsFolder());
    QDesktopServices::openUrl(QUrl::fromLocalFile(recordingsFolder()));
}

void AppController::refreshRecordings()
{
    QVariantList list;
    QDir dir(recordingsFolder());
    const QFileInfoList files = dir.entryInfoList({QStringLiteral("*.") + QLatin1String(Recording::kSuffix)},
                                                  QDir::Files, QDir::Time);
    for (const QFileInfo &f : files) {
        if (list.size() >= kRecordingsListed)
            break;
        QJsonObject header;
        qint64 duration = 0;
        if (!Recording::peek(f.absoluteFilePath(), &header, &duration))
            continue;
        const auto [car, when] = recordingLabel(header, f);
        list << QVariantMap{{QStringLiteral("path"), f.absoluteFilePath()},
                            {QStringLiteral("title"), car},
                            {QStringLiteral("when"), when},
                            {QStringLiteral("duration"), clockText(duration)},
                            {QStringLiteral("fileName"), f.fileName()}};
    }
    m_recordings = list;
    emit recordingsChanged();
}

// ---------------------------------------------------------------------------
// Shareable report

QString AppController::reportsFolder() const
{
    const QString overridden = qEnvironmentVariable("OBDARYN_REPORTS_DIR");
    if (!overridden.isEmpty())
        return overridden;
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QStringLiteral("/Obdaryn/Reports");
}

QUrl AppController::suggestedReportUrl() const
{
    ReportInput input;
    input.createdAt = QDateTime::currentDateTime();
    input.source = m_demo ? ReportInput::Source::Demo : ReportInput::Source::Car;
    input.info.vin = m_vin;
    QDir().mkpath(reportsFolder());
    return QUrl::fromLocalFile(reportsFolder() + QLatin1Char('/') + VehicleReport::suggestedBaseName(input) + QStringLiteral(".pdf"));
}

void AppController::saveReport(const QString &pathOrUrl, bool open)
{
    if (m_client->session()->state() != ElmSession::State::Ready) {
        emit notice(false, tr("Connect to a car first: the report shows what it reads."));
        return;
    }
    if (scanning() || busyWithModules()) {
        emit notice(false, tr("Wait for the current scan to finish, then save the report."));
        return;
    }
    if (m_manufacturerReading) {
        // A manufacturer poll has the adapter pointed at one module for a moment:
        // save once it's done (its one request goes ahead of the next poll).
        m_afterManufacturerRead = [this, pathOrUrl, open] { saveReport(pathOrUrl, open); };
        return;
    }
    QString path = localPath(pathOrUrl);
    const QString suffix = QFileInfo(path).suffix().toLower();
    const bool asHtml = suffix == QLatin1String("html") || suffix == QLatin1String("htm");
    if (!asHtml && suffix != QLatin1String("pdf"))
        path += QStringLiteral(".pdf");

    // The "since codes were cleared" counters are read fresh: on a car for
    // sale, they are among the most telling numbers in the report.
    m_client->readPids({0x21, 0x30, 0x31, 0x4D, 0x4E}, [this, path, asHtml, open](const QList<ObdClient::PidReading> &readings) {
        ReportInput input;
        input.source = m_replay ? ReportInput::Source::Replay : m_demo ? ReportInput::Source::Demo : ReportInput::Source::Car;
        input.sourceDetail = transportDescription();
        input.appVersion = appVersion();
        input.createdAt = QDateTime::currentDateTime();
        input.scanTime = m_lastScanTime;
        input.brand = m_profileId;
        input.language = m_language;
        input.adapter = adapterVersion();
        input.protocol = protocolName();
        input.batteryVolts = m_batteryVoltage;
        input.info = m_info;
        input.obdStandard = m_obdStandard;
        input.fuelType = m_fuelType;
        input.readiness = m_readiness;
        input.dtcs = m_dtcs->dtcList();
        input.freezeFrame = m_freeze;
        if (m_tripActive) {
            const TripComputer::Stats &st = m_trip.stats();
            input.trip = {st.distanceKm, st.fuelLiters, st.fuelKnown, st.durationMs, st.movingMs, st.idleMs,
                          st.averageSpeedKmh(), st.maxSpeedKmh, m_tripCache.value(QStringLiteral("method")).toString(),
                          m_tripCache.value(QStringLiteral("calibrationFillUps")).toInt(),
                          m_tripCache.value(QStringLiteral("costText")).toString()};
        }
        input.monitorTestsRead = m_monitorTestsRead && m_monitorTestsError.isEmpty();
        input.monitorTests = m_monitorTests;
        input.modulesScanned = moduleScanDone();
        input.modules = m_moduleScanner->results();
        for (const auto &r : readings) {
            if (!input.pids.contains(r.pid)) // the engine (first ECU) wins
                input.pids.insert(r.pid, r.raw);
        }

        QString error;
        QDir().mkpath(QFileInfo(path).absolutePath());
        const bool ok = asHtml ? ReportWriter::writeHtml(VehicleReport::html(input, VehicleReport::Target::Web), path, &error)
                               : ReportWriter::writePdf(VehicleReport::html(input, VehicleReport::Target::Pdf), path,
                                                        VehicleReport::suggestedBaseName(input), &error);
        if (!ok) {
            emit notice(false, tr("Couldn't save the report: %1").arg(error));
            return;
        }
        emit notice(true, tr("Report saved: %1").arg(QFileInfo(path).fileName()));
        if (open)
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    });
}

// ---------------------------------------------------------------------------
// Trip computer

namespace {
constexpr int kTripHistoryLength = 20;
constexpr double kMinSavedTripKm = 0.5;        // shorter "trips" are just connecting in the driveway
constexpr qint64 kTripBankIntervalMs = 30000;  // fuel is banked toward the fill-up count this often
constexpr qint64 kTripEmitIntervalMs = 250;    // the screen refreshes four times a second

/// "40 s", "23 min" or "1 h 05 min".
QString durationText(qint64 ms)
{
    if (ms < 60000)
        return AppController::tr("%1 s").arg(ms / 1000);
    const qint64 minutes = ms / 60000;
    if (minutes < 60)
        return AppController::tr("%1 min").arg(minutes);
    return AppController::tr("%1 h %2 min").arg(minutes / 60).arg(minutes % 60, 2, 10, QLatin1Char('0'));
}

QString methodKey(TripComputer::FuelMethod m)
{
    switch (m) {
    case TripComputer::FuelMethod::FuelRate: return QStringLiteral("fuelRate");
    case TripComputer::FuelMethod::MassAirFlow: return QStringLiteral("maf");
    case TripComputer::FuelMethod::SpeedDensity: return QStringLiteral("speedDensity");
    case TripComputer::FuelMethod::NeedsEngineSize: return QStringLiteral("needsEngineSize");
    case TripComputer::FuelMethod::None: break;
    }
    return QStringLiteral("none");
}
} // namespace

QString AppController::tripKey() const
{
    // Per car: calibration and engine size belong to one vehicle.
    if (m_demo)
        return QStringLiteral("trips/demo");
    return QStringLiteral("trips/") + (m_vin.isEmpty() ? QStringLiteral("unknown") : m_vin);
}

void AppController::updateRequiredPids()
{
    m_liveData->setRequiredPids(m_tripInputs | m_charts->requiredPids());
}

void AppController::startTrip()
{
    const QSet<quint8> supported = m_liveData->supportedPids();
    // Diesel: the compression-ignition readiness flag, or PID 51 (diesel, hybrid diesel, bifuel diesel).
    const bool diesel = m_readiness.compressionIgnition || m_fuelTypeCode == 0x04 || m_fuelTypeCode == 0x13 || m_fuelTypeCode == 0x17;

    QSettings settings;
    settings.beginGroup(tripKey());
    TripComputer::Settings ts;
    ts.diesel = diesel;
    // The demo car is a 1.8 L Elantra; real cars need the owner to say.
    ts.engineLiters = settings.value(QStringLiteral("engineLiters"), m_demo ? 1.8 : 0.0).toDouble();
    ts.calibration = settings.value(QStringLiteral("calibration"), 1.0).toDouble();
    settings.endGroup();

    m_trip.setSupported(supported);
    m_trip.setSettings(ts);
    m_trip.reset();
    m_tripInputs = TripComputer::inputPids(supported, diesel);
    updateRequiredPids();
    m_tripBanked = 0;
    m_tripLastBank = 0;
    m_tripLastEmit = 0;
    m_tripStarted = QDateTime::currentDateTime();
    m_tripClock.start();
    m_tripActive = true;
    loadTripHistory();
    refreshTrip();
}

void AppController::feedTrip(const QList<ObdClient::PidReading> &readings)
{
    if (!m_tripActive)
        return;
    QHash<quint8, double> values;
    for (const auto &r : readings) {
        if (values.contains(r.pid))
            continue; // first ECU (the engine) wins
        if (r.pid == 0x03) { // fuel system status: a bit field, not a number
            if (!r.raw.isEmpty())
                values.insert(r.pid, quint8(r.raw[0]));
            continue;
        }
        const PidDefinition *def = PidTable::find(r.pid);
        double v = 0;
        if (def && def->value(r.raw, &v) && std::isfinite(v))
            values.insert(r.pid, v);
    }
    const qint64 now = m_tripClock.elapsed();
    m_trip.addSample(now, values);
    if (now - m_tripLastBank >= kTripBankIntervalMs) {
        m_tripLastBank = now;
        bankTripFuel();
    }
    if (now - m_tripLastEmit >= kTripEmitIntervalMs) {
        m_tripLastEmit = now;
        refreshTrip();
    }
}

void AppController::bankTripFuel()
{
    // Adds this trip's new fuel to the car's "since the last fill-up" count,
    // as it goes, so a crash or a flat laptop battery loses little.
    const double delta = m_trip.stats().rawFuelLiters - m_tripBanked;
    if (!tripSaved() || delta <= 0 || !m_trip.stats().fuelKnown)
        return;
    QSettings settings;
    settings.beginGroup(tripKey());
    if (settings.value(QStringLiteral("fillUpStarted")).toBool())
        settings.setValue(QStringLiteral("rawSinceFillUp"), settings.value(QStringLiteral("rawSinceFillUp")).toDouble() + delta);
    settings.endGroup();
    m_tripBanked = m_trip.stats().rawFuelLiters;
}

void AppController::endTrip()
{
    if (!m_tripActive)
        return;
    bankTripFuel();
    const TripComputer::Stats &st = m_trip.stats();
    if (tripSaved() && st.distanceKm >= kMinSavedTripKm) {
        QSettings settings;
        settings.beginGroup(tripKey());
        QJsonArray history = QJsonDocument::fromJson(settings.value(QStringLiteral("history")).toByteArray()).array();
        history.prepend(QJsonObject{{QStringLiteral("start"), m_tripStarted.toString(Qt::ISODate)},
                                    {QStringLiteral("km"), st.distanceKm},
                                    {QStringLiteral("liters"), st.fuelLiters},
                                    {QStringLiteral("ms"), double(st.durationMs)},
                                    {QStringLiteral("fuelKnown"), st.fuelKnown},
                                    // The cost at the price of that day, if one was set.
                                    {QStringLiteral("cost"), m_tripCache.value(QStringLiteral("costText")).toString()}});
        while (history.size() > kTripHistoryLength)
            history.removeLast();
        settings.setValue(QStringLiteral("history"), QJsonDocument(history).toJson(QJsonDocument::Compact));
        settings.endGroup();
    }
    m_tripActive = false;
    m_tripInputs.clear();
    updateRequiredPids();
    loadTripHistory();
    refreshTrip();
}

void AppController::newTrip()
{
    if (!m_tripActive)
        return;
    endTrip();
    startTrip();
}

void AppController::loadTripHistory()
{
    m_tripHistory.clear();
    if (tripSaved()) {
        QSettings settings;
        const QJsonArray history = QJsonDocument::fromJson(settings.value(tripKey() + QStringLiteral("/history")).toByteArray()).array();
        for (const QJsonValue &v : history) {
            const QJsonObject o = v.toObject();
            const double km = o.value(QStringLiteral("km")).toDouble();
            const double liters = o.value(QStringLiteral("liters")).toDouble();
            const bool known = o.value(QStringLiteral("fuelKnown")).toBool();
            const QDateTime start = QDateTime::fromString(o.value(QStringLiteral("start")).toString(), Qt::ISODate);
            const QLocale locale;
            const Units::Shown d = Units::show(km, "km", 1);
            const Units::Shown f = Units::show(liters, "L", 2);
            const Units::Shown a = Units::show(known && km >= 0.2 ? liters / km * 100.0 : 0.0, "L/100 km", 1);
            m_tripHistory << QVariantMap{{QStringLiteral("when"), QLocale().toString(start, QLocale::ShortFormat)},
                                         {QStringLiteral("distanceText"), QStringLiteral("%1 %2").arg(locale.toString(d.value, 'f', d.decimals), d.unit)},
                                         {QStringLiteral("fuelText"), known ? QStringLiteral("%1 %2").arg(locale.toString(f.value, 'f', f.decimals), f.unit) : QString()},
                                         {QStringLiteral("averageText"), a.value > 0 ? QStringLiteral("%1 %2").arg(locale.toString(a.value, 'f', a.decimals), a.unit) : QString()},
                                         {QStringLiteral("cost"), o.value(QStringLiteral("cost")).toString()},
                                         {QStringLiteral("duration"), durationText(qint64(o.value(QStringLiteral("ms")).toDouble()))},
                                         {QStringLiteral("fuelKnown"), known}};
        }
    }
    emit tripHistoryChanged();
}

QVariantMap AppController::buildTripMap() const
{
    const TripComputer::Stats &st = m_trip.stats();
    const TripComputer::Settings &ts = m_trip.settings();
    QSettings settings;
    settings.beginGroup(tripKey());
    const bool fillUpStarted = settings.value(QStringLiteral("fillUpStarted")).toBool();
    // Fuel since the last fill-up, including what this trip hasn't banked yet.
    const double sinceFillUp = (settings.value(QStringLiteral("rawSinceFillUp")).toDouble()
                                + (tripSaved() ? st.rawFuelLiters - m_tripBanked : 0)) * ts.calibration;
    const int fillUps = settings.value(QStringLiteral("calibrationFillUps")).toInt();
    settings.endGroup();
    QSettings global;
    const double pricePerLiter = global.value(QStringLiteral("trip/fuelPrice")).toDouble();
    QString currency = global.value(QStringLiteral("trip/currency")).toString();
    if (currency.isEmpty())
        currency = QLocale::system().currencySymbol(QLocale::CurrencySymbol); // the user's country, not the UI language
    const Units::Shown distance = Units::show(st.distanceKm, "km", 1);
    const Units::Shown fuel = Units::show(st.fuelLiters, "L", 2);
    const Units::Shown average = Units::show(st.litersPer100Km(), "L/100 km", 1);
    const Units::Shown instant = Units::show(st.instantLitersPer100Km(), "L/100 km", 1);
    const Units::Shown flow = Units::show(st.instantLph, "L/h", 1);
    const Units::Shown avgSpeed = Units::show(st.averageSpeedKmh(), "km/h", 0);
    const Units::Shown maxSpeed = Units::show(st.maxSpeedKmh, "km/h", 0);
    const Units::Shown sinceFill = Units::show(sinceFillUp, "L", 1);
    // The average needs a little distance first (TripComputer::Stats::litersPer100Km).
    const Units::Shown averageFrom = Units::show(0.2, "km", 1);
    const QLocale locale;
    return {
        // Shown in the chosen units (values + unit text).
        {QStringLiteral("distanceValue"), distance.value},
        {QStringLiteral("distanceUnit"), distance.unit},
        {QStringLiteral("fuelValue"), fuel.value},
        {QStringLiteral("fuelUnit"), fuel.unit},
        {QStringLiteral("averageValue"), average.value},
        {QStringLiteral("consumptionUnit"), average.unit},
        //: Trip, under an empty "Average": it appears once the car has driven this far. %1 = e.g. "0.2 km"
        {QStringLiteral("averageNote"), st.fuelKnown && st.litersPer100Km() <= 0
             ? tr("Shown after %1").arg(QStringLiteral("%1 %2").arg(locale.toString(averageFrom.value, 'f', 1), averageFrom.unit)) : QString()},
        {QStringLiteral("instantValue"), instant.value},
        {QStringLiteral("flowValue"), flow.value},
        {QStringLiteral("flowUnit"), flow.unit},
        {QStringLiteral("averageSpeedText"), st.averageSpeedKmh() > 0 ? QStringLiteral("%1 %2").arg(locale.toString(avgSpeed.value, 'f', 0), avgSpeed.unit) : QString()},
        {QStringLiteral("maxSpeedText"), QStringLiteral("%1 %2").arg(locale.toString(maxSpeed.value, 'f', 0), maxSpeed.unit)},
        {QStringLiteral("sinceFillUpText"), QStringLiteral("%1 %2").arg(locale.toString(sinceFill.value, 'f', 1), sinceFill.unit)},
        {QStringLiteral("volumeUnit"), Units::unitFor("L")},
        // Fuel price (per shown volume unit) and the trip's cost.
        {QStringLiteral("fuelPrice"), pricePerLiter * Units::litersFromShownVolume(1.0)},
        {QStringLiteral("currency"), currency},
        {QStringLiteral("costText"), pricePerLiter > 0 && st.fuelKnown
             ? QStringLiteral("%1 %2").arg(locale.toString(st.fuelLiters * pricePerLiter, 'f', 2), currency) : QString()},
        {QStringLiteral("active"), m_tripActive},
        {QStringLiteral("saved"), tripSaved()},
        {QStringLiteral("startedAt"), m_tripStarted.isValid() ? QLocale().toString(m_tripStarted.time(), QLocale::ShortFormat) : QString()},
        {QStringLiteral("distanceKm"), st.distanceKm},
        {QStringLiteral("fuelLiters"), st.fuelLiters},
        {QStringLiteral("fuelKnown"), st.fuelKnown},
        {QStringLiteral("averageL100"), st.litersPer100Km()},
        {QStringLiteral("instantL100"), st.instantLitersPer100Km()},
        {QStringLiteral("instantLph"), st.instantLph},
        {QStringLiteral("moving"), st.instantSpeedKmh >= 3},
        {QStringLiteral("averageSpeed"), st.averageSpeedKmh()},
        {QStringLiteral("maxSpeed"), st.maxSpeedKmh},
        {QStringLiteral("duration"), durationText(st.durationMs)},
        {QStringLiteral("drivingTime"), durationText(st.movingMs)},
        {QStringLiteral("idleTime"), durationText(st.idleMs)},
        {QStringLiteral("method"), methodKey(m_trip.method())},
        {QStringLiteral("diesel"), ts.diesel},
        {QStringLiteral("engineLiters"), ts.engineLiters},
        {QStringLiteral("calibration"), ts.calibration},
        {QStringLiteral("calibrationFillUps"), fillUps},
        {QStringLiteral("fillUpStarted"), fillUpStarted},
        {QStringLiteral("sinceFillUpLiters"), sinceFillUp},
    };
}

void AppController::refreshTrip()
{
    // Built once per change, not per QML binding: it reads the settings.
    m_tripCache = buildTripMap();
    emit tripChanged();
}

QString AppController::units() const
{
    return Units::key(Units::system());
}

void AppController::setUnits(const QString &units)
{
    const Units::System system = Units::fromKey(units);
    if (system == Units::system())
        return;
    Units::setSystem(system);
    QSettings().setValue(QStringLiteral("ui/units"), Units::key(system));
    m_liveData->retranslate();
    emit freezeFrameChanged();
    refreshTrip();
    loadTripHistory();
    m_charts->retranslate();
    m_manufacturerValues->refreshTexts();
    emit unitsChanged();
}

void AppController::setFuelPrice(double pricePerShownUnit, const QString &currency)
{
    QSettings settings;
    if (pricePerShownUnit <= 0) {
        settings.remove(QStringLiteral("trip/fuelPrice"));
    } else {
        // Stored per liter; typed per liter or per gallon.
        settings.setValue(QStringLiteral("trip/fuelPrice"), pricePerShownUnit / Units::litersFromShownVolume(1.0));
        settings.setValue(QStringLiteral("trip/currency"), currency.trimmed());
    }
    refreshTrip();
}

void AppController::setEngineLiters(double liters)
{
    if (liters < 0.5 || liters > 10 || !m_tripActive)
        return;
    QSettings().setValue(tripKey() + QStringLiteral("/engineLiters"), liters);
    TripComputer::Settings ts = m_trip.settings();
    ts.engineLiters = liters;
    m_trip.setSettings(ts);
    refreshTrip();
}

void AppController::recordFillUp(double shownAmount)
{
    // Typed in liters, or in gallons with US/UK units.
    const double liters = Units::litersFromShownVolume(shownAmount);
    if (!tripSaved()) {
        emit notice(false, tr("Fill-ups can only be recorded with your own car connected, not the demo car or a recording."));
        return;
    }
    if (liters <= 0 || liters > 200)
        return;
    bankTripFuel();
    QSettings settings;
    settings.beginGroup(tripKey());
    if (!settings.value(QStringLiteral("fillUpStarted")).toBool()) {
        emit notice(true, tr("Fill-up recorded. Obdaryn now counts the fuel it estimates; at your next fill-up, enter the liters again to calibrate."));
    } else {
        const auto c = TripComputer::learnCalibration(settings.value(QStringLiteral("calibration"), 1.0).toDouble(),
                                                      settings.value(QStringLiteral("calibrationLiters")).toDouble(), liters,
                                                      settings.value(QStringLiteral("rawSinceFillUp")).toDouble());
        switch (c.result) {
        case TripComputer::Calibration::Result::Learned: {
            settings.setValue(QStringLiteral("calibration"), c.factor);
            settings.setValue(QStringLiteral("calibrationLiters"), c.liters);
            settings.setValue(QStringLiteral("calibrationFillUps"), settings.value(QStringLiteral("calibrationFillUps")).toInt() + 1);
            TripComputer::Settings ts = m_trip.settings();
            ts.calibration = c.factor;
            m_trip.setSettings(ts);
            const int percent = qRound(qAbs(c.measuredFactor - 1.0) * 100);
            emit notice(true, c.measuredFactor >= 1.0
                                  ? tr("Calibrated: the pump took %1 % more than Obdaryn estimated. Fuel figures now include the correction.").arg(percent)
                                  : tr("Calibrated: the pump took %1 % less than Obdaryn estimated. Fuel figures now include the correction.").arg(percent));
            break;
        }
        case TripComputer::Calibration::Result::TooLittleFuel:
            emit notice(false, tr("Too little driving since the last fill-up to compare (under 5 L estimated). Counting starts again from this fill-up."));
            break;
        case TripComputer::Calibration::Result::TooFarOff:
            emit notice(false, tr("The pump and the estimate are more than twice apart, so this fill-up wasn't used. Usually some drives happened without Obdaryn. Counting starts again from this fill-up."));
            break;
        }
    }
    // The tank is full now, whatever the outcome: count from here.
    settings.setValue(QStringLiteral("fillUpStarted"), true);
    settings.setValue(QStringLiteral("rawSinceFillUp"), 0.0);
    settings.endGroup();
    refreshTrip();
}

void AppController::resetFuelCalibration()
{
    QSettings settings;
    settings.beginGroup(tripKey());
    settings.remove(QStringLiteral("calibration"));
    settings.remove(QStringLiteral("calibrationLiters"));
    settings.remove(QStringLiteral("calibrationFillUps"));
    settings.endGroup();
    TripComputer::Settings ts = m_trip.settings();
    ts.calibration = 1.0;
    m_trip.setSettings(ts);
    refreshTrip();
}

// ---------------------------------------------------------------------------
// Data for QML

QVariantList AppController::modules() const
{
    QVariantList list;
    QStringList addresses = m_info.ecuNames.keys();
    addresses.sort();
    for (const QString &a : addresses)
        list << QVariantMap{{QStringLiteral("address"), a}, {QStringLiteral("name"), m_info.ecuNames.value(a)}};
    return list;
}

QVariantList AppController::monitors() const
{
    QVariantList list;
    for (const Monitor &m : m_readiness.monitors) {
        list << QVariantMap{{QStringLiteral("name"), m.translatedName()},
                            {QStringLiteral("complete"), m.complete},
                            {QStringLiteral("continuous"), m.continuous}};
    }
    return list;
}

QVariantList AppController::freezeFrameValues() const
{
    QVariantList list;
    for (const auto &r : m_freeze.values) {
        const PidDefinition *def = PidTable::find(r.pid);
        double v = 0;
        if (!def || !def->value(r.raw, &v))
            continue;
        list << QVariantMap{{QStringLiteral("name"), def->translatedName()},
                            {QStringLiteral("value"), QLocale().toString(Units::show(v, def->unit, def->decimals).value, 'f',
                                                                         Units::show(v, def->unit, def->decimals).decimals)},
                            {QStringLiteral("unit"), Units::show(v, def->unit, def->decimals).unit}};
    }
    return list;
}

int AppController::brandCodeCount() const
{
    return DtcDatabase::instance().brandSize(m_profileId);
}

QVariantMap AppController::explainCode(const QString &code, const QString &brand, const QString &module) const
{
    const DtcInfo info = DtcDatabase::instance().lookup(code, brand.isEmpty() ? m_profileId : brand, m_language, module);
    QVariantList causes;
    for (const auto &c : info.causes)
        causes << QVariantMap{{QStringLiteral("likelihood"), c.first}, {QStringLiteral("text"), c.second}};
    return {
        {QStringLiteral("code"), info.code},
        {QStringLiteral("known"), info.known},
        {QStringLiteral("manufacturerSpecific"), info.manufacturerSpecific},
        {QStringLiteral("title"), info.title},
        {QStringLiteral("description"), info.description},
        {QStringLiteral("system"), info.system},
        {QStringLiteral("causes"), causes},
        {QStringLiteral("symptoms"), info.symptoms},
        {QStringLiteral("turnsOnMil"), info.turnsOnMil},
        {QStringLiteral("emissionsRelevant"), info.emissionsRelevant},
        {QStringLiteral("needsDriveCycle"), info.needsDriveCycle},
        {QStringLiteral("limpModePossible"), info.limpModePossible},
        {QStringLiteral("repairDifficulty"), info.repairDifficulty},
        {QStringLiteral("diyPossible"), info.diyPossible},
        {QStringLiteral("costMinEur"), info.costMinEur},
        {QStringLiteral("costMaxEur"), info.costMaxEur},
        {QStringLiteral("related"), info.related},
        {QStringLiteral("warningLamp"), info.warningLamp},
        {QStringLiteral("source"), info.source},
        {QStringLiteral("sourceLabel"), info.sourceLabel},
        {QStringLiteral("language"), info.language},
        {QStringLiteral("confidence"), info.confidence},
        {QStringLiteral("references"), [&info] {
             QVariantList refs;
             for (int i = 0; i < info.references.size(); ++i)
                 refs << QVariantMap{{QStringLiteral("url"), info.references[i]},
                                     {QStringLiteral("official"), info.referenceOfficial.value(i)}};
             return refs;
         }()},
        {QStringLiteral("brandName"), info.brandName},
        {QStringLiteral("module"), info.module},
    };
}

void AppController::copyToClipboard(const QString &text) const
{
    if (QClipboard *cb = QGuiApplication::clipboard())
        cb->setText(text);
}

// ---------------------------------------------------------------------------
// Language

void AppController::setLanguage(const QString &code)
{
    const QString lang = code == QLatin1String("ro") ? QStringLiteral("ro") : QStringLiteral("en");

    QCoreApplication::removeTranslator(&m_appTranslator);
    QCoreApplication::removeTranslator(&m_qtTranslator);
    // Our strings (compiled into the app). The English file only carries plural
    // forms ("1 code" / "2 codes"); every other language translates everything.
    if (m_appTranslator.load(QStringLiteral(":/i18n/obdaryn_%1.qm").arg(lang)))
        QCoreApplication::installTranslator(&m_appTranslator);
    if (lang != QLatin1String("en")) {
        // Qt's own strings (standard dialogs).
        if (m_qtTranslator.load(QStringLiteral("qtbase_%1").arg(lang),
                                QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
            QCoreApplication::installTranslator(&m_qtTranslator);
    }
    QLocale::setDefault(QLocale(lang == QLatin1String("ro") ? QLocale::Romanian : QLocale::English));

    m_language = lang;
    QSettings().setValue(kLanguageKey, lang);
    // Re-evaluate every qsTr() binding. Deferred by one event-loop turn: the
    // first call happens while QML is still creating this singleton, inside a
    // binding, and retranslating right there would re-enter that binding.
    if (m_engine) {
        QPointer<QQmlEngine> engine = m_engine;
        QTimer::singleShot(0, this, [engine] {
            if (engine)
                engine->retranslate();
        });
    }
    m_liveData->retranslate();
    m_dtcs->setContext(m_profileId, lang);
    m_moduleModel->setLanguage(lang);
    refreshRecordings();         // car names and dates are localized
    m_charts->retranslate();
    m_manufacturerValues->refreshTexts(); // value and group names
    emit languageChanged();
    emit readinessChanged();     // monitor names
    emit freezeFrameChanged();   // PID names
    emit vehicleChanged();       // region names
    emit adapterInfoChanged();   // protocol names
    emit profileChanged();       // brand notes
}

QString AppController::appVersion() const
{
    return QCoreApplication::applicationVersion();
}

QString AppController::buildInfo() const
{
    return QStringLiteral("Qt %1 · %2").arg(QString::fromLatin1(qVersion()), QSysInfo::prettyProductName());
}

QString AppController::dtcAttribution() const
{
    return DtcDatabase::instance().attribution();
}

int AppController::dtcDatabaseSize() const
{
    return DtcDatabase::instance().size();
}

} // namespace obdaryn
