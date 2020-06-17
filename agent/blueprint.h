#ifndef _BLUEPRINT_H
#define _BLUEPRINT_H

#include <string>
#include "game.h"
#include "abstraction.h"

const int NUM_SAMPLE_BR = 1000;
const int NUM_SAMPLE_APPROX = 10000;

struct Blueprint {
    double regret[ABS_SIZE][ACTION_SIZE];
    double sigma[ABS_SIZE][ACTION_SIZE];
    double sum_sigma[ABS_SIZE][ACTION_SIZE];
    int visited[ABS_SIZE];
    double ev_sum[ABS_SIZE];
    int ev_num[ABS_SIZE];
    int best_response[ABS_SIZE];

    int sample_player;
    int exp_sample_player;
    int cnt;
    long long cur_cnt;
    int iter;
    int miss_cnt;

    void init();
    void load(std::string file);
    void save(std::string file) const;
    void train(int num_iter, std::string save_file);

    double getExploitability(int player);
    double mccfr(GameState g);
    double sample(GameState g);

    Action sampleAction(const GameState &g, bool purification = false) const;
    Action sampleBR(const GameState &g) const;
};

inline void Blueprint::load(std::string file) {
    FILE* f = fopen(file.c_str(), "r");
    for (int s = 0; s < ABS_SIZE; ++s) {
        for (int a = 0; a < ACTION_SIZE; ++a)
            fscanf(f, "%lf%lf%lf", &regret[s][a], &sigma[s][a], &sum_sigma[s][a]);
        // double tmp;
        // fscanf(f, "%lf", &tmp);
    }
    fclose(f);
}

inline void Blueprint::save(std::string file) const {
    FILE* f = fopen(file.c_str(), "w");
    for (int s = 0; s < ABS_SIZE; ++s) {
        for (int a = 0; a < ACTION_SIZE; ++a)
            fprintf(f, "%.2f %.2f %.2f\n", regret[s][a], sigma[s][a], sum_sigma[s][a]);
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

inline Action Blueprint::sampleAction(const GameState& g, bool purification) const {
    assert(g.hasAction());

    double avg_sigma[ACTION_SIZE];
    int infoset = getGameAbstraction(g);
    int num_action = getNumAction(g);
    assert(infoset < ABS_SIZE);

    double s = 0;
    for (int i = 0; i < num_action; ++i)
        s += sum_sigma[infoset][i];
    for (int i = 0; i < num_action; ++i)
        avg_sigma[i] = sum_sigma[infoset][i] / s;

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
    return getAction(g, action_id);
}

inline Action Blueprint::sampleBR(const GameState& g) const {
    assert(g.hasAction());

    int infoset = getGameAbstraction(g);
    int num_action = getNumAction(g);
    assert(infoset < ABS_SIZE);
    assert(best_response[infoset] < num_action);

    return getAction(g, best_response[infoset]);
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
