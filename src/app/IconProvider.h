// SPDX-License-Identifier: GPL-3.0-or-later
//
// Serves tinted SVG icons to QML: Image { source: "image://icon/check/3CBF7A" }.

#pragma once

#include <QQuickImageProvider>

namespace obdaryn {

/**
 * @brief Renders the stroke icons in qrc:/icons in any colour.
 *
 * The icons (Lucide, ISC licence, plus Obdaryn's own engine telltale) draw with
 * "currentColor". The provider substitutes the requested colour and renders
 * the SVG at the size QML asks for, so icons stay sharp on Retina screens and
 * follow the light/dark theme.
 *
 * Image id format: "<name>/<RRGGBB>", e.g. "triangle-alert/F2A93B".
 */
class IconProvider : public QQuickImageProvider
{
public:
    IconProvider();
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
};

} // namespace obdaryn
