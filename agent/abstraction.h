#ifndef _ABSTRACTION_H
#define _ABSTRACTION_H

#include "cards.h"
#include "game.h"

const int EHS_SAMPLE = 1000;
const int CARD_ABS_BLOCK_MAX = 10;
const int CARD_ABS_SIZE = CARD_ABS_BLOCK_MAX + 1;

float getEHS(CardState state, int player = 0, int numSample = EHS_SAMPLE);
void clearEHS();

int getCardAbstraction(const CardState& state, int player, int round_hint = 4, int block_size = CARD_ABS_BLOCK_MAX);

const int STACK_ABS_MAX = 200;
const int STACK_ABS_SIZE = STACK_ABS_MAX + 1;
const int STACK_DIVISOR = STACK_SIZE / STACK_ABS_MAX;

const int BET_ABS_SIZE = 2;     // without all-in
const int ACTION_SIZE = 3 + BET_ABS_SIZE;
const float BET_ABS_LIST[BET_ABS_SIZE] = {1.0, 0.8}; // relative to pot size, must be sorted decreasingly

const int ABS_SIZE = 4 * CARD_ABS_SIZE * STACK_ABS_SIZE * STACK_ABS_SIZE * 2;

Action getAction(const GameState& game, int action_id);
int getNumAction(const GameState& game);
int getGameAbstraction(const GameState& game);

#endif
