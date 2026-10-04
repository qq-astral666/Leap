#pragma once

#include "Config.h"
#include "Engine.h"

#include <QFileSystemWatcher>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QQuickWindow;

// The app: owns the config and the key engine, feeds it from the system-wide
// key tap, drives the overlay and runs actions. Also the settings window's
// backend (the key tree is edited through paths: lists of child indices).
class Controller : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created in main.cpp")

    // ---- overlay
    Q_PROPERTY(bool overlayShown READ overlayShown NOTIFY overlayChanged)
    Q_PROPERTY(QVariantList overlayItems READ overlayItems NOTIFY overlayChanged)
    Q_PROPERTY(QStringList overlayPath READ overlayPath NOTIFY overlayChanged)
    Q_PROPERTY(bool overlaySticky READ overlaySticky NOTIFY overlayChanged)

    // ---- settings
    Q_PROPERTY(QVariantList rows READ rows NOTIFY keymapChanged)
    Q_PROPERTY(int conflictCount READ conflictCount NOTIFY keymapChanged)
    Q_PROPERTY(QString leader READ leader WRITE setLeader NOTIFY settingsChanged)
    Q_PROPERTY(QString leaderLabel READ leaderLabel NOTIFY settingsChanged)
    Q_PROPERTY(QVariantList leaderOptions READ leaderOptions CONSTANT)
    Q_PROPERTY(int overlayDelay READ overlayDelay WRITE setOverlayDelay NOTIFY settingsChanged)
    Q_PROPERTY(int windowGap READ windowGap WRITE setWindowGap NOTIFY settingsChanged)
    Q_PROPERTY(bool launchAtLogin READ launchAtLogin WRITE setLaunchAtLogin NOTIFY settingsChanged)
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY statusChanged)
    Q_PROPERTY(bool accessibilityGranted READ accessibilityGranted NOTIFY statusChanged)
    Q_PROPERTY(bool listening READ listening NOTIFY statusChanged)
    Q_PROPERTY(QVariantList actionTypes READ actionTypes CONSTANT)
    Q_PROPERTY(QVariantList windowCommands READ windowCommands CONSTANT)
    Q_PROPERTY(QVariantList systemCommands READ systemCommands CONSTANT)
    Q_PROPERTY(QString configPath READ configPath CONSTANT)
    Q_PROPERTY(QString version READ version CONSTANT)

public:
    explicit Controller(QObject* parent = nullptr);
    ~Controller() override;

    // True if there was no config yet (first launch).
    bool firstRun() const { return m_firstRun; }
    void attachOverlay(QQuickWindow* window);
    void attachSettings(QQuickWindow* window);
    // Restores Caps Lock and stops the key tap.
    void shutdown();

    bool overlayShown() const { return m_overlayShown; }
    QVariantList overlayItems() const { return m_overlayItems; }
    QStringList overlayPath() const { return m_overlayPath; }
    bool overlaySticky() const { return m_overlaySticky; }

    QVariantList rows() const { return m_rows; }
    int conflictCount() const { return m_conflictCount; }
    QString leader() const;
    void setLeader(const QString& id);
    QString leaderLabel() const;
    QVariantList leaderOptions() const;
    int overlayDelay() const { return m_config.overlayDelayMs; }
    void setOverlayDelay(int ms);
    int windowGap() const { return m_config.windowGap; }
    void setWindowGap(int px);
    bool launchAtLogin() const;
    void setLaunchAtLogin(bool enable);
    bool paused() const { return m_paused; }
    void setPaused(bool paused);
    bool accessibilityGranted() const { return m_accessibility; }
    bool listening() const { return m_tapRunning && !m_paused; }
    QVariantList actionTypes() const;
    QVariantList windowCommands() const;
    QVariantList systemCommands() const;
    QString configPath() const;
    QString version() const;

    // ---- key tree editing
    Q_INVOKABLE QVariantMap node(const QVariantList& path) const;
    // fields: key, title, type, value, sticky (any subset)
    Q_INVOKABLE void updateNode(const QVariantList& path, const QVariantMap& fields);
    // Appends to the group at `parent`; returns the new node's path.
    Q_INVOKABLE QVariantList addNode(const QVariantList& parent, bool group);
    Q_INVOKABLE void removeNode(const QVariantList& path);
    // Moves among its siblings; returns the new path.
    Q_INVOKABLE QVariantList moveNode(const QVariantList& path, int delta);
    // Drag and drop: into the group at `toParent`, before its child at
    // `toIndex` (indices as shown before the move). Returns the new path,
    // or an empty list if the move is invalid.
    Q_INVOKABLE QVariantList moveNodeTo(const QVariantList& from, const QVariantList& toParent, int toIndex);
    Q_INVOKABLE void resetToDefaults();

    // ---- helpers for the settings window
    Q_INVOKABLE QString normalizeKey(const QString& input) const;
    Q_INVOKABLE QString keyLabel(const QString& keyName) const;
    Q_INVOKABLE QString appName(const QString& bundlePath) const;
    Q_INVOKABLE QString localPath(const QUrl& url) const;
    Q_INVOKABLE QString iconUrl(const QString& path) const;
    // [x, y, w, h] in 0..1 of where a window command puts a window.
    Q_INVOKABLE QVariantList windowPreview(const QString& command) const;
    Q_INVOKABLE QString commandTitle(const QString& type, const QString& id) const;
    Q_INVOKABLE void testNode(const QVariantList& path);
    Q_INVOKABLE void openSettings();
    Q_INVOKABLE void requestAccessibility();
    Q_INVOKABLE void revealConfig();

signals:
    void overlayChanged();
    void overlayShake();
    void keymapChanged();
    void settingsChanged();
    void statusChanged();
    void toast(const QString& message);
    // An action failed while the settings window may be hidden.
    void actionFailed(const QString& message);

private:
    bool onKey(const leap::KeyEvent& e);
    void applyEffect(const leap::Engine::Effect& effect);
    void run(const leap::Action& action);
    void pasteText(const QString& text);

    void showOverlay();
    void hideOverlay();
    void rebuildOverlay();
    QVariantMap describe(const leap::Node& node) const;

    void refreshStatus();
    void tryStartTap();
    void applyConfig(bool save);
    void rebuildRows();
    void reloadFromDisk();
    void watchConfig();

    Config m_config;
    QByteArray m_lastWritten;
    bool m_firstRun = false;
    leap::Engine m_engine;
    bool m_capsRemapped = false;

    QPointer<QQuickWindow> m_overlay;
    QPointer<QQuickWindow> m_settings;
    bool m_overlayShown = false;
    QVariantList m_overlayItems;
    QStringList m_overlayPath;
    bool m_overlaySticky = false;
    QTimer m_showTimer;     // overlay delay
    QTimer m_idleTimer;     // cancels a forgotten sequence
    QTimer m_orderOutTimer; // lets the fade-out finish
    QTimer m_statusTimer;   // permission / tap health

    QVariantList m_rows;
    int m_conflictCount = 0;
    bool m_paused = false;
    bool m_accessibility = false;
    bool m_tapRunning = false;
    QFileSystemWatcher m_watcher;
};
