#ifndef GAMEVIEW_H
#define GAMEVIEW_H

#include "game.h"
#include "cardtheme.h"
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsRectItem>
#include <QKeyEvent>
#include <memory>

class CardItem;

class GameView : public QGraphicsView {
    Q_OBJECT

public:
    explicit GameView(QWidget* parent = nullptr);
    ~GameView();

    void setGame(Game* game);
    void setTheme(CardTheme* theme);
    void setCardScale(qreal scale);
    qreal cardScale() const { return m_cardScale; }

protected:
    void resizeEvent(QResizeEvent* event) override;
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onStateChanged(GameState state);
    void onCardsDealt();
    void onPassDirectionAnnounced(PassDirection dir);
    void onPassingComplete(Cards receivedCards);
    void onCardPlayed(int player, Card card);
    void onTrickWon(int winner, int points);
    void onRoundEnded();
    void onGameEnded(int winner);
    void onScoresChanged();
    void onCurrentPlayerChanged(int player);
    void onHeartsBroken();

    void onCardClicked(CardItem* item);
    void clearReceivedCardHighlight();

public slots:
    void onUndoPerformed();

private:
    void createScene();
    void clearCards();
    void updateCards();
    void updatePlayerHand();
    void updateOpponentHands();
    void updateTrickArea();
    void updateScoreboards();
    void updatePlayableCards();

    void layoutCards();
    void layoutPlayerHand();
    void layoutOpponentHand(int player);
    void layoutScoreboards();
    void layoutTrickCards();

    QPointF playerHandPosition() const;
    QPointF opponentHandPosition(int player) const;
    QPointF trickCardPosition(int player) const;
    QPointF scoreboardPosition(int player) const;

    void showMessage(const QString& text, int durationMs = 2000);
    void hideMessage();
    void showPassArrow(PassDirection dir);
    void hidePassArrow();
    void showGameOver(int winner);

    CardItem* createCardItem(const Card& card);

    Game* m_game;
    CardTheme* m_theme;
    QGraphicsScene* m_scene;

    // Card items
    QVector<CardItem*> m_playerCards;
    QVector<QVector<CardItem*>> m_opponentCards;
    QVector<CardItem*> m_trickCards;

    // UI elements
    QGraphicsTextItem* m_messageText;
    QGraphicsRectItem* m_messageBackground;
    QGraphicsTextItem* m_passArrow;
    QGraphicsRectItem* m_overlay;
    QGraphicsTextItem* m_overlayText;

    struct Scoreboard {
        QGraphicsRectItem* background;
        QGraphicsTextItem* nameText;
        QGraphicsTextItem* scoreText;
    };
    QVector<Scoreboard> m_scoreboards;

    // Passing state
    Cards m_selectedPassCards;
    Cards m_receivedCards;  // Cards received from passing
    bool m_inputBlocked;
    bool m_showingReceivedCards;  // True while received cards are highlighted
    bool m_passConfirmed;  // True after 3 cards confirmed, until next round
    QTimer* m_receivedHighlightTimer;

    // Keyboard navigation
    int m_keyboardFocusIndex;  // Currently focused card (-1 = none)
    void updateKeyboardFocus();
    void selectFocusedCard();

    // Sizes
    qreal m_cardWidth;
    qreal m_cardHeight;
    qreal m_cardSpacing;
    qreal m_cardScale;
};

#endif // GAMEVIEW_H
