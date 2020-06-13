#include <bit>
#include <cstdio>
#include "cards.h"
#include "random.h"
#include "evalHandTables"

void CardState::print() const {
    printf("holeCards[0]: ");
    for (auto c: holeCards[0]) {
        printCard(c);
        putchar(' ');
    }
    printf("\nholeCards[1]: ");
    for (auto c: holeCards[1]) {
        printCard(c);
        putchar(' ');
    }
    printf("\nboardCards: ");
    for (auto c: boardCards) {
        printCard(c);
        putchar(' ');
    }
    printf("\n");
}

uint64_t CardState::getMask() const {
    uint64_t mask = 0;
    for (auto c: holeCards[0])
        mask |= 1 << c;
    for (auto c: holeCards[1])
        mask |= 1 << c;
    for (auto c: boardCards)
        mask |= 1 << c;
    return mask;
}

int CardState::getHandValue(int player) const {
    Cardset set = emptyCardset();
    for (auto c: holeCards[player])
        addCardToCardset(&set, getSuit(c), getRank(c));
    for (auto c: boardCards)
        addCardToCardset(&set, getSuit(c), getRank(c));
    return rankCardset(set);
}

void CardState::sampleHoleCard(int player, int size) {
    uint64_t mask = getMask();
    for (int i = 0; i < size; ++i)
        sample(holeCards[player], mask);
}

void CardState::sampleBoardCard(int size) {
    uint64_t mask = getMask();
    for (int i = 0; i < size; ++i)
        sample(boardCards, mask);
}

void CardState::sampleAll() {
    uint64_t mask = getMask();
    for (int i = holeCards[0].size(); i < 2; ++i)
        sample(holeCards[0], mask);
    for (int i = holeCards[1].size(); i < 2; ++i)
        sample(holeCards[1], mask);
    for (int i = boardCards.size(); i < 5; ++i)
        sample(boardCards, mask);
}

void sample(Cards& cards, uint64_t& mask) {
    uint8_t c = Random::randint(0, MAX_CARD);
    while (mask & (1 << c))
        c = Random::randint(0, MAX_CARD);
    cards.push_back(c);
    mask |= (1 << c);
}
