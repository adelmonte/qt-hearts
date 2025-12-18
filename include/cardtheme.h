#ifndef CARDTHEME_H
#define CARDTHEME_H

#include "card.h"
#include <QString>
#include <QPixmap>
#include <QSvgRenderer>
#include <QHash>
#include <memory>

struct ThemeInfo {
    QString name;
    QString path;
    QString svgFile;
};

class CardTheme {
public:
    CardTheme();

    // Find available themes
    static QVector<ThemeInfo> findThemes();

    // Load a theme
    bool loadTheme(const QString& path);
    void loadBuiltinTheme();

    // Render cards
    QPixmap cardFront(const Card& card, const QSize& size);
    QPixmap cardBack(const QSize& size);

    QString themeName() const { return m_themeName; }
    bool isLoaded() const { return m_loaded; }

private:
    QPixmap renderSvgElement(const QString& elementId, const QSize& size);
    QPixmap generateCard(const Card& card, const QSize& size);
    QPixmap generateCardBack(const QSize& size);
    void drawSuitSymbol(QPainter& painter, Suit suit, const QRectF& rect, const QColor& color);

    std::unique_ptr<QSvgRenderer> m_renderer;
    QString m_themeName;
    QString m_themePath;
    bool m_loaded;
    bool m_usingSvg;
    mutable QHash<QString, QPixmap> m_cache;
};

#endif // CARDTHEME_H
