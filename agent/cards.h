#ifndef _CARDS_H
#define _CARDS_H

#include <vector>
#include <cstdio>
#include <cinttypes>

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

using Cards = std::vector<uint8_t>;

struct CardState {
    Cards holeCards[2];
    Cards boardCards;

    CardState() { }

    CardState(const Cards& hc) {
        holeCards[0] = hc;
    }

    CardState(const Cards& hc, const Cards& bc) {
        holeCards[0] = hc;
        boardCards = bc;
    }

    CardState(const Cards& hc0, const Cards& hc1, const Cards& bc) {
        holeCards[0] = hc0;
        holeCards[1] = hc1;
        boardCards = bc;
    }

    void print() const;
    uint64_t getMask() const;
    int getHandValue(int player = 0) const;

    void sampleHoleCard(int player, int size);
    void sampleBoardCard(int size);
    void sampleAll();
};

void sample(Cards& cards, uint64_t& mask);

#endif
