#include "card.h"
#include <QStringList>

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

Card Card::fromElementId(const QString& id) {
    QStringList parts = id.split('_');
    if (parts.size() != 2) return Card();

    QString rankPart = parts[0].toLower();
    QString suitPart = parts[1].toLower();

    Rank rank;
    if (rankPart == "1" || rankPart == "ace") rank = Rank::Ace;
    else if (rankPart == "2") rank = Rank::Two;
    else if (rankPart == "3") rank = Rank::Three;
    else if (rankPart == "4") rank = Rank::Four;
    else if (rankPart == "5") rank = Rank::Five;
    else if (rankPart == "6") rank = Rank::Six;
    else if (rankPart == "7") rank = Rank::Seven;
    else if (rankPart == "8") rank = Rank::Eight;
    else if (rankPart == "9") rank = Rank::Nine;
    else if (rankPart == "10") rank = Rank::Ten;
    else if (rankPart == "jack") rank = Rank::Jack;
    else if (rankPart == "queen") rank = Rank::Queen;
    else if (rankPart == "king") rank = Rank::King;
    else return Card();

    Suit suit;
    if (suitPart == "club" || suitPart == "clubs") suit = Suit::Clubs;
    else if (suitPart == "diamond" || suitPart == "diamonds") suit = Suit::Diamonds;
    else if (suitPart == "spade" || suitPart == "spades") suit = Suit::Spades;
    else if (suitPart == "heart" || suitPart == "hearts") suit = Suit::Hearts;
    else return Card();

    return Card(suit, rank);
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

Card highestCard(const Cards& cards) {
    Card highest;
    for (const Card& c : cards) {
        if (!highest.isValid() || c.rank() > highest.rank()) {
            highest = c;
        }
    }
    return highest;
}

Card lowestCard(const Cards& cards) {
    Card lowest;
    for (const Card& c : cards) {
        if (!lowest.isValid() || c.rank() < lowest.rank()) {
            lowest = c;
        }
    }
    return lowest;
}

Card highestBelow(const Cards& cards, Rank maxRank) {
    Card best;
    for (const Card& c : cards) {
        if (c.rank() < maxRank) {
            if (!best.isValid() || c.rank() > best.rank()) {
                best = c;
            }
        }
    }
    return best;
}

Card lowestAbove(const Cards& cards, Rank minRank) {
    Card best;
    for (const Card& c : cards) {
        if (c.rank() > minRank) {
            if (!best.isValid() || c.rank() < best.rank()) {
                best = c;
            }
        }
    }
    return best;
}

int countSuit(const Cards& cards, Suit suit) {
    int count = 0;
    for (const Card& c : cards) {
        if (c.suit() == suit) count++;
    }
    return count;
}
