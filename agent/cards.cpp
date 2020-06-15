#include <bit>
#include <cstdio>
#include "cards.h"

void CardState::print() const {
    printf("holeCards[0]: ");
    Cards mask;
    mask = holeCards[0];
    while (mask) {
        uint8_t c = __builtin_ctzll(mask);
        mask ^= 1ll << c;
        printCard(c);
        putchar(' ');
    }
    printf("\nholeCards[1]: ");
    mask = holeCards[1];
    while (mask) {
        uint8_t c = __builtin_ctzll(mask);
        mask ^= 1ll << c;
        printCard(c);
        putchar(' ');
    }
    printf("\nboardCards: ");
    mask = boardCards;
    while (mask) {
        uint8_t c = __builtin_ctzll(mask);
        mask ^= 1ll << c;
        printCard(c);
        putchar(' ');
    }
    printf("\n");
}
