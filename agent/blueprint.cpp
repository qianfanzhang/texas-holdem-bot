#include "blueprint.h"
#include "random.h"
#include <cstring>

void normalize(int n, float p[]) {
    float s = 0;
    for (int i = 0; i < n; ++i)
        s += p[i];
    assert(s > 0);
    for (int i = 0; i < n; ++i)
        p[i] /= s;
}

void Blueprint::init() {
    for (int s = 0; s < ABS_SIZE; ++s) {
        for (int a = 0; a < ACTION_SIZE; ++a) {
            regret[s][a] = 0;
            sigma[s][a] = 1. / ACTION_SIZE;
            sum_sigma[s][a] = 0;
        }
        sum_pi[s] = 0;
    }
}

void Blueprint::load(std::string file) {
    
}

void Blueprint::save(std::string file) const {
    
}

void Blueprint::train(int num_iter) {
    for (int i = 0; i < num_iter; ++i) {
        printf("Iteration %d/%d\n", i, num_iter);

        memset(visited, 0, sizeof(visited));
        sample_player = i % 2;
        GameState g;
        g.presampleCard();
        mccfr(g, 1);

        if (i % 5000 <= 1) {
            printf("Calculating exploitability...\n");
            memset(visited, 0, sizeof(visited));
            float e = getExploitability();
            printf("%d Exploitablity = %.2f\n", sample_player, e);
            clearEHS();
        }
    }
}

float Blueprint::getExploitability(int num_sample) {
    float sum = 0;
    for (int i = 0; i < num_sample; ++i) {
        memset(visited, 0, sizeof(visited));
        GameState g;
        // g.presampleCard();
        sum += exploit(g);
    }
    return sum / num_sample;
}

float Blueprint::mccfr(GameState g, float prob) {
    if (g.isTerminal())
        return g.getValue() * (sample_player == 0 ? 1 : -1);
    if (g.isChance()) {
        g.sampleCard();
        return mccfr(g, prob);
    }

    // reuse calculated utility
    // FIXME: there might be a severe bug
    int infoset = getGameAbstraction(g);
    if (visited[infoset])
        return utility[infoset];
    visited[infoset] = true;

    int num_action = getNumAction(g);
    normalize(num_action, sigma[infoset]);

    if (g.player != sample_player) {
        int action_id = Random::choice(num_action, sigma[infoset]);
        Action action = getAction(g, action_id);
        g.doAction(action);
        return utility[infoset] = mccfr(g, prob);
    }

    float v[ACTION_SIZE];
    float ev = 0;
    for (int i = 0; i < num_action; ++i) {
        // printf("trying action %d\n", i);
        if (sigma[infoset][i] == 0)
            continue;
        Action action = getAction(g, i);
        GameState g0 = g;
        g0.doAction(action);
        v[i] = mccfr(g0, prob * sigma[infoset][i]);
        ev += sigma[infoset][i] * v[i];
    }

    // regret matching
    float sum_regret = 0;
    for (int i = 0; i < num_action; ++i) {
        v[i] -= ev;
        regret[infoset][i] += v[i];
        sum_regret += std::max<float>(regret[infoset][i], 0);
    }
    if (sum_regret == 0) {
        for (int i = 0; i < num_action; ++i)
            sigma[infoset][i] = 1. / num_action;
    } else {
        for (int i = 0; i < num_action; ++i)
            sigma[infoset][i] = std::max<float>(regret[infoset][i], 0) / sum_regret;
    }

    // update average strategy
    for (int i = 0; i < num_action; ++i)
        sum_sigma[infoset][i] += prob * sigma[infoset][i];
    sum_pi[infoset] += prob;

    return utility[infoset] = ev;
}

float Blueprint::exploit(GameState g) {
    if (g.isTerminal())
        return g.getValue() * (sample_player == 0 ? -1 : 1);
    if (g.isChance()) {
        g.sampleCard();
        return exploit(g);
    }

    int infoset = getGameAbstraction(g);
    if (visited[infoset])
        return utility[infoset];
    visited[infoset] = true;

    int num_action = getNumAction(g);

    if (g.player == sample_player) {
        float avg_sigma[ACTION_SIZE];
        float s = 0;
        for (int i = 0; i < num_action; ++i) {
            avg_sigma[i] = (sum_pi[infoset] == 0 ? 1. / num_action : sum_sigma[infoset][i] / sum_pi[infoset]);
            s += avg_sigma[i];
        }

        float ev = 0;
        for (int i = 0; i < num_action; ++i) {
            Action action = getAction(g, i);
            GameState g0 = g;
            g0.doAction(action);
            ev += avg_sigma[i] * exploit(g0);
        }
        return utility[infoset] = ev;
    }

    float max_exploit = -STACK_SIZE;
    for (int i = 0; i < num_action; ++i) {
        Action action = getAction(g, i);
        GameState g0 = g;
        g0.doAction(action);
        max_exploit = std::max(max_exploit, exploit(g0));
    }

    return utility[infoset] = max_exploit;
}

Blueprint bp;

int main() {
    printf("Initialization start\n");
    bp.init();
    printf("Initialization done\n");
    printf("Occupied space: %ld MB\n", sizeof(bp) / 1024 / 1024);

    printf("Training start\n");
    bp.train(1000000);
    printf("Training done\n");
}
