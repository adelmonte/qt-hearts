#include "cardtheme.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QApplication>

CardTheme::CardTheme()
    : m_loaded(false)
    , m_usingSvg(false)
{
}

QVector<ThemeInfo> CardTheme::findThemes() {
    QVector<ThemeInfo> themes;

    // Search paths for KDE card decks
    QStringList searchPaths = {
        QDir::homePath() + "/.local/share/carddecks",
        "/usr/share/carddecks",
        "/usr/share/kde4/apps/carddecks",
        "/usr/share/apps/carddecks",
        "/usr/share/kdegames/carddecks"
    };

    // Add XDG data directories
    for (const QString& dir : QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
        searchPaths << dir + "/carddecks";
    }

    for (const QString& basePath : searchPaths) {
        QDir baseDir(basePath);
        if (!baseDir.exists()) continue;

        // Look for subdirectories with index.desktop or SVG files
        for (const QString& subdir : baseDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QDir themeDir(baseDir.filePath(subdir));

            // Check for index.desktop
            QString indexFile = themeDir.filePath("index.desktop");
            if (QFile::exists(indexFile)) {
                QSettings index(indexFile, QSettings::IniFormat);
                index.beginGroup("KDE Backdeck");

                ThemeInfo info;
                info.name = index.value("Name", subdir).toString();
                info.path = themeDir.absolutePath();
                info.svgFile = index.value("SVG").toString();

                if (!info.svgFile.isEmpty()) {
                    QString svgPath = themeDir.filePath(info.svgFile);
                    if (QFile::exists(svgPath)) {
                        themes.append(info);
                    }
                }
            } else {
                // Look for SVG/SVGZ files directly
                QStringList svgFiles = themeDir.entryList({"*.svg", "*.svgz"}, QDir::Files);
                for (const QString& svg : svgFiles) {
                    ThemeInfo info;
                    info.name = QFileInfo(svg).baseName();
                    info.name[0] = info.name[0].toUpper();
                    info.path = themeDir.absolutePath();
                    info.svgFile = svg;
                    themes.append(info);
                }
            }
        }

        // Also check for SVG files directly in the base path
        QStringList svgFiles = baseDir.entryList({"*.svg", "*.svgz"}, QDir::Files);
        for (const QString& svg : svgFiles) {
            ThemeInfo info;
            info.name = QFileInfo(svg).baseName();
            info.name[0] = info.name[0].toUpper();
            info.path = basePath;
            info.svgFile = svg;
            themes.append(info);
        }
    }

    return themes;
}

bool CardTheme::loadTheme(const QString& path) {
    m_cache.clear();
    m_loaded = false;
    m_usingSvg = false;

    QFileInfo info(path);

    QString svgPath;
    if (info.isDir()) {
        // Look for deck.svgz or index.desktop
        QDir dir(path);

        QString indexFile = dir.filePath("index.desktop");
        if (QFile::exists(indexFile)) {
            QSettings index(indexFile, QSettings::IniFormat);
            index.beginGroup("KDE Backdeck");
            QString svgFile = index.value("SVG").toString();
            if (!svgFile.isEmpty()) {
                svgPath = dir.filePath(svgFile);
            }
        }

        if (svgPath.isEmpty()) {
            // Try common names
            for (const QString& name : {"deck.svgz", "deck.svg", "cards.svgz", "cards.svg"}) {
                if (QFile::exists(dir.filePath(name))) {
                    svgPath = dir.filePath(name);
                    break;
                }
            }
        }

        if (svgPath.isEmpty()) {
            // Use first SVG found
            QStringList svgs = dir.entryList({"*.svg", "*.svgz"}, QDir::Files);
            if (!svgs.isEmpty()) {
                svgPath = dir.filePath(svgs.first());
            }
        }
    } else if (info.suffix().toLower() == "svg" || info.suffix().toLower() == "svgz") {
        svgPath = path;
    }

    if (svgPath.isEmpty() || !QFile::exists(svgPath)) {
        return false;
    }

    m_renderer = std::make_unique<QSvgRenderer>(svgPath);
    if (!m_renderer->isValid()) {
        m_renderer.reset();
        return false;
    }

    m_themePath = svgPath;
    m_themeName = QFileInfo(svgPath).baseName();
    m_themeName[0] = m_themeName[0].toUpper();
    m_loaded = true;
    m_usingSvg = true;

    return true;
}

void CardTheme::loadBuiltinTheme() {
    m_cache.clear();
    m_renderer.reset();
    m_themeName = "Built-in";
    m_themePath.clear();
    m_loaded = true;
    m_usingSvg = false;
}

QPixmap CardTheme::renderSvgElement(const QString& elementId, const QSize& size) {
    if (!m_renderer || !m_renderer->isValid()) {
        return QPixmap();
    }

    qreal dpr = qApp->devicePixelRatio();
    QString cacheKey = elementId + "_" + QString::number(size.width()) + "x" + QString::number(size.height()) + "@" + QString::number(dpr);
    if (m_cache.contains(cacheKey)) {
        return m_cache[cacheKey];
    }

    // Create HiDPI-aware pixmap
    QPixmap pixmap(size * dpr);
    pixmap.setDevicePixelRatio(dpr);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Target rectangle fills the entire pixmap in LOGICAL coordinates
    // The painter already has the DPR transform applied via the pixmap
    QRectF targetRect(0, 0, size.width(), size.height());

    // Try the element ID directly
    if (m_renderer->elementExists(elementId)) {
        m_renderer->render(&painter, elementId, targetRect);
    } else {
        // Try alternative formats
        QStringList alternatives = {
            elementId.toLower(),
            elementId.toUpper(),
            QString(elementId).replace("_", "-")
        };

        bool found = false;
        for (const QString& alt : alternatives) {
            if (m_renderer->elementExists(alt)) {
                m_renderer->render(&painter, alt, targetRect);
                found = true;
                break;
            }
        }

        if (!found) {
            painter.end();
            return QPixmap(); // Element not found
        }
    }

    painter.end();
    m_cache[cacheKey] = pixmap;
    return pixmap;
}

QPixmap CardTheme::cardFront(const Card& card, const QSize& size) {
    if (m_usingSvg && m_renderer) {
        QString elementId = card.elementId();
        QPixmap pix = renderSvgElement(elementId, size);
        if (!pix.isNull()) {
            return pix;
        }
    }

    return generateCard(card, size);
}

QPixmap CardTheme::cardBack(const QSize& size) {
    if (m_usingSvg && m_renderer) {
        QPixmap pix = renderSvgElement("back", size);
        if (!pix.isNull()) {
            return pix;
        }
    }

    return generateCardBack(size);
}

QPixmap CardTheme::generateCard(const Card& card, const QSize& size) {
    qreal dpr = qApp->devicePixelRatio();
    QString cacheKey = "gen_" + card.elementId() + "_" + QString::number(size.width()) + "x" + QString::number(size.height()) + "@" + QString::number(dpr);
    if (m_cache.contains(cacheKey)) {
        return m_cache[cacheKey];
    }

    QPixmap pixmap(size * dpr);
    pixmap.setDevicePixelRatio(dpr);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    qreal w = size.width();
    qreal h = size.height();
    qreal radius = w * 0.08;
    qreal margin = w * 0.08;

    // Card background
    QRectF cardRect(1, 1, w - 2, h - 2);
    QLinearGradient bgGrad(0, 0, 0, h);
    bgGrad.setColorAt(0, QColor(255, 255, 255));
    bgGrad.setColorAt(1, QColor(240, 240, 235));

    painter.setPen(QPen(QColor(100, 100, 100), 1));
    painter.setBrush(bgGrad);
    painter.drawRoundedRect(cardRect, radius, radius);

    // Card color
    QColor color = (card.suit() == Suit::Hearts || card.suit() == Suit::Diamonds)
                   ? QColor(200, 30, 30) : QColor(20, 20, 20);

    // Rank in corners
    QString rankStr = card.rankString();
    QFont rankFont("Arial", static_cast<int>(w * 0.2), QFont::Bold);
    painter.setFont(rankFont);
    painter.setPen(color);
    painter.drawText(QRectF(margin, margin, w * 0.3, h * 0.15), Qt::AlignLeft | Qt::AlignTop, rankStr);

    // Small suit symbol (below rank text)
    qreal smallSize = w * 0.15;
    drawSuitSymbol(painter, card.suit(), QRectF(margin, margin + h * 0.18, smallSize, smallSize), color);

    // Bottom right (rotated)
    painter.save();
    painter.translate(w - margin, h - margin);
    painter.rotate(180);
    painter.drawText(QRectF(0, 0, w * 0.3, h * 0.15), Qt::AlignLeft | Qt::AlignTop, rankStr);
    drawSuitSymbol(painter, card.suit(), QRectF(0, h * 0.18, smallSize, smallSize), color);
    painter.restore();

    // Center suit symbol
    qreal centerSize = w * 0.35;
    qreal cx = (w - centerSize) / 2;
    qreal cy = (h - centerSize) / 2;
    drawSuitSymbol(painter, card.suit(), QRectF(cx, cy, centerSize, centerSize), color);

    painter.end();
    m_cache[cacheKey] = pixmap;
    return pixmap;
}

void CardTheme::drawSuitSymbol(QPainter& painter, Suit suit, const QRectF& rect, const QColor& color) {
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);

    qreal x = rect.x(), y = rect.y();
    qreal w = rect.width(), h = rect.height();

    QPainterPath path;

    switch (suit) {
        case Suit::Hearts: {
            qreal cx = x + w / 2;
            path.moveTo(cx, y + h);
            path.cubicTo(x, y + h * 0.6, x, y + h * 0.2, x + w * 0.25, y + h * 0.2);
            path.cubicTo(x + w * 0.4, y, cx, y + h * 0.15, cx, y + h * 0.3);
            path.cubicTo(cx, y + h * 0.15, x + w * 0.6, y, x + w * 0.75, y + h * 0.2);
            path.cubicTo(x + w, y + h * 0.2, x + w, y + h * 0.6, cx, y + h);
            break;
        }
        case Suit::Diamonds: {
            path.moveTo(x + w / 2, y);
            path.lineTo(x + w, y + h / 2);
            path.lineTo(x + w / 2, y + h);
            path.lineTo(x, y + h / 2);
            path.closeSubpath();
            break;
        }
        case Suit::Clubs: {
            qreal r = w * 0.2;
            qreal cx = x + w / 2;
            path.addEllipse(QPointF(cx, y + r), r, r);
            path.addEllipse(QPointF(x + r * 1.2, y + h * 0.55), r, r);
            path.addEllipse(QPointF(x + w - r * 1.2, y + h * 0.55), r, r);
            QPainterPath stem;
            stem.moveTo(cx - r * 0.4, y + h * 0.65);
            stem.lineTo(cx - r * 0.6, y + h);
            stem.lineTo(cx + r * 0.6, y + h);
            stem.lineTo(cx + r * 0.4, y + h * 0.65);
            path.addPath(stem);
            break;
        }
        case Suit::Spades: {
            qreal cx = x + w / 2;
            path.moveTo(cx, y);
            path.cubicTo(x + w, y + h * 0.4, x + w, y + h * 0.7, cx + w * 0.15, y + h * 0.55);
            path.lineTo(cx + w * 0.15, y + h);
            path.lineTo(cx - w * 0.15, y + h);
            path.lineTo(cx - w * 0.15, y + h * 0.55);
            path.cubicTo(x, y + h * 0.7, x, y + h * 0.4, cx, y);
            break;
        }
    }

    painter.drawPath(path);
    painter.restore();
}

QPixmap CardTheme::generateCardBack(const QSize& size) {
    qreal dpr = qApp->devicePixelRatio();
    QString cacheKey = "back_" + QString::number(size.width()) + "x" + QString::number(size.height()) + "@" + QString::number(dpr);
    if (m_cache.contains(cacheKey)) {
        return m_cache[cacheKey];
    }

    QPixmap pixmap(size * dpr);
    pixmap.setDevicePixelRatio(dpr);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    qreal w = size.width();
    qreal h = size.height();
    qreal radius = w * 0.08;
    qreal margin = w * 0.06;

    // Card background
    QRectF cardRect(1, 1, w - 2, h - 2);
    QLinearGradient bgGrad(0, 0, w, h);
    bgGrad.setColorAt(0, QColor(30, 60, 140));
    bgGrad.setColorAt(0.5, QColor(50, 90, 170));
    bgGrad.setColorAt(1, QColor(30, 60, 140));

    painter.setPen(QPen(QColor(20, 40, 80), 1));
    painter.setBrush(bgGrad);
    painter.drawRoundedRect(cardRect, radius, radius);

    // Inner border
    QRectF innerRect(margin, margin, w - margin * 2, h - margin * 2);
    painter.setPen(QPen(QColor(200, 180, 120), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(innerRect, radius - 2, radius - 2);

    // Pattern
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(200, 180, 120, 60));
    qreal spacing = w * 0.12;
    for (qreal py = margin + spacing; py < h - margin; py += spacing) {
        for (qreal px = margin + spacing; px < w - margin; px += spacing) {
            qreal s = spacing * 0.25;
            QPainterPath diamond;
            diamond.moveTo(px, py - s);
            diamond.lineTo(px + s, py);
            diamond.lineTo(px, py + s);
            diamond.lineTo(px - s, py);
            diamond.closeSubpath();
            painter.drawPath(diamond);
        }
    }

    // Center emblem
    qreal emblemSize = w * 0.35;
    QRectF emblemRect((w - emblemSize) / 2, (h - emblemSize) / 2, emblemSize, emblemSize);
    QRadialGradient emblemGrad(emblemRect.center(), emblemSize / 2);
    emblemGrad.setColorAt(0, QColor(220, 200, 140));
    emblemGrad.setColorAt(1, QColor(160, 140, 80));
    painter.setBrush(emblemGrad);
    painter.setPen(QPen(QColor(120, 100, 50), 2));
    painter.drawEllipse(emblemRect);

    painter.end();
    m_cache[cacheKey] = pixmap;
    return pixmap;
}
