#include <algorithm>
#include <utility>
#include <map>
#include <cstdio>
#include <cassert>
#include <cmath>

#include "abstraction.h"
#include "random.h"

static std::map<std::pair<Cards, Cards>, float> EHS_table;

// Expected Hand Strength
float getEHS(CardState state, int player, int numSample) {
    std::pair<Cards, Cards> c = std::make_pair(state.holeCards[player], state.boardCards);
    auto it = EHS_table.find(c);
    if (it != EHS_table.end())
        return it->second;

    float hs = 0;
    state.holeCards[player ^ 1].clear();
    for (int i = 0; i < numSample; ++i) {
        CardState s0(state);
        s0.sampleAll();
        // s0.print();
        int v0 = s0.getHandValue(player);
        int v1 = s0.getHandValue(player ^ 1);
        if (v0 > v1)
            hs += 1;
        else if (v0 == v1)
            hs += 0.5;
    }

    return EHS_table[c] = hs / numSample;
}

void clearEHS() {
    EHS_table.clear();
}

int getCardAbstraction(const CardState& state, int player, int round_hint, int blockSize) {
    int vPreFlop = std::ceil(getEHS(CardState(state.holeCards[player])) * blockSize);
    int vFlop = state.boardCards.size() >= 1 && round_hint >= 1
        ? std::ceil(getEHS(CardState(state.holeCards[player], Cards(state.boardCards.begin(), state.boardCards.begin() + 1))) * blockSize)
        : 0;
    int vTurn = state.boardCards.size() >= 2 && round_hint >= 2
        ? std::ceil(getEHS(CardState(state.holeCards[player], Cards(state.boardCards.begin(), state.boardCards.begin() + 2))) * blockSize)
        : 0;
    int vRiver = state.boardCards.size() >= 5 && round_hint >= 3
        ? std::ceil(getEHS(CardState(state.holeCards[player], Cards(state.boardCards.begin(), state.boardCards.begin() + 5))) * blockSize)
        : 0;
    return vPreFlop + (blockSize + 1) * (vFlop + (blockSize + 1) * (vTurn + (blockSize + 1) * vRiver));
}

Action getAction(const GameState& game, int action_id) {
    assert(game.hasAction());

    Action action;
    if (action_id == 0)
        action.type = FOLD;
    else if (action_id == 1)
        action.type = CALL;
    else if (action_id == 2) {
        assert(game.canAllin());
        action.type = ALLIN;
    } else {
        assert(game.canRaise());
        action_id -= 3;
        assert(0 <= action_id && action_id < BET_ABS_SIZE);
        int bet = int(game.pot() * BET_ABS_LIST[action_id]);
        if (bet > game.maxRaise()) {
            // printf("WARNING: raise too big, forcing maxRaise");
            bet = game.maxRaise();
        } else if (bet < game.minRaise()) {
            // printf("WARNING: raise too small, forcing minRaise");
            bet = game.minRaise();
        }
        action.type = RAISE;
        action.bet = bet;
    }
    return action;
}

int getNumAction(const GameState& game) {
    assert(game.hasAction());
    int n = 2;                  // fold & call
    if (game.canAllin())
        n += 1;                 // all-in
    if (game.canRaise())
        n += BET_ABS_SIZE;
    return n;
}

int getGameAbstraction(const GameState& game) {
    assert(game.hasAction());
    int card_abs = getCardAbstraction(game.cards, game.player, game.round);
    int stack0_abs = game.spent[0] / STACK_DIVISOR;
    int stack1_abs = game.spent[1] / STACK_DIVISOR;
    // printf("Game abstraction: %d %d %d %d\n", card_abs, stack0_abs, stack1_abs, game.player);
    return card_abs * (STACK_ABS_SIZE + 1) * (STACK_ABS_SIZE + 1) * 2
        + stack0_abs * (STACK_ABS_SIZE + 1) * 2
        + stack1_abs * 2
        + game.player;
}

/*
int main() {
    CardState s({makeCard(11, 0), makeCard(10, 0)});
    s.print();
    printf("%.5f %d\n", getEHS(s, 0), getCardAbstraction(s, 0));

    s.boardCards = {makeCard(4, 0), makeCard(2, 1), makeCard(6, 0), makeCard(9, 0), makeCard(12, 1)};
    s.sampleAll();
    s.print();
    printf("ABS_SIZE = %d\n", CARD_ABS_SIZE);
    printf("%.9f %d\n", getEHS(s, 0), getCardAbstraction(s, 0));

    GameState g;
    g.presampleCard();

    g.sampleCard();

    g.print();
    printf("%d\n\n", getGameAbstraction(g));
    g.doAction(getAction(g, 1));

    g.print();
    printf("%d\n\n", getGameAbstraction(g));
    g.doAction(getAction(g, 1));

    g.print();
    g.sampleCard();

    g.print();
    printf("%d\n\n", getGameAbstraction(g));
    g.doAction(getAction(g, 1));

    g.print();
    printf("%d\n\n", getGameAbstraction(g));
    g.doAction(getAction(g, 3));

    g.print();
    printf("%d\n\n", getGameAbstraction(g));
    g.doAction(getAction(g, 3));

    g.print();
    printf("%d\n\n", getGameAbstraction(g));
    g.doAction(getAction(g, 1));

    g.print();
    g.sampleCard();

    g.print();
    printf("%d\n\n", getGameAbstraction(g));
    g.doAction(getAction(g, 4));

    g.print();
    printf("%d\n\n", getGameAbstraction(g));
    g.doAction(getAction(g, 1));

    g.print();
    g.sampleCard();

    g.print();
    printf("GABS=%d NACT=%d\n", getGameAbstraction(g), getNumAction(g));
    g.doAction(getAction(g, 1));

    g.print();
    printf("GABS=%d NACT=%d\n", getGameAbstraction(g), getNumAction(g));
    g.doAction(getAction(g, 3));

    g.print();
    printf("GABS=%d NACT=%d\n", getGameAbstraction(g), getNumAction(g));
    g.doAction(getAction(g, 3));

    g.print();
    printf("GABS=%d NACT=%d\n", getGameAbstraction(g), getNumAction(g));
    g.doAction(getAction(g, 2));

    g.print();
    printf("GABS=%d NACT=%d\n", getGameAbstraction(g), getNumAction(g));
    g.doAction(getAction(g, 1));


    g.print();
    printf("%d\n", g.isTerminal());
    printf("%d\n", g.getValue());

    printf("ABS_SIZE=%d\n", ABS_SIZE);
}
*/