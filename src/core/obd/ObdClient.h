// SPDX-License-Identifier: GPL-3.0-or-later
//
// High-level, asynchronous OBD-II operations built on ElmSession.

#pragma once

#include "elm327/ElmSession.h"
#include "obd/Dtc.h"
#include "obd/MonitorTests.h"
#include "obd/Readiness.h"
#include "obd/ResponseParser.h"

#include <QHash>
#include <QList>
#include <QObject>

#include <functional>
#include <memory>

namespace obdaryn {

/**
 * @brief "Read the trouble codes", "read engine speed", ... as single calls.
 *
 * ObdClient owns an ElmSession and translates diagnostic intents into ELM327
 * requests, then decodes the answers. Every operation is asynchronous and
 * reports through a callback that is always called exactly once.
 *
 * When can each operation be used? (see docs/CAR_INTERACTION.md)
 *  - Ignition ON, engine off: everything. Clearing codes ONLY in this state.
 *  - Engine running: everything except clearing (refused by clearDtcs()).
 */
class ObdClient : public QObject
{
    Q_OBJECT
public:
    explicit ObdClient(QObject *parent = nullptr);

    ElmSession *session() const { return m_session; }
    bool isCan() const { return obdaryn::isCan(m_session->protocol()); }

    // ---- Generic request ---------------------------------------------------

    /// The decoded answer to one request.
    struct Response {
        ElmSession::Reply reply;
        QList<EcuMessage> positive;   ///< messages whose first byte is SID + 0x40
        QList<EcuMessage> negative;   ///< 7F <sid> <code> messages
        bool ok() const { return !positive.isEmpty(); }
        /// Translated reason when !ok(): negative response text, NO DATA, timeout...
        QString errorText() const;
    };
    using ResponseCallback = std::function<void(const Response &)>;

    /// Sends one request (hex string, e.g. "010C") and splits the answer per ECU.
    void request(const QByteArray &command, ResponseCallback callback, ElmSession::Options options);
    void request(const QByteArray &command, ResponseCallback callback)
    {
        request(command, std::move(callback), ElmSession::Options{});
    }

    // ---- Live data (service 01) --------------------------------------------

    /// Asks which PIDs the car supports, walking the 00/20/40/... bitmaps.
    /// service: 0x01 (live data), 0x02 (freeze frame) or 0x09 (vehicle info).
    void readSupportedPids(quint8 service, std::function<void(const QList<quint8> &)> callback);

    struct PidReading {
        quint8 pid = 0;
        QByteArray raw;   ///< data bytes A, B, C, ...
        QString ecu;      ///< which module answered
    };
    /// Reads current values. On CAN, up to 6 PIDs are sent in one request,
    /// which makes live data several times faster than one-by-one polling.
    void readPids(const QList<quint8> &pids, std::function<void(const QList<PidReading> &)> callback);

    /// Readiness monitors + MIL (PID 01), taken from the engine ECU.
    void readMonitorStatus(std::function<void(const MonitorStatus &)> callback);

    /**
     * @brief Reads the on-board monitor test results (service 06).
     *
     * Asks which monitor IDs the car supports ("0600", "0620", ...), then
     * reads each one. CAN cars only: older protocols use a different,
     * manufacturer-specific layout. `error` is empty on success.
     */
    void readMonitorTests(std::function<void(const QList<MonitorTest> &, const QString &error)> callback);

    // ---- Trouble codes ------------------------------------------------------

    /// Reads one DTC memory (stored 03, pending 07 or permanent 0A) from all ECUs.
    void readDtcs(DtcKind kind, std::function<void(const QList<Dtc> &, const QString &error)> callback);
    /// Reads stored, pending and permanent codes in one go.
    void readAllDtcs(std::function<void(const QList<Dtc> &, const QString &error)> callback);

    struct FreezeFrame {
        bool available = false;
        QString dtc;                 ///< code that triggered the snapshot
        QList<PidReading> values;    ///< raw values; decode with PidTable
    };
    /// Reads freeze frame 0: the snapshot of sensor values taken when a code was stored.
    void readFreezeFrame(std::function<void(const FreezeFrame &)> callback);

    enum class ClearResult {
        Cleared,        ///< at least one ECU confirmed (0x44)
        EngineRunning,  ///< refused by Obdaryn: engine speed > 0
        Rejected,       ///< the ECU answered with a negative response
        NoResponse,     ///< nobody answered
    };
    /**
     * @brief Clears emission-related codes (service 04) on all ECUs.
     *
     * Call only after the user explicitly confirmed. Before sending, Obdaryn
     * reads the engine speed and refuses while the engine runs. Clearing also
     * erases freeze frames and resets all readiness monitors, so the car will
     * not pass an emissions test until it has been driven enough. Permanent
     * codes are not removed: they clear themselves once the car re-checks.
     */
    void clearDtcs(std::function<void(ClearResult, const QString &detail)> callback);

    // ---- Vehicle information (service 09) and adapter ------------------------

    struct VehicleInformation {
        QString vin;
        QStringList calibrationIds;   ///< software versions of emission-related ECUs
        QStringList cvns;             ///< calibration verification numbers (checksums)
        QHash<QString, QString> ecuNames; ///< address → "ECM-EngineControl"
    };
    void readVehicleInfo(std::function<void(const VehicleInformation &)> callback);

    /// Battery voltage measured by the adapter at the OBD socket (AT RV).
    void readBatteryVoltage(std::function<void(double volts)> callback);

    /// Translated meaning of a UDS / KWP negative response code.
    static QString negativeResponseText(quint8 nrc);

    // ---- Adapter capabilities found while talking -----------------------------

    /// Whether combined requests ("010C0D05") work; false once an ECU ignored them.
    bool multiPidSupported() const { return m_multiPidSupported; }

    /// Response counts: the ELM327 accepts one extra hex digit at the end of a
    /// request ("010C1") saying how many answers to expect. It then returns as
    /// soon as they arrived instead of waiting for its timeout, which can make
    /// live data much faster. Obdaryn adds it only when it knows which ECUs
    /// will answer and every answer fits in one CAN frame (with multi-frame
    /// answers some adapters count frames, not answers). Adapters that reject
    /// it get plain requests for the rest of the connection.
    enum class ResponseCounts { Unknown, Working, NotSupported, Off };
    ResponseCounts responseCounts() const { return m_responseCounts; }
    /// The probe's benchmark compares polling with and without them.
    void setResponseCountsEnabled(bool enabled);

private:
    struct SupportedState;
    struct PidBatchState;
    struct FreezeState;
    struct InfoState;
    void supportedStep(std::shared_ptr<SupportedState> state);
    void pidBatchStep(std::shared_ptr<PidBatchState> state);
    void freezeStep(std::shared_ptr<FreezeState> state);
    void infoStep(std::shared_ptr<InfoState> state);

    /// The response-count digit for a live-data request, or 0 for none.
    int expectedResponses(const QList<quint8> &group) const;

    ElmSession *m_session;
    bool m_multiPidSupported = true; ///< turned off if the ECU ignores extra PIDs
    bool m_responseCountsEnabled = true;
    ResponseCounts m_responseCounts = ResponseCounts::Unknown;
    QHash<QString, QSet<quint8>> m_ecuPids; ///< service 01 PIDs per answering ECU
};

/// Builds an ELM327 hex command: hexCommand({0x01, 0x0C}) → "010C".
QByteArray hexCommand(std::initializer_list<quint8> bytes);
QByteArray hexCommand(const QList<quint8> &bytes);

} // namespace obdaryn
