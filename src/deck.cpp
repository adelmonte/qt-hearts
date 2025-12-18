#include "deck.h"
#include <algorithm>
#include <chrono>

Deck::Deck() {
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    m_rng.seed(static_cast<unsigned>(seed));

    // Create all 52 cards
    for (int s = 0; s < 4; ++s) {
        for (int r = 2; r <= 14; ++r) {
            m_cards.append(Card(static_cast<Suit>(s), static_cast<Rank>(r)));
        }
    }
}

void Deck::shuffle() {
    std::shuffle(m_cards.begin(), m_cards.end(), m_rng);
}

Card Deck::deal() {
    if (m_cards.isEmpty()) return Card();
    return m_cards.takeFirst();
}

QVector<Cards> Deck::dealAll(int numPlayers) {
    shuffle();

    QVector<Cards> hands(numPlayers);
    int cardsPerPlayer = 52 / numPlayers;

    for (int i = 0; i < numPlayers; ++i) {
        for (int j = 0; j < cardsPerPlayer; ++j) {
            hands[i].append(deal());
        }
        std::sort(hands[i].begin(), hands[i].end());
    }

    return hands;
}
