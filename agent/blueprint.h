#ifndef _BLUEPRINT_H
#define _BLUEPRINT_H

#include <string>
#include "game.h"
#include "abstraction.h"

struct Blueprint {
    double regret[ABS_SIZE][ACTION_SIZE];
    double sigma[ABS_SIZE][ACTION_SIZE];
    double sum_sigma[ABS_SIZE][ACTION_SIZE];
    long long num_visit[ABS_SIZE];

    int sample_player;
    int cnt;
    long long cur_cnt;
    int iter;
    int decay_iter;

    void init();
    void load(std::string file);
    void save(std::string file) const;
    void train(int num_iter, std::string save_file);

    double getExploitability(int player);
    double mccfr(GameState g);
    double sample(GameState g);

    void decayRegret();
    Action sampleAction(const GameState &g, bool purification = false, bool always_fold = false) const;
};

inline void Blueprint::load(std::string file) {
    FILE* f = fopen(file.c_str(), "r");
    fscanf(f, "%d", &decay_iter);
    for (int s = 0; s < ABS_SIZE; ++s) {
        fscanf(f, "%lld", &num_visit[s]);
        for (int a = 0; a < ACTION_SIZE; ++a)
            fscanf(f, "%lf%lf%lf", &regret[s][a], &sigma[s][a], &sum_sigma[s][a]);
    }
    fclose(f);
}

inline void Blueprint::save(std::string file) const {
    FILE* f = fopen(file.c_str(), "w");
    fprintf(f, "%d\n", decay_iter);
    for (int s = 0; s < ABS_SIZE; ++s) {
        fprintf(f, "%lld\n", num_visit[s]);
        for (int a = 0; a < ACTION_SIZE; ++a)
            fprintf(f, "%.4f %.4f %.4f\n", regret[s][a], sigma[s][a], sum_sigma[s][a]);
    }
    fclose(f);
}

inline void normalize(int n, double p[]) {
    double s = 0;
    for (int i = 0; i < n; ++i)
        s += p[i];
    assert(s > 0);
    for (int i = 0; i < n; ++i)
        p[i] /= s;
}

inline Action Blueprint::sampleAction(const GameState& g, bool purification, bool always_fold) const {
    assert(g.hasAction());

    double avg_sigma[ACTION_SIZE];
    int infoset = getGameAbstraction(g);
    int num_action = getNumAction(g);
    assert(infoset < ABS_SIZE);

    // printf("Infoset info:\n");
    // printf("\tround=%d\n", infoset / (CARD_ABS_SIZE * STACK_ABS_SIZE * STACK_ABS_SIZE * 2 * 2 * 2));
    // printf("\tcard_abs=%d\n", infoset / (STACK_ABS_SIZE * STACK_ABS_SIZE * 2 * 2 * 2) % CARD_ABS_SIZE);
    // printf("\tstack0_abs=%d\n", infoset / (STACK_ABS_SIZE * 2 * 2 * 2) % STACK_ABS_SIZE);
    // printf("\tstack1_abs=%d\n", infoset / (2 * 2 * 2) % STACK_ABS_SIZE);
    // printf("\traised0=%d\n", infoset / (2 * 2) % 2);
    // printf("\traised1=%d\n", infoset / (2) % 2);
    // printf("\tplayer=%d\n", infoset % 2);
    // g.print();

    if (always_fold) {
        printf("WARNING: ALWAYS FOLD on\n");
        return getAction(g, 0);
    }

    printf("Num visit: %lld\n", num_visit[infoset]);
    if (num_visit[infoset] < 5) {
        printf("WARNING: an unvisited node, forcing FOLD\n");
        return getAction(g, 0);
    }

    // if (getEHS(g.cards, g.player, false) < 0.5) {
    //     printf("WARNING: winning rate too low, forcing FOLD\n");
    //     return getAction(g, 0);
    // }

    double s = 0;
    for (int i = 0; i < num_action; ++i)
        s += sum_sigma[infoset][i];
    for (int i = 0; i < num_action; ++i)
        avg_sigma[i] = sum_sigma[infoset][i] / s;

    printf("Action prob: ");
    for (int i = 0; i < num_action; ++i)
        printf("%.3f ", avg_sigma[i]);
    printf("\n");

    if (purification) {
        // s = 0;
        // for (int i = 0; i < num_action; ++i) {
        //     if (avg_sigma[i] < 0.15)
        //         avg_sigma[i] = 0;
        //     s += avg_sigma[i];
        // }
        // for (int i = 0; i < num_action; ++i)
        //     avg_sigma[i] /= s;
        int s_cnt = 0;
        s = 0;
        for (int i = 0; i < num_action; ++i) {
            if (avg_sigma[i] > s) {
                s = avg_sigma[i];
                s_cnt = 0;
            }
            if (avg_sigma[i] == s)
                ++s_cnt;
        }
        assert(s > 0);
        for (int i = 0; i < num_action; ++i) {
            if (avg_sigma[i] == s)
                avg_sigma[i] = 1. / s_cnt;
            else
                avg_sigma[i] = 0;
        }
    }

    int action_id = Random::choice(num_action, avg_sigma);
    return getAction(g, action_id, true);
}

inline double match(const Blueprint& a, const Blueprint& b) {
    GameState g;
    for (;;) {
        if (g.isTerminal())
            return g.getValue();
        if (g.isChance())
            g.sampleCard();
        else if (g.player == 0) {
            Action action = a.sampleAction(g);
            g.doAction(action);
        } else {
            assert(g.player == 1);
            Action action = b.sampleAction(g);
            g.doAction(action);
        }
    }
}

#endif
