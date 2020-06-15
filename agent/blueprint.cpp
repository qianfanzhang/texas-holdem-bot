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
            sum_sigma[s][a] = 1. / ACTION_SIZE;
        }
        sum_pi[s] = 1;
    }
}

void Blueprint::load(std::string file) {
    FILE* f = fopen(file.c_str(), "r");
    for (int s = 0; s < ABS_SIZE; ++s) {
        for (int a = 0; a < ACTION_SIZE; ++a)
            fscanf(f, "%f%f%f", &regret[s][a], &sigma[s][a], &sum_sigma[s][a]);
        fscanf(f, "%f", &sum_pi[s]);
    }
    fclose(f);
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

        if (iter % 1000 == 0) {
            printf("Iteration %d/%d\n", iter, num_iter);
            printf("\tVisited nodes %d/%d\n", cnt, ABS_SIZE);
        }

        if (iter % 100000 <= 1 && iter > 1) {
            printf("Calculating exploitability...\n");
            float e = getExploitability();
            printf("\tExploitablity(%d) = %.2f\n", sample_player, e);
        }

        if (iter % 100000 == 1 && iter > 1) {
            printf("Saving blueprint...\n");
            save("blueprint_checkpoint.TX");
            clearEHS();
            printf("Saving success.\n");
        }
    }
}

float Blueprint::getExploitability(int num_sample_br, int num_sample_approx) {
    memset(ev_sum, 0, sizeof(ev_sum));
    memset(ev_num, 0, sizeof(ev_num));
    memset(best_response, -1, sizeof(best_response));

    update_br = true;
    for (int i = 0; i < num_sample_br; ++i) {
        GameState g;
        g.presampleCard();
        float v = sample(g);
        printf("\tBR Iteration %d/%d, v=%.2f\n", i, num_sample_br, v);
    }

    float sum_ev = 0;
    update_br = false;
    for (int i = 0; i < num_sample_approx; ++i) {
        GameState g;
        g.presampleCard();
        float v = sample(g);
        sum_ev += v;
        // printf("\tAPPROX Iteration %d/%d, v=%.2f\n", i, num_sample_approx, v);
    }
    return sum_ev / num_sample_approx;
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
        const float exploration = 0.5;
        if (Random::decide(exploration)) {
            action_id = Random::randint(0, num_action - 1);
        } else
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

    if (g.player == sample_player) {
        float ev = 0;
        for (int i = 0; i < num_action; ++i) {
            float s = (sum_pi[infoset] == 0 ? 1. / num_action : sum_sigma[infoset][i] / sum_pi[infoset]);
            if (s < 1e-5)
                continue;
            Action action = getAction(g, i);
            GameState g0 = g;
            g0.doAction(action);
            ev += s * sample(g0);
        }
        ev_sum[infoset] += ev;
        ev_num[infoset] += 1;

        return ev_sum[infoset] / ev_num[infoset];
    }

    if (!update_br && best_response[infoset] != -1) {
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
        if (ev >= max_exp) {
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

    printf("Training start\n");
    bp.train(100000002);
    printf("Training done\n");
}
