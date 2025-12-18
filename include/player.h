#ifndef PLAYER_H
#define PLAYER_H

#include "card.h"
#include <QString>
#include <functional>

enum class AIDifficulty {
    Easy,
    Medium,
    Hard
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

    // AI decision making
    Cards selectPassCards();
    Card selectPlay(Suit leadSuit, bool isFirstTrick, bool heartsBroken, const Cards& trickCards);

    // AI difficulty
    AIDifficulty difficulty() const { return m_difficulty; }
    void setDifficulty(AIDifficulty diff) { m_difficulty = diff; }

    // Get valid cards for current situation
    Cards getValidPlays(Suit leadSuit, bool isFirstTrick, bool heartsBroken) const;

private:
    int m_id;
    QString m_name;
    bool m_isHuman;
    Cards m_hand;
    int m_roundScore;
    int m_totalScore;
    AIDifficulty m_difficulty;

    // AI helpers
    Card aiSelectLead(const Cards& valid, bool heartsBroken);
    Card aiSelectFollow(const Cards& valid, Suit leadSuit, const Cards& trickCards);
    Card aiSelectSlough(const Cards& valid);

    // Difficulty-based helpers
    Card aiSelectLeadEasy(const Cards& valid);
    Card aiSelectLeadHard(const Cards& valid, bool heartsBroken);
    Card aiSelectFollowEasy(const Cards& valid);
    Card aiSelectFollowHard(const Cards& valid, Suit leadSuit, const Cards& trickCards);
    Card aiSelectSloughHard(const Cards& valid, const Cards& trickCards);
};

#endif // PLAYER_H
