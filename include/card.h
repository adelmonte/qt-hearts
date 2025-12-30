#ifndef CARD_H
#define CARD_H

#include <QString>
#include <QVector>
#include <QHash>

enum class Suit { Clubs, Diamonds, Spades, Hearts };
enum class Rank { Two = 2, Three, Four, Five, Six, Seven, Eight, Nine, Ten, Jack, Queen, King, Ace };

class Card {
public:
    Card();
    Card(Suit suit, Rank rank);

    Suit suit() const { return m_suit; }
    Rank rank() const { return m_rank; }

    bool isValid() const { return m_valid; }
    bool isHeart() const { return m_suit == Suit::Hearts; }
    bool isQueenOfSpades() const { return m_suit == Suit::Spades && m_rank == Rank::Queen; }
    bool isPointCard() const { return isHeart() || isQueenOfSpades(); }
    bool isTwoOfClubs() const { return m_suit == Suit::Clubs && m_rank == Rank::Two; }

    int pointValue() const;

    // For SVG element IDs (e.g., "1_club", "queen_spade")
    QString elementId() const;

    // Display strings
    QString rankString() const;
    QString suitString() const;
    QString toString() const;

    bool operator==(const Card& other) const;
    bool operator!=(const Card& other) const;
    bool operator<(const Card& other) const;

    // For use in QSet/QHash
    uint hash() const { return static_cast<uint>(m_suit) * 16 + static_cast<uint>(m_rank); }

private:
    Suit m_suit;
    Rank m_rank;
    bool m_valid;
};

// Hash function for QSet/QHash
inline size_t qHash(const Card& card, size_t seed = 0) {
    return qHash(card.hash(), seed);
}

using Cards = QVector<Card>;

// Utility functions
Cards cardsOfSuit(const Cards& cards, Suit suit);
bool hasSuit(const Cards& cards, Suit suit);
bool hasOnlyHearts(const Cards& cards);
Card highestOfSuit(const Cards& cards, Suit suit);
Card lowestOfSuit(const Cards& cards, Suit suit);

// Additional consolidated helpers
Card highestCard(const Cards& cards);
Card lowestCard(const Cards& cards);
Card highestBelow(const Cards& cards, Rank maxRank);  // Highest card below a given rank
Card lowestAbove(const Cards& cards, Rank minRank);   // Lowest card above a given rank
int countSuit(const Cards& cards, Suit suit);

#endif // CARD_H
