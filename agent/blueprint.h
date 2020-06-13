#ifndef _BLUEPRINT_H
#define _BLUEPRINT_H

#include <string>
#include "game.h"
#include "abstraction.h"

const int NUM_EXPLOITABILITY_SAMPLE = 5;

struct Blueprint {
    float regret[ABS_SIZE][ACTION_SIZE];
    float sigma[ABS_SIZE][ACTION_SIZE];
    float sum_sigma[ABS_SIZE][ACTION_SIZE];
    float sum_pi[ABS_SIZE];
    bool visited[ABS_SIZE];
    float utility[ABS_SIZE];
    int sample_player;

    void init();
    void load(std::string file);
    void save(std::string file) const;
    void train(int num_iter);

    float getExploitability(int num_sample = NUM_EXPLOITABILITY_SAMPLE);
    float mccfr(GameState g, float prob);
    float exploit(GameState g);
};

#endif
