#ifndef _CARDS_H
#define _CARDS_H

#include <cstdio>
#include <cinttypes>
#include "random.h"
#include "evalHandTables"

const int MAX_CARD = 51;

inline int getRank(uint8_t card) {
    return card >> 2;
}

inline int getSuit(uint8_t card) {
    return card & 3;
}

inline uint8_t makeCard(int rank, int suit) {
    return (rank << 2) | suit;
}

inline void printCard(uint8_t card) {
    static const char* suitChars = "cdhs";
    static const char* rankChars = "23456789TJQKA";
    putchar(rankChars[getRank(card)]);
    putchar(suitChars[getSuit(card)]);
}

using Cards = uint64_t;

inline void sample(Cards& cards, Cards& mask) {
    uint8_t c = Random::randint(0, MAX_CARD);
    while (mask & (1ll << c))
        c = Random::randint(0, MAX_CARD);
    cards |= (1ll << c);
    mask |= (1ll << c);
}

inline Cards getMask(uint8_t* cards, int n) {
    Cards mask = 0;
    for (int i = 0; i < n; ++i)
        mask |= (1ll << cards[i]);
    return mask;
}

struct CardState {
    Cards holeCards[2];
    Cards boardCards;

    CardState() {
        holeCards[0] = holeCards[1] = boardCards = 0;
    }

    CardState(Cards hc) {
        holeCards[0] = hc;
        holeCards[1] = boardCards = 0;
    }

    CardState(Cards hc, Cards bc) {
        holeCards[0] = hc;
        holeCards[1] = 0;
        boardCards = bc;
    }

    CardState(Cards hc0, Cards hc1, Cards bc) {
        holeCards[0] = hc0;
        holeCards[1] = hc1;
        boardCards = bc;
    }

    bool operator== (const CardState& rhs) const {
        return holeCards[0] == rhs.holeCards[0]
            && holeCards[1] == rhs.holeCards[1]
            && boardCards == rhs.boardCards;
    }

    void print() const;
    uint64_t getMask() const;
    uint64_t getHolecardMask(int player) const;
    uint64_t getBoardcardMask() const;
    int getHandValue(int player) const;

    void sampleHoleCard(int player, int size);
    void sampleBoardCard(int size);
    void sampleAll();
};

inline Cards CardState::getMask() const {
    return holeCards[0] | holeCards[1] | boardCards;
}

inline Cards CardState::getHolecardMask(int player) const {
    return holeCards[player];
}

inline Cards CardState::getBoardcardMask() const {
    return boardCards;
}

inline int CardState::getHandValue(int player) const {
    Cardset set = emptyCardset();
    Cards mask = holeCards[player] | boardCards;
    while (mask) {
        uint8_t c = __builtin_ctzll(mask);
        mask ^= 1ll << c;
        addCardToCardset(&set, getSuit(c), getRank(c));
    }
    return rankCardset(set);
}

inline void CardState::sampleHoleCard(int player, int size) {
    uint64_t mask = getMask();
    for (int i = 0; i < size; ++i)
        sample(holeCards[player], mask);
}

inline void CardState::sampleBoardCard(int size) {
    uint64_t mask = getMask();
    for (int i = 0; i < size; ++i)
        sample(boardCards, mask);
}

inline void CardState::sampleAll() {
    uint64_t mask = getMask();
    for (int i = __builtin_popcountll(holeCards[0]); i < 2; ++i)
        sample(holeCards[0], mask);
    for (int i = __builtin_popcountll(holeCards[1]); i < 2; ++i)
        sample(holeCards[1], mask);
    for (int i = __builtin_popcountll(boardCards); i < 5; ++i)
        sample(boardCards, mask);
}

#endif
