// SPDX-License-Identifier: GPL-3.0-or-later

#include "report/ReportWriter.h"

#include "report/VehicleReport.h"

#include <QAbstractTextDocumentLayout>
#include <QCoreApplication>
#include <QFile>
#include <QGuiApplication>
#include <QScreen>
#include <QPainter>
#include <QPdfWriter>
#include <QTextDocument>
#include <QSvgRenderer>
#include <QImage>
#include <QUrl>

namespace obdaryn::ReportWriter {

bool writeHtml(const QString &html, const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        *error = file.errorString();
        return false;
    }
    file.write(html.toUtf8());
    return true;
}

bool writePdf(const QString &html, const QString &path, const QString &title, QString *error)
{
    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageMargins(QMarginsF(18, 16, 18, 14), QPageLayout::Millimeter);
    writer.setResolution(300);
    writer.setTitle(title);
    writer.setCreator(QStringLiteral("Obdaryn"));

    QTextDocument doc;
    // Lay the text out at screen resolution (96 dpi), as a browser would, and
    // scale it onto the page: CSS lengths and default margins then mean what
    // they mean on screen. Design metrics keep the scaled text from drifting.
    // Qt's text layout without a paint device uses the screen's logical DPI
    // (72 on macOS, 96 on Windows) to turn points into pixels.
    const qreal kLayoutDpi = QGuiApplication::primaryScreen() ? QGuiApplication::primaryScreen()->logicalDotsPerInchY() : 96.0;
    doc.setUseDesignMetrics(true);
    // Static faces only: the bundled Inter is a variable font, and PDF output
    // would render every weight of it the same.
    QFont base(QStringLiteral("Helvetica Neue"));
    base.setFamilies({QStringLiteral("Helvetica Neue"), QStringLiteral("Segoe UI"), QStringLiteral("Arial")});
    base.setPointSizeF(10);
    doc.setDefaultFont(base);
    doc.setDocumentMargin(0);
    // The logo, drawn from its SVG at 8x so it stays sharp at print resolution.
    {
        QSvgRenderer svg(VehicleReport::logoSvg(false));
        QImage logo(256, 256, QImage::Format_ARGB32_Premultiplied);
        logo.fill(Qt::transparent);
        QPainter p(&logo);
        p.setRenderHint(QPainter::Antialiasing);
        svg.render(&p);
        p.end();
        doc.addResource(QTextDocument::ImageResource, QUrl(QString::fromLatin1(VehicleReport::kLogoImageName)), logo);
    }
    doc.setHtml(html);

    const qreal scale = writer.resolution() / kLayoutDpi;
    const QRect page = writer.pageLayout().paintRectPixels(writer.resolution());
    const qreal footerHeight = 0.25 * kLayoutDpi; // a quarter inch for the page number
    const QSizeF body(page.width() / scale, page.height() / scale - footerHeight);
    doc.setPageSize(body); // (setTextWidth would undo the page height)

    QPainter painter;
    if (!painter.begin(&writer)) {
        *error = QCoreApplication::translate("Report", "the PDF file couldn't be created (is the folder writable?)");
        return false;
    }
    painter.scale(scale, scale);
    QFont footerFont = base;
    footerFont.setPointSizeF(8);
    const int pages = doc.pageCount();
    for (int i = 0; i < pages; ++i) {
        if (i > 0)
            writer.newPage();
        // Draw the slice of the long document that belongs on this page.
        painter.save();
        painter.translate(0, -i * body.height());
        const QRectF slice(0, i * body.height(), body.width(), body.height());
        painter.setClipRect(slice);
        // Paper is white whatever the system appearance: never take the text
        // color from the (possibly dark) application palette.
        QAbstractTextDocumentLayout::PaintContext context;
        context.palette.setColor(QPalette::Text, QColor(0x15, 0x18, 0x1C));
        context.clip = slice;
        doc.documentLayout()->draw(&painter, context);
        painter.restore();

        // The footer in page coordinates, so its point size is real points.
        painter.save();
        painter.resetTransform();
        painter.setFont(footerFont);
        painter.setPen(QColor(0x6B, 0x73, 0x7E));
        const QRectF footer(0, page.height() - footerHeight * scale, page.width(), footerHeight * scale);
        painter.drawText(footer, Qt::AlignLeft | Qt::AlignBottom, title);
        //: PDF footer, e.g. "2 / 5"
        painter.drawText(footer, Qt::AlignRight | Qt::AlignBottom,
                         QCoreApplication::translate("Report", "%1 / %2").arg(i + 1).arg(pages));
        painter.restore();
    }
    painter.end();
    return true;
}

} // namespace obdaryn::ReportWriter
