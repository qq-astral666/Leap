#include "Controller.h"

#include "CapsLockRemap.h"
#include "IconProvider.h"
#include "Layout.h"
#include "Native.h"

#include <QClipboard>
#include <QCursor>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMimeData>
#include <QProcess>
#include <QQuickWindow>
#include <QScreen>
#include <QUrl>

#include <algorithm>
#include <functional>

using leap::ActionType;
using leap::Node;

namespace {

constexpr int kIdleTimeoutMs = 12000;
constexpr int kFadeOutMs = 140;

leap::Path toPath(const QVariantList& list)
{
    leap::Path path;
    path.reserve(static_cast<size_t>(list.size()));
    for (const auto& v : list)
        path.push_back(v.toInt());
    return path;
}

QVariantList fromPath(const leap::Path& path)
{
    QVariantList list;
    for (const int i : path)
        list.append(i);
    return list;
}

QString qs(const std::string& s) { return QString::fromStdString(s); }

QString expandHome(QString path)
{
    path = path.trimmed();
    if (path == QLatin1String("~"))
        return QDir::homePath();
    if (path.startsWith(QLatin1String("~/")))
        return QDir::homePath() + path.mid(1);
    return path;
}

QString normalizedUrl(QString url)
{
    url = url.trimmed();
    if (!url.contains(QLatin1String("://")) && !url.startsWith(QLatin1String("mailto:"))
        && !url.startsWith(QLatin1String("tg:")))
        url.prepend(QStringLiteral("https://"));
    return url;
}

QString firstLine(const QString& text, int max = 48)
{
    QString line = text.section(QLatin1Char('\n'), 0, 0).trimmed();
    if (line.size() > max)
        line = line.left(max - 1) + QStringLiteral("…");
    return line;
}

QString systemIcon(const std::string& id)
{
    if (id == "volume-up") return QStringLiteral("volumeUp");
    if (id == "volume-down") return QStringLiteral("volumeDown");
    if (id == "mute") return QStringLiteral("volumeX");
    if (id == "dark-mode") return QStringLiteral("moon");
    if (id == "sleep-display") return QStringLiteral("monitor");
    if (id == "lock") return QStringLiteral("lock");
    return QStringLiteral("zap");
}

} // namespace

Controller::Controller(QObject* parent)
    : QObject(parent)
{
    QString error;
    if (!Config::load(&m_config, &error)) {
        m_firstRun = !QFileInfo::exists(Config::path());
        if (!m_firstRun)
            qWarning() << "Leap: broken config, using defaults:" << error;
        m_config = Config::defaults();
        if (m_firstRun)
            m_config.save(nullptr);
    }

    m_showTimer.setSingleShot(true);
    connect(&m_showTimer, &QTimer::timeout, this, [this] {
        if (m_engine.active())
            showOverlay();
    });
    m_idleTimer.setSingleShot(true);
    m_idleTimer.setInterval(kIdleTimeoutMs);
    connect(&m_idleTimer, &QTimer::timeout, this, [this] {
        m_engine.cancel();
        hideOverlay();
    });
    m_orderOutTimer.setSingleShot(true);
    m_orderOutTimer.setInterval(kFadeOutMs);
    connect(&m_orderOutTimer, &QTimer::timeout, this, [this] {
        if (!m_overlayShown && m_overlay)
            m_overlay->hide();
    });
    m_statusTimer.setInterval(1500);
    connect(&m_statusTimer, &QTimer::timeout, this, &Controller::refreshStatus);
    m_statusTimer.start();

    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &Controller::reloadFromDisk);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &Controller::reloadFromDisk);

    m_lastWritten = m_config.toJson();
    applyConfig(false);
    watchConfig();
    refreshStatus();
}

Controller::~Controller()
{
    shutdown();
}

void Controller::shutdown()
{
    native::stopKeyTap();
    m_tapRunning = false;
    if (m_capsRemapped) {
        capslock::setRemapped(false, nullptr);
        m_capsRemapped = false;
    }
}

void Controller::attachOverlay(QQuickWindow* window)
{
    m_overlay = window;
    if (!window)
        return;
    window->setFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus
                     | Qt::WindowTransparentForInput);
    window->setColor(Qt::transparent);
    // Create the native panel now (winId) but keep it hidden: a hidden Qt
    // window can't sit over other windows and catch their clicks.
    native::configureOverlay(window);
    window->hide();
}

void Controller::attachSettings(QQuickWindow* window)
{
    m_settings = window;
}

// ------------------------------------------------------------------ keys

bool Controller::onKey(const leap::KeyEvent& e)
{
    // Runs inside the event tap: decide now, do the visible work later.
    const auto decision = m_engine.handle(e);
    if (decision.effect.kind != leap::Engine::Effect::Kind::None) {
        const auto effect = decision.effect;
        QMetaObject::invokeMethod(this, [this, effect] { applyEffect(effect); }, Qt::QueuedConnection);
    }
    return decision.swallow;
}

void Controller::applyEffect(const leap::Engine::Effect& effect)
{
    using Kind = leap::Engine::Effect::Kind;
    switch (effect.kind) {
    case Kind::Activated:
        rebuildOverlay();
        m_idleTimer.start();
        if (m_config.overlayDelayMs <= 0)
            showOverlay();
        else
            m_showTimer.start(m_config.overlayDelayMs);
        break;
    case Kind::Changed:
        rebuildOverlay();
        m_idleTimer.start();
        break;
    case Kind::Unknown:
        m_idleTimer.start();
        showOverlay(); // a wrong key: show what's available right away
        emit overlayShake();
        break;
    case Kind::Cancelled:
        hideOverlay();
        break;
    case Kind::Executed:
        if (m_engine.active()) {
            rebuildOverlay(); // sticky group stays open
            m_idleTimer.start();
        } else {
            hideOverlay();
        }
        run(effect.action);
        break;
    case Kind::None:
        break;
    }
}

void Controller::run(const leap::Action& action)
{
    const QString value = qs(action.value);
    QString error;
    bool ok = true;
    switch (action.type) {
    case ActionType::App:
        ok = native::openApp(expandHome(value), &error);
        break;
    case ActionType::Open:
        ok = native::openWithDefaultApp(expandHome(value), false, &error);
        break;
    case ActionType::Url:
        ok = native::openWithDefaultApp(normalizedUrl(value), true, &error);
        break;
    case ActionType::Shell:
        ok = QProcess::startDetached(QStringLiteral("/bin/zsh"), { QStringLiteral("-lc"), value }, QDir::homePath());
        if (!ok)
            error = QStringLiteral("Не удалось запустить команду");
        break;
    case ActionType::Text:
        pasteText(value);
        break;
    case ActionType::Window:
        ok = native::runWindowCommand(value, m_config.windowGap, &error);
        break;
    case ActionType::System:
        ok = native::runSystemCommand(value, &error);
        break;
    case ActionType::None:
        break;
    }
    if (!ok) {
        qWarning() << "Leap:" << error;
        emit actionFailed(error);
    }
}

void Controller::pasteText(const QString& text)
{
    // Put the text on the clipboard, ⌘V, then give the old clipboard back.
    QClipboard* clipboard = QGuiApplication::clipboard();
    auto* saved = new QMimeData;
    if (const QMimeData* current = clipboard->mimeData())
        for (const QString& format : current->formats())
            saved->setData(format, current->data(format));
    clipboard->setText(text);
    QTimer::singleShot(40, this, [] { native::postPaste(); });
    QTimer::singleShot(800, this, [saved] {
        QGuiApplication::clipboard()->setMimeData(saved); // takes ownership
    });
}

// ------------------------------------------------------------------ overlay

void Controller::showOverlay()
{
    m_showTimer.stop();
    m_orderOutTimer.stop();
    if (!m_overlay)
        return;
    if (!m_overlayShown) {
        // Centered on the display with the mouse pointer.
        QScreen* screen = QGuiApplication::screenAt(QCursor::pos());
        if (!screen)
            screen = QGuiApplication::primaryScreen();
        const QRect area = screen->availableGeometry();
        const QSize size = m_overlay->size();
        m_overlay->setPosition(area.x() + (area.width() - size.width()) / 2,
                               area.y() + (area.height() - size.height()) / 2 - area.height() / 12);
        m_overlay->show();
        native::configureOverlay(m_overlay); // Qt resets some panel flags on show
        native::orderOverlay(m_overlay, true);
    }
    m_overlayShown = true;
    emit overlayChanged();
}

void Controller::hideOverlay()
{
    m_showTimer.stop();
    m_idleTimer.stop();
    if (!m_overlayShown)
        return;
    m_overlayShown = false;
    emit overlayChanged();
    m_orderOutTimer.start();
}

QVariantMap Controller::describe(const Node& node) const
{
    QVariantMap m;
    m[QStringLiteral("key")] = qs(node.key);
    m[QStringLiteral("keyLabel")] = qs(leap::keyLabel(node.key));
    m[QStringLiteral("title")] = qs(node.title);
    m[QStringLiteral("group")] = node.group;
    m[QStringLiteral("sticky")] = node.sticky;
    m[QStringLiteral("type")] = node.group ? QStringLiteral("group") : QString::fromUtf8(leap::actionTypeId(node.action.type).data());
    m[QStringLiteral("value")] = qs(node.action.value);

    QString icon;     // image://fileicon/... for apps, files and folders
    QString glyph;    // Icon.qml name otherwise
    QString subtitle;
    QVariantList preview; // window commands: target rect on a unit screen
    const QString value = qs(node.action.value);
    if (node.group) {
        glyph = QStringLiteral("layers");
        subtitle = QString::number(node.children.size());
    } else {
        switch (node.action.type) {
        case ActionType::App:
            icon = IconProvider::urlFor(expandHome(value));
            break;
        case ActionType::Open:
            icon = IconProvider::urlFor(expandHome(value));
            subtitle = value;
            break;
        case ActionType::Url:
            glyph = QStringLiteral("globe");
            subtitle = QUrl(normalizedUrl(value)).host();
            break;
        case ActionType::Shell:
            glyph = QStringLiteral("terminal");
            subtitle = firstLine(value);
            break;
        case ActionType::Text:
            glyph = QStringLiteral("type");
            subtitle = firstLine(value);
            break;
        case ActionType::Window: {
            glyph = QStringLiteral("window");
            preview = windowPreview(value);
            break;
        }
        case ActionType::System:
            glyph = systemIcon(node.action.value);
            break;
        case ActionType::None:
            glyph = QStringLiteral("zap");
            break;
        }
    }
    m[QStringLiteral("icon")] = icon;
    m[QStringLiteral("glyph")] = glyph;
    m[QStringLiteral("subtitle")] = subtitle;
    m[QStringLiteral("preview")] = preview;
    return m;
}

void Controller::rebuildOverlay()
{
    const auto& seq = m_engine.sequencer();
    const Node& group = seq.current();
    m_overlayItems.clear();
    for (const auto& child : group.children)
        if (!child.key.empty())
            m_overlayItems.append(describe(child));
    m_overlayPath.clear();
    for (const auto& title : seq.breadcrumb())
        m_overlayPath.append(qs(title));
    m_overlaySticky = group.sticky;
    emit overlayChanged();
}

// ------------------------------------------------------------------ status

void Controller::refreshStatus()
{
    const bool trusted = native::accessibilityTrusted();
    const bool running = native::keyTapRunning();
    const bool changed = trusted != m_accessibility || running != m_tapRunning;
    m_accessibility = trusted;
    m_tapRunning = running;
    if (trusted && !running)
        tryStartTap();
    else if (changed)
        emit statusChanged();
}

void Controller::tryStartTap()
{
    if (native::startKeyTap([this](const leap::KeyEvent& e) { return onKey(e); }))
        m_tapRunning = true;
    emit statusChanged();
}

void Controller::requestAccessibility()
{
    native::requestAccessibility();
    native::openAccessibilitySettings();
}

// ------------------------------------------------------------------ settings

void Controller::applyConfig(bool save)
{
    m_engine.setRoot(m_config.root);
    m_engine.setLeader(m_config.leader);
    hideOverlay();

    const bool wantCaps = m_config.leader.kind == leap::Leader::Kind::CapsLock;
    if (wantCaps != m_capsRemapped) {
        QString error;
        if (capslock::setRemapped(wantCaps, &error))
            m_capsRemapped = wantCaps;
        else
            emit actionFailed(QStringLiteral("Caps Lock: %1").arg(error));
    }

    if (save) {
        QString error;
        m_lastWritten = m_config.toJson();
        if (!m_config.save(&error))
            emit toast(QStringLiteral("Не удалось сохранить: %1").arg(error));
        watchConfig();
    }
    rebuildRows();
}

void Controller::rebuildRows()
{
    m_rows.clear();
    const auto conflicts = leap::findConflicts(m_config.root);
    m_conflictCount = static_cast<int>(conflicts.size());

    std::function<void(const Node&, leap::Path&)> walk = [&](const Node& group, leap::Path& path) {
        for (int i = 0; i < static_cast<int>(group.children.size()); ++i) {
            const Node& child = group.children[static_cast<size_t>(i)];
            path.push_back(i);
            QVariantMap row = describe(child);
            row[QStringLiteral("path")] = fromPath(path);
            row[QStringLiteral("depth")] = static_cast<int>(path.size()) - 1;
            bool conflict = child.key.empty();
            for (const auto& c : conflicts)
                if (c.key == child.key && c.group == leap::Path(path.begin(), path.end() - 1))
                    conflict = true;
            row[QStringLiteral("conflict")] = conflict;
            m_rows.append(row);
            if (child.group)
                walk(child, path);
            path.pop_back();
        }
    };
    leap::Path path;
    walk(m_config.root, path);
    emit keymapChanged();
}

QString Controller::leader() const
{
    return qs(m_config.leader.id());
}

void Controller::setLeader(const QString& id)
{
    const auto parsed = leap::Leader::fromId(id.toStdString());
    if (!parsed || *parsed == m_config.leader)
        return;
    m_config.leader = *parsed;
    applyConfig(true);
    emit settingsChanged();
}

QVariantList Controller::leaderOptions() const
{
    auto option = [](const char* id, const QString& title, const QString& hint) {
        return QVariantMap { { QStringLiteral("id"), QString::fromLatin1(id) },
                             { QStringLiteral("title"), title },
                             { QStringLiteral("hint"), hint } };
    };
    return {
        option("capslock", QStringLiteral("Caps Lock"),
               QStringLiteral("Caps Lock перестаёт включать заглавные и переключать раскладку, пока Leap запущен.")),
        option("tap:right_command", QStringLiteral("Правый ⌘"),
               QStringLiteral("Нажмите и отпустите. В сочетаниях вроде ⌘C работает как обычно.")),
        option("tap:right_option", QStringLiteral("Правый ⌥"),
               QStringLiteral("Нажмите и отпустите. В сочетаниях работает как обычно.")),
        option("chord:option+space", QStringLiteral("⌥ Space"), QString()),
        option("chord:control+option+space", QStringLiteral("⌃⌥ Space"), QString()),
    };
}

QString Controller::leaderLabel() const
{
    const QString id = leader();
    for (const auto& v : leaderOptions()) {
        const auto m = v.toMap();
        if (m.value(QStringLiteral("id")).toString() == id)
            return m.value(QStringLiteral("title")).toString();
    }
    return id;
}

void Controller::setOverlayDelay(int ms)
{
    ms = std::clamp(ms, 0, 2000);
    if (ms == m_config.overlayDelayMs)
        return;
    m_config.overlayDelayMs = ms;
    applyConfig(true);
    emit settingsChanged();
}

void Controller::setWindowGap(int px)
{
    px = std::clamp(px, 0, 64);
    if (px == m_config.windowGap)
        return;
    m_config.windowGap = px;
    applyConfig(true);
    emit settingsChanged();
}

bool Controller::launchAtLogin() const
{
    return native::launchAtLoginEnabled();
}

void Controller::setLaunchAtLogin(bool enable)
{
    QString error;
    if (!native::setLaunchAtLogin(enable, &error))
        emit toast(error);
    emit settingsChanged();
}

void Controller::setPaused(bool paused)
{
    if (paused == m_paused)
        return;
    m_paused = paused;
    m_engine.setEnabled(!paused);
    hideOverlay();
    emit statusChanged();
}

QVariantList Controller::actionTypes() const
{
    auto t = [](const char* id, const QString& title, const char* glyph) {
        return QVariantMap { { QStringLiteral("id"), QString::fromLatin1(id) },
                             { QStringLiteral("title"), title },
                             { QStringLiteral("glyph"), QString::fromLatin1(glyph) } };
    };
    return {
        t("app", QStringLiteral("Открыть приложение"), "app"),
        t("open", QStringLiteral("Открыть файл или папку"), "folder"),
        t("url", QStringLiteral("Открыть ссылку"), "globe"),
        t("shell", QStringLiteral("Выполнить команду"), "terminal"),
        t("text", QStringLiteral("Вставить текст"), "type"),
        t("window", QStringLiteral("Переместить окно"), "window"),
        t("system", QStringLiteral("Системное действие"), "zap"),
    };
}

QVariantList Controller::windowCommands() const
{
    QVariantList list;
    for (const auto& c : leap::windowCommands())
        list.append(QVariantMap { { QStringLiteral("id"), QString::fromLatin1(c.id) },
                                  { QStringLiteral("title"), QString::fromUtf8(c.title) } });
    return list;
}

QVariantList Controller::systemCommands() const
{
    QVariantList list;
    for (const auto& c : leap::systemCommands())
        list.append(QVariantMap { { QStringLiteral("id"), QString::fromLatin1(c.id) },
                                  { QStringLiteral("title"), QString::fromUtf8(c.title) } });
    return list;
}

QString Controller::configPath() const
{
    return QDir::toNativeSeparators(Config::path()).replace(QDir::homePath(), QStringLiteral("~"));
}

QString Controller::version() const
{
    return QCoreApplication::applicationVersion();
}

// ------------------------------------------------------------------ editing

QVariantMap Controller::node(const QVariantList& path) const
{
    const Node* n = leap::nodeAt(m_config.root, toPath(path));
    if (!n || path.isEmpty())
        return {};
    QVariantMap m = describe(*n);
    m[QStringLiteral("path")] = path;
    return m;
}

void Controller::updateNode(const QVariantList& path, const QVariantMap& fields)
{
    Node* n = leap::nodeAt(m_config.root, toPath(path));
    if (!n || path.isEmpty())
        return;
    Node before = *n;
    if (fields.contains(QStringLiteral("key")))
        n->key = normalizeKey(fields.value(QStringLiteral("key")).toString()).toStdString();
    if (fields.contains(QStringLiteral("title")))
        n->title = fields.value(QStringLiteral("title")).toString().toStdString();
    if (fields.contains(QStringLiteral("sticky")))
        n->sticky = fields.value(QStringLiteral("sticky")).toBool();
    if (!n->group) {
        if (fields.contains(QStringLiteral("type"))) {
            const auto type = leap::actionTypeFromId(fields.value(QStringLiteral("type")).toString().toStdString());
            if (type != n->action.type) {
                n->action.type = type;
                // Window/system values are ids: start from the first one.
                if (type == ActionType::Window)
                    n->action.value = leap::windowCommands().front().id;
                else if (type == ActionType::System)
                    n->action.value = leap::systemCommands().front().id;
                else if (before.action.type == ActionType::Window || before.action.type == ActionType::System
                         || before.action.type == ActionType::App)
                    n->action.value.clear();
            }
        }
        if (fields.contains(QStringLiteral("value")))
            n->action.value = fields.value(QStringLiteral("value")).toString().toStdString();
    }
    if (*n == before)
        return;
    qInfo() << "Leap: edit" << path << fields;
    applyConfig(true);
}

QVariantList Controller::addNode(const QVariantList& parentPath, bool group)
{
    Node* parent = leap::nodeAt(m_config.root, toPath(parentPath));
    if (!parent || !parent->group)
        parent = &m_config.root;
    Node n;
    n.group = group;
    n.title = group ? "Новая группа" : "Новое действие";
    n.action.type = group ? ActionType::None : ActionType::App;
    n.key = leap::suggestKey(*parent, n.title);
    parent->children.push_back(std::move(n));

    leap::Path path = parent == &m_config.root ? leap::Path {} : toPath(parentPath);
    path.push_back(static_cast<int>(parent->children.size()) - 1);
    qInfo() << "Leap: add" << (group ? "group" : "action") << fromPath(path);
    applyConfig(true);
    return fromPath(path);
}

void Controller::removeNode(const QVariantList& pathList)
{
    leap::Path path = toPath(pathList);
    if (path.empty())
        return;
    const int index = path.back();
    path.pop_back();
    Node* parent = leap::nodeAt(m_config.root, path);
    if (!parent || index < 0 || index >= static_cast<int>(parent->children.size()))
        return;
    parent->children.erase(parent->children.begin() + index);
    qInfo() << "Leap: remove" << pathList;
    applyConfig(true);
}

QVariantList Controller::moveNode(const QVariantList& pathList, int delta)
{
    leap::Path path = toPath(pathList);
    if (path.empty())
        return pathList;
    const int index = path.back();
    leap::Path parentPath(path.begin(), path.end() - 1);
    Node* parent = leap::nodeAt(m_config.root, parentPath);
    const int target = index + delta;
    if (!parent || index < 0 || target < 0 || target >= static_cast<int>(parent->children.size()))
        return pathList;
    std::swap(parent->children[static_cast<size_t>(index)], parent->children[static_cast<size_t>(target)]);
    path.back() = target;
    applyConfig(true);
    return fromPath(path);
}

QVariantList Controller::moveNodeTo(const QVariantList& from, const QVariantList& toParent, int toIndex)
{
    const auto moved = leap::moveNode(m_config.root, toPath(from), toPath(toParent), toIndex);
    if (!moved)
        return {};
    if (*moved != toPath(from)) {
        qInfo() << "Leap: move" << from << "->" << fromPath(*moved);
        applyConfig(true);
    }
    return fromPath(*moved);
}

void Controller::resetToDefaults()
{
    const auto leaderBefore = m_config.leader;
    m_config = Config::defaults();
    m_config.leader = leaderBefore;
    applyConfig(true);
    emit settingsChanged();
    emit toast(QStringLiteral("Клавиши сброшены к стандартным"));
}

// ------------------------------------------------------------------ helpers

QString Controller::normalizeKey(const QString& input) const
{
    return qs(leap::normalizeKeyInput(input.toStdString()));
}

QString Controller::keyLabel(const QString& keyName) const
{
    return qs(leap::keyLabel(keyName.toStdString()));
}

QString Controller::appName(const QString& bundlePath) const
{
    return bundlePath.isEmpty() ? QString() : native::bundleDisplayName(expandHome(bundlePath));
}

QVariantList Controller::windowPreview(const QString& command) const
{
    const leap::Rect unit { 0, 0, 1000, 1000 };
    // A 60% window, so "center" shows something sensible.
    const auto r = leap::layoutWindow(command.toStdString(), unit, { 0, 0, 600, 600 });
    if (!r)
        return {};
    return { r->x / 1000.0, r->y / 1000.0, r->w / 1000.0, r->h / 1000.0 };
}

QString Controller::commandTitle(const QString& type, const QString& id) const
{
    return qs(leap::commandTitle(leap::actionTypeFromId(type.toStdString()), id.toStdString()));
}

QString Controller::localPath(const QUrl& url) const
{
    QString path = url.isLocalFile() ? url.toLocalFile() : url.toString();
    if (path.startsWith(QDir::homePath() + QLatin1Char('/')))
        path.replace(0, QDir::homePath().size(), QStringLiteral("~"));
    return path;
}

QString Controller::iconUrl(const QString& path) const
{
    return IconProvider::urlFor(expandHome(path));
}

void Controller::testNode(const QVariantList& path)
{
    const Node* n = leap::nodeAt(m_config.root, toPath(path));
    if (!n || n->group || path.isEmpty())
        return;
    const leap::Action action = n->action;
    // Window and paste actions act on the front app: give the user a moment
    // to see it isn't the settings window that's meant.
    const bool needsOtherApp = action.type == ActionType::Window || action.type == ActionType::Text;
    if (needsOtherApp)
        emit toast(QStringLiteral("Действия с окнами и текстом проверяйте в другом приложении"));
    run(action);
}

void Controller::openSettings()
{
    if (!m_settings)
        return;
    m_settings->show();
    m_settings->raise();
    m_settings->requestActivate();
    native::activateApp();
}

void Controller::revealConfig()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(Config::path()).absolutePath()));
}

// ------------------------------------------------------------------ file

void Controller::watchConfig()
{
    const QString file = Config::path();
    const QString dir = QFileInfo(file).absolutePath();
    if (!m_watcher.directories().contains(dir) && QFileInfo::exists(dir))
        m_watcher.addPath(dir);
    // QSaveFile replaces the file, which drops it from the watcher.
    if (!m_watcher.files().contains(file) && QFileInfo::exists(file))
        m_watcher.addPath(file);
}

void Controller::reloadFromDisk()
{
    watchConfig();
    QFile file(Config::path());
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QByteArray json = file.readAll();
    if (json == m_lastWritten)
        return;
    Config loaded;
    QString error;
    if (!Config::fromJson(json, &loaded, &error)) {
        emit toast(error);
        return;
    }
    m_lastWritten = json;
    if (loaded == m_config)
        return;
    m_config = loaded;
    applyConfig(false);
    emit settingsChanged();
    emit toast(QStringLiteral("Настройки перечитаны из config.json"));
}
