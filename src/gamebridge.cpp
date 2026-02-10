#include "gamebridge.h"
#include <QTimer>
#include <QDebug>
#include <QSettings>
#include <QCoreApplication>

GameBridge::GameBridge(QObject* parent)
    : QObject(parent)
    , m_game(new Game(this))
    , m_theme(new CardTheme())
    , m_previewTheme(new CardTheme())
    , m_sound(new SoundEngine(this))
{
    connect(m_game, &Game::stateChanged, this, &GameBridge::onStateChanged);
    connect(m_game, &Game::cardsDealt, this, &GameBridge::onCardsDealt);
    connect(m_game, &Game::passDirectionAnnounced, this, &GameBridge::onPassDirectionAnnounced);
    connect(m_game, &Game::passingComplete, this, &GameBridge::onPassingComplete);
    connect(m_game, &Game::cardPlayed, this, &GameBridge::onCardPlayed);
    connect(m_game, &Game::trickWon, this, &GameBridge::onTrickWon);
    connect(m_game, &Game::roundEnded, this, &GameBridge::onRoundEnded);
    connect(m_game, &Game::gameEnded, this, &GameBridge::onGameEnded);
    connect(m_game, &Game::scoresChanged, this, &GameBridge::onScoresChanged);
    connect(m_game, &Game::currentPlayerChanged, this, &GameBridge::onCurrentPlayerChanged);
    connect(m_game, &Game::heartsBrokenSignal, this, &GameBridge::onHeartsBroken);
    connect(m_game, &Game::shootTheMoonOccurred, this, [this](int shooter) {
        QString name = m_game->player(shooter)->name();
        showMessage(name + " shot the moon!", 2500);
        m_shootTheMoonCount++;
        emit statisticsChanged();
    });
    connect(m_game, &Game::undoAvailableChanged, this, [this](bool available) {
        m_undoAvailable = available;
        emit undoAvailableChanged();
    });

    connect(m_game, &Game::cardsDealt, m_sound, &SoundEngine::playCardShuffle);
    connect(m_game, &Game::cardPlayed, m_sound, &SoundEngine::playCardPutDown);
    connect(m_game, &Game::gameEnded, this, [this](int winner) {
        if (winner == 0) {
            m_sound->playWin();
        } else {
            m_sound->playLose();
        }
    });

    loadSettings();
}

GameBridge::~GameBridge() {
    saveSettings();
    delete m_theme;
    delete m_previewTheme;
}

void GameBridge::newGame() {
    m_selectedCards.clear();
    m_receivedCards.clear();
    m_gameOver = false;
    m_winner = -1;
    m_inputBlocked = false;
    m_showingReceivedCards = false;
    m_passConfirmed = false;

    emit gameOverChanged();
    emit winnerChanged();
    emit inputBlockedChanged();
    emit selectedCountChanged();

    m_game->newGame();
}

QVariantList GameBridge::playerHand() const {
    QVariantList result;
    if (!m_game) return result;

    const Cards& hand = m_game->player(0)->hand();
    for (int i = 0; i < hand.size(); ++i) {
        const Card& card = hand[i];
        QVariantMap cardMap;
        cardMap["suit"] = static_cast<int>(card.suit());
        cardMap["rank"] = static_cast<int>(card.rank());
        cardMap["elementId"] = card.elementId();
        cardMap["playable"] = m_validPlays.contains(card);
        cardMap["selected"] = m_selectedCards.contains(card);
        cardMap["received"] = m_receivedCards.contains(card);
        cardMap["index"] = i;
        result.append(cardMap);
    }
    return result;
}

QVariantList GameBridge::opponentCardCounts() const {
    QVariantList result;
    if (!m_game) return result;

    for (int p = 1; p <= 3; ++p) {
        result.append(m_game->player(p)->hand().size());
    }
    return result;
}

QVariantList GameBridge::trickCards() const {
    QVariantList result;
    if (!m_game) return result;

    const Cards& trick = m_game->currentTrick();
    const QVector<int>& trickPlayers = m_game->trickPlayers();

    for (int i = 0; i < trick.size(); ++i) {
        const Card& card = trick[i];
        QVariantMap cardMap;
        cardMap["player"] = trickPlayers[i];
        cardMap["suit"] = static_cast<int>(card.suit());
        cardMap["rank"] = static_cast<int>(card.rank());
        cardMap["elementId"] = card.elementId();
        result.append(cardMap);
    }
    return result;
}

QVariantList GameBridge::players() const {
    QVariantList result;
    if (!m_game) return result;

    for (int i = 0; i < 4; ++i) {
        const Player* p = m_game->player(i);
        QVariantMap playerMap;
        playerMap["name"] = p->name();
        playerMap["score"] = p->totalScore();
        playerMap["isCurrentPlayer"] = (i == m_currentPlayer);
        result.append(playerMap);
    }
    return result;
}

int GameBridge::passDirection() const {
    return static_cast<int>(m_passDirection);
}

int GameBridge::gameState() const {
    if (!m_game) return 0;
    return static_cast<int>(m_game->state());
}

QString GameBridge::themePath() const {
    return m_theme->themePath();
}

void GameBridge::setThemePath(const QString& path) {
    if (path.isEmpty()) {
        m_theme->loadBuiltinTheme();
    } else {
        m_theme->loadTheme(path);
    }
    m_themeVersion++;
    emit themeVersionChanged();
    emit themePathChanged();
    emit playerHandChanged();
    emit trickCardsChanged();
    emit opponentCardCountsChanged();
    saveSettings();
}

void GameBridge::setCardScale(qreal scale) {
    if (qFuzzyCompare(m_cardScale, scale)) return;
    m_cardScale = qBound(0.5, scale, 2.0);
    emit cardScaleChanged();
    saveSettings();
}

void GameBridge::setSoundEnabled(bool enabled) {
    if (m_soundEnabled == enabled) return;
    m_soundEnabled = enabled;
    m_sound->setEnabled(enabled);
    emit soundEnabledChanged();
    saveSettings();
}

int GameBridge::aiDifficulty() const {
    return static_cast<int>(m_game->aiDifficulty());
}

void GameBridge::setAIDifficulty(int difficulty) {
    m_game->setAIDifficulty(static_cast<AIDifficulty>(difficulty));
    emit aiDifficultyChanged();
    saveSettings();
}

QVariantList GameBridge::availableThemes() const {
    QVariantList result;
    QVector<ThemeInfo> themes = CardTheme::findThemes();
    for (const auto& theme : themes) {
        QVariantMap themeMap;
        themeMap["name"] = theme.name;
        themeMap["path"] = theme.path;
        result.append(themeMap);
    }
    return result;
}

void GameBridge::setAnimateCardRotation(bool v) {
    if (m_animateCardRotation == v) return;
    m_animateCardRotation = v;
    emit animateCardRotationChanged();
    saveSettings();
}

void GameBridge::setAnimateAICards(bool v) {
    if (m_animateAICards == v) return;
    m_animateAICards = v;
    emit animateAICardsChanged();
    saveSettings();
}

void GameBridge::setAnimatePassingCards(bool v) {
    if (m_animatePassingCards == v) return;
    m_animatePassingCards = v;
    emit animatePassingCardsChanged();
    saveSettings();
}

int GameBridge::endScore() const {
    return m_game->rules().endScore;
}

void GameBridge::setEndScore(int score) {
    GameRules rules = m_game->rules();
    rules.endScore = score;
    m_game->setRules(rules);
    emit endScoreChanged();
    saveSettings();
}

bool GameBridge::exactResetTo50() const {
    return m_game->rules().exactResetTo50;
}

void GameBridge::setExactResetTo50(bool v) {
    GameRules rules = m_game->rules();
    rules.exactResetTo50 = v;
    m_game->setRules(rules);
    emit exactResetTo50Changed();
    saveSettings();
}

bool GameBridge::queenBreaksHearts() const {
    return m_game->rules().queenBreaksHearts;
}

void GameBridge::setQueenBreaksHearts(bool v) {
    GameRules rules = m_game->rules();
    rules.queenBreaksHearts = v;
    m_game->setRules(rules);
    emit queenBreaksHeartsChanged();
    saveSettings();
}

bool GameBridge::moonProtection() const {
    return m_game->rules().moonProtection;
}

void GameBridge::setMoonProtection(bool v) {
    GameRules rules = m_game->rules();
    rules.moonProtection = v;
    m_game->setRules(rules);
    emit moonProtectionChanged();
    saveSettings();
}

bool GameBridge::fullPolish() const {
    return m_game->rules().fullPolish;
}

void GameBridge::setFullPolish(bool v) {
    GameRules rules = m_game->rules();
    rules.fullPolish = v;
    m_game->setRules(rules);
    emit fullPolishChanged();
    saveSettings();
}

void GameBridge::cardClicked(int suit, int rank) {
    if (m_inputBlocked || !m_game) return;

    Card card(static_cast<Suit>(suit), static_cast<Rank>(rank));
    GameState state = m_game->state();

    if (state == GameState::WaitingForPass) {
        if (m_passConfirmed) return;

        if (m_selectedCards.contains(card)) {
            m_selectedCards.removeOne(card);
            emit selectedCountChanged();
            emit playerHandChanged();
        } else if (m_selectedCards.size() < 3) {
            m_selectedCards.append(card);
            emit selectedCountChanged();
            emit playerHandChanged();

            // Auto-confirm when 3 cards selected
            if (m_selectedCards.size() == 3) {
                m_inputBlocked = true;
                m_passConfirmed = true;
                emit inputBlockedChanged();

                QTimer::singleShot(400, this, [this]() {
                    hideMessage();
                    Cards toPass = m_selectedCards;
                    m_selectedCards.clear();
                    emit selectedCountChanged();
                    emit playerHandChanged();
                    m_game->humanPassCards(toPass);
                });
            }
        }
    } else if (state == GameState::WaitingForPlay) {
        if (m_validPlays.contains(card)) {
            m_inputBlocked = true;
            emit inputBlockedChanged();
            m_game->humanPlayCard(card);
        }
    }
}

QString GameBridge::cardImageSource(int suit, int rank) const {
    return QString("image://cards/%1_%2").arg(suit).arg(rank);
}

QString GameBridge::cardBackSource() const {
    return "image://cards/back";
}

QVariantList GameBridge::getValidPlays() const {
    QVariantList result;
    for (const Card& card : m_validPlays) {
        QVariantMap cardMap;
        cardMap["suit"] = static_cast<int>(card.suit());
        cardMap["rank"] = static_cast<int>(card.rank());
        result.append(cardMap);
    }
    return result;
}

void GameBridge::undo() {
    if (m_game && m_undoAvailable) {
        m_game->undo();
        emit playerHandChanged();
        emit trickCardsChanged();
        emit opponentCardCountsChanged();
        updateValidPlays();
    }
}

void GameBridge::quit() {
    saveSettings();
    QCoreApplication::quit();
}

void GameBridge::resetStatistics() {
    m_gamesPlayed = 0;
    m_gamesWon = 0;
    m_totalScore = 0;
    m_bestScore = 999;
    m_shootTheMoonCount = 0;
    emit statisticsChanged();
    saveSettings();
}

QString GameBridge::scoresText() const {
    if (!m_game) return "";

    QString text;
    for (int i = 0; i < 4; ++i) {
        const Player* p = m_game->player(i);
        text += QString("%1: %2\n").arg(p->name()).arg(p->totalScore());
    }
    return text;
}

void GameBridge::loadPreviewTheme(const QString& path) {
    if (path.isEmpty()) {
        m_previewTheme->loadBuiltinTheme();
    } else {
        m_previewTheme->loadTheme(path);
    }
    m_previewVersion++;
    emit previewVersionChanged();
}

void GameBridge::setShowMenuBar(bool v) {
    if (m_showMenuBar == v) return;
    m_showMenuBar = v;
    emit showMenuBarChanged();
    saveSettings();
}

void GameBridge::updateValidPlays() {
    m_validPlays.clear();
    if (!m_game) return;

    GameState state = m_game->state();
    if (state == GameState::WaitingForPass && !m_passConfirmed) {
        m_validPlays = m_game->player(0)->hand();
    } else if (state == GameState::WaitingForPlay && m_game->currentPlayer() == 0) {
        if (!m_showingReceivedCards) {
            m_validPlays = m_game->getValidPlays();
            m_inputBlocked = false;
            emit inputBlockedChanged();
        }
    } else {
        m_inputBlocked = true;
        emit inputBlockedChanged();
    }

    emit playerHandChanged();
}

void GameBridge::showMessage(const QString& text, int durationMs) {
    m_message = text;
    emit messageChanged();

    if (durationMs > 0) {
        if (m_messageTimer) {
            m_messageTimer->stop();
            m_messageTimer->deleteLater();
        }
        m_messageTimer = new QTimer(this);
        m_messageTimer->setSingleShot(true);
        connect(m_messageTimer, &QTimer::timeout, this, &GameBridge::hideMessage);
        m_messageTimer->start(durationMs);
    }
}

void GameBridge::hideMessage() {
    m_message.clear();
    emit messageChanged();
    if (m_messageTimer) {
        m_messageTimer->deleteLater();
        m_messageTimer = nullptr;
    }
}

void GameBridge::onStateChanged(GameState state) {
    emit gameStateChanged();

    if (state == GameState::WaitingForPass || state == GameState::WaitingForPlay) {
        m_inputBlocked = m_showingReceivedCards;
    } else {
        m_inputBlocked = true;
    }
    emit inputBlockedChanged();

    updateValidPlays();
}

void GameBridge::onCardsDealt() {
    m_selectedCards.clear();
    m_receivedCards.clear();
    m_passConfirmed = false;
    m_showingReceivedCards = false;
    m_inputBlocked = false;

    emit selectedCountChanged();
    emit inputBlockedChanged();
    emit playerHandChanged();
    emit opponentCardCountsChanged();
    emit trickCardsChanged();

    updateValidPlays();
}

void GameBridge::onPassDirectionAnnounced(PassDirection dir) {
    m_passDirection = dir;
    emit passDirectionChanged();

    if (dir == PassDirection::None) {
        showMessage("No passing this round - Hold", 1500);
    } else {
        showMessage("Select 3 cards to pass", 0);
    }
}

void GameBridge::onPassingComplete(Cards receivedCards) {
    hideMessage();
    m_selectedCards.clear();
    m_receivedCards = receivedCards;
    m_validPlays.clear();
    m_inputBlocked = true;
    m_showingReceivedCards = true;

    emit selectedCountChanged();
    emit inputBlockedChanged();
    emit playerHandChanged();

    QVariantList cardsList;
    for (const Card& card : receivedCards) {
        QVariantMap cardMap;
        cardMap["suit"] = static_cast<int>(card.suit());
        cardMap["rank"] = static_cast<int>(card.rank());
        cardMap["elementId"] = card.elementId();
        cardsList.append(cardMap);
    }
    emit cardsReceived(cardsList);

    showMessage("Cards received!", 1500);

    QTimer::singleShot(1500, this, [this]() {
        m_receivedCards.clear();
        m_showingReceivedCards = false;
        m_inputBlocked = false;
        emit inputBlockedChanged();
        emit playerHandChanged();
        updateValidPlays();
    });
}

void GameBridge::onCardPlayed(int player, Card card) {
    // Signal before model change so TrickArea's expectedCardCount prevents duplicates
    emit cardPlayedToTrick(player, static_cast<int>(card.suit()), static_cast<int>(card.rank()), 0, 0);

    emit trickCardsChanged();
    emit playerHandChanged();
    emit opponentCardCountsChanged();
    updateValidPlays();
}

void GameBridge::onTrickWon(int winner, int points) {
    QString name = m_game->player(winner)->name();
    showMessage(name + " wins trick" + (points > 0 ? QString(" (+%1)").arg(points) : ""), 1500);

    emit trickWonByPlayer(winner, points);

    QTimer::singleShot(800, this, [this]() {
        emit trickCardsChanged();
    });
}

void GameBridge::onRoundEnded() {
    showMessage("Round complete!", 2000);
}

void GameBridge::onGameEnded(int winner) {
    m_gameOver = true;
    m_winner = winner;
    emit gameOverChanged();
    emit winnerChanged();

    // Update statistics
    m_gamesPlayed++;
    int myScore = m_game->player(0)->totalScore();
    m_totalScore += myScore;
    if (myScore < m_bestScore) {
        m_bestScore = myScore;
    }
    if (winner == 0) {
        m_gamesWon++;
    }
    emit statisticsChanged();
    saveSettings();
}

void GameBridge::onScoresChanged() {
    emit playersChanged();
}

void GameBridge::onCurrentPlayerChanged(int player) {
    m_currentPlayer = player;
    emit playersChanged();

    if (player == 0) {
        showMessage("Your turn", 1000);
    }

    updateValidPlays();
}

void GameBridge::onHeartsBroken() {
    showMessage("Hearts broken!", 1500);
    emit heartsBrokenSignal();
}

void GameBridge::loadSettings() {
    QSettings settings("Hearts", "Hearts");

    // Theme
    QString savedTheme = settings.value("theme", "").toString();
    if (!savedTheme.isEmpty() && m_theme->loadTheme(savedTheme)) {
        // OK
    } else {
        QVector<ThemeInfo> themes = CardTheme::findThemes();
        bool themeLoaded = false;
        for (const auto& themeInfo : themes) {
            if (m_theme->loadTheme(themeInfo.path)) {
                themeLoaded = true;
                break;
            }
        }
        if (!themeLoaded) {
            m_theme->loadBuiltinTheme();
        }
    }

    // Card scale
    m_cardScale = settings.value("cardScale", 1.0).toDouble();

    // Sound
    m_soundEnabled = settings.value("soundEnabled", true).toBool();
    m_sound->setEnabled(m_soundEnabled);

    // AI Difficulty
    int difficulty = settings.value("aiDifficulty", static_cast<int>(AIDifficulty::Medium)).toInt();
    m_game->setAIDifficulty(static_cast<AIDifficulty>(difficulty));

    // Game rules
    GameRules rules;
    rules.endScore = settings.value("rules/endScore", 100).toInt();
    rules.exactResetTo50 = settings.value("rules/exactResetTo50", false).toBool();
    rules.queenBreaksHearts = settings.value("rules/queenBreaksHearts", true).toBool();
    rules.moonProtection = settings.value("rules/moonProtection", false).toBool();
    rules.fullPolish = settings.value("rules/fullPolish", false).toBool();
    m_game->setRules(rules);

    // Animation settings
    m_animateCardRotation = settings.value("animations/cardRotation", true).toBool();
    m_animateAICards = settings.value("animations/aiCards", true).toBool();
    m_animatePassingCards = settings.value("animations/passingCards", true).toBool();

    // UI
    m_showMenuBar = settings.value("ui/showMenuBar", true).toBool();

    // Statistics
    m_gamesPlayed = settings.value("stats/gamesPlayed", 0).toInt();
    m_gamesWon = settings.value("stats/gamesWon", 0).toInt();
    m_totalScore = settings.value("stats/totalScore", 0).toInt();
    m_bestScore = settings.value("stats/bestScore", 999).toInt();
    m_shootTheMoonCount = settings.value("stats/shootTheMoon", 0).toInt();
}

void GameBridge::saveSettings() {
    QSettings settings("Hearts", "Hearts");
    settings.setValue("theme", m_theme->themePath());
    settings.setValue("cardScale", m_cardScale);
    settings.setValue("soundEnabled", m_soundEnabled);
    settings.setValue("aiDifficulty", static_cast<int>(m_game->aiDifficulty()));

    // Game rules
    const GameRules& rules = m_game->rules();
    settings.setValue("rules/endScore", rules.endScore);
    settings.setValue("rules/exactResetTo50", rules.exactResetTo50);
    settings.setValue("rules/queenBreaksHearts", rules.queenBreaksHearts);
    settings.setValue("rules/moonProtection", rules.moonProtection);
    settings.setValue("rules/fullPolish", rules.fullPolish);

    // Animation settings
    settings.setValue("animations/cardRotation", m_animateCardRotation);
    settings.setValue("animations/aiCards", m_animateAICards);
    settings.setValue("animations/passingCards", m_animatePassingCards);

    // UI
    settings.setValue("ui/showMenuBar", m_showMenuBar);

    // Statistics
    settings.setValue("stats/gamesPlayed", m_gamesPlayed);
    settings.setValue("stats/gamesWon", m_gamesWon);
    settings.setValue("stats/totalScore", m_totalScore);
    settings.setValue("stats/bestScore", m_bestScore);
    settings.setValue("stats/shootTheMoon", m_shootTheMoonCount);
}

