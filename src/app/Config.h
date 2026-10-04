#pragma once

#include "Engine.h"
#include "Keymap.h"

#include <QByteArray>
#include <QJsonObject>
#include <QString>

// Everything the user sets up, stored as JSON in
// ~/Library/Application Support/Leap/config.json (hand-editable: Leap picks
// up changes made to the file while it runs).
struct Config {
    leap::Leader leader;
    int overlayDelayMs = 150; // how long to wait before showing the hints
    int windowGap = 0;        // px between snapped windows
    leap::Node root;

    static Config defaults();
    static QString path();

    // Loads `path()`. False if the file is missing or broken; `out` is then
    // untouched and `error` says why.
    static bool load(Config* out, QString* error);
    QByteArray toJson() const;
    bool save(QString* error) const;

    static QJsonObject nodeToJson(const leap::Node& node);
    static leap::Node nodeFromJson(const QJsonObject& object);
    static bool fromJson(const QByteArray& json, Config* out, QString* error);

    bool operator==(const Config&) const = default;
};
