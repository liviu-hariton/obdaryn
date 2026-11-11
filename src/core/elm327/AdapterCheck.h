// SPDX-License-Identifier: GPL-3.0-or-later
//
// What an ELM327-compatible adapter really supports.

#pragma once

#include <QList>
#include <QString>

#include <functional>

namespace obdaryn {

class ElmSession;

/**
 * @brief Asks the adapter a few read-only questions and sums up what it can do.
 *
 * Almost every adapter sold today is ELM327-*compatible*: its own firmware
 * on another chip, reporting an ELM327 version number. That is normal and
 * most work well, but some skip commands their claimed version should know,
 * and a few report "v1.5", a version Elm Electronics never released (they
 * went from 1.4b to 2.0). The check:
 *
 *  - reads the identification (AT I, AT @1, AT @2);
 *  - tries commands every genuine v1.4+ knows: AT IGN, AT CS, AT PPS;
 *  - checks flow-control settings (AT FC SM 0), which module scans of
 *    VW-group, Renault/Dacia and GM cars need;
 *  - reads the battery voltage (AT RV).
 *
 * Every command is read-only, on the allowlist, and never reaches the car.
 */
class AdapterCheck
{
public:
    struct Item {
        QString command;   ///< e.g. "AT IGN"
        QString purpose;   ///< translated: what it is for
        bool supported = false;
        QString answer;    ///< what the adapter said
    };

    enum class Verdict {
        Genuine,      ///< reports a real ELM327 version and knows every command tried
        Compatible,   ///< ELM327-compatible: some commands missing, or a version that doesn't exist
        Unknown,      ///< doesn't identify as an ELM327
    };

    struct Result {
        QString identification;  ///< "ELM327 v1.5"
        QString version;         ///< "1.5"
        QString description;     ///< AT @1
        QString deviceId;        ///< AT @2, if any
        Verdict verdict = Verdict::Unknown;
        bool flowControl = false; ///< can set flow control (module scans of some brands)
        QList<Item> items;
        /// Translated, calm one-line summary for the UI.
        QString summary() const;
    };

    /// Runs the check on a Ready session; `done` is called once.
    static void run(ElmSession *session, std::function<void(const Result &)> done);

    /// Exposed for tests: the verdict from the identification and the answers.
    static Verdict verdictFor(const QString &version, const QList<Item> &items);
};

} // namespace obdaryn
