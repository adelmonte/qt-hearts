#ifndef CARDITEM_H
#define CARDITEM_H

#include "card.h"
#include <QGraphicsObject>

class CardTheme;

class CardItem : public QGraphicsObject {
    Q_OBJECT
    Q_PROPERTY(QPointF pos READ pos WRITE setPos)
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
    Q_PROPERTY(qreal scale READ scale WRITE setScale)
    Q_PROPERTY(qreal rotation READ rotation WRITE setRotation)

public:
    CardItem(const Card& card, CardTheme* theme, QGraphicsItem* parent = nullptr);

    Card card() const { return m_card; }
    void setCard(const Card& card);

    bool isFaceUp() const { return m_faceUp; }
    void setFaceUp(bool up);

    bool isSelected() const { return m_selected; }
    void setSelected(bool selected);

    bool isPlayable() const { return m_playable; }
    void setPlayable(bool playable);

    void resetVisualState();

    bool isInTrick() const { return m_inTrick; }
    void setInTrick(bool inTrick);

    bool isReceived() const { return m_received; }
    void setReceived(bool received);

    bool isKeyboardFocused() const { return m_keyboardFocused; }
    void setKeyboardFocused(bool focused);

    void setCardSize(const QSize& size);
    QSize cardSize() const { return m_size; }

    void updatePixmap();

    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

signals:
    void clicked(CardItem* item);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private:
    Card m_card;
    CardTheme* m_theme;
    QSize m_size;
    bool m_faceUp;
    bool m_selected;
    bool m_playable;
    bool m_hovered;
    bool m_inTrick;
    bool m_received;
    bool m_keyboardFocused;
    QPixmap m_frontPixmap;
    QPixmap m_backPixmap;
};

#endif // CARDITEM_H
