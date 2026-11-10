// SPDX-License-Identifier: GPL-3.0-or-later
//
// Writes the report HTML to disk as a web page or a PDF.

#pragma once

#include <QString>

namespace obdaryn::ReportWriter {

/// Saves the HTML as it is (UTF-8).
bool writeHtml(const QString &html, const QString &path, QString *error);

/**
 * @brief Renders the HTML to an A4 PDF with Qt's rich-text engine.
 *
 * Lives in the app, not in src/core, because it needs QtGui (fonts, painting).
 * Pages get a small footer with the page number.
 */
bool writePdf(const QString &html, const QString &path, const QString &title, QString *error);

} // namespace obdaryn::ReportWriter
