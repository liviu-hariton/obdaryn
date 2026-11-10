// SPDX-License-Identifier: GPL-3.0-or-later
//
// Obdaryn desktop entry point.

#include "AppController.h"
#include "IconProvider.h"

#include <QDirIterator>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSettings>
#include <QTimer>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    // These names decide where QSettings stores preferences
    // (~/Library/Preferences/ro.obdaryn.Obdaryn.plist on macOS).
    QGuiApplication::setOrganizationName(QStringLiteral("Obdaryn"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("obdaryn.ro"));
    QGuiApplication::setApplicationName(QStringLiteral("Obdaryn"));
    QGuiApplication::setApplicationVersion(QStringLiteral(OBDARYN_VERSION));

    // Bundled typefaces (SIL Open Font License), so the app looks the same on
    // every platform. They live in qrc:/fonts.
    QDirIterator fonts(QStringLiteral(":/fonts"), {QStringLiteral("*.ttf"), QStringLiteral("*.otf")},
                       QDir::Files, QDirIterator::Subdirectories);
    while (fonts.hasNext())
        QFontDatabase::addApplicationFont(fonts.next());

    // "Basic" is Qt's fully customizable control style: Obdaryn draws its own look.
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    // ---- Developer capture mode -------------------------------------------
    // Renders one screen to a PNG and quits; used for design reviews and docs.
    //   OBDARYN_CAPTURE=out.png  OBDARYN_PAGE=0..7  OBDARYN_DEMO=1
    //   OBDARYN_APPEARANCE=light|dark  OBDARYN_LANG=en|ro  OBDARYN_DELAY_MS=4000  OBDARYN_SIZE=WxH
    //   OBDARYN_SCAN_MODULES=1
    const QString capturePath = qEnvironmentVariable("OBDARYN_CAPTURE");
    if (qEnvironmentVariableIsSet("OBDARYN_APPEARANCE"))
        QSettings().setValue(QStringLiteral("ui/appearance"), qEnvironmentVariable("OBDARYN_APPEARANCE"));
    if (qEnvironmentVariableIsSet("OBDARYN_LANG"))
        QSettings().setValue(QStringLiteral("ui/language"), qEnvironmentVariable("OBDARYN_LANG"));

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("icon"), new obdaryn::IconProvider); // engine takes ownership
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("Obdaryn", "Main");

    if (qEnvironmentVariableIntValue("OBDARYN_DEMO") == 1) {
        if (auto *controller = engine.singletonInstance<obdaryn::AppController *>("Obdaryn", "App")) {
            controller->connectDemo();
            // OBDARYN_SCAN_MODULES=1 also runs the full-vehicle module scan once the first scan ends.
            if (qEnvironmentVariableIntValue("OBDARYN_SCAN_MODULES") == 1) {
                QObject::connect(controller, &obdaryn::AppController::scanFinished, controller, [controller] {
                    if (!controller->moduleScanDone() && !controller->vin().isEmpty())
                        QTimer::singleShot(300, controller, [controller] { controller->scanModules(); });
                });
            }
        }
    }
    if (!capturePath.isEmpty() && !engine.rootObjects().isEmpty()) {
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        // OBDARYN_SIZE=1040x680 resizes the window before capturing.
        const QStringList size = qEnvironmentVariable("OBDARYN_SIZE").split(QLatin1Char('x'));
        if (window && size.size() == 2)
            window->resize(size[0].toInt(), size[1].toInt());
        // OBDARYN_OPEN_CODE=hk-abs/C0035 unfolds that code's explanation on the modules page.
        if (window && qEnvironmentVariableIsSet("OBDARYN_OPEN_CODE")) {
            const QString key = qEnvironmentVariable("OBDARYN_OPEN_CODE");
            if (QObject *modules = window->findChild<QObject *>(QStringLiteral("modulesPage")))
                modules->setProperty("openCode", key);
        }
        if (window && qEnvironmentVariableIsSet("OBDARYN_PAGE"))
            window->setProperty("page", qEnvironmentVariableIntValue("OBDARYN_PAGE"));
        const int delay = qEnvironmentVariableIsSet("OBDARYN_DELAY_MS") ? qEnvironmentVariableIntValue("OBDARYN_DELAY_MS") : 4000;
        QTimer::singleShot(delay, &app, [window, capturePath] {
            if (window)
                window->grabWindow().save(capturePath);
            QCoreApplication::quit();
        });
    }
    return QGuiApplication::exec();
}
