#include <algorithm>
#include <utility>
#include <unordered_map>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cmath>
#include "abstraction.h"
#include "random.h"

struct CardStateHash {
    size_t operator()(const CardState& s) const {
        return s.getMask();
    }
};

static std::unordered_map<CardState, double, CardStateHash> EHS_table[2];
static std::unordered_map<CardState, int, CardStateHash> CABS_table[2];
static PEHS centroids[4][CARD_ABS_SIZE];
static int num_centroids[4];
// static int preflop_map[MAX_CARD + 1][MAX_CARD + 1];
static int num_EHS_sample, num_PEHS_sample;
// static bool ready;

void clearEHS() {
    EHS_table[0].clear();
    EHS_table[1].clear();
    CABS_table[0].clear();
    CABS_table[1].clear();
}

void initAbstraction(std::string path, int n1, int n2) {
    const std::string file_name[4] = {"preflop.abs", "flop.abs", "turn.abs", "river.abs"};
    for (int i = 0; i < 4; ++i) {
        FILE* f = fopen((path + file_name[i]).c_str(), "r");
        fscanf(f, "%d", &num_centroids[i]);
        assert(num_centroids[i] > 0 && num_centroids[i] <= CARD_ABS_SIZE);
        for (int j = 0; j < num_centroids[i]; ++j)
            centroids[i][j].scanf(f);
        fclose(f);
    }

    // ready = false;
    // if (init_preflop) {
    //     num_EHS_sample = 1000;
    //     num_PEHS_sample = 1000;

    //     for (int i = 0; i <= MAX_CARD; ++i) {
    //         for (int j = 0; j < i; ++j) {
    //             CardState s1;
    //             s1.holeCards[0] = (1ll << i) | (1ll << j);
    //             preflop_map[i][j] = getCardAbstraction(s1, 0, 0);
    //         }
    //     }
    //     ready = true;
    // }

    num_EHS_sample = n1;
    num_PEHS_sample = n2;
}

// Expected Hand Strength
double getEHS(CardState state, int player, bool posterior, int num_sample) {
    if (!posterior)
        state.holeCards[player ^ 1] = 0;

    auto it = EHS_table[posterior].find(state);
    if (it != EHS_table[posterior].end())
        return (player == 0 ? it->second : 1. - it->second);

    if (num_sample == -1)
        num_sample = num_EHS_sample;

    double hs = 0;
    for (int i = 0; i < num_sample; ++i) {
        CardState s0 = state;
        s0.sampleAll();
        int v0 = s0.getHandValue(player);
        int v1 = s0.getHandValue(player ^ 1);
        if (v0 > v1)
            hs += 1;
        else if (v0 == v1)
            hs += 0.5;
    }

    double ev = hs / num_sample;
    EHS_table[posterior][state] = (player == 0 ? ev : 1. - ev);
    return ev;
}

PEHS getPEHS(CardState state, int player, int num_sample, int num_ehs_sample) {
    state.holeCards[player ^ 1] = 0;

    if (num_sample == -1)
        num_sample = num_PEHS_sample;

    double a[PEHS::BUCKET_SIZE];
    int b[PEHS::BUCKET_SIZE];
    memset(a, 0, sizeof(a));
    memset(b, 0, sizeof(b));

    for (int i = 0; i < num_sample; ++i) {
        CardState s0 = state;
        s0.sampleHoleCard(player ^ 1, 2);
        double ehs0 = getEHS(s0, player ^ 1, false, num_ehs_sample);
        double ehs1 = getEHS(s0, player, true, num_ehs_sample);
        int idx = (ehs0 - 1e-5) * PEHS::BUCKET_SIZE;
        // assert(idx >= 0 && idx < PEHS::BUCKET_SIZE);
        a[idx] += ehs1;
        b[idx] += 1;
    }

    return PEHS(a, b);
}

int getCardAbstraction(const CardState& state, int round, int player) {
    assert(num_centroids[round] > 0 && num_centroids[round] <= CARD_ABS_SIZE);

    // if (ready && round == 0) {
    //     Cards mask = state.holeCards[player];
    //     uint8_t a = __builtin_ctzll(mask);
    //     uint8_t b = __builtin_ctzll(mask ^ (1ll << a));
    //     return preflop_map[b][a];
    // }

    auto it = CABS_table[player].find(state);
    if (it != CABS_table[player].end())
        return it->second;

    PEHS p = getPEHS(state, player);
    double min = p.distance(centroids[round][0]);
    int idx = 0;
    for (int i = 1; i < num_centroids[round]; ++i) {
        double d = p.distance(centroids[round][i]);
        if (d < min) {
            min = d;
            idx = i;
        }
    }
    // printf("%d: ", idx); p.print();

    return CABS_table[player][state] = idx;
}

Action getAction(const GameState& game, int action_id, bool conservative) {
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

        int bet;
        if (BET_ABS_LIST[action_id] < 0) { // uniform random
            if (!conservative)
                bet = rand() % (game.maxRaise() - game.minRaise() + 1) + game.minRaise();
            else
                bet = game.minRaise();
        }
        else
            bet = int(game.pot() * BET_ABS_LIST[action_id]);
        if (bet > game.maxRaise())
            bet = game.maxRaise();
        else if (bet < game.minRaise())
            bet = game.minRaise();

        action.type = RAISE;
        action.bet = bet;
    }

    return action;
}

/*
int main() {
    srand(time(NULL));

    initAbstraction("", 1000, 1000);

    CardState s1;
    s1.holeCards[0] = (1ll << makeCard(0, 2)) | (1ll << makeCard(12, 2));
    s1.holeCards[1] = (1ll << makeCard(7, 0)) | (1ll << makeCard(10, 0)) | (1ll << makeCard(9, 1));
    s1.print();
    printf("%.5f\n", getEHS(s1, 0));
    int c1 = getCardAbstraction(s1, 0, 0);
    printf("1ABS %d: ", c1); centroids[0][c1].print();

    // for (int i = 0; i < 10; ++i) {
    //     CardState s1;
    //     s1.sampleHoleCard(0, 2);
    //     s1.sampleHoleCard(1, 2);
    //     // s1.sampleAll();
    //     // s1.holeCards[1] = (1ll << makeCard(8, 0)) | (1ll << makeCard(10, 1));
    //     // s1.boardCards = (1ll << makeCard(5, 2)) | (1ll << makeCard(7, 2)) | (1ll << makeCard(10, 2));
    //     s1.print();
    //     printf("%.5f %.5f\n", getEHS(s1, 0), getEHS(s1, 1));

    //     CardState s2;
    //     s2.sampleHoleCard(0, 2);
    //     s2.sampleHoleCard(1, 2);
    //     // s2.sampleAll();
    //     // s2.holeCards[0] = (1ll << makeCard(3, 0)) | (1ll << makeCard(7, 1));
    //     // s2.boardCards = (1ll << makeCard(1, 1)) | (1ll << makeCard(3, 1)) | (1ll << makeCard(5, 1));
    //     s2.print();
    //     printf("%.5f %.5f\n", getEHS(s2, 0), getEHS(s2, 1));

    //     int c1 = getCardAbstraction(s1, 0, 0);
    //     int c2 = getCardAbstraction(s2, 0, 0);
    //     PEHS p1 = getPEHS(s1, 0);
    //     PEHS p2 = getPEHS(s2, 0);
    //     p1.print();
    //     printf("ABS %d: ", c1); centroids[1][c1].print();
    //     p2.print();
    //     printf("ABS %d: ", c2); centroids[1][c2].print();
    //     printf("\n");
    // }

    // for (int i = 0; i < 10; ++i) {
    //     PEHS t = getPEHS(s1, 0);
    //     printf("\t%d dist=%.5f %.5f\n", getCardAbstraction(s1, 0, 0), p1.distance(t), p2.distance(t));
    // }
    // for (int i = 0; i < 10; ++i) {
    //     PEHS t = getPEHS(s2, 0);
    //     printf("\t%d dist=%.5f %.5f\n", getCardAbstraction(s2, 0, 0), p1.distance(t), p2.distance(t));
    // }

    // s.boardCards = (1ll << makeCard(4, 0)) | (1ll << makeCard(2, 1)) | (1ll << makeCard(6, 0)) | (1ll << makeCard(9, 0)) | (1ll << makeCard(12, 1));
    // s.sampleAll();
    // s.print();
    // printf("ABS_SIZE = %d\n", CARD_ABS_SIZE);
    // printf("%.9f %d\n", getEHS(s, 0), getCardAbstraction(s, 0));

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
}
*/