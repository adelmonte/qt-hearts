#include "cardimageprovider.h"
#include "card.h"
#include <QGuiApplication>

CardImageProvider::CardImageProvider(CardTheme* theme)
    : QQuickImageProvider(QQuickImageProvider::Pixmap)
    , m_theme(theme)
{
}

QPixmap CardImageProvider::requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) {
    // Strip query string used for cache busting
    QString cleanId = id;
    int queryIdx = id.indexOf('?');
    if (queryIdx >= 0) {
        cleanId = id.left(queryIdx);
    }

    QSize targetSize(80, 116);

    // QML sends logical pixels; CardTheme handles DPR internally
    if (requestedSize.isValid() && requestedSize.width() > 0 && requestedSize.height() > 0) {
        targetSize = requestedSize;
    }

    // Report logical size back to QML
    if (size) {
        *size = targetSize;
    }

    if (cleanId == "back") {
        return m_theme->cardBack(targetSize);
    }

    // Parse element ID ("1_club", "queen_spade") or integer format ("suit_rank")
    if (cleanId.contains('_')) {
        QStringList parts = cleanId.split('_');
        if (parts.size() == 2) {
            // Try integer format first
            bool suitOk, rankOk;
            int suitInt = parts[0].toInt(&suitOk);
            int rankInt = parts[1].toInt(&rankOk);

            if (suitOk && rankOk) {
                Card card(static_cast<Suit>(suitInt), static_cast<Rank>(rankInt));
                return m_theme->cardFront(card, targetSize);
            }

            // Fall back to element ID format
            Card card = Card::fromElementId(cleanId);
            if (card.isValid()) {
                return m_theme->cardFront(card, targetSize);
            }
        }
    }

    // Fallback
    Card card = Card::fromElementId(cleanId);
    return m_theme->cardFront(card, targetSize);
}
