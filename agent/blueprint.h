#ifndef _BLUEPRINT_H
#define _BLUEPRINT_H

#include <string>
#include "game.h"
#include "abstraction.h"

const int NUM_SAMPLE_BR = 200;
const int NUM_SAMPLE_APPROX = 5000;

struct Blueprint {
    float regret[ABS_SIZE][ACTION_SIZE];
    float sigma[ABS_SIZE][ACTION_SIZE];
    float sum_sigma[ABS_SIZE][ACTION_SIZE];
    float sum_pi[ABS_SIZE];
    float last_regret[ABS_SIZE][ACTION_SIZE];
    int visited[ABS_SIZE];
    float utility[ABS_SIZE];
    float ev_sum[ABS_SIZE];
    int ev_num[ABS_SIZE];
    int best_response[ABS_SIZE];

    int sample_player;
    int cnt;
    int iter;
    bool update_br;

    void init();
    void load(std::string file);
    void save(std::string file) const;
    void train(int num_iter);

    float getExploitability(int num_sample_br = NUM_SAMPLE_BR, int num_sample_approx = NUM_SAMPLE_APPROX);
    float mccfr(GameState g, float prob);
    float sample(GameState g);
};

#endif
