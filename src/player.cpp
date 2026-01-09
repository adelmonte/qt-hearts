#include "player.h"
#include <algorithm>
#include <chrono>

// Static RNG shared across all AI decisions - seeded once
std::mt19937& Player::rng() {
    static std::mt19937 instance(std::chrono::steady_clock::now().time_since_epoch().count());
    return instance;
}

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

// ============================================================================
// PASS CARD SELECTION
// ============================================================================

Cards Player::selectPassCards() {
    if (m_difficulty == AIDifficulty::Hard) {
        return selectPassCardsHard();
    }

    // Medium/Easy: Pass dangerous cards
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

Cards Player::selectPassCardsHard() {
    // Hard mode: Strategic passing with void creation, Q♠ protection, and score awareness
    Cards toPass;
    Cards remaining = m_hand;
    const GameContext& ctx = m_gameContext;

    // Count cards in each suit
    int suitCounts[4] = {0, 0, 0, 0};
    for (const Card& c : m_hand) {
        suitCounts[static_cast<int>(c.suit())]++;
    }

    // Check if we have Q♠ protection (K♠ and/or A♠ with Q♠)
    bool hasQoS = hasCard(Card(Suit::Spades, Rank::Queen));
    bool hasKoS = hasCard(Card(Suit::Spades, Rank::King));
    bool hasAoS = hasCard(Card(Suit::Spades, Rank::Ace));
    int spadeCount = suitCounts[static_cast<int>(Suit::Spades)];
    int heartCount = suitCounts[static_cast<int>(Suit::Hearts)];

    // Strategic assessment: are we far behind?
    int myScore = ctx.playerScores[m_id];
    int lowestOtherScore = 999;
    for (int i = 0; i < 4; ++i) {
        if (i != m_id && ctx.playerScores[i] < lowestOtherScore) {
            lowestOtherScore = ctx.playerScores[i];
        }
    }
    bool significantlyBehind = myScore - lowestOtherScore > 25;

    // Check for potential shoot-the-moon hand
    // Need: lots of hearts, high cards, control
    Cards hearts = cardsOfSuit(m_hand, Suit::Hearts);
    int highHearts = 0;
    for (const Card& c : hearts) {
        if (c.rank() >= Rank::Jack) highHearts++;
    }
    bool potentialMoonHand = (heartCount >= 6 && highHearts >= 3) ||
                              (heartCount >= 5 && highHearts >= 4 && hasAoS);

    // If significantly behind and have a potential moon hand, keep hearts and high cards
    if (significantlyBehind && potentialMoonHand && ctx.moonProtection) {
        // Pass low cards instead of high cards
        Cards lowCards;
        for (const Card& c : m_hand) {
            if (c.rank() <= Rank::Six && !c.isHeart()) {
                lowCards.append(c);
            }
        }
        std::sort(lowCards.begin(), lowCards.end(), [](const Card& a, const Card& b) {
            return a.rank() < b.rank();
        });
        for (const Card& c : lowCards) {
            if (toPass.size() >= 3) break;
            toPass.append(c);
        }
        if (toPass.size() >= 3) return toPass;
    }

    // If we have Q♠ with good protection (A+K or 5+ spades), consider keeping it
    bool keepQoS = hasQoS && ((hasKoS && hasAoS) || spadeCount >= 5);

    // Find shortest non-spade suit for void creation
    Suit shortestSuit = Suit::Clubs;
    int shortestCount = 14;
    for (int s = 0; s < 4; ++s) {
        Suit suit = static_cast<Suit>(s);
        if (suit != Suit::Spades && suitCounts[s] > 0 && suitCounts[s] < shortestCount) {
            shortestCount = suitCounts[s];
            shortestSuit = suit;
        }
    }

    // Strategy: Try to create a void by passing all cards of shortest suit
    // But prioritize getting rid of dangerous cards first
    Cards dangerousCards;

    // Q♠ unless well protected
    if (hasQoS && !keepQoS) {
        dangerousCards.append(Card(Suit::Spades, Rank::Queen));
    }

    // A♠ and K♠ are dangerous if we don't have Q♠ (we might catch it)
    if (!hasQoS) {
        if (hasAoS) dangerousCards.append(Card(Suit::Spades, Rank::Ace));
        if (hasKoS) dangerousCards.append(Card(Suit::Spades, Rank::King));
    }

    // High hearts
    Cards highHeartCards = cardsOfSuit(m_hand, Suit::Hearts);
    std::sort(highHeartCards.begin(), highHeartCards.end(), [](const Card& a, const Card& b) {
        return a.rank() > b.rank();
    });
    for (const Card& h : highHeartCards) {
        if (h.rank() >= Rank::Queen) {
            dangerousCards.append(h);
        }
    }

    // Add dangerous cards first
    for (const Card& c : dangerousCards) {
        if (toPass.size() >= 3) break;
        if (!toPass.contains(c)) {
            toPass.append(c);
        }
    }

    // If we can complete a void with remaining passes, do it
    if (toPass.size() < 3 && shortestCount <= (3 - toPass.size())) {
        Cards shortSuitCards = cardsOfSuit(m_hand, shortestSuit);
        for (const Card& c : shortSuitCards) {
            if (toPass.size() >= 3) break;
            if (!toPass.contains(c)) {
                toPass.append(c);
            }
        }
    }

    // Fill remaining with highest cards (non-spade if we're protecting Q♠)
    if (toPass.size() < 3) {
        Cards candidates = remaining;
        std::sort(candidates.begin(), candidates.end(), [keepQoS](const Card& a, const Card& b) {
            // If protecting spades, deprioritize them
            if (keepQoS) {
                if (a.suit() == Suit::Spades && b.suit() != Suit::Spades) return false;
                if (a.suit() != Suit::Spades && b.suit() == Suit::Spades) return true;
            }
            return a.rank() > b.rank();
        });

        for (const Card& c : candidates) {
            if (toPass.size() >= 3) break;
            if (!toPass.contains(c)) {
                toPass.append(c);
            }
        }
    }

    return toPass;
}

// ============================================================================
// MAIN PLAY SELECTION
// ============================================================================

Card Player::selectPlay(Suit leadSuit, bool isFirstTrick, bool heartsBroken,
                        const Cards& trickCards, const QVector<int>& trickPlayers) {
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
            if (trickCards.isEmpty()) {
                return aiSelectLeadEasy(valid);
            }
            if (hasSuit(valid, leadSuit)) {
                return aiSelectFollowEasy(valid);
            }
            return aiSelectSloughEasy(valid);

        case AIDifficulty::Hard:
            if (trickCards.isEmpty()) {
                return aiSelectLeadHard(valid, heartsBroken);
            }
            if (hasSuit(valid, leadSuit)) {
                return aiSelectFollowHard(valid, leadSuit, trickCards, trickPlayers);
            }
            return aiSelectSloughHard(valid, trickCards, trickPlayers);

        case AIDifficulty::Medium:
        default:
            if (trickCards.isEmpty()) {
                return aiSelectLead(valid, heartsBroken);
            }
            if (hasSuit(valid, leadSuit)) {
                return aiSelectFollow(valid, leadSuit, trickCards);
            }
            return aiSelectSlough(valid);
    }
}

// ============================================================================
// EASY DIFFICULTY
// ============================================================================

Card Player::aiSelectLeadEasy(const Cards& valid) {
    // Easy: 50% random, 50% highest card (bad strategy)
    if (std::uniform_int_distribution<>(0, 1)(rng()) == 0) {
        std::uniform_int_distribution<> dist(0, valid.size() - 1);
        return valid[dist(rng())];
    }
    return highestCard(valid);
}

Card Player::aiSelectFollowEasy(const Cards& valid) {
    // Easy: Play random card when following suit
    std::uniform_int_distribution<> dist(0, valid.size() - 1);
    return valid[dist(rng())];
}

Card Player::aiSelectSloughEasy(const Cards& valid) {
    // Easy: Just plays high cards randomly, no strategic thinking about Q♠ or spades
    // 50% chance to play a random card, 50% chance to play highest card
    if (std::uniform_int_distribution<>(0, 1)(rng()) == 0) {
        std::uniform_int_distribution<> dist(0, valid.size() - 1);
        return valid[dist(rng())];
    }
    return highestCard(valid);
}

// ============================================================================
// MEDIUM DIFFICULTY
// ============================================================================

Card Player::aiSelectLead(const Cards& valid, bool heartsBroken) {
    // Medium difficulty: Prefer leading low non-point cards with basic Q♠ awareness

    // Check if Q♠ is still out there
    bool qosOut = !m_cardMemory.queenSpadesPlayed && !hasCard(Card(Suit::Spades, Rank::Queen));

    // If Q♠ is out and we have low spades, consider flushing it
    if (qosOut) {
        Card lowSpade = lowestOfSuit(valid, Suit::Spades);
        if (lowSpade.isValid() && lowSpade.rank() < Rank::Queen) {
            // 40% chance to lead low spade to flush Q♠ (medium isn't as aggressive)
            if (std::uniform_int_distribution<>(0, 4)(rng()) < 2) {
                return lowSpade;
            }
        }
    }

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
    return lowestCard(valid);
}

Card Player::aiSelectFollow(const Cards& valid, Suit leadSuit, const Cards& trickCards) {
    // Find highest card played in lead suit
    Card highest = highestOfSuit(trickCards, leadSuit);

    // Try to play under if possible
    Cards validInSuit = cardsOfSuit(valid, leadSuit);
    Card under = highestBelow(validInSuit, highest.rank());

    if (under.isValid()) {
        return under;
    }

    // Must play over - play lowest to minimize winning future bad tricks
    return lowestCard(valid);
}

Card Player::aiSelectSlough(const Cards& valid) {
    // Medium difficulty: Dump dangerous cards with basic card counting
    // Priority: QoS, high spades (if Q♠ still out), high hearts, high cards

    // QoS first - always dump it if we have it
    for (const Card& c : valid) {
        if (c.isQueenOfSpades()) return c;
    }

    // Check if Q♠ is still out there using card memory
    bool qosOut = !m_cardMemory.queenSpadesPlayed && !hasCard(Card(Suit::Spades, Rank::Queen));

    // High spades (A, K) - especially dangerous if Q♠ is still out
    if (qosOut) {
        for (const Card& c : valid) {
            if (c.suit() == Suit::Spades && c.rank() == Rank::Ace) return c;
        }
        for (const Card& c : valid) {
            if (c.suit() == Suit::Spades && c.rank() == Rank::King) return c;
        }
    }

    // Highest heart
    Card highHeart = highestOfSuit(valid, Suit::Hearts);
    if (highHeart.isValid()) return highHeart;

    // High spades even if Q♠ is out (still somewhat dangerous)
    for (const Card& c : valid) {
        if (c.suit() == Suit::Spades && c.rank() >= Rank::King) return c;
    }

    // Highest card overall
    return highestCard(valid);
}

// ============================================================================
// HARD DIFFICULTY - Uses card counting and positional awareness
// ============================================================================

Card Player::aiSelectLeadHard(const Cards& valid, bool heartsBroken) {
    // Use card memory and game context for smarter decisions
    const CardMemory& mem = m_cardMemory;
    const GameContext& ctx = m_gameContext;

    // Check if Q♠ is still out there
    bool qosOut = !mem.queenSpadesPlayed && !hasCard(Card(Suit::Spades, Rank::Queen));

    // Strategic assessment: are we ahead or behind?
    int myScore = ctx.playerScores[m_id] + ctx.roundScores[m_id];
    int lowestOtherScore = 999;
    int highestOtherScore = 0;
    for (int i = 0; i < 4; ++i) {
        if (i != m_id) {
            int score = ctx.playerScores[i] + ctx.roundScores[i];
            if (score < lowestOtherScore) lowestOtherScore = score;
            if (score > highestOtherScore) highestOtherScore = score;
        }
    }
    bool amLeading = myScore < lowestOtherScore;
    bool amBehind = myScore > highestOtherScore;

    // If Q♠ is out and we have low spades, consider flushing it
    // Be more aggressive about flushing when we're behind
    if (qosOut) {
        Card lowSpade = lowestOfSuit(valid, Suit::Spades);
        if (lowSpade.isValid() && lowSpade.rank() < Rank::Queen) {
            int spadesPlayed = mem.countPlayedInSuit(Suit::Spades);
            // Flush if not too many spades played yet
            // More likely to flush when behind (want to hurt others)
            int flushThreshold = amBehind ? 8 : 6;
            if (spadesPlayed < flushThreshold) {
                return lowSpade;
            }
        }
    }

    // If we're leading by a lot, play very conservatively - lead lowest cards
    if (amLeading && (lowestOtherScore - myScore) > 15) {
        return lowestCard(valid);
    }

    // Lead from long suits where we have low cards (safe leads)
    for (Suit s : {Suit::Clubs, Suit::Diamonds}) {
        Cards suitCards = cardsOfSuit(valid, s);
        if (suitCards.size() >= 2) {
            Card lowest = lowestOfSuit(valid, s);
            if (lowest.isValid() && lowest.rank() <= Rank::Seven) {
                return lowest;
            }
        }
    }

    // If hearts broken and we have low hearts, can lead them
    if (heartsBroken) {
        Card lowHeart = lowestOfSuit(valid, Suit::Hearts);
        if (lowHeart.isValid() && lowHeart.rank() <= Rank::Six) {
            return lowHeart;
        }
    }

    // Safe low card from any suit
    for (Suit s : {Suit::Clubs, Suit::Diamonds}) {
        Card low = lowestOfSuit(valid, s);
        if (low.isValid()) return low;
    }

    // Low spade if safe
    Card lowSpade = lowestOfSuit(valid, Suit::Spades);
    if (lowSpade.isValid() && lowSpade.rank() < Rank::Jack) {
        return lowSpade;
    }

    // Hearts if broken
    if (heartsBroken) {
        Card low = lowestOfSuit(valid, Suit::Hearts);
        if (low.isValid()) return low;
    }

    return lowestCard(valid);
}

Card Player::aiSelectFollowHard(const Cards& valid, Suit leadSuit, const Cards& trickCards,
                                 const QVector<int>& trickPlayers) {
    Card highestPlayed = highestOfSuit(trickCards, leadSuit);
    int numCardsPlayed = trickCards.size();

    // Calculate trick points
    int trickPoints = 0;
    for (const Card& c : trickCards) {
        trickPoints += c.pointValue();
    }

    // Cards we can play in the lead suit
    Cards validInSuit = cardsOfSuit(valid, leadSuit);

    // If we're LAST to play (position 4)
    if (numCardsPlayed == 3) {
        if (trickPoints == 0) {
            // No points in trick - safe to win with highest under, or win if must
            Card under = highestBelow(validInSuit, highestPlayed.rank());
            if (under.isValid()) return under;
            // Must win - that's fine, no points
            return lowestCard(valid);
        } else {
            // Points in trick - try hard to duck
            Card under = highestBelow(validInSuit, highestPlayed.rank());
            if (under.isValid()) return under;
            // Must win points - minimize by playing lowest
            return lowestCard(valid);
        }
    }

    // If we're THIRD to play (position 3)
    if (numCardsPlayed == 2) {
        // Check if we know player 4 is void in this suit
        int lastPlayer = (m_id + 1) % 4;
        bool lastPlayerVoid = m_cardMemory.isPlayerVoid(lastPlayer, leadSuit);

        if (lastPlayerVoid && trickPoints == 0) {
            // Last player might dump points - try to not win
            Card under = highestBelow(validInSuit, highestPlayed.rank());
            if (under.isValid()) return under;
        }

        // Standard play - try to duck
        Card under = highestBelow(validInSuit, highestPlayed.rank());
        if (under.isValid()) return under;

        return lowestCard(valid);
    }

    // SECOND to play - be conservative
    Card under = highestBelow(validInSuit, highestPlayed.rank());
    if (under.isValid()) return under;

    return lowestCard(valid);
}

Card Player::aiSelectSloughHard(const Cards& valid, const Cards& trickCards,
                                 const QVector<int>& trickPlayers) {
    const GameContext& ctx = m_gameContext;

    // Calculate current trick points
    int trickPoints = 0;
    for (const Card& c : trickCards) {
        trickPoints += c.pointValue();
    }

    // Strategic assessment
    int myScore = ctx.playerScores[m_id] + ctx.roundScores[m_id];
    int lowestOtherScore = 999;
    int highestOtherScore = 0;
    int leaderId = 0;
    for (int i = 0; i < 4; ++i) {
        if (i != m_id) {
            int score = ctx.playerScores[i] + ctx.roundScores[i];
            if (score < lowestOtherScore) {
                lowestOtherScore = score;
                leaderId = i;
            }
            if (score > highestOtherScore) highestOtherScore = score;
        }
    }
    bool amBehind = myScore > highestOtherScore;

    // Check who's winning this trick
    int currentWinnerId = -1;
    if (!trickPlayers.isEmpty()) {
        // Find who played the highest card of lead suit
        Card highestInLead;
        Suit trickLeadSuit = trickCards[0].suit();
        for (int i = 0; i < trickCards.size(); ++i) {
            if (trickCards[i].suit() == trickLeadSuit) {
                if (!highestInLead.isValid() || trickCards[i].rank() > highestInLead.rank()) {
                    highestInLead = trickCards[i];
                    currentWinnerId = trickPlayers[i];
                }
            }
        }
    }

    // If trick already has points, dump high point cards
    if (trickPoints > 0) {
        // Q♠ first - this is huge
        for (const Card& c : valid) {
            if (c.isQueenOfSpades()) return c;
        }

        // Highest hearts next
        Card highHeart = highestOfSuit(valid, Suit::Hearts);
        if (highHeart.isValid()) return highHeart;
    }

    // If the leader is winning this trick, consider dumping points on them
    if (currentWinnerId == leaderId && trickPoints == 0 && amBehind) {
        // Try to dump points on the leader!
        for (const Card& c : valid) {
            if (c.isQueenOfSpades()) return c;
        }
        Card highHeart = highestOfSuit(valid, Suit::Hearts);
        if (highHeart.isValid()) return highHeart;
    }

    // Check if Q♠ is still out and we have A♠ or K♠
    bool qosOut = !m_cardMemory.queenSpadesPlayed;
    bool weHaveQoS = hasCard(Card(Suit::Spades, Rank::Queen));

    if (qosOut && !weHaveQoS) {
        // Dump A♠ to avoid catching Q♠ later
        for (const Card& c : valid) {
            if (c.suit() == Suit::Spades && c.rank() == Rank::Ace) return c;
        }
        // Dump K♠
        for (const Card& c : valid) {
            if (c.suit() == Suit::Spades && c.rank() == Rank::King) return c;
        }
    }

    // If we have Q♠, try to dump it when safe
    if (weHaveQoS) {
        for (const Card& c : valid) {
            if (c.isQueenOfSpades()) return c;
        }
    }

    // Dump from longest suit to work toward creating voids
    int suitCounts[4] = {0, 0, 0, 0};
    for (const Card& c : m_hand) {
        suitCounts[static_cast<int>(c.suit())]++;
    }

    // Find longest non-heart suit in valid cards
    Suit longestSuit = Suit::Clubs;
    int longestCount = 0;
    for (const Card& c : valid) {
        if (c.suit() != Suit::Hearts) {
            int count = suitCounts[static_cast<int>(c.suit())];
            if (count > longestCount) {
                longestCount = count;
                longestSuit = c.suit();
            }
        }
    }

    // Dump highest from longest suit
    if (longestCount > 0) {
        Card high = highestOfSuit(valid, longestSuit);
        if (high.isValid()) return high;
    }

    // Fallback: highest hearts, then highest overall
    Card highHeart = highestOfSuit(valid, Suit::Hearts);
    if (highHeart.isValid()) return highHeart;

    return highestCard(valid);
}
