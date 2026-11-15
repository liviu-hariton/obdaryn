// SPDX-License-Identifier: GPL-3.0-or-later
//
// Full-vehicle scan: trouble codes and identification from every known module.

#pragma once

#include "obd/Dtc.h"
#include "vehicles/ModuleCatalog.h"

#include <QList>
#include <QSet>
#include <QObject>

#include <functional>

namespace obdaryn {

class ObdClient;

/// A trouble code read from a module with UDS or KWP2000.
struct ModuleDtc {
    QString code;                ///< "C0035"
    bool hasFailureType = false; ///< UDS codes carry a third byte, the failure type
    quint8 failureType = 0;      ///< e.g. 0x1A; what kind of fault (short, open, implausible...)
    quint8 status = 0;           ///< raw status byte, for power users
    DtcKind kind = DtcKind::Stored;
    bool activeNow = false;      ///< the fault is present right now
    bool warningLamp = false;    ///< the module asks for a warning light

    /// "C0035" or "C0035-1A" when a failure type is present.
    QString displayCode() const;
};

/// What one module answered during a scan.
struct ModuleResult {
    enum class State {
        Waiting,   ///< not scanned yet
        Scanning,  ///< being asked right now
        Absent,    ///< no answer: not fitted, different address, or on another bus
        Answered,  ///< answered; dtcs is valid (may be empty)
        ObdOnly,   ///< answers generic OBD-II, but not manufacturer code requests
        Error,     ///< present, but the codes couldn't be read
    };
    ModuleDef module;
    State state = State::Waiting;
    DiagProtocol protocol = DiagProtocol::Auto; ///< protocol that worked
    QList<ModuleDtc> dtcs;
    QString partNumber;
    QString softwareVersion;
    QString error;
};

/// Decoders for module trouble-code answers (exposed for tests).
namespace ModuleDtcCodec {
/// UDS 0x59 0x02 answer: availability mask, then 4 bytes per code (3 DTC + status).
QList<ModuleDtc> fromUds(const QByteArray &payload);
/// KWP2000 0x58 answer: count, then 3 bytes per code (2 DTC + status).
QList<ModuleDtc> fromKwp(const QByteArray &payload);
/// Printable text from an identification answer (part numbers etc.).
QString asciiField(const QByteArray &bytes);
}

/**
 * @brief Asks every module of a vehicle profile for its trouble codes.
 *
 * It first sends one generic "0100" to learn which modules answer standard
 * OBD-II at all. Then, for each module the scanner:
 *  1. points the adapter at it (AT SH <request ID>, AT CRA <response ID>, and
 *     the standard flow-control header when the brand doesn't use the +8 rule);
 *  2. reads the codes with UDS 0x19 02 FF or KWP2000 0x18 02 FF 00, falling
 *     back from one to the other when the module refuses the first (a module
 *     that answers "busy" is asked again a couple of times first);
 *  3. reads its spare-part number and software version (UDS 0x22, KWP 0x1A).
 *
 * Before the first module it reads the engine speed: some modules only give
 * their codes with the engine off, and the advice shown for a refusal says so.
 * A module that refuses both protocols but answers generic OBD-II (usually the
 * engine ECU) is reported as "OBD-II only", since its codes come from the
 * generic scan.
 *
 * Afterwards it always restores the standard OBD-II addressing (7DF, all
 * modules), so live data and generic requests keep working. Everything sent
 * is read-only; clearing a module's codes is a separate, confirmed action.
 *
 * Modules are asked one at a time. A missing module costs one adapter
 * timeout (~0.2–0.5 s), so a full scan typically takes 5–15 seconds.
 */
class ModuleScanner : public QObject
{
    Q_OBJECT
public:
    explicit ModuleScanner(ObdClient *client, QObject *parent = nullptr);

    /// Starts scanning every module of the profile. Ignored while running.
    void scan(const VehicleProfile &profile);
    /// Stops after the module being asked now, then restores the addressing.
    void cancel();
    bool running() const { return m_running; }
    const QList<ModuleResult> &results() const { return m_results; }
    QString profileId() const { return m_profileId; }

    /**
     * @brief Clears one module's codes (UDS/KWP 0x14). Call only after the
     *        user confirmed. Refuses while the engine runs, then re-reads the
     *        module's codes so the list shows what remains.
     */
    void clearModule(int index, std::function<void(bool ok, const QString &message)> callback);

signals:
    void started(int moduleCount);
    void moduleUpdated(int index);
    void finished();

private:
    void scanNext();
    void address(const ModuleDef &m, std::function<void(bool ok)> done);
    void restoreAddressing(std::function<void()> done);
    /// One trouble-code request and what came before it for this module.
    struct DtcAttempt {
        DiagProtocol protocol = DiagProtocol::Auto;
        bool isFallback = false;  ///< the other protocol was tried already (or no fallback wanted)
        int busyRetries = 0;      ///< times the module said "busy, ask again" (NRC 0x21)
        quint8 firstRefusal = 0;  ///< negative response code of the first protocol, 0 for none
    };
    void readDtcs(int index, DtcAttempt attempt, std::function<void()> done);
    /// Sets the Error / ObdOnly state of a module that refused both protocols.
    void explainRefusal(int index, quint8 nrc);
    void readIdentification(int index, std::function<void()> done);

    ObdClient *m_client;
    QList<ModuleResult> m_results;
    QSet<quint16> m_obdResponders; ///< response IDs that answered a generic 0100
    double m_engineRpm = -1;       ///< engine speed when the scan started; -1 if unknown
    QString m_profileId;
    int m_index = -1;
    bool m_running = false;
    bool m_cancel = false;
};

} // namespace obdaryn
