#include "IconProvider.h"

#include "Native.h"

#include <QUrl>

IconProvider::IconProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QString IconProvider::urlFor(const QString& path)
{
    if (path.isEmpty())
        return {};
    return QStringLiteral("image://fileicon/") + QString::fromUtf8(QUrl::toPercentEncoding(path));
}

QImage IconProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize)
{
    const QString path = QUrl::fromPercentEncoding(id.toUtf8());
    const int px = requestedSize.width() > 0 ? requestedSize.width() : 64;
    const QString cacheKey = QString::number(px) + QLatin1Char(':') + path;

    QImage image;
    {
        QMutexLocker lock(&m_mutex);
        image = m_cache.value(cacheKey);
    }
    if (image.isNull()) {
        image = native::fileIcon(path, px);
        if (image.isNull()) {
            image = QImage(px, px, QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::transparent);
        }
        QMutexLocker lock(&m_mutex);
        m_cache.insert(cacheKey, image);
    }
    if (size)
        *size = image.size();
    return image;
}
