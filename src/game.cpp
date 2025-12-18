#include "game.h"
#include <QTimer>

Game::Game(QObject* parent)
    : QObject(parent)
    , m_state(GameState::NotStarted)
    , m_roundNumber(0)
    , m_passDirection(PassDirection::Left)
    , m_currentPlayer(0)
    , m_heartsBroken(false)
    , m_isFirstTrick(true)
    , m_leadSuit(Suit::Clubs)
{
    // Create players: human + 3 AI
    m_players[0] = std::make_unique<Player>(0, "You", true);
    m_players[1] = std::make_unique<Player>(1, "West", false);
    m_players[2] = std::make_unique<Player>(2, "North", false);
    m_players[3] = std::make_unique<Player>(3, "East", false);

    m_passedCards.resize(NUM_PLAYERS);
}

void Game::setPlayerName(int index, const QString& name) {
    if (index >= 0 && index < NUM_PLAYERS) {
        // Can't change name of existing player, but we track it
    }
}

void Game::setAIDifficulty(AIDifficulty difficulty) {
    for (int i = 1; i < NUM_PLAYERS; ++i) {
        m_players[i]->setDifficulty(difficulty);
    }
}

AIDifficulty Game::aiDifficulty() const {
    // All AI players have the same difficulty, so just return the first one's
    return m_players[1]->difficulty();
}

Player* Game::player(int index) {
    if (index >= 0 && index < NUM_PLAYERS) {
        return m_players[index].get();
    }
    return nullptr;
}

const Player* Game::player(int index) const {
    if (index >= 0 && index < NUM_PLAYERS) {
        return m_players[index].get();
    }
    return nullptr;
}

void Game::setState(GameState state) {
    m_state = state;
    emit stateChanged(state);
}

void Game::newGame() {
    m_roundNumber = 0;
    m_undoHistory.clear();

    for (auto& p : m_players) {
        p->resetScores();
    }

    emit scoresChanged();
    emit undoAvailableChanged(false);
    dealCards();
}

void Game::dealCards() {
    setState(GameState::Dealing);
    m_roundNumber++;

    // Determine pass direction (Left, Right, Across cycle)
    switch ((m_roundNumber - 1) % 3) {
        case 0: m_passDirection = PassDirection::Left; break;
        case 1: m_passDirection = PassDirection::Right; break;
        case 2: m_passDirection = PassDirection::Across; break;
    }

    // Deal cards
    Deck deck;
    QVector<Cards> hands = deck.dealAll(NUM_PLAYERS);

    for (int i = 0; i < NUM_PLAYERS; ++i) {
        m_players[i]->setHand(hands[i]);
    }

    emit cardsDealt();

    // Start passing
    QTimer::singleShot(500, this, &Game::startPassing);
}

void Game::startPassing() {
    setState(GameState::Passing);
    emit passDirectionAnnounced(m_passDirection);

    // Clear passed cards
    for (int i = 0; i < NUM_PLAYERS; ++i) {
        m_passedCards[i].clear();
    }

    // AI players select their pass cards immediately
    for (int i = 1; i < NUM_PLAYERS; ++i) {
        m_passedCards[i] = m_players[i]->selectPassCards();
    }

    // Wait for human
    setState(GameState::WaitingForPass);
}

Cards Game::getValidPassCards() const {
    // All cards in hand are valid for passing
    return m_players[0]->hand();
}

void Game::humanPassCards(const Cards& cards) {
    if (m_state != GameState::WaitingForPass) return;
    if (cards.size() != CARDS_TO_PASS) return;

    m_passedCards[0] = cards;
    executePassing();
}

void Game::executePassing() {
    // Determine who passes to whom
    auto getTarget = [this](int from) -> int {
        switch (m_passDirection) {
            case PassDirection::Left:   return (from + 1) % NUM_PLAYERS;
            case PassDirection::Right:  return (from + 3) % NUM_PLAYERS;
            case PassDirection::Across: return (from + 2) % NUM_PLAYERS;
        }
        return (from + 1) % NUM_PLAYERS;
    };

    // Collect cards to give to each player
    QVector<Cards> receiving(NUM_PLAYERS);
    for (int i = 0; i < NUM_PLAYERS; ++i) {
        int target = getTarget(i);
        receiving[target] = m_passedCards[i];
    }

    // Store cards received by human player for display
    Cards humanReceivedCards = receiving[0];

    // Remove passed cards, add received cards
    for (int i = 0; i < NUM_PLAYERS; ++i) {
        m_players[i]->removeCards(m_passedCards[i]);
        m_players[i]->addCards(receiving[i]);
    }

    emit passingComplete(humanReceivedCards);

    // Start playing (with longer delay to allow user to see received cards)
    QTimer::singleShot(1500, this, &Game::startPlaying);
}

void Game::startPlaying() {
    setState(GameState::Playing);

    m_heartsBroken = false;
    m_isFirstTrick = true;
    m_currentTrick.clear();
    m_trickPlayers.clear();

    // Find who has 2 of clubs
    m_currentPlayer = findTwoOfClubsPlayer();
    m_leadSuit = Suit::Clubs;

    emit currentPlayerChanged(m_currentPlayer);

    if (m_players[m_currentPlayer]->isHuman()) {
        setState(GameState::WaitingForPlay);
    } else {
        QTimer::singleShot(500, this, &Game::aiTurn);
    }
}

int Game::findTwoOfClubsPlayer() const {
    Card twoClubs(Suit::Clubs, Rank::Two);
    for (int i = 0; i < NUM_PLAYERS; ++i) {
        if (m_players[i]->hasCard(twoClubs)) {
            return i;
        }
    }
    return 0;
}

Cards Game::getValidPlays() const {
    if (m_currentPlayer != 0) return Cards();

    const Player* human = m_players[0].get();

    if (m_currentTrick.isEmpty()) {
        // Leading
        if (m_isFirstTrick) {
            // Must lead 2 of clubs
            return Cards{Card(Suit::Clubs, Rank::Two)};
        }

        // Can lead anything except hearts (unless broken or only have hearts)
        if (!m_heartsBroken && !hasOnlyHearts(human->hand())) {
            Cards valid;
            for (const Card& c : human->hand()) {
                if (!c.isHeart()) valid.append(c);
            }
            return valid;
        }

        return human->hand();
    }

    // Following
    return human->getValidPlays(m_leadSuit, m_isFirstTrick, m_heartsBroken);
}

void Game::humanPlayCard(const Card& card) {
    if (m_state != GameState::WaitingForPlay) return;
    if (m_currentPlayer != 0) return;

    Cards valid = getValidPlays();
    if (!valid.contains(card)) return;

    // Save state before human plays
    saveSnapshot();

    // Immediately change state to prevent double-play
    setState(GameState::Playing);

    // Play the card
    m_players[0]->removeCard(card);

    if (m_currentTrick.isEmpty()) {
        m_leadSuit = card.suit();
    }

    m_currentTrick.append(card);
    m_trickPlayers.append(0);

    if (card.isHeart() && !m_heartsBroken) {
        m_heartsBroken = true;
        emit heartsBrokenSignal();
    }

    emit cardPlayed(0, card);

    nextTurn();
}

void Game::aiTurn() {
    if (m_currentPlayer == 0) return; // Not AI's turn

    Player* ai = m_players[m_currentPlayer].get();

    Suit leadSuit = m_currentTrick.isEmpty() ? Suit::Clubs : m_leadSuit;
    Card card = ai->selectPlay(leadSuit, m_isFirstTrick, m_heartsBroken, m_currentTrick);

    ai->removeCard(card);

    if (m_currentTrick.isEmpty()) {
        m_leadSuit = card.suit();
    }

    m_currentTrick.append(card);
    m_trickPlayers.append(m_currentPlayer);

    if (card.isHeart() && !m_heartsBroken) {
        m_heartsBroken = true;
        emit heartsBrokenSignal();
    }

    emit cardPlayed(m_currentPlayer, card);

    nextTurn();
}

void Game::nextTurn() {
    // Check if trick is complete
    if (m_currentTrick.size() == NUM_PLAYERS) {
        // Longer delay so player can see the completed trick
        QTimer::singleShot(1500, this, &Game::completeTrick);
        return;
    }

    // Next player
    m_currentPlayer = (m_currentPlayer + 1) % NUM_PLAYERS;
    emit currentPlayerChanged(m_currentPlayer);

    if (m_players[m_currentPlayer]->isHuman()) {
        setState(GameState::WaitingForPlay);
    } else {
        QTimer::singleShot(500, this, &Game::aiTurn);
    }
}

void Game::completeTrick() {
    setState(GameState::TrickComplete);

    int winner = determineTrickWinner();

    // Calculate points
    int points = 0;
    for (const Card& c : m_currentTrick) {
        points += c.pointValue();
    }

    m_players[winner]->addRoundPoints(points);
    emit trickWon(winner, points);
    emit scoresChanged();

    // Clear trick
    m_currentTrick.clear();
    m_trickPlayers.clear();
    m_isFirstTrick = false;

    // Check if round is over
    if (m_players[0]->hand().isEmpty()) {
        QTimer::singleShot(500, this, &Game::endRound);
        return;
    }

    // Winner leads next trick
    m_currentPlayer = winner;
    emit currentPlayerChanged(m_currentPlayer);

    setState(GameState::Playing);

    if (m_players[m_currentPlayer]->isHuman()) {
        setState(GameState::WaitingForPlay);
    } else {
        QTimer::singleShot(500, this, &Game::aiTurn);
    }
}

int Game::determineTrickWinner() const {
    int winner = m_trickPlayers[0];
    Rank highestRank = m_currentTrick[0].rank();

    for (int i = 1; i < m_currentTrick.size(); ++i) {
        const Card& c = m_currentTrick[i];
        // Must follow lead suit to win
        if (c.suit() == m_leadSuit && c.rank() > highestRank) {
            highestRank = c.rank();
            winner = m_trickPlayers[i];
        }
    }

    return winner;
}

bool Game::checkShootTheMoon() {
    // Check if anyone got all 26 points
    for (int i = 0; i < NUM_PLAYERS; ++i) {
        if (m_players[i]->roundScore() == 26) {
            // Shoot the moon! Give 26 to everyone else
            for (int j = 0; j < NUM_PLAYERS; ++j) {
                if (j == i) {
                    // Reset shooter's round score to 0
                    // (it's currently 26, so subtract 26)
                } else {
                    m_players[j]->addRoundPoints(26 - m_players[j]->roundScore() + 26);
                }
            }
            return true;
        }
    }
    return false;
}

void Game::endRound() {
    setState(GameState::RoundComplete);

    // Check for shoot the moon
    for (int i = 0; i < NUM_PLAYERS; ++i) {
        if (m_players[i]->roundScore() == 26) {
            // Shooter gets 0, everyone else gets 26
            for (int j = 0; j < NUM_PLAYERS; ++j) {
                if (j != i) {
                    // Add 26 minus what they already have
                    m_players[j]->addRoundPoints(26 - m_players[j]->roundScore());
                }
            }
            // Reset shooter to 0
            m_players[i]->addRoundPoints(-26);
            break;
        }
    }

    // Add round scores to totals
    for (auto& p : m_players) {
        p->endRound();
    }

    emit roundEnded();
    emit scoresChanged();

    // Check for game over
    for (const auto& p : m_players) {
        if (p->totalScore() >= MAX_SCORE) {
            endGame();
            return;
        }
    }

    // Start next round
    QTimer::singleShot(2000, this, &Game::dealCards);
}

void Game::endGame() {
    setState(GameState::GameOver);

    // Find winner (lowest score)
    int winner = 0;
    int lowestScore = m_players[0]->totalScore();

    for (int i = 1; i < NUM_PLAYERS; ++i) {
        if (m_players[i]->totalScore() < lowestScore) {
            lowestScore = m_players[i]->totalScore();
            winner = i;
        }
    }

    emit gameEnded(winner);
}

bool Game::canUndo() const {
    // Can only undo when waiting for human input and there's history
    if (m_undoHistory.isEmpty()) return false;
    // Allow undo during human's turn or after game over
    return m_state == GameState::WaitingForPlay ||
           m_state == GameState::WaitingForPass ||
           m_state == GameState::GameOver;
}

void Game::undo() {
    if (!canUndo()) return;

    GameSnapshot snapshot = m_undoHistory.pop();
    restoreSnapshot(snapshot);

    emit undoPerformed();
    emit undoAvailableChanged(!m_undoHistory.isEmpty());
}

void Game::saveSnapshot() {
    GameSnapshot snapshot;
    snapshot.state = m_state;
    snapshot.roundNumber = m_roundNumber;
    snapshot.passDirection = m_passDirection;
    snapshot.currentPlayer = m_currentPlayer;
    snapshot.heartsBroken = m_heartsBroken;
    snapshot.isFirstTrick = m_isFirstTrick;
    snapshot.leadSuit = m_leadSuit;
    snapshot.currentTrick = m_currentTrick;
    snapshot.trickPlayers = m_trickPlayers;

    for (int i = 0; i < NUM_PLAYERS; ++i) {
        snapshot.playerStates[i].hand = m_players[i]->hand();
        snapshot.playerStates[i].roundScore = m_players[i]->roundScore();
        snapshot.playerStates[i].totalScore = m_players[i]->totalScore();
    }

    m_undoHistory.push(snapshot);

    // Limit history size
    while (m_undoHistory.size() > MAX_UNDO_HISTORY) {
        m_undoHistory.removeFirst();
    }

    emit undoAvailableChanged(true);
}

void Game::restoreSnapshot(const GameSnapshot& snapshot) {
    m_state = snapshot.state;
    m_roundNumber = snapshot.roundNumber;
    m_passDirection = snapshot.passDirection;
    m_currentPlayer = snapshot.currentPlayer;
    m_heartsBroken = snapshot.heartsBroken;
    m_isFirstTrick = snapshot.isFirstTrick;
    m_leadSuit = snapshot.leadSuit;
    m_currentTrick = snapshot.currentTrick;
    m_trickPlayers = snapshot.trickPlayers;

    for (int i = 0; i < NUM_PLAYERS; ++i) {
        m_players[i]->setHand(snapshot.playerStates[i].hand);
        m_players[i]->setRoundScore(snapshot.playerStates[i].roundScore);
        m_players[i]->setTotalScore(snapshot.playerStates[i].totalScore);
    }

    emit stateChanged(m_state);
    emit scoresChanged();
    emit currentPlayerChanged(m_currentPlayer);
}
