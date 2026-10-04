#pragma once

#include <QHash>
#include <QMutex>
#include <QQuickImageProvider>

// image://fileicon/<url-encoded path>: Finder icons of apps, files and
// folders for the overlay and the settings list. Cached per path and size.
class IconProvider : public QQuickImageProvider {
public:
    IconProvider();
    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

    static QString urlFor(const QString& path);

private:
    QMutex m_mutex;
    QHash<QString, QImage> m_cache;
};
