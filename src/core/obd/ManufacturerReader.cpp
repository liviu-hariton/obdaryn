// SPDX-License-Identifier: GPL-3.0-or-later

#include "obd/ManufacturerReader.h"

#include "obd/ModuleAddressing.h"
#include "obd/ObdClient.h"

#include <QHash>

#include <algorithm>

#include <memory>

namespace obdaryn {

namespace {

// An absent module costs this much per request; present ones answer in ~50 ms.
constexpr int kRequestTimeoutMs = 1000;

struct ReadState {
    ObdClient *client = nullptr;
    QList<ManufacturerCommand> commands;
    QList<QList<int>> groups;   ///< command indices per module
    QList<ManufacturerReader::Result> results;
    int group = 0, item = 0, finished = 0;
    bool lost = false; ///< the connection went away: skip what's left
    std::function<void(const QList<ManufacturerReader::Result> &)> done;
    std::function<void(int, int)> progress;
};

void step(const std::shared_ptr<ReadState> &st);

void finish(const std::shared_ptr<ReadState> &st)
{
    ModuleAddressing::restore(st->client->session(), [st] { st->done(st->results); });
}

void readItem(const std::shared_ptr<ReadState> &st)
{
    if (st->lost) {
        // Nothing more can be asked; mark the rest unknown and finish.
        for (ManufacturerReader::Result &r : st->results) {
            if (!r.answered && !r.refused)
                r.lostConnection = true;
        }
        finish(st);
        return;
    }
    const QList<int> &indices = st->groups[st->group];
    if (st->item >= indices.size()) {
        ++st->group;
        st->item = 0;
        step(st);
        return;
    }
    const int index = indices[st->item++];
    const ManufacturerCommand &c = st->commands[index];
    st->client->request(c.request(), [st, index](const ObdClient::Response &r) {
        const ManufacturerCommand &c = st->commands[index];
        ManufacturerReader::Result &res = st->results[index];
        const QString receiver = QString::fromLatin1(ModuleAddressing::hex3(c.receiver));
        for (const EcuMessage &m : r.positive) {
            if ((m.ecu.isEmpty() || m.ecu == receiver) && ManufacturerReader::decodeAnswer(c, m.data, &res.values)) {
                res.answered = true;
                break;
            }
        }
        res.refused = !res.answered && !r.negative.isEmpty();
        if (!res.answered && r.reply.status == ElmSession::Reply::Status::NotConnected)
            st->lost = true;
        if (st->progress)
            st->progress(++st->finished, int(st->commands.size()));
        readItem(st);
    }, ElmSession::Options{.timeoutMs = kRequestTimeoutMs});
}

void step(const std::shared_ptr<ReadState> &st)
{
    if (st->group >= st->groups.size()) {
        finish(st);
        return;
    }
    const ManufacturerCommand &first = st->commands[st->groups[st->group].first()];
    ModuleAddressing::address(st->client->session(), first.header, first.receiver, first.flowControlHeader,
                              [st](bool ok) {
        // The adapter didn't take the header: the connection is gone, or it
        // refuses module addressing. Either way nothing can be learned now, and
        // "no answer" must not be mistaken for "not supported".
        if (!ok)
            st->lost = true;
        readItem(st);
    });
}

} // namespace

bool ManufacturerReader::interrupted(const QList<Result> &results)
{
    return std::any_of(results.cbegin(), results.cend(), [](const Result &r) { return r.lostConnection; });
}

bool ManufacturerReader::decodeAnswer(const ManufacturerCommand &command, const QByteArray &answer, QList<Value> *values)
{
    // "61 01 ..." / "62 E0 10 ...": the service + 0x40, then the identifier echoed.
    const int head = 1 + int(command.identifier.size());
    if (answer.size() < head || quint8(answer[0]) != command.service + 0x40
        || answer.mid(1, command.identifier.size()) != command.identifier)
        return false;
    const QByteArray data = answer.mid(head);
    values->clear();
    for (int i = 0; i < command.signalList.size(); ++i) {
        double v = 0;
        if (command.signalList[i].decode(data, &v))
            values->append({i, v});
    }
    return true;
}

void ManufacturerReader::read(ObdClient *client, const QList<ManufacturerCommand> &commands,
                              std::function<void(const QList<Result> &)> done,
                              std::function<void(int, int)> progress)
{
    auto st = std::make_shared<ReadState>();
    st->client = client;
    st->commands = commands;
    st->done = std::move(done);
    st->progress = std::move(progress);
    // One group per module (same request and answer IDs and flow-control need).
    QHash<QString, int> groupOf;
    for (int i = 0; i < commands.size(); ++i) {
        const ManufacturerCommand &c = commands[i];
        Result r;
        r.key = c.key();
        st->results << r;
        const QString module = QStringLiteral("%1/%2/%3").arg(c.header).arg(c.receiver).arg(c.flowControlHeader);
        if (!groupOf.contains(module)) {
            groupOf.insert(module, int(st->groups.size()));
            st->groups << QList<int>{};
        }
        st->groups[groupOf.value(module)] << i;
    }
    if (st->groups.isEmpty()) {
        st->done(st->results);
        return;
    }
    step(st);
}

} // namespace obdaryn
