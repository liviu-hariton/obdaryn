// SPDX-License-Identifier: GPL-3.0-or-later
//
// TCP transport for Wi-Fi ELM327 adapters such as the Vgate iCar Pro Wi-Fi.

#pragma once

#include "transport/Transport.h"

#include <QTcpSocket>
#include <QTimer>

namespace obdaryn {

/**
 * @brief Connects to a Wi-Fi OBD adapter over TCP.
 *
 * Wi-Fi ELM327 adapters run their own access point (the Vgate iCar Pro calls
 * it "V-LINK") and expose the ELM327 as a raw TCP socket, usually at
 * 192.168.0.10:35000. Join that Wi-Fi network first, then connect.
 *
 * On macOS 15+ the app must be allowed to use the local network
 * (System Settings → Privacy & Security → Local Network), otherwise the
 * connection fails with "host unreachable".
 */
class TcpTransport : public Transport
{
    Q_OBJECT
public:
    explicit TcpTransport(QString host, quint16 port, QObject *parent = nullptr);

    void open() override;
    void close() override;
    bool isOpen() const override;
    void write(const QByteArray &data) override;
    QString description() const override;

    /// How long open() waits before giving up (default 6 s).
    void setConnectTimeoutMs(int ms) { m_connectTimer.setInterval(ms); }

private:
    void onErrorOccurred(QAbstractSocket::SocketError error);

    QString m_host;
    quint16 m_port;
    QTcpSocket m_socket;
    QTimer m_connectTimer;
};

} // namespace obdaryn
