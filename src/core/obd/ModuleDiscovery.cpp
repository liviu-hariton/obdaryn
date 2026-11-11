// SPDX-License-Identifier: GPL-3.0-or-later

#include "obd/ModuleDiscovery.h"

#include "obd/ModuleAddressing.h"
#include "obd/ObdClient.h"

namespace obdaryn {

namespace {

// Bound for one "are you there?". A real ELM327 says NO DATA after its own
// timeout (~0.2 s); this only limits adapters that stay quiet instead.
constexpr int kProbeTimeoutMs = 800;

} // namespace

ModuleDiscovery::ModuleDiscovery(ObdClient *client, QObject *parent) : QObject(parent), m_client(client) {}

QList<quint16> ModuleDiscovery::defaultCandidates()
{
    QList<quint16> ids;
    for (quint16 id = 0x700; id <= 0x7F7; ++id) {
        if (id == 0x7DF || (id >= 0x7E8 && id <= 0x7EF))
            continue; // functional OBD-II address, and the standard reply IDs
        ids << id;
    }
    return ids;
}

void ModuleDiscovery::run(const QList<quint16> &candidates)
{
    if (m_running)
        return;
    m_candidates = candidates;
    m_found.clear();
    m_replyIds = {0x7E8, 0x7E9, 0x7EA, 0x7EB, 0x7EC, 0x7ED, 0x7EE, 0x7EF};
    m_index = 0;
    m_running = true;
    m_cancel = false;
    emit progress(0, total());

    // Listen to every reply ID: the module's answer address is what we're after.
    m_client->session()->send("ATCRA", [this](const ElmSession::Reply &) { next(); });
}

void ModuleDiscovery::cancel()
{
    m_cancel = true;
}

void ModuleDiscovery::next()
{
    // Skip IDs that turned out to be some module's reply address.
    while (m_index < m_candidates.size() && m_replyIds.contains(m_candidates[m_index]))
        ++m_index;

    if (m_cancel || m_index >= m_candidates.size()) {
        ModuleAddressing::restore(m_client->session(), [this] {
            m_running = false;
            emit progress(total(), total());
            emit finished();
        });
        return;
    }

    const quint16 tx = m_candidates[m_index];
    m_client->session()->send("ATSH" + ModuleAddressing::hex3(tx), [this, tx](const ElmSession::Reply &header) {
        if (!header.ok()) { // the adapter refused the header: nothing sent to the car
            ++m_index;
            emit progress(m_index, total());
            next();
            return;
        }
        m_client->request("3E00", [this, tx](const ObdClient::Response &r) {
            auto record = [this, tx](const EcuMessage &m, bool positive) {
                bool ok = false;
                const uint rx = m.ecu.toUInt(&ok, 16);
                // 11-bit replies only, each address once.
                if (!ok || m.ecu.size() != 3 || rx == tx)
                    return;
                for (const DiscoveredModule &d : std::as_const(m_found)) {
                    if (d.tx == tx && d.rx == rx)
                        return;
                }
                DiscoveredModule d;
                d.tx = tx;
                d.rx = quint16(rx);
                d.positive = positive;
                d.nrc = (!positive && m.data.size() >= 3) ? quint8(m.data[2]) : 0;
                m_found << d;
                m_replyIds.insert(d.rx);
                emit moduleFound(d);
            };
            for (const EcuMessage &m : r.positive) {
                if (m.data.size() >= 1 && quint8(m.data[0]) == 0x7E)
                    record(m, true);
            }
            for (const EcuMessage &m : r.negative) {
                if (m.data.size() >= 2 && quint8(m.data[1]) == 0x3E)
                    record(m, false);
            }
            ++m_index;
            emit progress(m_index, total());
            next();
        }, ElmSession::Options{.timeoutMs = kProbeTimeoutMs});
    });
}

VehicleProfile ModuleDiscovery::asProfile(const VehicleProfile &known) const
{
    VehicleProfile p;
    p.id = known.id;
    p.label = known.label;
    for (const DiscoveredModule &d : m_found) {
        ModuleDef m;
        bool listed = false;
        for (const ModuleDef &k : known.modules) {
            if (k.tx == d.tx && k.rx == d.rx) {
                m = k;
                listed = true;
                break;
            }
        }
        if (!listed) {
            m.id = QStringLiteral("found-%1").arg(ModuleAddressing::hex3(d.tx).toLower());
            m.category = QStringLiteral("other");
            m.tx = d.tx;
            m.rx = d.rx;
            m.dtc = DiagProtocol::Auto;
            m.verified = false;
            m.source = QStringLiteral("Found by module discovery");
        }
        p.modules << m;
    }
    return p;
}

} // namespace obdaryn
