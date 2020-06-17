#ifndef _ABSTRACTION_H
#define _ABSTRACTION_H

#include <cmath>
#include "cards.h"
#include "game.h"


const int EHS_SAMPLE = 100;
const int CARD_ABS_BLOCK_MAX = 50;
const int CARD_ABS_SIZE = CARD_ABS_BLOCK_MAX + 1;
const int STACK_ABS_SIZE = 10;
const int STACK_ABS_LIST[STACK_ABS_SIZE] = {100, 200, 400, 800, 1600, 3200, 6400, 12800, 20000, 20001};
const int ABS_SIZE = 4 * CARD_ABS_SIZE * STACK_ABS_SIZE * STACK_ABS_SIZE * 2;

void clearEHS();
double getEHS(CardState state, int player);

inline int getCardAbstraction(const CardState& state, int player) {
    return std::round(getEHS(state, player) * CARD_ABS_BLOCK_MAX);
}

inline int getStackAbstraction(int stack) {
    for (int i = 0; i < STACK_ABS_SIZE; ++i)
        if (STACK_ABS_LIST[i] > stack)
            return i;
    abort();
}

inline int getGameAbstraction(const GameState& game) {
    assert(game.hasAction());
    int card_abs = getCardAbstraction(game.cards, game.player);
    int stack0_abs = getStackAbstraction(game.spent[0]);
    int stack1_abs = getStackAbstraction(game.spent[1]);
    assert(card_abs < CARD_ABS_SIZE);
    assert(stack0_abs < STACK_ABS_SIZE);
    assert(stack1_abs < STACK_ABS_SIZE);
    // printf("Game abstraction: %d %d %d %d %d\n", game.round, card_abs, stack0_abs, stack1_abs, game.player);
    return game.round * CARD_ABS_SIZE * STACK_ABS_SIZE * STACK_ABS_SIZE * 2
        + card_abs * STACK_ABS_SIZE * STACK_ABS_SIZE * 2
        + stack0_abs * STACK_ABS_SIZE * 2
        + stack1_abs * 2
        + game.player;
}

const int BET_ABS_SIZE = 3;     // without all-in
const int ACTION_SIZE = 3 + BET_ABS_SIZE;
const double BET_ABS_LIST[BET_ABS_SIZE] = {1.0, 0.5}; // relative to pot size, must be sorted decreasingly

Action getAction(const GameState& game, int action_id);

inline int getNumAction(const GameState& game) {
    assert(game.hasAction());
    int n = 2;                  // fold & call
    if (game.canAllin())
        n += 1 + BET_ABS_SIZE;  // all-in
    // if (game.canRaise())
    //     n += BET_ABS_SIZE;
    return n;
}

#endif
