#ifndef DECK_H
#define DECK_H

#include "card.h"
#include <random>

class Deck {
public:
    Deck();

    void shuffle();
    Card deal();
    bool isEmpty() const { return m_cards.isEmpty(); }
    int remaining() const { return m_cards.size(); }

    // Deal to multiple hands
    QVector<Cards> dealAll(int numPlayers);

private:
    Cards m_cards;
    std::mt19937 m_rng;
};

#endif // DECK_H
