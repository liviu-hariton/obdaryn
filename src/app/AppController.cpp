// SPDX-License-Identifier: GPL-3.0-or-later

#include "AppController.h"

#include "obd/DtcDatabase.h"
#include "vehicles/ModuleCatalog.h"
#include "obd/Pids.h"
#include "obd/VehicleInfo.h"
#include "transport/SimulatedTransport.h"
#include "transport/TcpTransport.h"

#include <QClipboard>
#include <QCoreApplication>
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
} // namespace

AppController *AppController::create(QQmlEngine *qmlEngine, QJSEngine *)
{
    return new AppController(qmlEngine);
}

AppController::AppController(QQmlEngine *engine, QObject *parent)
    : QObject(parent),
      m_engine(engine),
      m_client(new ObdClient(this)),
      m_liveData(new LiveDataModel(this)),
      m_dtcs(new DtcModel(this)),
      m_trafficLog(new TrafficLogModel(this)),
      m_moduleScanner(new ModuleScanner(m_client, this)),
      m_moduleModel(new ModuleScanModel(m_moduleScanner, this))
{
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
}

// ---------------------------------------------------------------------------
// Connection

void AppController::connectToAdapter(const QString &host, int port)
{
    QSettings settings;
    settings.setValue(kHostKey, host.trimmed());
    settings.setValue(kPortKey, port);
    emit settingsChanged();
    startSession(new TcpTransport(host.trimmed(), quint16(port)), false);
}

void AppController::connectDemo()
{
    auto *sim = new SimulatedTransport;
    m_simulator = sim;
    startSession(sim, true);
    emit demoEngineRunningChanged();
}

void AppController::startSession(Transport *transport, bool demo)
{
    m_afterPoll = nullptr;
    m_pollingEnabled = false;
    m_pollTimer.stop();
    m_voltageTimer.stop();
    m_demo = demo;
    resetVehicleData();
    m_trafficLog->append(QStringLiteral("!"), demo ? tr("Starting demo car") : tr("Opening %1").arg(transport->description()));
    m_client->session()->setTransport(transport);
    m_client->session()->open();
    emit connectionChanged();
}

void AppController::disconnectAdapter()
{
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
        m_statusMessage = s->lastError();
        m_pollingEnabled = false;
        m_voltageTimer.stop();
        break;
    case ElmSession::State::Ready:
        m_statusMessage.clear();
        m_voltageTimer.start();
        readVoltage(); // right away, then every 5 s
        runFullScan();
        break;
    case ElmSession::State::Disconnected:
        m_statusMessage.clear();
        m_pollingEnabled = false;
        m_voltageTimer.stop();
        break;
    }
    emit connectionChanged();
    emit adapterInfoChanged();
}

AppController::ConnectionState AppController::connectionState() const
{
    return static_cast<ConnectionState>(m_client->session()->state());
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
                        readStatusPids();
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
            if (r.pid == 0x51 && m_fuelType.isEmpty())
                m_fuelType = PidTable::describeStatus(0x51, r.raw);
        }
        emit vehicleChanged();
    });
}

void AppController::refreshCodes()
{
    if (scanning() || busyWithModules())
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
    if (busyWithModules())
        return;
    m_client->readMonitorStatus([this](const MonitorStatus &status) {
        m_readiness = status;
        emit readinessChanged();
    });
}

void AppController::refreshFreezeFrame()
{
    if (busyWithModules())
        return;
    m_client->readFreezeFrame([this](const ObdClient::FreezeFrame &ff) {
        m_freeze = ff;
        emit freezeFrameChanged();
    });
}

void AppController::clearCodes()
{
    if (m_client->session()->state() != ElmSession::State::Ready) {
        emit clearFinished(false, tr("Not connected to a car."));
        return;
    }
    if (busyWithModules()) {
        emit clearFinished(false, tr("Wait for the module scan to finish."));
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
        m_samplesThisSecond += int(seen.size());
        if (m_rateClock.elapsed() >= 1000) {
            m_liveData->setSamplesPerSecond(m_samplesThisSecond * 1000.0 / m_rateClock.restart());
            m_samplesThisSecond = 0;
        }
        schedulePoll();
    });
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
                            {QStringLiteral("value"), QLocale().toString(v, 'f', def->decimals)},
                            {QStringLiteral("unit"), def->translatedUnit()}};
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

QString AppController::dtcAttribution() const
{
    return DtcDatabase::instance().attribution();
}

int AppController::dtcDatabaseSize() const
{
    return DtcDatabase::instance().size();
}

} // namespace obdaryn
