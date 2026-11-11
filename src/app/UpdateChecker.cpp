// SPDX-License-Identifier: GPL-3.0-or-later

#include "UpdateChecker.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QVersionNumber>

namespace obdaryn {

namespace {

// Settings keys.
const QString kAutomatic = QStringLiteral("updates/automatic");
const QString kLastCheck = QStringLiteral("updates/lastCheck");
const QString kLatest = QStringLiteral("updates/latest");

constexpr int kTimeoutMs = 10000;
constexpr qint64 kDaySecs = 20 * 3600; // "once a day", with slack for a morning launch

QUrl feedUrl()
{
    // OBDARYN_UPDATE_URL points the check at a local file for testing.
    const QString overrideUrl = qEnvironmentVariable("OBDARYN_UPDATE_URL");
    return QUrl(overrideUrl.isEmpty() ? QStringLiteral("https://obdaryn.eu/latest.json") : overrideUrl);
}

} // namespace

UpdateChecker::UpdateChecker(const QString &currentVersion, QObject *parent)
    : QObject(parent), m_current(currentVersion)
{
    QSettings s;
    m_automatic = s.value(kAutomatic, false).toBool();
    m_latest = s.value(kLatest).toString(); // so a known update is shown even offline

    // While the app stays open: look again every few hours whether a day has passed.
    m_daily.setInterval(3 * 3600 * 1000);
    connect(&m_daily, &QTimer::timeout, this, &UpdateChecker::checkIfDue);
    if (m_automatic) {
        m_daily.start();
        QTimer::singleShot(5000, this, &UpdateChecker::checkIfDue); // after start-up, not during it
    }
}

void UpdateChecker::setAutomatic(bool on)
{
    if (on == m_automatic)
        return;
    m_automatic = on;
    QSettings().setValue(kAutomatic, on);
    if (on) {
        m_daily.start();
        checkIfDue();
    } else {
        m_daily.stop();
    }
    emit changed();
}

bool UpdateChecker::available() const
{
    if (m_latest.isEmpty())
        return false;
    return QVersionNumber::fromString(m_latest) > QVersionNumber::fromString(m_current);
}

QString UpdateChecker::downloadUrl() const
{
    return m_language == QLatin1String("ro") ? QStringLiteral("https://obdaryn.eu/ro/descarca/")
                                             : QStringLiteral("https://obdaryn.eu/download/");
}

void UpdateChecker::setLanguage(const QString &code)
{
    if (code == m_language)
        return;
    m_language = code;
    emit changed();
}

void UpdateChecker::checkIfDue()
{
    if (!m_automatic || checking())
        return;
    const QDateTime last = QSettings().value(kLastCheck).toDateTime();
    if (last.isValid() && last.secsTo(QDateTime::currentDateTimeUtc()) < kDaySecs)
        return;
    check(false);
}

void UpdateChecker::check(bool manual)
{
    if (checking()) {
        m_manual = m_manual || manual;
        return;
    }
    m_manual = manual;
    if (!m_net)
        m_net = new QNetworkAccessManager(this);

    QNetworkRequest request(feedUrl());
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Obdaryn/%1").arg(m_current));
    request.setTransferTimeout(kTimeoutMs);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    m_reply = m_net->get(request);
    emit changed();

    connect(m_reply, &QNetworkReply::finished, this, [this, reply = m_reply.data()] {
        reply->deleteLater();
        const bool manual = m_manual;
        QString version;
        if (reply->error() == QNetworkReply::NoError) {
            const QJsonObject o = QJsonDocument::fromJson(reply->read(4096)).object();
            const QString v = o.value(QStringLiteral("version")).toString();
            if (!QVersionNumber::fromString(v).isNull())
                version = v;
        }
        m_reply = nullptr;

        if (version.isEmpty()) {
            emit changed();
            if (manual) {
                emit finished(false, reply->error() == QNetworkReply::NoError
                    ? tr("obdaryn.eu answered, but not with a version number. Try again later.")
                    : tr("Couldn't reach obdaryn.eu. The adapter's Wi-Fi has no internet: check again when your Mac is online."));
            }
            return; // automatic checks fail silently and try again later
        }

        QSettings s;
        s.setValue(kLastCheck, QDateTime::currentDateTimeUtc());
        s.setValue(kLatest, version);
        m_latest = version;
        emit changed();
        if (manual) {
            emit finished(true, available() ? tr("Obdaryn %1 is available. You have %2.").arg(m_latest, m_current)
                                            : tr("You have the latest version, Obdaryn %1.").arg(m_current));
        }
    });
}

} // namespace obdaryn
