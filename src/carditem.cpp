#include "carditem.h"
#include "cardtheme.h"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QCursor>

CardItem::CardItem(const Card& card, CardTheme* theme, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , m_card(card)
    , m_theme(theme)
    , m_size(80, 116)
    , m_faceUp(false)
    , m_selected(false)
    , m_playable(false)
    , m_hovered(false)
    , m_inTrick(false)
    , m_received(false)
    , m_keyboardFocused(false)
{
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    updatePixmap();
}

void CardItem::setCard(const Card& card) {
    m_card = card;
    updatePixmap();
    update();
}

void CardItem::setFaceUp(bool up) {
    if (m_faceUp != up) {
        m_faceUp = up;
        update();
    }
}

void CardItem::setSelected(bool selected) {
    if (m_selected != selected) {
        m_selected = selected;
        update();
    }
}

void CardItem::setPlayable(bool playable) {
    if (m_playable != playable) {
        m_playable = playable;
        setCursor(playable ? Qt::PointingHandCursor : Qt::ArrowCursor);
        update();
    }
}

void CardItem::resetVisualState() {
    m_selected = false;
    m_playable = false;
    m_hovered = false;
    m_inTrick = false;
    m_received = false;
    m_keyboardFocused = false;
    setCursor(Qt::ArrowCursor);
    update();
}

void CardItem::setInTrick(bool inTrick) {
    if (m_inTrick != inTrick) {
        m_inTrick = inTrick;
        update();
    }
}

void CardItem::setReceived(bool received) {
    if (m_received != received) {
        m_received = received;
        update();
    }
}

void CardItem::setKeyboardFocused(bool focused) {
    if (m_keyboardFocused != focused) {
        m_keyboardFocused = focused;
        update();
    }
}

void CardItem::setCardSize(const QSize& size) {
    if (m_size != size) {
        prepareGeometryChange();
        m_size = size;
        updatePixmap();
    }
}

void CardItem::updatePixmap() {
    if (m_theme) {
        m_frontPixmap = m_theme->cardFront(m_card, m_size);
        m_backPixmap = m_theme->cardBack(m_size);
    }
}

QRectF CardItem::boundingRect() const {
    return QRectF(0, 0, m_size.width() + 4, m_size.height() + 4);
}

void CardItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    qreal yOffset = 0;

    // Selected cards float up
    if (m_selected) {
        yOffset = -15;
    } else if ((m_hovered || m_keyboardFocused) && m_playable) {
        yOffset = -8;
    }

    QRectF cardRect(2, 2 + yOffset, m_size.width(), m_size.height());

    // Draw shadow
    if (yOffset != 0) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 0, 0, 40));
        painter->drawRoundedRect(cardRect.translated(3, 3 - yOffset), 5, 5);
    }

    // Draw card
    const QPixmap& pixmap = m_faceUp ? m_frontPixmap : m_backPixmap;
    if (!pixmap.isNull()) {
        painter->drawPixmap(cardRect.toRect(), pixmap);
    } else {
        // Fallback rectangle
        painter->setBrush(m_faceUp ? Qt::white : QColor(30, 60, 140));
        painter->setPen(QPen(Qt::black, 1));
        painter->drawRoundedRect(cardRect, 5, 5);
    }

    // Highlight for playable cards (hover or keyboard focus)
    if (m_playable && (m_hovered || m_keyboardFocused)) {
        QColor highlightColor = m_keyboardFocused ? QColor(255, 220, 100) : QColor(100, 200, 100);
        painter->setPen(QPen(highlightColor, 3));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(cardRect.adjusted(-1, -1, 1, 1), 6, 6);
    }

    // Selection indicator
    if (m_selected) {
        painter->setPen(QPen(QColor(50, 150, 255), 3));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(cardRect.adjusted(-1, -1, 1, 1), 6, 6);
    }

    // Received card indicator (golden glow)
    if (m_received) {
        painter->setPen(QPen(QColor(255, 200, 50), 3));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(cardRect.adjusted(-1, -1, 1, 1), 6, 6);
        // Add a subtle glow effect
        painter->setPen(QPen(QColor(255, 200, 50, 100), 5));
        painter->drawRoundedRect(cardRect.adjusted(-2, -2, 2, 2), 7, 7);
    }

    // Dim non-playable cards when it's player's turn (but not cards in the trick area or received cards)
    if (!m_playable && m_faceUp && !m_inTrick && !m_received) {
        painter->fillRect(cardRect, QColor(0, 0, 0, 60));
    }
}

void CardItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked(this);
        event->accept();
    } else {
        QGraphicsObject::mousePressEvent(event);
    }
}

void CardItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
    m_hovered = true;
    update();
    QGraphicsObject::hoverEnterEvent(event);
}

void CardItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    m_hovered = false;
    update();
    QGraphicsObject::hoverLeaveEvent(event);
}
