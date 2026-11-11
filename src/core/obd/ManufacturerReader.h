// SPDX-License-Identifier: GPL-3.0-or-later
//
// Reads manufacturer values from the car's modules.

#pragma once

#include "obd/ManufacturerData.h"

#include <QList>
#include <QString>

#include <functional>

namespace obdaryn {

class ObdClient;

/**
 * @brief Sends manufacturer requests (services 0x21 / 0x22) and decodes the answers.
 *
 * Commands are grouped by module, so the adapter is pointed at each module
 * once (ModuleAddressing), and the standard OBD-II addressing is restored at
 * the end, whatever happened. The caller makes sure no live-data poll or module
 * scan runs meanwhile, since they need the standard addressing.
 */
class ManufacturerReader
{
public:
    struct Value {
        int signal = 0;   ///< index into the command's signalList
        double value = 0; ///< in the signal's OBDb unit (see ManufacturerSignal::toMetric)
    };
    struct Result {
        QString key;           ///< ManufacturerCommand::key()
        bool answered = false; ///< a positive answer came back
        bool refused = false;  ///< the module answered "not supported" (negative response)
        /// The connection was lost before this command got an answer: nothing is
        /// known about it. Once that happens, the remaining commands aren't sent.
        bool lostConnection = false;
        QList<Value> values;
    };
    /// True if any result lost the connection (the read is incomplete).
    static bool interrupted(const QList<Result> &results);

    /// Reads every command once; done() gets one result per command, in order.
    /// progress(requests done so far, total), if given, runs after each request.
    static void read(ObdClient *client, const QList<ManufacturerCommand> &commands,
                     std::function<void(const QList<Result> &)> done,
                     std::function<void(int done, int total)> progress = {});

    /// Decodes a positive answer ("61 01 ..." / "62 E0 10 ..."): the values of
    /// every signal it holds. Returns false if it isn't this command's answer.
    static bool decodeAnswer(const ManufacturerCommand &command, const QByteArray &answer, QList<Value> *values);
};

} // namespace obdaryn
