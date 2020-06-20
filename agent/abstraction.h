#ifndef _ABSTRACTION_H
#define _ABSTRACTION_H

#include <cmath>
#include <cstring>
#include "cards.h"
#include "game.h"


const int EHS_SAMPLE = 100;
const int PEHS_SAMPLE = 100;
const int CARD_ABS_SIZE = 100;
const int STACK_ABS_SIZE = 10;
const int STACK_ABS_LIST[STACK_ABS_SIZE] = {100, 200, 400, 800, 1600, 3200, 6400, 12800, 20000, 20001};
const int ABS_SIZE = 4 * CARD_ABS_SIZE * STACK_ABS_SIZE * STACK_ABS_SIZE * 2 * 2 * 2;

// Posterior EHS Distribution
struct PEHS {
    static const int BUCKET_SIZE = 10;
    double x[BUCKET_SIZE];

    PEHS() {
        memset(x, 0, sizeof(0));
    }

    PEHS(double* a, int* b) {
        for (int i = 0; i < BUCKET_SIZE; ++i)
            x[i] = (b[i] > 0 ? a[i] / b[i] : 1. - (double)i / BUCKET_SIZE) ;
    }

    void scanf(FILE* f) {
        for (int i = 0; i < BUCKET_SIZE; ++i)
            fscanf(f, "%lf", &x[i]);
    }

    void print() const {
        for (int i = 0; i < BUCKET_SIZE; ++i)
            printf("%.3f ", x[i]);
        printf("\n");
    }

    PEHS operator+= (const PEHS& rhs) {
        for (int i = 0; i < BUCKET_SIZE; ++i)
            x[i] += rhs.x[i];
        return *this;
    }

    PEHS operator/ (double rhs) const {
        PEHS result;
        for (int i = 0; i < BUCKET_SIZE; ++i)
            result.x[i] = x[i] / rhs;
        return result;
    }

    double distance2(const PEHS& rhs) const {
        double d = 0;
        for (int i = 0; i < PEHS::BUCKET_SIZE; ++i)
            d += (x[i] - rhs.x[i]) * (x[i] - rhs.x[i]);
        return d;
    }

    double distance(const PEHS& rhs) const {
        return std::sqrt(distance2(rhs));
    }
};

void clearEHS();
void initAbstraction(std::string path, int n1 = EHS_SAMPLE, int n2 = PEHS_SAMPLE);
double getEHS(CardState state, int player, bool posterior = false, int num_sample = -1);
PEHS getPEHS(CardState state, int player, int num_sample = -1, int num_ehs_sample = -1);

int getCardAbstraction(const CardState& state, int round, int player);

inline int getStackAbstraction(int stack) {
    for (int i = 0; i < STACK_ABS_SIZE; ++i)
        if (STACK_ABS_LIST[i] > stack)
            return i;
    assert(false);
}

inline int getGameAbstraction(const GameState& game) {
    assert(game.hasAction());
    int card_abs = getCardAbstraction(game.cards, game.round, game.player);
    int stack0_abs = getStackAbstraction(game.spent[0]);
    int stack1_abs = getStackAbstraction(game.spent[1]);
    int raised0 = game.raised[0];
    int raised1 = game.raised[1];
    assert(card_abs < CARD_ABS_SIZE);
    assert(stack0_abs < STACK_ABS_SIZE);
    assert(stack1_abs < STACK_ABS_SIZE);
    // printf("Game abstraction: %d %d %d %d %d\n", game.round, card_abs, stack0_abs, stack1_abs, game.player);
    return game.round * (CARD_ABS_SIZE * STACK_ABS_SIZE * STACK_ABS_SIZE * 2 * 2 * 2)
        + card_abs * (STACK_ABS_SIZE * STACK_ABS_SIZE * 2 * 2 * 2)
        + stack0_abs * (STACK_ABS_SIZE * 2 * 2 * 2)
        + stack1_abs * (2 * 2 * 2)
        + raised0 * (2 * 2)
        + raised1 * 2
        + game.player;
}

const int BET_ABS_SIZE = 3;     // without all-in
const int ACTION_SIZE = 3 + BET_ABS_SIZE;
const double BET_ABS_LIST[BET_ABS_SIZE] = {1.0, 0.7, -1}; // relative to pot size, must be sorted decreasingly

Action getAction(const GameState& game, int action_id, bool conservative = false);

inline int getNumAction(const GameState& game) {
    assert(game.hasAction());
    int n = 2;                  // fold & call
    if (game.canAllin())
        n += 1 + BET_ABS_SIZE;  // all-in
    return n;
}

#endif
