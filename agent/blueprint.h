#ifndef _BLUEPRINT_H
#define _BLUEPRINT_H

#include <string>
#include "game.h"
#include "abstraction.h"

const int NUM_SAMPLE_BR = 500;
const int NUM_SAMPLE_APPROX = 10000;

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
    int miss_cnt;

    void init();
    void load(std::string file);
    void save(std::string file) const;
    void train(int num_iter);

    float getExploitability();
    float mccfr(GameState g, float prob);
    float sample(GameState g);
};

inline void Blueprint::load(std::string file) {
    FILE* f = fopen(file.c_str(), "r");
    for (int s = 0; s < ABS_SIZE; ++s) {
        for (int a = 0; a < ACTION_SIZE; ++a)
            fscanf(f, "%f%f%f", &regret[s][a], &sigma[s][a], &sum_sigma[s][a]);
        fscanf(f, "%f", &sum_pi[s]);
    }
    fclose(f);
}

#endif
