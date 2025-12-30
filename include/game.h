#ifndef GAME_H
#define GAME_H

#include "card.h"
#include "player.h"
#include "deck.h"
#include <QObject>
#include <memory>
#include <array>
#include <QStack>

enum class GameState {
    NotStarted,
    Dealing,
    Passing,
    WaitingForPass,    // Waiting for human to select pass cards
    Playing,
    WaitingForPlay,    // Waiting for human to play a card
    TrickComplete,
    RoundComplete,
    GameOver
};

enum class PassDirection { Left, Right, Across, None };

// Game rules configuration
struct GameRules {
    int endScore = 100;              // Game ends when a player reaches this score
    bool exactResetTo50 = false;     // If score is exactly endScore, reset to 50
    bool queenBreaksHearts = true;   // Q♠ breaks hearts when played (standard: true)
    bool moonProtection = false;     // If +26 to others would cause shooter to lose, they can take -26 instead
    bool fullPolish = false;         // 99 points + takes 25 = reset to 98

    // Default standard rules
    static GameRules standard() {
        return GameRules{100, false, true, false, false};
    }
};

// Snapshot of game state for undo functionality
struct GameSnapshot {
    GameState state;
    int roundNumber;
    PassDirection passDirection;
    int currentPlayer;
    bool heartsBroken;
    bool isFirstTrick;
    Suit leadSuit;
    Cards currentTrick;
    QVector<int> trickPlayers;
    // Player states
    struct PlayerState {
        Cards hand;
        int roundScore;
        int totalScore;
    };
    std::array<PlayerState, 4> playerStates;
};

class Game : public QObject {
    Q_OBJECT

public:
    static const int NUM_PLAYERS = 4;
    static const int CARDS_TO_PASS = 3;

    explicit Game(QObject* parent = nullptr);

    // Game control
    void newGame();
    void setPlayerName(int index, const QString& name);
    void setAIDifficulty(AIDifficulty difficulty);
    AIDifficulty aiDifficulty() const;

    // Game rules
    void setRules(const GameRules& rules) { m_rules = rules; }
    const GameRules& rules() const { return m_rules; }

    // Human interactions
    void humanPassCards(const Cards& cards);
    void humanPlayCard(const Card& card);

    // Undo functionality
    bool canUndo() const;
    void undo();

    // State queries
    GameState state() const { return m_state; }
    int currentPlayer() const { return m_currentPlayer; }
    int roundNumber() const { return m_roundNumber; }
    PassDirection passDirection() const { return m_passDirection; }
    bool heartsBroken() const { return m_heartsBroken; }
    bool isFirstTrick() const { return m_isFirstTrick; }

    // Player queries
    Player* player(int index);
    const Player* player(int index) const;
    const Cards& currentTrick() const { return m_currentTrick; }
    const QVector<int>& trickPlayers() const { return m_trickPlayers; }
    Suit leadSuit() const { return m_leadSuit; }

    // Valid moves for human
    Cards getValidPassCards() const;
    Cards getValidPlays() const;

signals:
    void stateChanged(GameState state);
    void cardsDealt();
    void passDirectionAnnounced(PassDirection dir);
    void passingComplete(Cards receivedCards);  // Cards received by human player
    void cardPlayed(int player, Card card);
    void trickWon(int winner, int points);
    void roundEnded();
    void gameEnded(int winner);
    void scoresChanged();
    void currentPlayerChanged(int player);
    void heartsBrokenSignal();
    void undoAvailableChanged(bool available);
    void undoPerformed();  // Signal to refresh the view

private:
    void setState(GameState state);
    void dealCards();
    void startPassing();
    void executePassing();
    void startPlaying();
    void nextTurn();
    void aiTurn();
    void completeTrick();
    void endRound();
    void endGame();
    int determineTrickWinner() const;
    int findTwoOfClubsPlayer() const;
    bool checkShootTheMoon();

    // Undo helpers
    void saveSnapshot();
    void restoreSnapshot(const GameSnapshot& snapshot);

    GameState m_state;
    int m_roundNumber;
    PassDirection m_passDirection;
    int m_currentPlayer;
    bool m_heartsBroken;
    bool m_isFirstTrick;
    GameRules m_rules;

    std::array<std::unique_ptr<Player>, NUM_PLAYERS> m_players;
    QVector<Cards> m_passedCards; // Cards each player is passing

    // Current trick
    Cards m_currentTrick;
    QVector<int> m_trickPlayers;
    Suit m_leadSuit;

    // Undo history
    QStack<GameSnapshot> m_undoHistory;
    static const int MAX_UNDO_HISTORY = 50;
};

#endif // GAME_H
