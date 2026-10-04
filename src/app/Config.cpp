#include "Config.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>

#include <algorithm>
#include <vector>

using leap::ActionType;
using leap::Node;

namespace {

constexpr int kVersion = 1;

Node leaf(const char* key, const QString& title, ActionType type, const QString& value)
{
    Node n;
    n.key = key;
    n.title = title.toStdString();
    n.action = { type, value.toStdString() };
    return n;
}

Node group(const char* key, const QString& title, std::vector<Node> children, bool sticky = false)
{
    Node n;
    n.key = key;
    n.title = title.toStdString();
    n.group = true;
    n.sticky = sticky;
    n.children = std::move(children);
    return n;
}

// First existing bundle out of the usual install locations.
QString findApp(const std::vector<const char*>& names)
{
    const QStringList dirs = {
        QStringLiteral("/Applications"),
        QDir::homePath() + QStringLiteral("/Applications"),
        QStringLiteral("/System/Applications"),
        QStringLiteral("/System/Applications/Utilities"),
        QStringLiteral("/System/Library/CoreServices"),
        QStringLiteral("/System/Volumes/Preboot/Cryptexes/App/System/Applications"),
    };
    for (const char* name : names)
        for (const QString& dir : dirs) {
            const QString path = dir + QLatin1Char('/') + QLatin1String(name) + QStringLiteral(".app");
            if (QFileInfo::exists(path))
                return path;
        }
    return {};
}

} // namespace

Config Config::defaults()
{
    Config c;
    c.root.group = true;

    struct AppDefault {
        const char* key;
        std::vector<const char*> names;
    };
    const AppDefault apps[] = {
        { "t", { "Telegram", "Telegram Desktop" } },
        { "c", { "CLion" } },
        { "p", { "PyCharm", "PyCharm CE", "PyCharm Professional Edition" } },
        { "b", { "Google Chrome", "Arc", "Yandex", "Safari" } },
        { "f", { "Finder" } },
        { "x", { "Terminal" } },
    };
    for (const auto& app : apps) {
        const QString path = findApp(app.names);
        if (path.isEmpty())
            continue;
        c.root.children.push_back(leaf(app.key, QFileInfo(path).completeBaseName(), ActionType::App, path));
    }

    c.root.children.push_back(group("w", QStringLiteral("Окна"), {
        leaf("h", QStringLiteral("Левая половина"), ActionType::Window, QStringLiteral("left-half")),
        leaf("l", QStringLiteral("Правая половина"), ActionType::Window, QStringLiteral("right-half")),
        leaf("m", QStringLiteral("Развернуть"), ActionType::Window, QStringLiteral("maximize")),
        leaf("c", QStringLiteral("По центру"), ActionType::Window, QStringLiteral("center")),
        leaf("1", QStringLiteral("Левая треть"), ActionType::Window, QStringLiteral("left-third")),
        leaf("2", QStringLiteral("Средняя треть"), ActionType::Window, QStringLiteral("center-third")),
        leaf("3", QStringLiteral("Правая треть"), ActionType::Window, QStringLiteral("right-third")),
        leaf("n", QStringLiteral("На следующий монитор"), ActionType::Window, QStringLiteral("next-display")),
    }));

    c.root.children.push_back(group("o", QStringLiteral("Открыть"), {
        leaf("d", QStringLiteral("Загрузки"), ActionType::Open, QStringLiteral("~/Downloads")),
        leaf("p", QStringLiteral("Проекты CLion"), ActionType::Open, QStringLiteral("~/CLionProjects")),
        leaf("g", QStringLiteral("GitHub"), ActionType::Url, QStringLiteral("https://github.com")),
    }));

    c.root.children.push_back(group("i", QStringLiteral("Вставить текст"), {
        leaf("s", QStringLiteral("Спасибо, вернусь с ответом"), ActionType::Text,
             QStringLiteral("Спасибо! Посмотрю и вернусь с ответом сегодня.")),
    }));

    c.root.children.push_back(group(";", QStringLiteral("Система"), {
        leaf("u", QStringLiteral("Громче"), ActionType::System, QStringLiteral("volume-up")),
        leaf("d", QStringLiteral("Тише"), ActionType::System, QStringLiteral("volume-down")),
        leaf("m", QStringLiteral("Выключить звук"), ActionType::System, QStringLiteral("mute")),
        leaf("n", QStringLiteral("Тёмная тема"), ActionType::System, QStringLiteral("dark-mode")),
        leaf("l", QStringLiteral("Заблокировать"), ActionType::System, QStringLiteral("lock")),
    }, true));

    return c;
}

QString Config::path()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/config.json");
}

QJsonObject Config::nodeToJson(const Node& node)
{
    QJsonObject o;
    o[QStringLiteral("key")] = QString::fromStdString(node.key);
    o[QStringLiteral("title")] = QString::fromStdString(node.title);
    if (node.group) {
        o[QStringLiteral("type")] = QStringLiteral("group");
        if (node.sticky)
            o[QStringLiteral("sticky")] = true;
        QJsonArray children;
        for (const auto& child : node.children)
            children.append(nodeToJson(child));
        o[QStringLiteral("children")] = children;
    } else {
        o[QStringLiteral("type")] = QString::fromUtf8(leap::actionTypeId(node.action.type).data());
        o[QStringLiteral("value")] = QString::fromStdString(node.action.value);
    }
    return o;
}

Node Config::nodeFromJson(const QJsonObject& o)
{
    Node n;
    n.key = o.value(QStringLiteral("key")).toString().toStdString();
    n.title = o.value(QStringLiteral("title")).toString().toStdString();
    const QString type = o.value(QStringLiteral("type")).toString();
    if (type == QLatin1String("group")) {
        n.group = true;
        n.sticky = o.value(QStringLiteral("sticky")).toBool();
        for (const auto& child : o.value(QStringLiteral("children")).toArray())
            n.children.push_back(nodeFromJson(child.toObject()));
    } else {
        n.action.type = leap::actionTypeFromId(type.toStdString());
        n.action.value = o.value(QStringLiteral("value")).toString().toStdString();
    }
    return n;
}

QByteArray Config::toJson() const
{
    QJsonObject o;
    o[QStringLiteral("version")] = kVersion;
    o[QStringLiteral("leader")] = QString::fromStdString(leader.id());
    o[QStringLiteral("overlayDelay")] = overlayDelayMs;
    o[QStringLiteral("windowGap")] = windowGap;
    o[QStringLiteral("keys")] = nodeToJson(root).value(QStringLiteral("children"));
    return QJsonDocument(o).toJson(QJsonDocument::Indented);
}

bool Config::fromJson(const QByteArray& json, Config* out, QString* error)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (!doc.isObject()) {
        if (error)
            *error = QStringLiteral("config.json: %1").arg(parseError.errorString());
        return false;
    }
    const QJsonObject o = doc.object();
    Config c;
    c.leader = leap::Leader::fromId(o.value(QStringLiteral("leader")).toString().toStdString())
                   .value_or(leap::Leader::capsLock());
    c.overlayDelayMs = std::clamp(o.value(QStringLiteral("overlayDelay")).toInt(150), 0, 2000);
    c.windowGap = std::clamp(o.value(QStringLiteral("windowGap")).toInt(0), 0, 64);
    c.root.group = true;
    for (const auto& child : o.value(QStringLiteral("keys")).toArray())
        c.root.children.push_back(nodeFromJson(child.toObject()));
    *out = std::move(c);
    return true;
}

bool Config::load(Config* out, QString* error)
{
    QFile file(path());
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = file.errorString();
        return false;
    }
    return fromJson(file.readAll(), out, error);
}

bool Config::save(QString* error) const
{
    QDir().mkpath(QFileInfo(path()).absolutePath());
    QSaveFile file(path());
    if (!file.open(QIODevice::WriteOnly) || file.write(toJson()) < 0 || !file.commit()) {
        if (error)
            *error = file.errorString();
        return false;
    }
    return true;
}
