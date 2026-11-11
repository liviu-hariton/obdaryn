// SPDX-License-Identifier: GPL-3.0-or-later

#include "obd/ModuleAddressing.h"

#include "elm327/ElmSession.h"

#include <memory>

namespace obdaryn::ModuleAddressing {

QByteArray hex3(quint16 id)
{
    return QByteArray::number(id, 16).toUpper().rightJustified(3, '0');
}

void address(ElmSession *s, quint16 tx, quint16 rx, bool flowControlHeader, std::function<void(bool)> done)
{
    // The commands run in order; only the header decides success, and done()
    // is called exactly once, from the last command.
    auto headerOk = std::make_shared<bool>(false);
    s->send("ATSH" + hex3(tx), [headerOk](const ElmSession::Reply &r) { *headerOk = r.ok(); });
    // Only listen to this module's answers.
    s->send("ATCRA" + hex3(rx), [](const ElmSession::Reply &) {});
    if (flowControlHeader) {
        // Multi-frame answers need a standard "continue" frame sent to the module's own ID.
        s->send("ATFCSH" + hex3(tx), [](const ElmSession::Reply &) {});
        s->send("ATFCSD300000", [](const ElmSession::Reply &) {});
        s->send("ATFCSM1", [headerOk, done](const ElmSession::Reply &) { done(*headerOk); });
    } else {
        s->send("ATFCSM0", [headerOk, done](const ElmSession::Reply &) { done(*headerOk); });
    }
}

void restore(ElmSession *s, std::function<void()> done)
{
    s->send("ATFCSM0", [](const ElmSession::Reply &) {});
    s->send("ATCRA", [](const ElmSession::Reply &) {});                  // listen to everyone again
    s->send("ATSH7DF", [done](const ElmSession::Reply &) { done(); });  // functional OBD-II address
}

} // namespace obdaryn::ModuleAddressing
