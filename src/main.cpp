#include "Controller.h"
#include "IconProvider.h"
#include "Tray.h"

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QLockFile>
#include <QQmlApplicationEngine>
#include <QQmlError>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardPaths>

namespace {

// Everything Qt and QML print (binding errors included) also goes to
// ~/Library/Logs/Leap/leap.log: a menu-bar app has no visible console.
QFile* g_log = nullptr;
QtMessageHandler g_previousHandler = nullptr;

void logHandler(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    static QMutex mutex;
    {
        QMutexLocker lock(&mutex);
        if (g_log && g_log->isOpen()) {
            static const char* kinds[] = { "debug", "warning", "critical", "fatal", "info" };
            const QString line = QStringLiteral("%1 %2 %3\n")
                                     .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz")),
                                          QLatin1String(kinds[type]), message);
            g_log->write(line.toUtf8());
            g_log->flush();
        }
    }
    if (g_previousHandler)
        g_previousHandler(type, context, message);
}

void startLog()
{
    const QString dir = QDir::homePath() + QStringLiteral("/Library/Logs/Leap");
    QDir().mkpath(dir);
    g_log = new QFile(dir + QStringLiteral("/leap.log"));
    // Fresh file per launch, so it only holds this session.
    if (g_log->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        g_previousHandler = qInstallMessageHandler(logHandler);
}

QQuickWindow* loadWindow(QQmlApplicationEngine& engine, const char* type, Controller* controller)
{
    const auto before = engine.rootObjects().size();
    engine.setInitialProperties({ { QStringLiteral("controller"), QVariant::fromValue(controller) } });
    engine.loadFromModule("Leap", type);
    if (engine.rootObjects().size() == before)
        return nullptr;
    return qobject_cast<QQuickWindow*>(engine.rootObjects().constLast());
}

} // namespace

int main(int argc, char* argv[])
{
    // QApplication (not QGuiApplication) for the native tray menu.
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Leap"));
    QApplication::setOrganizationName(QStringLiteral("Leap"));
    QApplication::setOrganizationDomain(QStringLiteral("leap.app"));
    QApplication::setApplicationVersion(QStringLiteral(PROJECT_VERSION_STRING));
    QApplication::setQuitOnLastWindowClosed(false);
    startLog();
    qInfo() << "Leap" << QApplication::applicationVersion() << "Qt" << qVersion();
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    QLockFile instanceLock(dataDir + QStringLiteral("/instance.lock"));
    if (!instanceLock.tryLock(100)) {
        qWarning() << "Leap is already running";
        return 0;
    }

    Controller controller;
    Tray tray(&controller);
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &controller, &Controller::shutdown);

    QQmlApplicationEngine engine;
    engine.setOutputWarningsToStandardError(false);
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, [](const QList<QQmlError>& warnings) {
        for (const auto& w : warnings)
            qWarning().noquote() << "QML:" << w.toString();
    });
    engine.addImageProvider(QStringLiteral("fileicon"), new IconProvider);

    QQuickWindow* overlay = loadWindow(engine, "HintPanel", &controller);
    QQuickWindow* settings = loadWindow(engine, "Settings", &controller);
    if (!overlay || !settings) {
        qCritical() << "Leap: failed to load the QML windows";
        return 1;
    }
    controller.attachOverlay(overlay);
    controller.attachSettings(settings);

    // First launch, or no permission yet: show the window that explains it.
    if (controller.firstRun() || !controller.accessibilityGranted())
        controller.openSettings();

    return app.exec();
}
