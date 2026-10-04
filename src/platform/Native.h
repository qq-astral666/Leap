#pragma once

#include "Engine.h"

#include <QImage>
#include <QString>

#include <functional>

class QWindow;

// macOS glue. Native_mac.mm / Platform_stub.cpp.
namespace native {

// ---- permissions
bool accessibilityTrusted();
// Shows the system prompt that sends the user to the Accessibility settings.
void requestAccessibility();
void openAccessibilitySettings();

// ---- keyboard
// Called for every key event in the system, on the main thread, inside the
// event tap: return true to swallow the event. Must be fast.
using KeyHandler = std::function<bool(const leap::KeyEvent&)>;
// Installs a CGEventTap. Fails without the Accessibility permission.
bool startKeyTap(KeyHandler handler);
void stopKeyTap();
bool keyTapRunning();
// "Secure input" (a password field, Terminal's Secure Keyboard Entry) hides
// all keys from event taps.
bool secureInputActive();

// ---- the overlay
// Borderless, non-activating, click-through panel on every Space, above
// full-screen apps: showing it never takes focus from the app you're in.
void configureOverlay(QWindow* window);
void orderOverlay(QWindow* window, bool shown);

// Brings this (menu-bar) app forward, for the settings window.
void activateApp();

// ---- actions
bool openApp(const QString& bundlePath, QString* error);
// Files, folders and URLs with their default app.
bool openWithDefaultApp(const QString& pathOrUrl, bool isUrl, QString* error);
// ⌘V into the front app (a synthetic event the key tap ignores).
void postPaste();
// leap::windowCommands(): moves the focused window of the front app.
bool runWindowCommand(const QString& command, double gap, QString* error);
// leap::systemCommands()
bool runSystemCommand(const QString& command, QString* error);

// ---- misc
// Finder's icon for a file or bundle, `size` px square.
QImage fileIcon(const QString& path, int size);
// Bundle path of the frontmost app ("" if unknown).
QString frontmostAppPath();
// CFBundleDisplayName / name of a bundle.
QString bundleDisplayName(const QString& bundlePath);

bool launchAtLoginEnabled();
bool setLaunchAtLogin(bool enable, QString* error);

} // namespace native
