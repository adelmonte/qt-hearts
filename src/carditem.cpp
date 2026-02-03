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
    // No caching - cards are already pre-rendered pixmaps, double-caching hurts at large sizes
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
    // Minimal paint - just blit the pre-rendered pixmap
    const QPixmap& pixmap = m_faceUp ? m_frontPixmap : m_backPixmap;
    if (!pixmap.isNull()) {
        painter->drawPixmap(2, 2, pixmap);
    }

    // Only draw overlays when needed (these are rare states)
    if (m_selected || m_received || ((m_hovered || m_keyboardFocused) && m_playable)) {
        QRectF cardRect(2, 2, m_size.width(), m_size.height());
        QColor borderColor;
        if (m_received) {
            borderColor = QColor(255, 200, 50);
        } else if (m_selected) {
            borderColor = QColor(50, 150, 255);
        } else {
            borderColor = m_keyboardFocused ? QColor(255, 220, 100) : QColor(100, 200, 100);
        }
        painter->setPen(QPen(borderColor, 3));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(cardRect.adjusted(-1, -1, 1, 1), 6, 6);
    }

    // Dim non-playable cards
    if (!m_playable && m_faceUp && !m_inTrick && !m_received) {
        painter->fillRect(QRectF(2, 2, m_size.width(), m_size.height()), QColor(0, 0, 0, 60));
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
