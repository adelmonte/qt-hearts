#include "gameview.h"
#include "carditem.h"
#include <QPainter>
#include <QResizeEvent>
#include <QTimer>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QRandomGenerator>
#include <cmath>

GameView::GameView(QWidget* parent)
    : QGraphicsView(parent)
    , m_game(nullptr)
    , m_theme(nullptr)
    , m_scene(new QGraphicsScene(this))
    , m_messageText(nullptr)
    , m_messageBackground(nullptr)
    , m_passArrow(nullptr)
    , m_overlay(nullptr)
    , m_overlayText(nullptr)
    , m_currentPassDirection(PassDirection::None)
    , m_inputBlocked(false)
    , m_showingReceivedCards(false)
    , m_passConfirmed(false)
    , m_receivedHighlightTimer(nullptr)
    , m_keyboardFocusIndex(-1)
    , m_cardWidth(80)
    , m_cardHeight(116)
    , m_cardSpacing(22)
    , m_cardScale(1.0)
    , m_animateCardRotation(true)
    , m_animateAICards(true)
    , m_animatePassingCards(true)
{
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameStyle(QFrame::NoFrame);
    setFocusPolicy(Qt::StrongFocus);  // Enable keyboard focus

    m_opponentCards.resize(4); // For players 1, 2, 3

    createScene();
}

GameView::~GameView() {
    clearCards();
}

void GameView::createScene() {
    // Message background
    m_messageBackground = m_scene->addRect(0, 0, 300, 40);
    m_messageBackground->setBrush(QColor(0, 0, 0, 180));
    m_messageBackground->setPen(Qt::NoPen);
    m_messageBackground->setZValue(500);
    m_messageBackground->setVisible(false);

    // Message text
    m_messageText = m_scene->addText("");
    m_messageText->setDefaultTextColor(Qt::white);
    m_messageText->setFont(QFont("Sans", 14, QFont::Bold));
    m_messageText->setZValue(501);
    m_messageText->setVisible(false);

    // Pass arrow
    m_passArrow = m_scene->addText("");
    m_passArrow->setFont(QFont("Sans", 48, QFont::Bold));
    m_passArrow->setDefaultTextColor(QColor(255, 255, 255, 200));
    m_passArrow->setZValue(400);
    m_passArrow->setVisible(false);

    // Game over overlay
    m_overlay = m_scene->addRect(0, 0, 100, 100);
    m_overlay->setBrush(QColor(0, 0, 0, 180));
    m_overlay->setPen(Qt::NoPen);
    m_overlay->setZValue(900);
    m_overlay->setVisible(false);

    m_overlayText = m_scene->addText("");
    m_overlayText->setFont(QFont("Sans", 36, QFont::Bold));
    m_overlayText->setZValue(901);
    m_overlayText->setVisible(false);

    // Scoreboards
    m_scoreboards.resize(4);
    for (int i = 0; i < 4; ++i) {
        auto& sb = m_scoreboards[i];
        sb.background = m_scene->addRect(0, 0, 100, 50);
        sb.background->setBrush(QColor(30, 30, 30, 200));
        sb.background->setPen(QPen(QColor(80, 80, 80), 1));
        sb.background->setZValue(100);

        sb.nameText = m_scene->addText("");
        sb.nameText->setFont(QFont("Sans", 10, QFont::Bold));
        sb.nameText->setDefaultTextColor(Qt::white);
        sb.nameText->setZValue(101);

        sb.scoreText = m_scene->addText("0");
        sb.scoreText->setFont(QFont("Sans", 12, QFont::Bold));
        sb.scoreText->setDefaultTextColor(QColor(255, 220, 80));
        sb.scoreText->setZValue(101);
    }
}

void GameView::setGame(Game* game) {
    if (m_game) {
        disconnect(m_game, nullptr, this, nullptr);
    }

    m_game = game;

    if (m_game) {
        connect(m_game, &Game::stateChanged, this, &GameView::onStateChanged);
        connect(m_game, &Game::cardsDealt, this, &GameView::onCardsDealt);
        connect(m_game, &Game::passDirectionAnnounced, this, &GameView::onPassDirectionAnnounced);
        connect(m_game, &Game::passingComplete, this, &GameView::onPassingComplete);
        connect(m_game, &Game::cardPlayed, this, &GameView::onCardPlayed);
        connect(m_game, &Game::trickWon, this, &GameView::onTrickWon);
        connect(m_game, &Game::roundEnded, this, &GameView::onRoundEnded);
        connect(m_game, &Game::gameEnded, this, &GameView::onGameEnded);
        connect(m_game, &Game::scoresChanged, this, &GameView::onScoresChanged);
        connect(m_game, &Game::currentPlayerChanged, this, &GameView::onCurrentPlayerChanged);
        connect(m_game, &Game::heartsBrokenSignal, this, &GameView::onHeartsBroken);
    }

    updateScoreboards();
}

void GameView::setTheme(CardTheme* theme) {
    m_theme = theme;
    updateCards();
}

void GameView::setCardScale(qreal scale) {
    m_cardScale = std::max(0.5, std::min(scale, 2.0));
    QResizeEvent event(size(), size());
    resizeEvent(&event);
}

void GameView::setAnimationSettings(bool cardRotation, bool aiCards, bool passingCards) {
    m_animateCardRotation = cardRotation;
    m_animateAICards = aiCards;
    m_animatePassingCards = passingCards;
}

void GameView::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);

    int w = event->size().width();
    int h = event->size().height();

    m_scene->setSceneRect(0, 0, w, h);

    if (m_overlay) {
        m_overlay->setRect(0, 0, w, h);
    }

    // Scale cards based on window size and user preference
    qreal scale = std::min(w / 1000.0, h / 750.0);
    scale = std::max(0.6, std::min(scale, 1.5)) * m_cardScale;

    m_cardWidth = 80 * scale;
    m_cardHeight = 116 * scale;
    m_cardSpacing = 22 * scale;

    // Update all card sizes
    QSize cardSize(static_cast<int>(m_cardWidth), static_cast<int>(m_cardHeight));
    for (CardItem* item : m_playerCards) {
        item->setCardSize(cardSize);
    }
    for (auto& hand : m_opponentCards) {
        for (CardItem* item : hand) {
            item->setCardSize(cardSize);
        }
    }
    for (CardItem* item : m_trickCards) {
        item->setCardSize(cardSize);
    }

    layoutCards();
    layoutScoreboards();
    layoutTrickCards();  // Also relayout trick cards on resize

    // Schedule a delayed relayout to correct any cards that were mid-animation
    QTimer::singleShot(250, this, [this]() {
        layoutTrickCards();
    });
}

void GameView::drawBackground(QPainter* painter, const QRectF& rect) {
    // Green felt background
    QRadialGradient gradient(rect.center(), std::max(rect.width(), rect.height()) * 0.7);
    gradient.setColorAt(0.0, QColor(45, 130, 45));
    gradient.setColorAt(0.5, QColor(35, 105, 35));
    gradient.setColorAt(1.0, QColor(25, 80, 25));
    painter->fillRect(rect, gradient);
}

void GameView::clearCards() {
    // Clear tracked card vectors
    m_playerCards.clear();
    m_trickCards.clear();
    for (auto& hand : m_opponentCards) {
        hand.clear();
    }

    // Remove ALL CardItem objects from the scene (including orphaned animated cards)
    // CardItem inherits from QGraphicsObject, so we can use qobject_cast
    QList<QGraphicsItem*> items = m_scene->items();
    QList<CardItem*> cardsToDelete;
    for (QGraphicsItem* item : items) {
        QGraphicsObject* obj = item->toGraphicsObject();
        if (obj) {
            CardItem* card = qobject_cast<CardItem*>(obj);
            if (card) {
                cardsToDelete.append(card);
            }
        }
    }
    for (CardItem* card : cardsToDelete) {
        m_scene->removeItem(card);
        delete card;
    }
}

CardItem* GameView::createCardItem(const Card& card) {
    CardItem* item = new CardItem(card, m_theme);
    item->setCardSize(QSize(static_cast<int>(m_cardWidth), static_cast<int>(m_cardHeight)));
    connect(item, &CardItem::clicked, this, &GameView::onCardClicked);
    m_scene->addItem(item);
    return item;
}

void GameView::updateCards() {
    // Clear selection state on old cards before destroying them
    for (CardItem* item : m_playerCards) {
        item->setSelected(false);
    }

    // Reset keyboard focus when cards change
    m_keyboardFocusIndex = -1;

    clearCards();

    if (!m_game || !m_theme) return;

    // Create player's cards
    const Cards& hand = m_game->player(0)->hand();
    for (const Card& card : hand) {
        CardItem* item = createCardItem(card);
        item->setFaceUp(true);
        m_playerCards.append(item);
    }

    // Create opponent cards (face down)
    for (int p = 1; p < 4; ++p) {
        int numCards = m_game->player(p)->hand().size();
        for (int i = 0; i < numCards; ++i) {
            CardItem* item = createCardItem(Card(Suit::Clubs, Rank::Two)); // Placeholder
            item->setFaceUp(false);
            m_opponentCards[p].append(item);
        }
    }

    // Recreate trick cards if there's a current trick in progress
    const Cards& trick = m_game->currentTrick();
    const QVector<int>& trickPlayers = m_game->trickPlayers();
    for (int i = 0; i < trick.size(); ++i) {
        CardItem* item = createCardItem(trick[i]);
        item->setFaceUp(true);
        item->setInTrick(true);
        item->setZValue(200 + i);
        QPointF dest = trickCardPosition(trickPlayers[i]);
        item->setPos(dest);
        m_trickCards.append(item);
    }

    layoutCards();
    updatePlayableCards();
}

void GameView::updatePlayerHand() {
    // Remove old cards
    for (CardItem* item : m_playerCards) {
        m_scene->removeItem(item);
        delete item;
    }
    m_playerCards.clear();

    if (!m_game || !m_theme) return;

    // Create new cards
    const Cards& hand = m_game->player(0)->hand();
    for (const Card& card : hand) {
        CardItem* item = createCardItem(card);
        item->setFaceUp(true);
        item->setSelected(m_selectedPassCards.contains(card));
        m_playerCards.append(item);
    }

    layoutPlayerHand();
    updatePlayableCards();
}

void GameView::updateOpponentHands() {
    for (int p = 1; p < 4; ++p) {
        // Remove old
        for (CardItem* item : m_opponentCards[p]) {
            m_scene->removeItem(item);
            delete item;
        }
        m_opponentCards[p].clear();

        if (!m_game || !m_theme) continue;

        // Create new
        int numCards = m_game->player(p)->hand().size();
        for (int i = 0; i < numCards; ++i) {
            CardItem* item = createCardItem(Card(Suit::Clubs, Rank::Two));
            item->setFaceUp(false);
            m_opponentCards[p].append(item);
        }

        layoutOpponentHand(p);
    }
}

void GameView::updatePlayableCards() {
    if (!m_game) return;

    // Never unblock input while showing received cards
    if (m_showingReceivedCards) {
        m_inputBlocked = true;
        for (CardItem* item : m_playerCards) {
            item->setPlayable(false);
        }
        return;
    }

    GameState state = m_game->state();
    Cards valid;

    if (state == GameState::WaitingForPass) {
        // Check if passing is already confirmed (prevents race condition where
        // clearReceivedCardHighlight runs before startPlaying)
        if (m_passConfirmed) {
            m_inputBlocked = true;
            for (CardItem* item : m_playerCards) {
                item->setPlayable(false);
            }
            return;
        }
        // All cards playable for selection
        m_inputBlocked = false;
        for (CardItem* item : m_playerCards) {
            item->setPlayable(true);
        }
        return;
    }

    if (state == GameState::WaitingForPlay && m_game->currentPlayer() == 0) {
        valid = m_game->getValidPlays();
        m_inputBlocked = false;  // Unblock input when it's player's turn
    } else {
        // Block input during non-interactive states (AI turns, transitions, etc.)
        m_inputBlocked = true;
    }

    for (CardItem* item : m_playerCards) {
        item->setPlayable(valid.contains(item->card()));
    }
}

void GameView::layoutCards() {
    layoutPlayerHand();
    for (int p = 1; p < 4; ++p) {
        layoutOpponentHand(p);
    }
}

void GameView::layoutPlayerHand(bool animate) {
    if (m_playerCards.isEmpty()) return;

    QRectF rect = m_scene->sceneRect();
    qreal totalWidth = m_playerCards.size() * m_cardSpacing + m_cardWidth - m_cardSpacing;
    qreal startX = (rect.width() - totalWidth) / 2;
    qreal y = rect.height() - m_cardHeight - 30;

    for (int i = 0; i < m_playerCards.size(); ++i) {
        QPointF targetPos(startX + i * m_cardSpacing, y);
        m_playerCards[i]->setZValue(10 + i);

        if (animate && m_playerCards[i]->pos() != targetPos) {
            QPropertyAnimation* anim = new QPropertyAnimation(m_playerCards[i], "pos");
            anim->setDuration(150);
            anim->setEndValue(targetPos);
            anim->setEasingCurve(QEasingCurve::OutCubic);
            anim->start(QAbstractAnimation::DeleteWhenStopped);
        } else {
            m_playerCards[i]->setPos(targetPos);
        }
    }
}

void GameView::layoutOpponentHand(int player) {
    if (player < 1 || player > 3) return;
    const auto& cards = m_opponentCards[player];
    if (cards.isEmpty()) return;

    QRectF rect = m_scene->sceneRect();
    qreal spacing = m_cardSpacing * 0.6;

    if (player == 1) { // West (left side, vertical)
        qreal totalHeight = cards.size() * spacing + m_cardWidth - spacing;
        qreal x = 20;
        qreal startY = (rect.height() - totalHeight) / 2;

        for (int i = 0; i < cards.size(); ++i) {
            cards[i]->setRotation(90);
            cards[i]->setPos(x + m_cardHeight, startY + i * spacing);
            cards[i]->setZValue(10 + i);
        }
    } else if (player == 2) { // North (top, horizontal)
        qreal totalWidth = cards.size() * spacing + m_cardWidth - spacing;
        qreal startX = (rect.width() - totalWidth) / 2;
        qreal y = 20;

        for (int i = 0; i < cards.size(); ++i) {
            cards[i]->setRotation(180);
            cards[i]->setPos(startX + i * spacing + m_cardWidth, y + m_cardHeight);
            cards[i]->setZValue(10 + i);
        }
    } else { // East (right side, vertical)
        qreal totalHeight = cards.size() * spacing + m_cardWidth - spacing;
        qreal x = rect.width() - 20 - m_cardHeight;
        qreal startY = (rect.height() - totalHeight) / 2;

        for (int i = 0; i < cards.size(); ++i) {
            cards[i]->setRotation(-90);
            cards[i]->setPos(x, startY + i * spacing + m_cardWidth);
            cards[i]->setZValue(10 + i);
        }
    }
}

void GameView::layoutScoreboards() {
    QRectF rect = m_scene->sceneRect();

    QPointF positions[4] = {
        QPointF(rect.width() / 2 - 50, rect.height() - 120 - m_cardHeight), // South
        QPointF(100, rect.height() / 2 - 25), // West
        QPointF(rect.width() / 2 - 50, 90), // North
        QPointF(rect.width() - 200, rect.height() / 2 - 25) // East
    };

    for (int i = 0; i < 4; ++i) {
        auto& sb = m_scoreboards[i];
        sb.background->setPos(positions[i]);
        sb.nameText->setPos(positions[i] + QPointF(5, 3));
        sb.scoreText->setPos(positions[i] + QPointF(5, 22));
    }
}

void GameView::layoutTrickCards() {
    if (!m_game) return;

    // Reposition trick cards based on who played them
    const QVector<int>& trickPlayers = m_game->trickPlayers();
    for (int i = 0; i < m_trickCards.size() && i < trickPlayers.size(); ++i) {
        QPointF pos = trickCardPosition(trickPlayers[i]);
        m_trickCards[i]->setPos(pos);
    }
}

QPointF GameView::trickCardPosition(int player) const {
    QRectF rect = m_scene->sceneRect();
    qreal cx = rect.width() / 2;
    qreal cy = rect.height() / 2 - 30;
    qreal offset = m_cardHeight * 0.7;

    switch (player) {
        case 0: return QPointF(cx - m_cardWidth / 2, cy + offset / 2);
        case 1: return QPointF(cx - offset - m_cardWidth / 2, cy - m_cardHeight / 2);
        case 2: return QPointF(cx - m_cardWidth / 2, cy - offset - m_cardHeight / 2);
        case 3: return QPointF(cx + offset - m_cardWidth / 2, cy - m_cardHeight / 2);
    }
    return QPointF(cx, cy);
}

QPointF GameView::opponentHandCenter(int player) const {
    QRectF rect = m_scene->sceneRect();

    switch (player) {
        case 1: // West (left side)
            return QPointF(20 + m_cardHeight, rect.height() / 2);
        case 2: // North (top)
            return QPointF(rect.width() / 2, 20 + m_cardHeight);
        case 3: // East (right side)
            return QPointF(rect.width() - 20, rect.height() / 2);
    }
    return rect.center();
}

void GameView::onStateChanged(GameState state) {
    // Block input during all non-interactive states
    if (state != GameState::WaitingForPass && state != GameState::WaitingForPlay) {
        m_inputBlocked = true;
    }

    if (state == GameState::NotStarted || state == GameState::Dealing) {
        clearCards();
        m_overlay->setVisible(false);
        m_overlayText->setVisible(false);
        updateCurrentPlayerHighlight(-1);  // Clear highlight when not playing
    }
    updatePlayableCards();
}

void GameView::onCardsDealt() {
    m_selectedPassCards.clear();
    m_passConfirmed = false;

    // Cancel any pending highlight timer
    if (m_receivedHighlightTimer) {
        m_receivedHighlightTimer->stop();
        delete m_receivedHighlightTimer;
        m_receivedHighlightTimer = nullptr;
    }
    m_showingReceivedCards = false;
    m_inputBlocked = false;

    // Note: clearCards() already called by onStateChanged(Dealing)
    updateCards();
}

void GameView::onPassDirectionAnnounced(PassDirection dir) {
    m_currentPassDirection = dir;
    if (dir == PassDirection::None) {
        showMessage("No passing this round - Hold", 1500);
        return;
    }
    showPassArrow(dir);
    showMessage("Select 3 cards to pass", 0);
}

void GameView::onPassingComplete(Cards receivedCards) {
    // Cancel any pending highlight timer
    if (m_receivedHighlightTimer) {
        m_receivedHighlightTimer->stop();
        delete m_receivedHighlightTimer;
        m_receivedHighlightTimer = nullptr;
    }

    hidePassArrow();
    m_selectedPassCards.clear();

    // Block input during received card highlight period
    m_inputBlocked = true;
    m_showingReceivedCards = true;

    // Clear any old received cards first
    m_receivedCards.clear();

    // Only take up to 3 cards from the signal
    for (int i = 0; i < receivedCards.size() && i < 3; ++i) {
        m_receivedCards.append(receivedCards[i]);
    }

    updateCards();

    // First, clear ALL received flags to ensure clean state
    for (CardItem* item : m_playerCards) {
        item->setReceived(false);
        item->setSelected(false);  // Also clear any stale selection state
    }

    // Highlight exactly the received cards with golden glow - max 3
    // Also animate them pushing out momentarily
    QVector<CardItem*> receivedItems;
    int highlightCount = 0;
    for (int i = 0; i < m_playerCards.size() && highlightCount < 3; ++i) {
        CardItem* item = m_playerCards[i];
        for (const Card& rc : m_receivedCards) {
            if (item->card().suit() == rc.suit() && item->card().rank() == rc.rank()) {
                item->setReceived(true);
                receivedItems.append(item);
                highlightCount++;
                break;
            }
        }
    }

    // Animate received cards being thrown from the opponent's hand, then push up
    if (m_animatePassingCards && !receivedItems.isEmpty()) {
        // Determine which opponent we receive from based on pass direction
        // Pass Left: we pass to player 1 (West), receive from player 3 (East)
        // Pass Right: we pass to player 3 (East), receive from player 1 (West)
        // Pass Across: we pass to player 2 (North), receive from player 2 (North)
        int fromPlayer = 2;  // Default to North
        qreal startRotation = 180;  // North cards are rotated 180
        switch (m_currentPassDirection) {
            case PassDirection::Left:
                fromPlayer = 3;  // East
                startRotation = -90;
                break;
            case PassDirection::Right:
                fromPlayer = 1;  // West
                startRotation = 90;
                break;
            case PassDirection::Across:
                fromPlayer = 2;  // North
                startRotation = 180;
                break;
            default:
                break;
        }

        QPointF throwFrom = opponentHandCenter(fromPlayer);

        for (int i = 0; i < receivedItems.size(); ++i) {
            CardItem* item = receivedItems[i];
            QPointF handPos = item->pos();
            QPointF pushedPos = handPos - QPointF(0, 20);  // Push up 20 pixels

            // Start card at opponent's hand position, face down
            item->setPos(throwFrom);
            item->setRotation(startRotation);
            item->setFaceUp(false);

            // Alternate rotation for variety
            qreal endRotation = (i % 2 == 0) ? 5.0 : -5.0;

            // Phase 1: Throw from opponent to hand position
            QPropertyAnimation* throwAnim = new QPropertyAnimation(item, "pos");
            throwAnim->setDuration(250);
            throwAnim->setStartValue(throwFrom);
            throwAnim->setEndValue(handPos);
            throwAnim->setEasingCurve(QEasingCurve::OutCubic);

            QPropertyAnimation* rotateAnim = new QPropertyAnimation(item, "rotation");
            rotateAnim->setDuration(250);
            rotateAnim->setStartValue(startRotation);
            rotateAnim->setEndValue(0.0);
            rotateAnim->setEasingCurve(QEasingCurve::OutCubic);

            // Flip face up partway through
            QTimer::singleShot(125, item, [item]() {
                item->setFaceUp(true);
            });

            QParallelAnimationGroup* throwGroup = new QParallelAnimationGroup(this);
            throwGroup->addAnimation(throwAnim);
            throwGroup->addAnimation(rotateAnim);

            // Phase 2: Push up to highlight (after throw completes)
            QPropertyAnimation* pushAnim = new QPropertyAnimation(item, "pos");
            pushAnim->setDuration(150);
            pushAnim->setStartValue(handPos);
            pushAnim->setEndValue(pushedPos);
            pushAnim->setEasingCurve(QEasingCurve::OutCubic);

            QPropertyAnimation* wobbleAnim = new QPropertyAnimation(item, "rotation");
            wobbleAnim->setDuration(150);
            wobbleAnim->setStartValue(0.0);
            wobbleAnim->setEndValue(endRotation);
            wobbleAnim->setEasingCurve(QEasingCurve::OutCubic);

            QParallelAnimationGroup* pushGroup = new QParallelAnimationGroup(this);
            pushGroup->addAnimation(pushAnim);
            pushGroup->addAnimation(wobbleAnim);

            // Chain: throw first, then push
            QSequentialAnimationGroup* sequence = new QSequentialAnimationGroup(this);
            sequence->addAnimation(throwGroup);
            sequence->addAnimation(pushGroup);
            sequence->start(QAbstractAnimation::DeleteWhenStopped);
        }
    } else if (!receivedItems.isEmpty()) {
        // No animation - just push up
        for (int i = 0; i < receivedItems.size(); ++i) {
            CardItem* item = receivedItems[i];
            QPointF pos = item->pos();
            item->setPos(pos - QPointF(0, 20));
        }
    }

    // Show message about received cards
    showMessage("Cards received!", 1500);

    // Clear highlight after delay using a member timer
    m_receivedHighlightTimer = new QTimer(this);
    m_receivedHighlightTimer->setSingleShot(true);
    connect(m_receivedHighlightTimer, &QTimer::timeout, this, &GameView::clearReceivedCardHighlight);
    m_receivedHighlightTimer->start(1500);
}

void GameView::clearReceivedCardHighlight() {
    // Move received cards back into position (animated or instant)
    for (CardItem* item : m_playerCards) {
        if (item->isReceived()) {
            // Calculate the proper position in the hand layout
            int index = m_playerCards.indexOf(item);
            QRectF rect = m_scene->sceneRect();
            qreal totalWidth = m_playerCards.size() * m_cardSpacing + m_cardWidth - m_cardSpacing;
            qreal startX = (rect.width() - totalWidth) / 2;
            qreal y = rect.height() - m_cardHeight - 30;
            QPointF targetPos(startX + index * m_cardSpacing, y);

            if (m_animatePassingCards) {
                QPropertyAnimation* slideBack = new QPropertyAnimation(item, "pos");
                slideBack->setDuration(200);
                slideBack->setEndValue(targetPos);
                slideBack->setEasingCurve(QEasingCurve::OutCubic);

                QPropertyAnimation* rotateBack = new QPropertyAnimation(item, "rotation");
                rotateBack->setDuration(200);
                rotateBack->setEndValue(0.0);
                rotateBack->setEasingCurve(QEasingCurve::OutCubic);

                QParallelAnimationGroup* group = new QParallelAnimationGroup(this);
                group->addAnimation(slideBack);
                group->addAnimation(rotateBack);
                group->start(QAbstractAnimation::DeleteWhenStopped);
            } else {
                item->setPos(targetPos);
                item->setRotation(0);
            }
        }
        item->setReceived(false);
        item->setSelected(false);  // Ensure no stale selection state
    }
    m_receivedCards.clear();
    hideMessage();

    if (m_receivedHighlightTimer) {
        m_receivedHighlightTimer->deleteLater();
        m_receivedHighlightTimer = nullptr;
    }

    // Clear the received cards display flag and update playable cards
    m_showingReceivedCards = false;
    updatePlayableCards();
}

void GameView::onUndoPerformed() {
    // Clear trick cards
    for (CardItem* item : m_trickCards) {
        m_scene->removeItem(item);
        delete item;
    }
    m_trickCards.clear();

    // Rebuild the display from game state
    m_selectedPassCards.clear();
    updateCards();

    // Recreate trick cards from current game state
    if (m_game) {
        const Cards& trick = m_game->currentTrick();
        const QVector<int>& trickPlayers = m_game->trickPlayers();
        for (int i = 0; i < trick.size(); ++i) {
            CardItem* item = createCardItem(trick[i]);
            item->setFaceUp(true);
            item->setInTrick(true);
            item->setZValue(200 + i);
            QPointF dest = trickCardPosition(trickPlayers[i]);
            item->setPos(dest);
            m_trickCards.append(item);
        }
    }

    // Hide overlay and messages
    m_overlay->setVisible(false);
    m_overlayText->setVisible(false);
    hideMessage();
    hidePassArrow();

    // Update playable cards
    updatePlayableCards();

    // Show undo message
    showMessage("Move undone", 1000);
}

void GameView::onCardPlayed(int player, Card card) {
    // Remove from player's hand display
    if (player == 0) {
        for (int i = 0; i < m_playerCards.size(); ++i) {
            if (m_playerCards[i]->card() == card) {
                CardItem* item = m_playerCards.takeAt(i);
                QPointF startPos = item->pos();  // Remember hand position before any changes
                item->resetVisualState();  // Clear hover/selected/playable state
                item->setFaceUp(true);
                item->setInTrick(true);  // Mark as in trick (prevents dimming)
                item->setZValue(200 + m_trickCards.size());
                item->setOpacity(1.0);  // Ensure full opacity
                m_trickCards.append(item);

                // Animate from hand position to trick area
                QPointF dest = trickCardPosition(player);

                QPropertyAnimation* posAnim = new QPropertyAnimation(item, "pos");
                posAnim->setDuration(200);
                posAnim->setStartValue(startPos);
                posAnim->setEndValue(dest);

                if (m_animateCardRotation) {
                    qreal trickRotation = QRandomGenerator::global()->bounded(11) - 5;  // Random rotation -5 to +5 degrees
                    QPropertyAnimation* rotAnim = new QPropertyAnimation(item, "rotation");
                    rotAnim->setDuration(200);
                    rotAnim->setStartValue(0.0);
                    rotAnim->setEndValue(trickRotation);

                    QParallelAnimationGroup* group = new QParallelAnimationGroup(this);
                    group->addAnimation(posAnim);
                    group->addAnimation(rotAnim);
                    group->start(QAbstractAnimation::DeleteWhenStopped);
                } else {
                    item->setRotation(0);
                    posAnim->start(QAbstractAnimation::DeleteWhenStopped);
                }
                break;
            }
        }
        layoutPlayerHand(true);  // Animate remaining cards sliding together
    } else {
        // Remove one opponent card
        QPointF startPos;
        qreal startRotation = 0;

        if (!m_opponentCards[player].isEmpty()) {
            CardItem* old = m_opponentCards[player].takeLast();
            startPos = old->pos();
            startRotation = old->rotation();
            m_scene->removeItem(old);
            delete old;
        } else {
            // Fallback position if no cards left (shouldn't happen)
            startPos = opponentHandCenter(player);
            startRotation = (player == 1) ? 90 : (player == 2) ? 180 : -90;
        }

        QPointF dest = trickCardPosition(player);
        qreal endRotation = m_animateCardRotation ? (QRandomGenerator::global()->bounded(11) - 5) : 0.0;

        CardItem* item = createCardItem(card);
        item->setInTrick(true);
        item->setZValue(200 + m_trickCards.size());
        m_trickCards.append(item);

        if (m_animateAICards) {
            // Animate the played card from opponent's hand
            item->setFaceUp(false);  // Start face-down
            item->setPos(startPos);
            item->setRotation(startRotation);

            // Animate position from hand to trick area
            QPropertyAnimation* posAnim = new QPropertyAnimation(item, "pos");
            posAnim->setDuration(200);
            posAnim->setStartValue(startPos);
            posAnim->setEndValue(dest);
            posAnim->setEasingCurve(QEasingCurve::OutCubic);

            // Animate rotation
            QPropertyAnimation* rotAnim = new QPropertyAnimation(item, "rotation");
            rotAnim->setDuration(200);
            rotAnim->setStartValue(startRotation);
            rotAnim->setEndValue(endRotation);
            rotAnim->setEasingCurve(QEasingCurve::OutCubic);

            // Flip the card face-up partway through the animation
            QTimer::singleShot(100, item, [item]() {
                item->setFaceUp(true);
            });

            QParallelAnimationGroup* group = new QParallelAnimationGroup(this);
            group->addAnimation(posAnim);
            group->addAnimation(rotAnim);
            group->start(QAbstractAnimation::DeleteWhenStopped);
        } else {
            // No animation - place card directly
            item->setFaceUp(true);
            item->setPos(dest);
            item->setRotation(endRotation);
        }

        layoutOpponentHand(player);
    }

    updatePlayableCards();
}

void GameView::onTrickWon(int winner, int points) {
    QString name = m_game->player(winner)->name();
    showMessage(name + " wins trick" + (points > 0 ? QString(" (+%1)").arg(points) : ""), 1500);

    // Move trick cards to a separate list for cleanup to avoid race condition
    // where rapid clicking causes a new card to be added to m_trickCards
    // before the animation cleanup runs
    QVector<CardItem*> cardsToAnimate = m_trickCards;
    m_trickCards.clear();

    // Animate cards away
    QPointF dest = scoreboardPosition(winner);

    QParallelAnimationGroup* group = new QParallelAnimationGroup(this);

    for (CardItem* item : cardsToAnimate) {
        QPropertyAnimation* moveAnim = new QPropertyAnimation(item, "pos");
        moveAnim->setDuration(300);
        moveAnim->setEndValue(dest);
        group->addAnimation(moveAnim);

        QPropertyAnimation* opacityAnim = new QPropertyAnimation(item, "opacity");
        opacityAnim->setDuration(300);
        opacityAnim->setEndValue(0.0);
        group->addAnimation(opacityAnim);
    }

    connect(group, &QParallelAnimationGroup::finished, this, [cardsToAnimate, this]() {
        for (CardItem* item : cardsToAnimate) {
            m_scene->removeItem(item);
            delete item;
        }
        // Don't clear m_trickCards - it may already have new cards from rapid play
    });

    group->start(QAbstractAnimation::DeleteWhenStopped);
}

QPointF GameView::scoreboardPosition(int player) const {
    QRectF rect = m_scene->sceneRect();
    switch (player) {
        case 0: return QPointF(rect.width() / 2, rect.height() - 120 - m_cardHeight);
        case 1: return QPointF(150, rect.height() / 2);
        case 2: return QPointF(rect.width() / 2, 115);
        case 3: return QPointF(rect.width() - 150, rect.height() / 2);
    }
    return rect.center();
}

void GameView::onRoundEnded() {
    showMessage("Round complete!", 2000);
}

void GameView::onGameEnded(int winner) {
    showGameOver(winner);
}

void GameView::onScoresChanged() {
    updateScoreboards();
}

void GameView::onCurrentPlayerChanged(int player) {
    // Block input unless it's player's turn
    if (player != 0) {
        m_inputBlocked = true;
    }

    if (player == 0) {
        showMessage("Your turn", 1000);
    }

    // Highlight the current player's scoreboard
    updateCurrentPlayerHighlight(player);

    updatePlayableCards();
}

void GameView::updateCurrentPlayerHighlight(int currentPlayer) {
    for (int i = 0; i < 4; ++i) {
        if (i == currentPlayer) {
            // Active player - bright green glow border
            m_scoreboards[i].background->setPen(QPen(QColor(100, 255, 100), 2));
            m_scoreboards[i].background->setBrush(QColor(40, 60, 40, 220));
        } else {
            // Inactive player - normal dim border
            m_scoreboards[i].background->setPen(QPen(QColor(80, 80, 80), 1));
            m_scoreboards[i].background->setBrush(QColor(30, 30, 30, 200));
        }
    }
}

void GameView::onHeartsBroken() {
    showMessage("Hearts broken!", 1500);
}

void GameView::updateScoreboards() {
    if (!m_game) return;

    for (int i = 0; i < 4; ++i) {
        const Player* p = m_game->player(i);
        m_scoreboards[i].nameText->setPlainText(p->name());
        m_scoreboards[i].scoreText->setPlainText(QString::number(p->totalScore()));
    }
}

void GameView::onCardClicked(CardItem* item) {
    if (!m_game || m_inputBlocked) return;

    GameState state = m_game->state();

    if (state == GameState::WaitingForPass) {
        Card card = item->card();
        if (m_selectedPassCards.contains(card)) {
            m_selectedPassCards.removeOne(card);
            item->setSelected(false);
        } else if (m_selectedPassCards.size() < 3) {
            m_selectedPassCards.append(card);
            item->setSelected(true);
        }

        // Auto-confirm when 3 cards selected
        if (m_selectedPassCards.size() == 3) {
            // Block all further input
            m_inputBlocked = true;
            m_passConfirmed = true;
            for (CardItem* c : m_playerCards) {
                c->setPlayable(false);
            }
            QTimer::singleShot(400, this, [this]() {
                hideMessage();
                // Store the cards to pass before clearing selection state
                Cards cardsToPass = m_selectedPassCards;
                // Clear visual selection state immediately
                for (CardItem* c : m_playerCards) {
                    c->setSelected(false);
                }
                m_game->humanPassCards(cardsToPass);
                // Note: m_inputBlocked is handled by updatePlayableCards() called from onPassingComplete
            });
        }
    } else if (state == GameState::WaitingForPlay) {
        if (item->isPlayable()) {
            // Block all further input immediately
            m_inputBlocked = true;
            for (CardItem* c : m_playerCards) {
                c->setPlayable(false);
            }
            m_game->humanPlayCard(item->card());
            // Input will be unblocked when updatePlayableCards is called
        }
    }
}

void GameView::showMessage(const QString& text, int durationMs) {
    m_messageText->setPlainText(text);
    m_messageText->setVisible(true);
    m_messageBackground->setVisible(true);

    QRectF rect = m_scene->sceneRect();
    qreal msgWidth = m_messageText->boundingRect().width();
    qreal msgHeight = m_messageText->boundingRect().height();

    m_messageBackground->setRect(
        (rect.width() - msgWidth - 30) / 2,
        rect.height() * 0.4 - 5,
        msgWidth + 30,
        msgHeight + 10
    );
    m_messageText->setPos((rect.width() - msgWidth) / 2, rect.height() * 0.4);

    if (durationMs > 0) {
        QTimer::singleShot(durationMs, this, &GameView::hideMessage);
    }
}

void GameView::hideMessage() {
    m_messageText->setVisible(false);
    m_messageBackground->setVisible(false);
}

void GameView::showPassArrow(PassDirection dir) {
    QString arrow;
    switch (dir) {
        case PassDirection::Left:   arrow = "\u2190"; break; // ←
        case PassDirection::Right:  arrow = "\u2192"; break; // →
        case PassDirection::Across: arrow = "\u2191"; break; // ↑
        case PassDirection::None:   return; // No arrow for hold round
    }

    m_passArrow->setPlainText(arrow);
    m_passArrow->setVisible(true);

    QRectF rect = m_scene->sceneRect();
    qreal w = m_passArrow->boundingRect().width();
    m_passArrow->setPos((rect.width() - w) / 2, rect.height() * 0.3);
}

void GameView::hidePassArrow() {
    m_passArrow->setVisible(false);
}

void GameView::showGameOver(int winner) {
    m_overlay->setVisible(true);
    m_overlayText->setVisible(true);

    QString text;
    QColor color;
    if (winner == 0) {
        text = "You Win!";
        color = QColor(100, 255, 100);
    } else {
        text = m_game->player(winner)->name() + " Wins";
        color = QColor(255, 100, 100);
    }

    m_overlayText->setPlainText(text);
    m_overlayText->setDefaultTextColor(color);

    QRectF rect = m_scene->sceneRect();
    qreal tw = m_overlayText->boundingRect().width();
    qreal th = m_overlayText->boundingRect().height();
    m_overlayText->setPos((rect.width() - tw) / 2, (rect.height() - th) / 2);
}

void GameView::keyPressEvent(QKeyEvent* event) {
    if (!m_game || m_inputBlocked) {
        QGraphicsView::keyPressEvent(event);
        return;
    }

    GameState state = m_game->state();
    if (state != GameState::WaitingForPass && state != GameState::WaitingForPlay) {
        QGraphicsView::keyPressEvent(event);
        return;
    }

    if (m_playerCards.isEmpty()) {
        QGraphicsView::keyPressEvent(event);
        return;
    }

    switch (event->key()) {
        case Qt::Key_Left:
            if (m_keyboardFocusIndex <= 0) {
                m_keyboardFocusIndex = m_playerCards.size() - 1;
            } else {
                m_keyboardFocusIndex--;
            }
            updateKeyboardFocus();
            event->accept();
            break;

        case Qt::Key_Right:
            if (m_keyboardFocusIndex >= m_playerCards.size() - 1) {
                m_keyboardFocusIndex = 0;
            } else {
                m_keyboardFocusIndex++;
            }
            updateKeyboardFocus();
            event->accept();
            break;

        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Space:
            if (m_keyboardFocusIndex >= 0 && m_keyboardFocusIndex < m_playerCards.size()) {
                selectFocusedCard();
                event->accept();
            }
            break;

        case Qt::Key_Home:
            m_keyboardFocusIndex = 0;
            updateKeyboardFocus();
            event->accept();
            break;

        case Qt::Key_End:
            m_keyboardFocusIndex = m_playerCards.size() - 1;
            updateKeyboardFocus();
            event->accept();
            break;

        default:
            QGraphicsView::keyPressEvent(event);
            break;
    }
}

void GameView::updateKeyboardFocus() {
    for (int i = 0; i < m_playerCards.size(); ++i) {
        m_playerCards[i]->setKeyboardFocused(i == m_keyboardFocusIndex);
    }
}

void GameView::selectFocusedCard() {
    if (m_keyboardFocusIndex < 0 || m_keyboardFocusIndex >= m_playerCards.size()) {
        return;
    }

    CardItem* item = m_playerCards[m_keyboardFocusIndex];

    // Trigger the same logic as mouse click
    onCardClicked(item);
}
