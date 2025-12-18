#include "card.h"

Card::Card() : m_suit(Suit::Clubs), m_rank(Rank::Two), m_valid(false) {}

Card::Card(Suit suit, Rank rank) : m_suit(suit), m_rank(rank), m_valid(true) {}

int Card::pointValue() const {
    if (isQueenOfSpades()) return 13;
    if (isHeart()) return 1;
    return 0;
}

QString Card::elementId() const {
    QString r;
    switch (m_rank) {
        case Rank::Ace:   r = "1"; break;
        case Rank::Two:   r = "2"; break;
        case Rank::Three: r = "3"; break;
        case Rank::Four:  r = "4"; break;
        case Rank::Five:  r = "5"; break;
        case Rank::Six:   r = "6"; break;
        case Rank::Seven: r = "7"; break;
        case Rank::Eight: r = "8"; break;
        case Rank::Nine:  r = "9"; break;
        case Rank::Ten:   r = "10"; break;
        case Rank::Jack:  r = "jack"; break;
        case Rank::Queen: r = "queen"; break;
        case Rank::King:  r = "king"; break;
    }

    QString s;
    switch (m_suit) {
        case Suit::Clubs:    s = "club"; break;
        case Suit::Diamonds: s = "diamond"; break;
        case Suit::Spades:   s = "spade"; break;
        case Suit::Hearts:   s = "heart"; break;
    }

    return r + "_" + s;
}

QString Card::rankString() const {
    switch (m_rank) {
        case Rank::Ace:   return "A";
        case Rank::Two:   return "2";
        case Rank::Three: return "3";
        case Rank::Four:  return "4";
        case Rank::Five:  return "5";
        case Rank::Six:   return "6";
        case Rank::Seven: return "7";
        case Rank::Eight: return "8";
        case Rank::Nine:  return "9";
        case Rank::Ten:   return "10";
        case Rank::Jack:  return "J";
        case Rank::Queen: return "Q";
        case Rank::King:  return "K";
    }
    return "?";
}

QString Card::suitString() const {
    switch (m_suit) {
        case Suit::Clubs:    return "♣";
        case Suit::Diamonds: return "♦";
        case Suit::Spades:   return "♠";
        case Suit::Hearts:   return "♥";
    }
    return "?";
}

QString Card::toString() const {
    return rankString() + suitString();
}

bool Card::operator==(const Card& other) const {
    return m_suit == other.m_suit && m_rank == other.m_rank;
}

bool Card::operator!=(const Card& other) const {
    return !(*this == other);
}

bool Card::operator<(const Card& other) const {
    if (m_suit != other.m_suit) {
        return static_cast<int>(m_suit) < static_cast<int>(other.m_suit);
    }
    return static_cast<int>(m_rank) < static_cast<int>(other.m_rank);
}

// Utility functions
Cards cardsOfSuit(const Cards& cards, Suit suit) {
    Cards result;
    for (const Card& c : cards) {
        if (c.suit() == suit) result.append(c);
    }
    return result;
}

bool hasSuit(const Cards& cards, Suit suit) {
    for (const Card& c : cards) {
        if (c.suit() == suit) return true;
    }
    return false;
}

bool hasOnlyHearts(const Cards& cards) {
    for (const Card& c : cards) {
        if (!c.isHeart()) return false;
    }
    return true;
}

Card highestOfSuit(const Cards& cards, Suit suit) {
    Card highest;
    for (const Card& c : cards) {
        if (c.suit() == suit) {
            if (!highest.isValid() || c.rank() > highest.rank()) {
                highest = c;
            }
        }
    }
    return highest;
}

Card lowestOfSuit(const Cards& cards, Suit suit) {
    Card lowest;
    for (const Card& c : cards) {
        if (c.suit() == suit) {
            if (!lowest.isValid() || c.rank() < lowest.rank()) {
                lowest = c;
            }
        }
    }
    return lowest;
}
