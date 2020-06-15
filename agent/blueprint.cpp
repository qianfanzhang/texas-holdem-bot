#include "blueprint.h"
#include "random.h"
#include <cstring>

inline void normalize(int n, float p[]) {
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
            sum_sigma[s][a] = 1. / ACTION_SIZE;
        }
        sum_pi[s] = 1;
    }
}

void Blueprint::save(std::string file) const {
    FILE* f = fopen(file.c_str(), "w");
    for (int s = 0; s < ABS_SIZE; ++s) {
        // fprintf(f, "abs %d %d %d %d\n", s / (STACK_ABS_SIZE * STACK_ABS_SIZE * 2), s / (STACK_ABS_SIZE * 2) % STACK_ABS_SIZE, s / 2 % STACK_ABS_SIZE, s % 2);
        for (int a = 0; a < ACTION_SIZE; ++a)
            fprintf(f, "%.2f %.2f %.2f\n", regret[s][a], sigma[s][a], sum_sigma[s][a]);
        fprintf(f, "%.2f\n", sum_pi[s]);
    }
    fclose(f);
}

void Blueprint::train(int num_iter) {
    memset(visited, -1, sizeof(visited));

    for (iter = 0; iter < num_iter; ++iter) {
        sample_player = iter % 2;
        GameState g;
        mccfr(g, 1);

        if (iter % 5000 == 0) {
            printf("Iteration %d/%d\n", iter, num_iter);
            printf("\tVisited nodes %d/%d\n", cnt, ABS_SIZE);
        }

        if (iter % 100000 <= 1) {
            printf("Calculating exploitability...\n");
            float e = getExploitability();
            printf("\tExploitablity(%d) = %.2f\n", sample_player, e);
        }

        if (iter % 100000 == 1 && iter > 1) {
            printf("Saving blueprint...\n");
            save("blueprint_checkpoint.txt");
            printf("Saving success.\n");
        }

        if (iter % 1000000)
            clearEHS();
    }
}

float Blueprint::getExploitability() {
    memset(ev_sum, 0, sizeof(ev_sum));
    memset(ev_num, 0, sizeof(ev_num));
    memset(best_response, 0, sizeof(best_response));

    float sum_ev = 0;
    update_br = true;
    for (int i = 0; i < NUM_SAMPLE_BR; ++i) {
        GameState g;
        float v = sample(g);
        sum_ev += v;
    }
    printf("\tBR avg ev=%.2f\n", sum_ev / NUM_SAMPLE_BR);

    sum_ev = 0;
    update_br = false;
    miss_cnt = 0;
    for (int i = 0; i < NUM_SAMPLE_APPROX; ++i) {
        GameState g;
        float v = sample(g);
        sum_ev += v;
    }
    printf("\tMiss: %d\n", miss_cnt);
    return sum_ev / NUM_SAMPLE_APPROX;
}

float Blueprint::mccfr(GameState g, float prob) {
    if (g.isTerminal())
        return g.getValue() * (sample_player == 0 ? 1 : -1);
    if (g.isChance()) {
        g.sampleCard();
        return mccfr(g, prob);
    }

    int infoset = getGameAbstraction(g);
    int num_action = getNumAction(g);

    assert(infoset < ABS_SIZE);
    if (visited[infoset] == -1)
        ++cnt;

    // reuse calculated utility
    // FIXME: there might be a severe bug
    if (visited[infoset] == iter) {
        // regret matching
        float sum_regret = 0;
        for (int i = 0; i < num_action; ++i) {
            regret[infoset][i] += last_regret[infoset][i];
            regret[infoset][i] = std::max<float>(regret[infoset][i], 0);
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

        return utility[infoset];
    }

    visited[infoset] = iter;

    normalize(num_action, sigma[infoset]);
    if (g.player != sample_player) {
        int action_id;
        action_id = Random::choice(num_action, sigma[infoset]);
        Action action = getAction(g, action_id);
        g.doAction(action);
        return utility[infoset] = mccfr(g, prob);
    }

    float ev = 0;
    for (int i = 0; i < num_action; ++i) {
        Action action = getAction(g, i);
        GameState g0 = g;
        g0.doAction(action);
        last_regret[infoset][i] = mccfr(g0, prob * sigma[infoset][i]);
        ev += sigma[infoset][i] * last_regret[infoset][i];
    }

    // regret matching
    float sum_regret = 0;
    for (int i = 0; i < num_action; ++i) {
        last_regret[infoset][i] -= ev;
        regret[infoset][i] += last_regret[infoset][i];
        regret[infoset][i] = std::max<float>(regret[infoset][i], 0);
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

float Blueprint::sample(GameState g) {
    if (g.isTerminal())
        return g.getValue() * (sample_player == 0 ? -1 : 1);
    if (g.isChance()) {
        g.sampleCard();
        return sample(g);
    }

    int infoset = getGameAbstraction(g);
    int num_action = getNumAction(g);

    if (visited[infoset] == -1)
        ++miss_cnt;

    if (g.player == sample_player) {
        float avg_sigma[ACTION_SIZE];
        for (int i = 0; i < num_action; ++i) {
            if (sum_pi[infoset] < 1e-5)
                avg_sigma[i] = 1. / num_action;
            else
                avg_sigma[i] = sum_sigma[infoset][i] / sum_pi[infoset];
        }

        int action_id = Random::choice(num_action, avg_sigma);
        Action action = getAction(g, action_id);
        g.doAction(action);
        ev_sum[infoset] += sample(g);
        ev_num[infoset] += 1;

        return ev_sum[infoset] / ev_num[infoset];
    }

    if (!update_br) {
        Action action = getAction(g, best_response[infoset]);
        g.doAction(action);
        return sample(g);
    }

    float max_exp = -STACK_SIZE;
    for (int i = 0; i < num_action; ++i) {
        Action action = getAction(g, i);
        GameState g0 = g;
        g0.doAction(action);
        float ev = sample(g0);
        if (ev > max_exp) {
            max_exp = ev;
            best_response[infoset] = i;
        }
    }

    return utility[infoset] = max_exp;
}

Blueprint bp;

int main() {
    printf("Initialization start\n");
    bp.init();
    printf("Initialization done\n");
    printf("Occupied space: %ld MB\n", sizeof(bp) / 1024 / 1024);

    bp.load("blueprint_checkpoint.txt");

    printf("Training start\n");
    bp.train(1000000002);
    printf("Training done\n");
}
