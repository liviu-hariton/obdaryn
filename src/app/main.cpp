// SPDX-License-Identifier: GPL-3.0-or-later
//
// Obdaryn desktop entry point.

#include "AppController.h"
#include "IconProvider.h"

#include <QDirIterator>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <QFontDatabase>
#include "report/ReportWriter.h"
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSettings>
#include <QMouseEvent>
#include <QTimer>

#include <functional>
#include <memory>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    // These names decide where QSettings stores preferences
    // (~/Library/Preferences/ro.obdaryn.Obdaryn.plist on macOS).
    QGuiApplication::setOrganizationName(QStringLiteral("Obdaryn"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("obdaryn.ro"));
    QGuiApplication::setApplicationName(QStringLiteral("Obdaryn"));
    QGuiApplication::setApplicationVersion(QStringLiteral(OBDARYN_VERSION));

    // Developer runs (screenshots, reports, rendering) get their own settings,
    // so they never change the user's real preferences (language, appearance,
    // charts, trips...). Each run starts fresh in a temporary folder, unless
    // OBDARYN_SETTINGS_DIR names one to keep state between runs.
    const bool developerRun = qEnvironmentVariableIsSet("OBDARYN_CAPTURE") || qEnvironmentVariableIsSet("OBDARYN_REPORT")
                              || qEnvironmentVariableIsSet("OBDARYN_RENDER_HTML");
    std::unique_ptr<QTemporaryDir> captureSettings;
    if (developerRun) {
        QString dir = qEnvironmentVariable("OBDARYN_SETTINGS_DIR");
        if (dir.isEmpty()) {
            captureSettings = std::make_unique<QTemporaryDir>();
            dir = captureSettings->path();
        }
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir);
    }

    // Bundled typefaces (SIL Open Font License), so the app looks the same on
    // every platform. They live in qrc:/fonts.
    QDirIterator fonts(QStringLiteral(":/fonts"), {QStringLiteral("*.ttf"), QStringLiteral("*.otf")},
                       QDir::Files, QDirIterator::Subdirectories);
    while (fonts.hasNext())
        QFontDatabase::addApplicationFont(fonts.next());

    // Developer shortcut: OBDARYN_RENDER_HTML=in.html OBDARYN_RENDER_PDF=out.pdf renders a
    // saved report to PDF and quits (for working on the report layout).
    if (qEnvironmentVariableIsSet("OBDARYN_RENDER_HTML")) {
        QFile in(qEnvironmentVariable("OBDARYN_RENDER_HTML"));
        QString error;
        if (!in.open(QIODevice::ReadOnly)
            || !obdaryn::ReportWriter::writePdf(QString::fromUtf8(in.readAll()), qEnvironmentVariable("OBDARYN_RENDER_PDF"),
                                                QStringLiteral("render test"), &error)) {
            qWarning("render failed: %s", qPrintable(error));
            return 1;
        }
        return 0;
    }

    // "Basic" is Qt's fully customizable control style: Obdaryn draws its own look.
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    // ---- Developer capture mode -------------------------------------------
    // Renders one screen to a PNG and quits; used for design reviews and docs.
    //   OBDARYN_CAPTURE=out.png  OBDARYN_PAGE=0..10  OBDARYN_DEMO=1
    //   OBDARYN_APPEARANCE=light|dark  OBDARYN_LANG=en|ro  OBDARYN_DELAY_MS=4000  OBDARYN_SIZE=WxH
    //   OBDARYN_SCAN_MODULES=1  OBDARYN_RECORD=1 (record once the first scan ends)
    //   OBDARYN_REPLAY=file.obdrec (replay instead of the demo car)
    //   OBDARYN_DEMO_TRIPS=1 (save the demo car's trips, to capture the trip history)
    //   OBDARYN_RECORDINGS_DIR / OBDARYN_REPORTS_DIR=folder (read by AppController; keeps captures out of ~/Documents)
    //   OBDARYN_REPORT=out.pdf|out.html (save the report after the scan, or after the module scan with OBDARYN_SCAN_MODULES=1)
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
    // OBDARYN_CONNECT=host:port connects to an adapter (to capture connection failures).
    if (qEnvironmentVariableIsSet("OBDARYN_CONNECT")) {
        const QStringList hostPort = qEnvironmentVariable("OBDARYN_CONNECT").split(QLatin1Char(':'));
        if (auto *controller = engine.singletonInstance<obdaryn::AppController *>("Obdaryn", "App"); controller && hostPort.size() == 2)
            controller->connectToAdapter(hostPort[0], hostPort[1].toInt());
    }
    if (qEnvironmentVariableIsSet("OBDARYN_REPLAY")) {
        if (auto *controller = engine.singletonInstance<obdaryn::AppController *>("Obdaryn", "App"))
            controller->replayRecording(qEnvironmentVariable("OBDARYN_REPLAY"));
    }
    if (qEnvironmentVariableIntValue("OBDARYN_RECORD") == 1) {
        if (auto *controller = engine.singletonInstance<obdaryn::AppController *>("Obdaryn", "App")) {
            QObject::connect(controller, &obdaryn::AppController::scanFinished, controller, [controller] {
                if (!controller->vin().isEmpty() && !controller->recording())
                    controller->startRecording();
            });
        }
    }
    // OBDARYN_DROP_AT_MS=9000 drops the demo adapter's link then (shows reconnecting);
    // OBDARYN_IGNITION_OFF_AT_MS=9000 turns the demo ignition off then (shows waiting for the car).
    if (auto *controller = engine.singletonInstance<obdaryn::AppController *>("Obdaryn", "App")) {
        if (qEnvironmentVariableIsSet("OBDARYN_DROP_AT_MS"))
            QTimer::singleShot(qEnvironmentVariableIntValue("OBDARYN_DROP_AT_MS"), controller, [controller] { controller->simulateLinkDrop(); });
        if (qEnvironmentVariableIsSet("OBDARYN_IGNITION_OFF_AT_MS"))
            QTimer::singleShot(qEnvironmentVariableIntValue("OBDARYN_IGNITION_OFF_AT_MS"), controller, [controller] { controller->setDemoIgnitionOn(false); });
    }
    // OBDARYN_READ_TESTS=1 reads the monitor test results (service 06) once the first scan ends.
    if (qEnvironmentVariableIntValue("OBDARYN_READ_TESTS") == 1) {
        if (auto *controller = engine.singletonInstance<obdaryn::AppController *>("Obdaryn", "App"))
            QObject::connect(controller, &obdaryn::AppController::scanFinished, controller, [controller] {
                if (!controller->vin().isEmpty() && !controller->monitorTestsRead())
                    QTimer::singleShot(300, controller, [controller] { controller->readMonitorTests(); });
            });
    }
    // OBDARYN_BRAND_MODEL=Elantra chooses the model for manufacturer values once the car
    // is known; OBDARYN_FIND_BRAND=1 also asks the modules which values they answer.
    if (qEnvironmentVariableIsSet("OBDARYN_BRAND_MODEL")) {
        if (auto *controller = engine.singletonInstance<obdaryn::AppController *>("Obdaryn", "App")) {
            QObject::connect(controller, &obdaryn::AppController::scanFinished, controller, [controller] {
                controller->setManufacturerModel(qEnvironmentVariable("OBDARYN_BRAND_MODEL"));
                if (qEnvironmentVariableIntValue("OBDARYN_FIND_BRAND") == 1)
                    QTimer::singleShot(300, controller, [controller] { controller->findManufacturerValues(); });
            });
        }
    }
    // OBDARYN_CHART_A / OBDARYN_CHART_B=file.obdrec load recordings into the Charts screen.
    if (auto *controller = engine.singletonInstance<obdaryn::AppController *>("Obdaryn", "App")) {
        if (qEnvironmentVariableIsSet("OBDARYN_CHART_A"))
            controller->charts()->loadRecording(0, qEnvironmentVariable("OBDARYN_CHART_A"));
        if (qEnvironmentVariableIsSet("OBDARYN_CHART_B"))
            controller->charts()->loadRecording(1, qEnvironmentVariable("OBDARYN_CHART_B"));
        // OBDARYN_CHART_PRESET=<name> applies a chart preset (after the recordings load).
        if (qEnvironmentVariableIsSet("OBDARYN_CHART_PRESET"))
            controller->charts()->applyPreset(qEnvironmentVariable("OBDARYN_CHART_PRESET"));
    }
    if (qEnvironmentVariableIsSet("OBDARYN_REPORT")) {
        if (auto *controller = engine.singletonInstance<obdaryn::AppController *>("Obdaryn", "App")) {
            const bool afterModules = qEnvironmentVariableIntValue("OBDARYN_SCAN_MODULES") == 1;
            auto save = [controller] {
                static bool saved = false;
                if (saved || controller->vin().isEmpty() || controller->scanning() || controller->moduleScanRunning())
                    return;
                saved = true;
                QTimer::singleShot(300, controller, [controller] { controller->saveReport(qEnvironmentVariable("OBDARYN_REPORT"), false); });
            };
            if (afterModules)
                QObject::connect(controller, &obdaryn::AppController::moduleScanChanged, controller, [controller, save] {
                    if (controller->moduleScanDone())
                        save();
                });
            else
                QObject::connect(controller, &obdaryn::AppController::scanFinished, controller, save);
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
        // OBDARYN_LIVE_SOURCE=brand shows the manufacturer values on the Live data screen.
        if (window && qEnvironmentVariableIsSet("OBDARYN_LIVE_SOURCE")) {
            if (QObject *live = window->findChild<QObject *>(QStringLiteral("livePage")))
                live->setProperty("source", qEnvironmentVariable("OBDARYN_LIVE_SOURCE"));
        }
        // OBDARYN_CLICK=x,y clicks there (window coordinates) shortly before the capture,
        // like a user would: checks that controls really take the mouse.
        if (window && qEnvironmentVariableIsSet("OBDARYN_CLICK")) {
            const QStringList xy = qEnvironmentVariable("OBDARYN_CLICK").split(QLatin1Char(','));
            const int clickDelay = qMax(0, (qEnvironmentVariableIsSet("OBDARYN_DELAY_MS") ? qEnvironmentVariableIntValue("OBDARYN_DELAY_MS") : 4000) - 1500);
            if (xy.size() == 2) {
                const QPointF at(xy[0].toDouble(), xy[1].toDouble());
                QTimer::singleShot(clickDelay, window, [window, at] {
                    QMouseEvent press(QEvent::MouseButtonPress, at, window->mapToGlobal(at), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    QMouseEvent release(QEvent::MouseButtonRelease, at, window->mapToGlobal(at), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                    QCoreApplication::sendEvent(window, &press);
                    QCoreApplication::sendEvent(window, &release);
                });
            }
        }
        // OBDARYN_HELP_SECTION=3 scrolls the Help page to that section (0 = Start here).
        if (window && qEnvironmentVariableIsSet("OBDARYN_HELP_SECTION")) {
            const int section = qEnvironmentVariableIntValue("OBDARYN_HELP_SECTION");
            QTimer::singleShot(1500, window, [window, section] {
                if (QObject *help = window->findChild<QObject *>(QStringLiteral("helpPage")))
                    QMetaObject::invokeMethod(help, "showSection", Q_ARG(QVariant, section));
            });
        }
        if (window && qEnvironmentVariableIsSet("OBDARYN_PAGE"))
            window->setProperty("page", qEnvironmentVariableIntValue("OBDARYN_PAGE"));
        const int delay = qEnvironmentVariableIsSet("OBDARYN_DELAY_MS") ? qEnvironmentVariableIntValue("OBDARYN_DELAY_MS") : 4000;
        // OBDARYN_SELECT_CODE=C1203 selects a code on the Trouble codes screen and
        // OBDARYN_FOCUS=<objectName> gives that item keyboard focus, just before the capture.
        QTimer::singleShot(qMax(0, delay - 800), &app, [window] {
            if (!window)
                return;
            if (qEnvironmentVariableIsSet("OBDARYN_SELECT_CODE")) {
                if (QObject *codes = window->findChild<QObject *>(QStringLiteral("codesPage")))
                    codes->setProperty("selectedCode", qEnvironmentVariable("OBDARYN_SELECT_CODE"));
            }
            if (qEnvironmentVariableIsSet("OBDARYN_FOCUS")) {
                window->requestActivate(); // keyboard focus only exists in the active window
                QTimer::singleShot(300, window, [window] {
                    // Walk the visual tree: delegates aren't always QObject children of the page.
                    QList<QQuickItem *> items;
                    std::function<void(QQuickItem *)> walk = [&](QQuickItem *it) {
                        if (it->objectName() == qEnvironmentVariable("OBDARYN_FOCUS"))
                            items << it;
                        for (QQuickItem *child : it->childItems())
                            walk(child);
                    };
                    walk(window->contentItem());
                    for (QQuickItem *item : items) {
                        if (item->isVisible()) {
                            item->forceActiveFocus(Qt::TabFocusReason);
                            qInfo("capture: focused %s (active focus: %d)", qPrintable(item->objectName()), item->hasActiveFocus());
                            break;
                        }
                    }
                });
            }
        });
        // OBDARYN_ABOUT=1 opens the About window and captures it instead.
        const bool aboutCapture = qEnvironmentVariableIntValue("OBDARYN_ABOUT") == 1;
        if (window && aboutCapture)
            QTimer::singleShot(qMax(0, delay - 1200), window, [window] { QMetaObject::invokeMethod(window, "showAbout"); });
        QTimer::singleShot(delay, &app, [window, capturePath, aboutCapture] {
            QQuickWindow *target = window;
            if (window && aboutCapture) {
                for (QWindow *w : QGuiApplication::allWindows()) {
                    if (w->objectName() == QLatin1String("aboutWindow"))
                        target = qobject_cast<QQuickWindow *>(w);
                }
            }
            if (target)
                target->grabWindow().save(capturePath);
            QCoreApplication::quit();
        });
    }
    return QGuiApplication::exec();
}
