#include <algorithm>
#include <utility>
#include <unordered_map>
#include <cstdio>
#include <cassert>
#include <cmath>

#include "abstraction.h"
#include "random.h"

static std::unordered_map<Cards, double> EHS_table;

void clearEHS() {
    EHS_table.clear();
}

// Expected Hand Strength
double getEHS(CardState state, int player) {
    Cards c = state.getMask(player);
    auto it = EHS_table.find(c);
    if (it != EHS_table.end())
        return it->second;

    double hs = 0;
    state.holeCards[player ^ 1] = 0;
    for (int i = 0; i < EHS_SAMPLE; ++i) {
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

    return EHS_table[c] = hs / EHS_SAMPLE;
    // return hs / EHS_SAMPLE;
}

Action getAction(const GameState& game, int action_id) {
    assert(game.hasAction());

    Action action;
    if (action_id == 0 && game.canFold()) {
        action.type = FOLD;
        action.bet = 0;
    } else if (action_id <= 1) {
        action.type = CALL;
        action.bet = 0;
    } else if (action_id == 2 || !game.canRaise()) { // treat as all-in
        assert(game.canAllin());
        action.type = ALLIN;
        action.bet = 0;
    } else {
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

// int main() {
//     CardState s((1ll << makeCard(11, 0)) | (1ll << makeCard(10, 0)));
//     s.print();
//     printf("%.5f %d\n", getEHS(s, 0), getCardAbstraction(s, 0));

//     s.boardCards = (1ll << makeCard(4, 0)) | (1ll << makeCard(2, 1)) | (1ll << makeCard(6, 0)) | (1ll << makeCard(9, 0)) | (1ll << makeCard(12, 1));
//     s.sampleAll();
//     s.print();
//     printf("ABS_SIZE = %d\n", CARD_ABS_SIZE);
//     printf("%.9f %d\n", getEHS(s, 0), getCardAbstraction(s, 0));

//     GameState g;

//     g.sampleCard();

//     g.print();
//     printf("%d\n\n", getGameAbstraction(g));
//     g.doAction(getAction(g, 1));

//     g.print();
//     printf("%d\n\n", getGameAbstraction(g));
//     g.doAction(getAction(g, 1));

//     g.print();
//     g.sampleCard();

//     g.print();
//     printf("%d\n\n", getGameAbstraction(g));
//     g.doAction(getAction(g, 1));

//     g.print();
//     printf("%d\n\n", getGameAbstraction(g));
//     g.doAction(getAction(g, 3));

//     g.print();
//     printf("%d\n\n", getGameAbstraction(g));
//     g.doAction(getAction(g, 3));

//     g.print();
//     printf("%d\n\n", getGameAbstraction(g));
//     g.doAction(getAction(g, 1));

//     g.print();
//     g.sampleCard();

//     g.print();
//     printf("%d\n\n", getGameAbstraction(g));
//     g.doAction(getAction(g, 4));

//     g.print();
//     printf("%d\n\n", getGameAbstraction(g));
//     g.doAction(getAction(g, 1));

//     g.print();
//     g.sampleCard();

//     g.print();
//     printf("GABS=%d NACT=%d\n", getGameAbstraction(g), getNumAction(g));
//     g.doAction(getAction(g, 1));

//     g.print();
//     printf("GABS=%d NACT=%d\n", getGameAbstraction(g), getNumAction(g));
//     g.doAction(getAction(g, 3));

//     g.print();
//     printf("GABS=%d NACT=%d\n", getGameAbstraction(g), getNumAction(g));
//     g.doAction(getAction(g, 3));

//     g.print();
//     printf("GABS=%d NACT=%d\n", getGameAbstraction(g), getNumAction(g));
//     g.doAction(getAction(g, 2));

//     g.print();
//     printf("GABS=%d NACT=%d\n", getGameAbstraction(g), getNumAction(g));
//     g.doAction(getAction(g, 1));


//     g.print();
//     printf("%d\n", g.isTerminal());
//     printf("%d\n", g.getValue());

//     printf("ABS_SIZE=%d\n", ABS_SIZE);
// }