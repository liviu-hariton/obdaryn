// SPDX-License-Identifier: GPL-3.0-or-later

#include "IconProvider.h"

#include <QFile>
#include <QHash>
#include <QMutex>
#include <QPainter>
#include <QSvgRenderer>

namespace obdaryn {

IconProvider::IconProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}

QImage IconProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    const QString name = id.section(QLatin1Char('/'), 0, 0);
    QString color = id.section(QLatin1Char('/'), 1, 1);
    if (color.isEmpty())
        color = QStringLiteral("000000");

    // SVG sources are tiny; cache the text, not the rendered images.
    static QHash<QString, QByteArray> cache;
    static QMutex mutex; // image providers may run on a loader thread
    QByteArray svg;
    {
        QMutexLocker lock(&mutex);
        auto it = cache.constFind(name);
        if (it == cache.constEnd()) {
            QFile f(QStringLiteral(":/icons/%1.svg").arg(name));
            it = cache.insert(name, f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray());
        }
        svg = *it;
    }
    svg.replace("currentColor", "#" + color.toLatin1());

    const QSize target = requestedSize.isValid() && !requestedSize.isEmpty() ? requestedSize : QSize(24, 24);
    QImage image(target, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QSvgRenderer renderer(svg);
    if (renderer.isValid()) {
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        renderer.render(&painter);
    }
    if (size)
        *size = target;
    return image;
}

} // namespace obdaryn
