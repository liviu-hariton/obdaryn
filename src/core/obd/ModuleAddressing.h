// SPDX-License-Identifier: GPL-3.0-or-later
//
// Pointing the adapter at one module, and back at all OBD-II ECUs.

#pragma once

#include <QByteArray>

#include <functional>

namespace obdaryn {

class ElmSession;

/**
 * @brief The adapter commands that address one module directly.
 *
 * Used by the module scan and by manufacturer values. Every command here is
 * an adapter setting (it never reaches the car) and is on the safety guard's
 * allowlist: AT SH (request header), AT CRA (listen only to the module's
 * answers) and the standard ISO-TP flow-control frame.
 */
namespace ModuleAddressing {

/// "7D1" for 0x7D1: the three hex digits the adapter expects.
QByteArray hex3(quint16 id);

/// Sends requests to `tx` and accepts answers from `rx` only. When the module
/// answers on an ID that isn't tx + 8, multi-frame answers need the flow-control
/// frame sent to `tx` explicitly (`flowControlHeader`). done(ok) runs once, after
/// the last command; ok tells whether the adapter took the header.
void address(ElmSession *session, quint16 tx, quint16 rx, bool flowControlHeader, std::function<void(bool ok)> done);

/// Back to the standard OBD-II addressing: functional 7DF, every ECU's answer,
/// automatic flow control. Always call it after address().
void restore(ElmSession *session, std::function<void()> done);

} // namespace ModuleAddressing
} // namespace obdaryn
