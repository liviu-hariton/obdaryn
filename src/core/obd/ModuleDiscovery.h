// SPDX-License-Identifier: GPL-3.0-or-later
//
// Module discovery: which diagnostic addresses on the car's CAN bus answer at all.

#pragma once

#include "vehicles/ModuleCatalog.h"

#include <QList>
#include <QObject>
#include <QSet>

namespace obdaryn {

class ObdClient;

/// One address that answered during discovery.
struct DiscoveredModule {
    quint16 tx = 0;        ///< request ID the probe was sent to, e.g. 0x7D1
    quint16 rx = 0;        ///< ID the answer came from, e.g. 0x7D9
    bool positive = false; ///< answered TesterPresent (7E 00); false = refused it (7F 3E xx), still present
    quint8 nrc = 0;        ///< the refusal's reason code when !positive
};

/**
 * @brief Finds the modules of a car whose addresses aren't known yet.
 *
 * Brand module lists (data/vehicles) only cover addresses someone has
 * confirmed. On a car whose brand has no list yet (Suzuki, today) discovery
 * asks every 11-bit diagnostic request ID in turn whether a module lives
 * there, so the ordinary module scan can then read the codes of whatever
 * answered.
 *
 * What it sends, per address, and nothing else (all on RequestGuard's
 * read-only allowlist):
 *   - AT SH <id>   the adapter's header: where the next request goes;
 *   - 3E 00        UDS/KWP TesterPresent, "are you there?". It reads nothing
 *                  and changes nothing in the module, in any session.
 * Before the sweep the adapter's receive filter is reset (AT CRA) so an answer
 * from any reply ID is seen; afterwards the standard addressing is restored
 * (AT SH 7DF). Any answer counts: a positive one (7E 00), or a refusal
 * (7F 3E xx), which still proves a module is there. Frames that aren't an
 * answer to TesterPresent (other traffic on the bus) are ignored.
 *
 * Addresses skipped: the functional OBD-II address 7DF, the standard reply IDs
 * 7E8-7EF, and every reply ID found during the sweep, so the probe never
 * transmits on an ID another module uses for its own answers.
 *
 * Run it with the ignition ON and the engine off. About 240 addresses at
 * ~0.2-0.3 s each: roughly a minute.
 */
class ModuleDiscovery : public QObject
{
    Q_OBJECT
public:
    explicit ModuleDiscovery(ObdClient *client, QObject *parent = nullptr);

    /// 0x700-0x7F7, without 7DF and 7E8-7EF.
    static QList<quint16> defaultCandidates();

    /// Starts the sweep. Ignored while running.
    void run(const QList<quint16> &candidates = defaultCandidates());
    /// Stops after the address being asked now, then restores the addressing.
    void cancel();

    bool running() const { return m_running; }
    int done() const { return m_index; }
    int total() const { return int(m_candidates.size()); }
    /// The address being asked now (0 when not running).
    quint16 currentId() const { return m_running && m_index < m_candidates.size() ? m_candidates[m_index] : 0; }
    /// The last run was stopped by cancel() before it asked every address.
    bool cancelled() const { return m_cancel; }
    const QList<DiscoveredModule> &found() const { return m_found; }

    /**
     * @brief The answering addresses as a profile the module scanner can read.
     * An address the car's own profile already lists keeps that entry (its
     * category and name); the others become "other" modules, unconfirmed.
     */
    VehicleProfile asProfile(const VehicleProfile &known) const;

signals:
    void progress(int done, int total);
    void moduleFound(const obdaryn::DiscoveredModule &module);
    void finished();

private:
    void next();

    ObdClient *m_client;
    QList<quint16> m_candidates;
    QList<DiscoveredModule> m_found;
    QSet<quint16> m_replyIds; ///< IDs modules answer from: never transmitted on
    int m_index = 0;
    bool m_running = false;
    bool m_cancel = false;
};

} // namespace obdaryn
