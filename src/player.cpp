#include "player.h"
#include <algorithm>
#include <random>
#include <chrono>

Player::Player(int id, const QString& name, bool isHuman)
    : m_id(id), m_name(name), m_isHuman(isHuman), m_roundScore(0), m_totalScore(0), m_difficulty(AIDifficulty::Medium) {}

void Player::setHand(const Cards& cards) {
    m_hand = cards;
    sortHand();
}

void Player::addCards(const Cards& cards) {
    m_hand.append(cards);
    sortHand();
}

void Player::removeCard(const Card& card) {
    m_hand.removeOne(card);
}

void Player::removeCards(const Cards& cards) {
    for (const Card& c : cards) {
        m_hand.removeOne(c);
    }
}

bool Player::hasCard(const Card& card) const {
    return m_hand.contains(card);
}

void Player::sortHand() {
    std::sort(m_hand.begin(), m_hand.end());
}

void Player::endRound() {
    m_totalScore += m_roundScore;
    m_roundScore = 0;
}

void Player::resetScores() {
    m_roundScore = 0;
    m_totalScore = 0;
}

Cards Player::getValidPlays(Suit leadSuit, bool isFirstTrick, bool heartsBroken) const {
    Cards valid;

    // First card of first trick must be 2 of clubs
    if (isFirstTrick && leadSuit == Suit::Clubs) {
        Card twoClubs(Suit::Clubs, Rank::Two);
        if (hasCard(twoClubs)) {
            valid.append(twoClubs);
            return valid;
        }
    }

    // Must follow suit if possible
    Cards suited = cardsOfSuit(m_hand, leadSuit);
    if (!suited.isEmpty()) {
        return suited;
    }

    // Can't follow suit - can play anything except:
    // - On first trick, can't play hearts or QoS (unless only have those)
    if (isFirstTrick) {
        for (const Card& c : m_hand) {
            if (!c.isPointCard()) {
                valid.append(c);
            }
        }
        if (valid.isEmpty()) {
            return m_hand; // Only have point cards
        }
        return valid;
    }

    // Not first trick, can play anything
    return m_hand;
}

Cards Player::selectPassCards() {
    // AI: Pass dangerous cards
    // Priority: QoS, high spades (A, K), high hearts, other high cards
    Cards toPass;
    Cards remaining = m_hand;

    // Sort by "danger level"
    std::sort(remaining.begin(), remaining.end(), [](const Card& a, const Card& b) {
        // QoS is most dangerous
        if (a.isQueenOfSpades()) return true;
        if (b.isQueenOfSpades()) return false;

        // Ace/King of spades are dangerous (can catch QoS)
        bool aHighSpade = a.suit() == Suit::Spades && a.rank() >= Rank::King;
        bool bHighSpade = b.suit() == Suit::Spades && b.rank() >= Rank::King;
        if (aHighSpade && !bHighSpade) return true;
        if (!aHighSpade && bHighSpade) return false;

        // High hearts are bad
        if (a.isHeart() && !b.isHeart()) return true;
        if (!a.isHeart() && b.isHeart()) return false;
        if (a.isHeart() && b.isHeart()) {
            return a.rank() > b.rank();
        }

        // Otherwise pass high cards
        return a.rank() > b.rank();
    });

    for (int i = 0; i < 3 && i < remaining.size(); ++i) {
        toPass.append(remaining[i]);
    }

    return toPass;
}

Card Player::selectPlay(Suit leadSuit, bool isFirstTrick, bool heartsBroken, const Cards& trickCards) {
    Cards valid = getValidPlays(leadSuit, isFirstTrick, heartsBroken);

    if (valid.isEmpty()) {
        return m_hand.first(); // Shouldn't happen
    }

    if (valid.size() == 1) {
        return valid.first();
    }

    // Route to different strategies based on difficulty
    switch (m_difficulty) {
        case AIDifficulty::Easy:
            // Easy: More random choices, makes mistakes
            if (trickCards.isEmpty()) {
                return aiSelectLeadEasy(valid);
            }
            if (hasSuit(valid, leadSuit)) {
                return aiSelectFollowEasy(valid);
            }
            return aiSelectSlough(valid);

        case AIDifficulty::Hard:
            // Hard: Smarter card counting and strategy
            if (trickCards.isEmpty()) {
                return aiSelectLeadHard(valid, heartsBroken);
            }
            if (hasSuit(valid, leadSuit)) {
                return aiSelectFollowHard(valid, leadSuit, trickCards);
            }
            return aiSelectSloughHard(valid, trickCards);

        case AIDifficulty::Medium:
        default:
            // Medium: Current balanced strategy
            if (trickCards.isEmpty()) {
                return aiSelectLead(valid, heartsBroken);
            }
            if (hasSuit(valid, leadSuit)) {
                return aiSelectFollow(valid, leadSuit, trickCards);
            }
            return aiSelectSlough(valid);
    }
}

Card Player::aiSelectLead(const Cards& valid, bool heartsBroken) {
    // Prefer leading low non-point cards
    // Avoid leading spades (might pull out QoS)

    // Try clubs or diamonds first
    for (Suit s : {Suit::Clubs, Suit::Diamonds}) {
        Card low = lowestOfSuit(valid, s);
        if (low.isValid()) return low;
    }

    // Low spades (not high ones)
    Cards spades = cardsOfSuit(valid, Suit::Spades);
    for (const Card& c : spades) {
        if (c.rank() < Rank::Queen) return c;
    }

    // Hearts if broken
    if (heartsBroken) {
        Card low = lowestOfSuit(valid, Suit::Hearts);
        if (low.isValid()) return low;
    }

    // Just play lowest
    Card lowest = valid.first();
    for (const Card& c : valid) {
        if (c.rank() < lowest.rank()) lowest = c;
    }
    return lowest;
}

Card Player::aiSelectFollow(const Cards& valid, Suit leadSuit, const Cards& trickCards) {
    // Find highest card played in lead suit
    Card highest = highestOfSuit(trickCards, leadSuit);

    // Try to play under if possible
    Cards under;
    for (const Card& c : valid) {
        if (c.suit() == leadSuit && c.rank() < highest.rank()) {
            under.append(c);
        }
    }

    if (!under.isEmpty()) {
        // Play highest card that's still under
        Card best = under.first();
        for (const Card& c : under) {
            if (c.rank() > best.rank()) best = c;
        }
        return best;
    }

    // Must play over - play lowest to minimize winning future bad tricks
    Card lowest = valid.first();
    for (const Card& c : valid) {
        if (c.rank() < lowest.rank()) lowest = c;
    }
    return lowest;
}

Card Player::aiSelectSlough(const Cards& valid) {
    // Dump dangerous cards!
    // Priority: QoS, high spades, high hearts, high cards

    // QoS first
    for (const Card& c : valid) {
        if (c.isQueenOfSpades()) return c;
    }

    // High spades (A, K)
    for (const Card& c : valid) {
        if (c.suit() == Suit::Spades && c.rank() >= Rank::King) return c;
    }

    // Highest heart
    Card highHeart;
    for (const Card& c : valid) {
        if (c.isHeart()) {
            if (!highHeart.isValid() || c.rank() > highHeart.rank()) {
                highHeart = c;
            }
        }
    }
    if (highHeart.isValid()) return highHeart;

    // Highest card overall
    Card highest = valid.first();
    for (const Card& c : valid) {
        if (c.rank() > highest.rank()) highest = c;
    }
    return highest;
}

// Easy difficulty: Makes suboptimal choices, plays somewhat randomly
Card Player::aiSelectLeadEasy(const Cards& valid) {
    // Easy: Pick a random card (50% of time) or just play highest (bad strategy)
    static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());

    if (std::uniform_int_distribution<>(0, 1)(rng) == 0) {
        // Random card
        std::uniform_int_distribution<> dist(0, valid.size() - 1);
        return valid[dist(rng)];
    }

    // Otherwise play highest card (suboptimal - tends to win tricks)
    Card highest = valid.first();
    for (const Card& c : valid) {
        if (c.rank() > highest.rank()) highest = c;
    }
    return highest;
}

Card Player::aiSelectFollowEasy(const Cards& valid) {
    // Easy: Play random card when following suit
    static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<> dist(0, valid.size() - 1);
    return valid[dist(rng)];
}

// Hard difficulty: Advanced strategies
Card Player::aiSelectLeadHard(const Cards& valid, bool heartsBroken) {
    // Hard: Lead cards strategically to flush out high cards

    // Count cards by suit to identify short suits in opponents
    // Lead from suits where we have control (A-K-Q sequence)

    // Priority 1: If we have low cards in a suit with protection, lead low
    for (Suit s : {Suit::Clubs, Suit::Diamonds}) {
        Cards suitCards = cardsOfSuit(valid, s);
        if (suitCards.size() >= 2) {
            // Have multiple cards in this suit - lead lowest
            Card lowest = lowestOfSuit(valid, s);
            if (lowest.isValid() && lowest.rank() <= Rank::Seven) {
                return lowest;
            }
        }
    }

    // Priority 2: Lead spades below Queen to flush out QoS
    Cards spades = cardsOfSuit(valid, Suit::Spades);
    bool hasHighSpades = false;
    for (const Card& c : spades) {
        if (c.rank() >= Rank::Queen) hasHighSpades = true;
    }

    // If we don't have dangerous spades, lead low spades to flush others
    if (!hasHighSpades && !spades.isEmpty()) {
        Card lowest = lowestOfSuit(valid, Suit::Spades);
        if (lowest.isValid() && lowest.rank() < Rank::Queen) {
            return lowest;
        }
    }

    // Priority 3: If hearts broken, lead low hearts to run out opponents
    if (heartsBroken) {
        Card lowHeart = lowestOfSuit(valid, Suit::Hearts);
        if (lowHeart.isValid()) {
            return lowHeart;
        }
    }

    // Fallback: lowest card overall
    Card lowest = valid.first();
    for (const Card& c : valid) {
        if (c.rank() < lowest.rank()) lowest = c;
    }
    return lowest;
}

Card Player::aiSelectFollowHard(const Cards& valid, Suit leadSuit, const Cards& trickCards) {
    Card highest = highestOfSuit(trickCards, leadSuit);
    int numCardsPlayed = trickCards.size();

    // If we're last to play and trick has no points, we can safely win
    if (numCardsPlayed == 3) {
        int trickPoints = 0;
        for (const Card& c : trickCards) {
            trickPoints += c.pointValue();
        }

        if (trickPoints == 0) {
            // Safe to win - play highest under if possible, else lowest over
            Cards under;
            for (const Card& c : valid) {
                if (c.suit() == leadSuit && c.rank() < highest.rank()) {
                    under.append(c);
                }
            }
            if (!under.isEmpty()) {
                // Play highest under
                Card best = under.first();
                for (const Card& c : under) {
                    if (c.rank() > best.rank()) best = c;
                }
                return best;
            }
            // Have to beat - play lowest that beats
            Card lowest = valid.first();
            for (const Card& c : valid) {
                if (c.rank() < lowest.rank()) lowest = c;
            }
            return lowest;
        }
    }

    // Trick has points or we're not last - try hard to duck
    Cards under;
    for (const Card& c : valid) {
        if (c.suit() == leadSuit && c.rank() < highest.rank()) {
            under.append(c);
        }
    }

    if (!under.isEmpty()) {
        // Play highest card that still ducks
        Card best = under.first();
        for (const Card& c : under) {
            if (c.rank() > best.rank()) best = c;
        }
        return best;
    }

    // Must beat - play lowest possible
    Card lowest = valid.first();
    for (const Card& c : valid) {
        if (c.rank() < lowest.rank()) lowest = c;
    }
    return lowest;
}

Card Player::aiSelectSloughHard(const Cards& valid, const Cards& trickCards) {
    // Calculate current trick points
    int trickPoints = 0;
    for (const Card& c : trickCards) {
        trickPoints += c.pointValue();
    }

    // If trick already has points, dump highest point cards
    if (trickPoints > 0) {
        // QoS first
        for (const Card& c : valid) {
            if (c.isQueenOfSpades()) return c;
        }
        // Highest heart
        Card highHeart;
        for (const Card& c : valid) {
            if (c.isHeart()) {
                if (!highHeart.isValid() || c.rank() > highHeart.rank()) {
                    highHeart = c;
                }
            }
        }
        if (highHeart.isValid()) return highHeart;
    }

    // Dump high spades to avoid catching QoS later
    for (const Card& c : valid) {
        if (c.suit() == Suit::Spades && c.rank() == Rank::Ace) return c;
    }
    for (const Card& c : valid) {
        if (c.suit() == Suit::Spades && c.rank() == Rank::King) return c;
    }

    // Dump high cards from long suits to create voids
    // Count our suits
    int clubCount = 0, diamondCount = 0;
    Card highClub, highDiamond;
    for (const Card& c : valid) {
        if (c.suit() == Suit::Clubs) {
            clubCount++;
            if (!highClub.isValid() || c.rank() > highClub.rank()) highClub = c;
        } else if (c.suit() == Suit::Diamonds) {
            diamondCount++;
            if (!highDiamond.isValid() || c.rank() > highDiamond.rank()) highDiamond = c;
        }
    }

    // Dump from longer suit to create void
    if (clubCount > diamondCount && clubCount > 1 && highClub.isValid()) {
        return highClub;
    }
    if (diamondCount > clubCount && diamondCount > 1 && highDiamond.isValid()) {
        return highDiamond;
    }

    // Fallback to medium strategy
    return aiSelectSlough(valid);
}
