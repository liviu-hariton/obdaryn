// SPDX-License-Identifier: GPL-3.0-or-later
//
// Asks obdaryn.eu whether a newer Obdaryn exists. Off unless the user turns it on.

#pragma once

#include <QDateTime>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

class QNetworkAccessManager;
class QNetworkReply;

namespace obdaryn {

/**
 * @brief The opt-in update check.
 *
 * What it does, and nothing more: a GET of https://obdaryn.eu/latest.json, a
 * file the website build writes with the current version ({"version":"0.4.17"}).
 * The request carries the app's version in its User-Agent ("Obdaryn/0.4.17") and
 * nothing about the user or the car. Obdaryn downloads and installs nothing: when
 * a newer version exists it says so and links to the download page, whose address
 * is fixed in the app (the server's answer can't send anyone elsewhere).
 *
 * Automatic checks run only when the user turned them on (Settings), at most once
 * a day, and fail silently: the adapter's Wi-Fi has no internet, so most checks
 * near the car can't get through. A check the user asks for reports its outcome.
 */
class UpdateChecker : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    /// Daily automatic checks (stored in the settings; off by default).
    Q_PROPERTY(bool automatic READ automatic WRITE setAutomatic NOTIFY changed)
    Q_PROPERTY(bool checking READ checking NOTIFY changed)
    /// A version newer than this one was announced.
    Q_PROPERTY(bool available READ available NOTIFY changed)
    /// The newest version obdaryn.eu announced, "0.4.18" (empty until a check got through).
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY changed)
    /// Where to get it, in the app's language.
    Q_PROPERTY(QString downloadUrl READ downloadUrl NOTIFY changed)

public:
    explicit UpdateChecker(const QString &currentVersion, QObject *parent = nullptr);

    bool automatic() const { return m_automatic; }
    void setAutomatic(bool on);
    bool checking() const { return !m_reply.isNull(); }
    bool available() const;
    QString latestVersion() const { return m_latest; }
    QString downloadUrl() const;
    void setLanguage(const QString &code);

    /// Asks now. `manual` = the user asked: the outcome is reported via finished().
    Q_INVOKABLE void check(bool manual = true);

signals:
    void changed();
    /// Outcome of a check the user asked for: a sentence to show.
    void finished(bool success, const QString &message);

private:
    void checkIfDue();

    QString m_current;
    QString m_latest;
    QString m_language = QStringLiteral("en");
    bool m_automatic = false;
    bool m_manual = false;
    QNetworkAccessManager *m_net = nullptr; ///< made on first use: no network code runs while checks are off
    QPointer<QNetworkReply> m_reply;
    QTimer m_daily;
};

} // namespace obdaryn
