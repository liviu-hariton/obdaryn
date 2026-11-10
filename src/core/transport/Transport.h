// SPDX-License-Identifier: GPL-3.0-or-later
//
// Abstract byte pipe to an ELM327 adapter (Wi-Fi today; BLE / USB later).

#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

namespace obdaryn {

/**
 * @brief A bidirectional byte stream to an adapter.
 *
 * The ELM327 protocol is plain text over a serial-like link, so every
 * transport (TCP for Wi-Fi adapters, Bluetooth LE, USB serial, the built-in
 * simulator) only has to move bytes. ElmSession sits on top and turns those
 * bytes into commands and replies.
 *
 * Implementations must emit opened() once the link is usable, received() for
 * every chunk of incoming bytes (chunks may split lines anywhere), and
 * failed() / closed() when the link goes away.
 */
class Transport : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;
    ~Transport() override = default;

    /// Starts connecting. Asynchronous: wait for opened() or failed().
    virtual void open() = 0;
    /// Closes the link. Emits closed() if it was open.
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    /// Writes raw bytes (ElmSession adds the trailing carriage return).
    virtual void write(const QByteArray &data) = 0;
    /// Short description for logs and the UI, e.g. "192.168.0.10:35000".
    virtual QString description() const = 0;

signals:
    void opened();
    void closed();
    void received(const QByteArray &data);
    void failed(const QString &message);
};

} // namespace obdaryn
