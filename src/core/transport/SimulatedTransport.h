// SPDX-License-Identifier: GPL-3.0-or-later
//
// A fake ELM327 + car, used for demo mode and for automated tests.

#pragma once

#include "transport/Transport.h"

#include <QElapsedTimer>
#include <QList>
#include <QPair>

namespace obdaryn {

/**
 * @brief Pretends to be an ELM327 adapter plugged into a 2014 Hyundai Elantra.
 *
 * It speaks the same text protocol as a real adapter (echo, headers, spaces,
 * the '>' prompt, ISO-TP multi-frame responses, "SEARCHING...", "NO DATA"),
 * so every layer above it runs exactly as it would against real hardware.
 *
 * Two ECUs answer generic OBD-II: the engine (7E0 → 7E8) and the
 * transmission (7E1 → 7E9). Addressed directly (module scan), more answer,
 * like on a 2014 Hyundai: ABS 7D1 (KWP2000, active wheel-speed fault C0035),
 * airbag 7D2 (UDS, stored U0100), body control 7A0 (KWP) and instrument
 * cluster 7C6 (UDS), both without codes. TPMS 7D6 answers a keep-alive but
 * ignores code requests (an "error" row); steering and climate stay silent
 * ("not found").
 * The engine has two stored codes (P0133, P0420), one pending (P0171) and one
 * permanent (P0420), a freeze frame, VIN / calibration / ECU name, and live
 * values that move over time. Clearing codes works only with the engine off,
 * like on many real cars, and — again like a real car — it does not remove the
 * permanent code and it resets the readiness monitors.
 */
class SimulatedTransport : public Transport
{
    Q_OBJECT
public:
    explicit SimulatedTransport(QObject *parent = nullptr);

    void open() override;
    void close() override;
    bool isOpen() const override { return m_open; }
    void write(const QByteArray &data) override;
    QString description() const override;

    /// Engine running or just ignition on. Affects live values and clearing.
    void setEngineRunning(bool running) { m_engineRunning = running; }
    bool engineRunning() const { return m_engineRunning; }

    /// Delay before each reply (default 20 ms). Tests set 0.
    void setLatencyMs(int ms) { m_latencyMs = ms; }

private:
    struct EcuReply {
        QString header;       ///< e.g. "7E8"
        QByteArray payload;   ///< response bytes starting with the response SID
    };

    QByteArray handleLine(const QByteArray &line);
    QByteArray handleAt(const QByteArray &body);
    QByteArray handleObd(const QByteArray &normalized);
    QList<EcuReply> engineReply(const QByteArray &req);
    QList<EcuReply> transmissionReply(const QByteArray &req);
    QList<EcuReply> moduleReply(const QByteArray &header, const QByteArray &req);
    QByteArray formatFrames(const EcuReply &reply) const;
    QByteArray joinBytes(const QByteArray &bytes) const;
    QByteArray pidData(quint8 pid) const;
    QByteArray freezeFrameData(quint8 pid) const;
    double seconds() const;
    void resetAdapterSettings();

    bool m_open = false;
    int m_latencyMs = 20;

    // Adapter settings changed by AT commands.
    bool m_echo = true;
    bool m_headers = false;
    bool m_spaces = true;
    bool m_linefeeds = false;
    bool m_protocolFound = false;
    QByteArray m_header = "7DF"; ///< target of requests (7DF = all OBD ECUs)

    // Car state.
    bool m_engineRunning = true;
    QList<quint16> m_stored{0x0133, 0x0420};
    QList<quint16> m_pending{0x0171};
    QList<quint16> m_permanent{0x0420};
    quint8 m_monitorsIncomplete = 0x05; ///< bit set = monitor not yet complete
    bool m_absFault = true;     ///< C0035 in the ABS module
    bool m_airbagFault = true;  ///< U0100 in the airbag module

    QByteArray m_rxBuffer;
    QElapsedTimer m_clock;
};

} // namespace obdaryn
