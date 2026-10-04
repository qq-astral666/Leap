// Non-macOS fallbacks: Leap builds and its settings window runs, but there
// is no global key tap. Kept so the app layer compiles and can be tested
// on Linux CI.

#include "Native.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QUrl>
#include <QWindow>

namespace native {

namespace {
void setError(QString* error, const QString& message)
{
    if (error)
        *error = message;
}
} // namespace

bool accessibilityTrusted() { return false; }
void requestAccessibility() {}
void openAccessibilitySettings() {}

bool startKeyTap(KeyHandler) { return false; }
void stopKeyTap() {}
bool keyTapRunning() { return false; }
bool secureInputActive() { return false; }

void configureOverlay(QWindow*) {}
void orderOverlay(QWindow* window, bool shown)
{
    if (window)
        window->setVisible(shown);
}
void activateApp() {}

bool openApp(const QString& bundlePath, QString* error)
{
    return openWithDefaultApp(bundlePath, false, error);
}

bool openWithDefaultApp(const QString& pathOrUrl, bool isUrl, QString* error)
{
    const QUrl url = isUrl ? QUrl(pathOrUrl) : QUrl::fromLocalFile(pathOrUrl);
    if (!QDesktopServices::openUrl(url)) {
        setError(error, QStringLiteral("Не удалось открыть: %1").arg(pathOrUrl));
        return false;
    }
    return true;
}

void postPaste() {}

bool runWindowCommand(const QString&, double, QString* error)
{
    setError(error, QStringLiteral("Только для macOS"));
    return false;
}

bool runSystemCommand(const QString&, QString* error)
{
    setError(error, QStringLiteral("Только для macOS"));
    return false;
}

QImage fileIcon(const QString&, int) { return {}; }
QString frontmostAppPath() { return {}; }
QString bundleDisplayName(const QString& bundlePath) { return QFileInfo(bundlePath).completeBaseName(); }

bool launchAtLoginEnabled() { return false; }
bool setLaunchAtLogin(bool, QString* error)
{
    setError(error, QStringLiteral("Только для macOS"));
    return false;
}

} // namespace native
