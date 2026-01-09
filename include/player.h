#ifndef PLAYER_H
#define PLAYER_H

#include "card.h"
#include <QString>
#include <QSet>
#include <QMap>
#include <functional>
#include <random>

enum class AIDifficulty {
    Easy,
    Medium,
    Hard
};

// Game context for AI strategic decisions
struct GameContext {
    int endScore = 100;              // Target score for game end
    bool moonProtection = false;     // Shoot the moon protection rule
    bool exactResetTo50 = false;     // Reset to 50 on exact endScore
    int playerScores[4] = {0,0,0,0}; // Current total scores of all players
    int roundScores[4] = {0,0,0,0};  // Current round scores
    int roundNumber = 1;
    int cardsRemaining = 13;         // Cards left in hand (for end-of-round decisions)
};

// Card memory for AI - tracks played cards and player voids
struct CardMemory {
    QSet<Card> playedCards;           // All cards played this round
    QMap<Suit, QSet<int>> voidPlayers; // Players known to be void in suit
    bool queenSpadesPlayed = false;    // Quick check for Q♠
    int pointsPlayedThisRound = 0;     // Track total points played

    void reset() {
        playedCards.clear();
        voidPlayers.clear();
        queenSpadesPlayed = false;
        pointsPlayedThisRound = 0;
    }

    void recordCard(const Card& card, int player, Suit leadSuit) {
        playedCards.insert(card);
        pointsPlayedThisRound += card.pointValue();
        if (card.isQueenOfSpades()) {
            queenSpadesPlayed = true;
        }
        // If player didn't follow suit, they're void
        if (card.suit() != leadSuit) {
            voidPlayers[leadSuit].insert(player);
        }
    }

    bool isPlayed(const Card& card) const {
        return playedCards.contains(card);
    }

    bool isPlayerVoid(int player, Suit suit) const {
        return voidPlayers.contains(suit) && voidPlayers[suit].contains(player);
    }

    int countPlayedInSuit(Suit suit) const {
        int count = 0;
        for (const Card& c : playedCards) {
            if (c.suit() == suit) count++;
        }
        return count;
    }

    // Count how many cards above a given rank are still out in a suit
    int countHigherCardsOut(Suit suit, Rank rank) const {
        int count = 0;
        for (int r = static_cast<int>(rank) + 1; r <= static_cast<int>(Rank::Ace); ++r) {
            Card c(suit, static_cast<Rank>(r));
            if (!playedCards.contains(c)) count++;
        }
        return count;
    }
};

class Player {
public:
    Player(int id, const QString& name, bool isHuman = false);

    int id() const { return m_id; }
    QString name() const { return m_name; }
    bool isHuman() const { return m_isHuman; }

    // Hand management
    const Cards& hand() const { return m_hand; }
    void setHand(const Cards& cards);
    void addCards(const Cards& cards);
    void removeCard(const Card& card);
    void removeCards(const Cards& cards);
    bool hasCard(const Card& card) const;
    void sortHand();

    // Scoring
    int roundScore() const { return m_roundScore; }
    int totalScore() const { return m_totalScore; }
    void addRoundPoints(int points) { m_roundScore += points; }
    void setRoundScore(int score) { m_roundScore = score; }
    void setTotalScore(int score) { m_totalScore = score; }
    void endRound();
    void resetScores();

    // Card memory for AI
    CardMemory& cardMemory() { return m_cardMemory; }
    const CardMemory& cardMemory() const { return m_cardMemory; }
    void resetCardMemory() { m_cardMemory.reset(); }
    void setCardMemory(const CardMemory& mem) { m_cardMemory = mem; }

    // Game context for AI strategic decisions
    void setGameContext(const GameContext& ctx) { m_gameContext = ctx; }
    const GameContext& gameContext() const { return m_gameContext; }

    // AI decision making
    Cards selectPassCards();
    Card selectPlay(Suit leadSuit, bool isFirstTrick, bool heartsBroken,
                    const Cards& trickCards, const QVector<int>& trickPlayers);

    // AI difficulty
    AIDifficulty difficulty() const { return m_difficulty; }
    void setDifficulty(AIDifficulty diff) { m_difficulty = diff; }

    // Get valid cards for current situation
    Cards getValidPlays(Suit leadSuit, bool isFirstTrick, bool heartsBroken) const;

    // Shared RNG for AI decisions
    static std::mt19937& rng();

private:
    int m_id;
    QString m_name;
    bool m_isHuman;
    Cards m_hand;
    int m_roundScore;
    int m_totalScore;
    AIDifficulty m_difficulty;
    CardMemory m_cardMemory;
    GameContext m_gameContext;

    // AI helpers
    Card aiSelectLead(const Cards& valid, bool heartsBroken);
    Card aiSelectFollow(const Cards& valid, Suit leadSuit, const Cards& trickCards);
    Card aiSelectSlough(const Cards& valid);

    // Difficulty-based helpers
    Card aiSelectLeadEasy(const Cards& valid);
    Card aiSelectLeadHard(const Cards& valid, bool heartsBroken);
    Card aiSelectFollowEasy(const Cards& valid);
    Card aiSelectFollowHard(const Cards& valid, Suit leadSuit, const Cards& trickCards,
                            const QVector<int>& trickPlayers);
    Card aiSelectSloughEasy(const Cards& valid);
    Card aiSelectSloughHard(const Cards& valid, const Cards& trickCards,
                            const QVector<int>& trickPlayers);

    // Smart pass selection for hard difficulty
    Cards selectPassCardsHard();
};

#endif // PLAYER_H
