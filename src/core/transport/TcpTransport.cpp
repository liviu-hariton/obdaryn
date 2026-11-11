// SPDX-License-Identifier: GPL-3.0-or-later

#include "transport/TcpTransport.h"

namespace obdaryn {

TcpTransport::TcpTransport(QString host, quint16 port, QObject *parent)
    : Transport(parent), m_host(std::move(host)), m_port(port), m_socket(this)
{
    m_connectTimer.setSingleShot(true);
    m_connectTimer.setInterval(6000);

    connect(&m_connectTimer, &QTimer::timeout, this, [this] {
        m_socket.abort();
        setLastFailure(Failure::Timeout);
        emit failed(tr("No answer from %1. Is this computer connected to the adapter's Wi-Fi?")
                        .arg(description()));
    });

    connect(&m_socket, &QTcpSocket::connected, this, [this] {
        m_connectTimer.stop();
        // The ELM327 protocol is many tiny request/response pairs. Disabling
        // Nagle's algorithm stops the OS from holding small writes back.
        m_socket.setSocketOption(QAbstractSocket::LowDelayOption, 1);
        m_socket.setSocketOption(QAbstractSocket::KeepAliveOption, 1);
        emit opened();
    });

    connect(&m_socket, &QTcpSocket::disconnected, this, [this] { emit closed(); });

    connect(&m_socket, &QTcpSocket::readyRead, this, [this] {
        emit received(m_socket.readAll());
    });

    connect(&m_socket, &QTcpSocket::errorOccurred, this, &TcpTransport::onErrorOccurred);
}

void TcpTransport::open()
{
    if (m_socket.state() != QAbstractSocket::UnconnectedState)
        m_socket.abort();
    setLastFailure(Failure::None);
    m_connectTimer.start();
    m_socket.connectToHost(m_host, m_port);
}

void TcpTransport::close()
{
    m_connectTimer.stop();
    if (m_socket.state() != QAbstractSocket::UnconnectedState)
        m_socket.disconnectFromHost();
}

bool TcpTransport::isOpen() const
{
    return m_socket.state() == QAbstractSocket::ConnectedState;
}

void TcpTransport::write(const QByteArray &data)
{
    if (isOpen())
        m_socket.write(data);
}

QString TcpTransport::description() const
{
    return QStringLiteral("%1:%2").arg(m_host).arg(m_port);
}

void TcpTransport::onErrorOccurred(QAbstractSocket::SocketError error)
{
    m_connectTimer.stop();
    QString message;
    switch (error) {
    case QAbstractSocket::ConnectionRefusedError:
        setLastFailure(Failure::Refused);
        message = tr("The adapter refused the connection."); // the connect screen explains why
        break;
    case QAbstractSocket::RemoteHostClosedError:
        setLastFailure(Failure::Closed);
        message = tr("The adapter closed the connection.");
        break;
    case QAbstractSocket::HostNotFoundError:
        setLastFailure(Failure::Unreachable);
        message = tr("Address %1 not found.").arg(m_host);
        break;
    case QAbstractSocket::NetworkError:
        // "No route to host": either this computer isn't on the adapter's Wi-Fi,
        // or macOS blocks local-network access for the app.
        setLastFailure(Failure::Unreachable);
        message = tr("Can't reach the adapter at %1.").arg(description());
        break;
    case QAbstractSocket::SocketTimeoutError:
        setLastFailure(Failure::Timeout);
        message = tr("No answer from %1. Is this computer connected to the adapter's Wi-Fi?").arg(description());
        break;
    default:
        setLastFailure(Failure::Other);
        message = m_socket.errorString();
        break;
    }
    emit failed(message);
}

} // namespace obdaryn
