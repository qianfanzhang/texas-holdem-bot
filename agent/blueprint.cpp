#include "blueprint.h"
#include "random.h"
#include <cstring>
#include <chrono>

void Blueprint::init() {
    for (int s = 0; s < ABS_SIZE; ++s) {
        for (int a = 0; a < ACTION_SIZE; ++a) {
            regret[s][a] = 0;
            sigma[s][a] = 1. / ACTION_SIZE;
            sum_sigma[s][a] = 1. / ACTION_SIZE;
        }
    }
}

void Blueprint::train(int num_iter, std::string save_file) {
    memset(visited, -1, sizeof(visited));

    cur_cnt = 0;
    auto last_time = std::chrono::high_resolution_clock::now();

    for (iter = 0; iter < num_iter; ++iter) {
        sample_player = iter % 2;
        GameState g;
        mccfr(g);

        if (iter % 10000 == 0) {
            auto cur_time = std::chrono::high_resolution_clock::now();
            double duration = std::chrono::duration_cast<std::chrono::seconds>(cur_time - last_time).count();
            printf("Iteration %d/%d\n", iter, num_iter);
            printf("\tNodes (this period)   %lld\n", cur_cnt);
            printf("\tNodes (overall)       %d/%d\n", cnt, ABS_SIZE);
            printf("\tSpeed (nodes per sec) %.2f\n", cur_cnt / duration);
            // printf("\tAvg node access: %.2f\n", (double)cur_cnt / (iter + 1));
            cur_cnt = 0;
            last_time = cur_time;
        }

        if (iter % 100000 == 0) {
            printf("Calculating exploitability...\n");
            printf("\tExploitablity(0) = %.2f\n", getExploitability(0));
            printf("\tExploitablity(1) = %.2f\n", getExploitability(1));
        }

        if (iter % 100000 == 0 && iter > 1) {
            printf("Saving blueprint...\n");
            save(save_file);
            printf("Saving success.\n");
        }

        if (iter % 500000 == 0)
            clearEHS();
    }
}

double Blueprint::getExploitability(int player) {
    memset(ev_sum, 0, sizeof(ev_sum));
    memset(ev_num, 0, sizeof(ev_num));
    memset(best_response, 0, sizeof(best_response));
    exp_sample_player = player;

    double sum_ev = 0;
    for (int i = 0; i < NUM_SAMPLE_BR; ++i) {
        GameState g;
        sum_ev += sample(g);
    }
    // printf("\tBR avg ev=%.2f\n", sum_ev / NUM_SAMPLE_BR);

    sum_ev = 0;
    for (int i = 0; i < NUM_SAMPLE_APPROX; ++i) {
        GameState g;
        while (!g.isTerminal()) {
            if (g.isChance())
                g.sampleCard();
            else if (g.player == player) {
                Action action = sampleAction(g);
                g.doAction(action);
            } else {
                Action action = sampleBR(g);
                g.doAction(action);
            }
        }
        sum_ev += g.getValue() * (player == 0 ? -1 : 1);
    }

    return sum_ev / NUM_SAMPLE_APPROX;
}

double Blueprint::mccfr(GameState g) {
    if (g.isTerminal())
        return g.getValue() * (sample_player == 0 ? 1 : -1);
    if (g.isChance()) {
        g.sampleCard();
        return mccfr(g);
    }

    ++cur_cnt;

    int infoset = getGameAbstraction(g);
    int num_action = getNumAction(g);

    assert(infoset < ABS_SIZE);
    if (visited[infoset] == -1)
        ++cnt;

    visited[infoset] = iter;

    // regret matching
    double sum_regret = 0;
    for (int i = 0; i < num_action; ++i) {
        double r = regret[infoset][i];
        sum_regret += r > 0 ? r : 0;
    }
    if (sum_regret < 1e-5) {
        for (int i = 0; i < num_action; ++i)
            sigma[infoset][i] = 1. / num_action;
    } else {
        for (int i = 0; i < num_action; ++i) {
            double r = regret[infoset][i];
            sigma[infoset][i] = (r > 0 ? r : 0) / sum_regret;
        }
    }

    double v[ACTION_SIZE];
    double ev = 0;

    if (g.player != sample_player) {
        int action_id;
        action_id = Random::choice(num_action, sigma[infoset]);
        Action action = getAction(g, action_id);
        g.doAction(action);
        ev = mccfr(g);

        // update average strategy
        for (int i = 0; i < num_action; ++i)
            sum_sigma[infoset][i] += sigma[infoset][i];
    } else {
        for (int i = 0; i < num_action; ++i) {
            Action action = getAction(g, i);
            GameState g0 = g;
            g0.doAction(action);
            v[i] = mccfr(g0);
            ev += sigma[infoset][i] * v[i];
        }

        // update regret
        for (int i = 0; i < num_action; ++i)
            regret[infoset][i] += v[i] - ev;
    }

    return ev;
}

double Blueprint::sample(GameState g) {
    if (g.isTerminal())
        return g.getValue() * (exp_sample_player == 0 ? -1 : 1);
    if (g.isChance()) {
        g.sampleCard();
        return sample(g);
    }

    int infoset = getGameAbstraction(g);
    int num_action = getNumAction(g);

    if (visited[infoset] == -1)
        ++miss_cnt;

    if (g.player == exp_sample_player) {
        double avg_sigma[ACTION_SIZE];

        double s = 0;
        for (int i = 0; i < num_action; ++i)
            s += sum_sigma[infoset][i];
        for (int i = 0; i < num_action; ++i)
            avg_sigma[i] = sum_sigma[infoset][i] / s;

        int action_id = Random::choice(num_action, avg_sigma);
        Action action = getAction(g, action_id);
        g.doAction(action);
        ev_sum[infoset] += sample(g);
        ev_num[infoset] += 1;

        return ev_sum[infoset] / ev_num[infoset];
    }

    double max_exp = -STACK_SIZE;
    for (int i = 0; i < num_action; ++i) {
        Action action = getAction(g, i);
        GameState g0 = g;
        g0.doAction(action);
        double ev = sample(g0);
        if (ev > max_exp) {
            max_exp = ev;
            best_response[infoset] = i;
        }
    }

    return max_exp;
}

Blueprint bp;
// Blueprint bp0;

int main() {
    printf("Initialization start\n");
    bp.init();
    printf("Initialization done\n");
    printf("Occupied space: %ld MB\n", sizeof(bp) / 1024 / 1024);

    // bp.load("blueprint_checkpoint.txt");

    // bp0.load("blueprint_checkpoint.nodecay.txt");
    // int num_sample = 1e6;
    // double sum = 0;
    // for (int i = 0; i < num_sample; ++i) {
    //     sum += match(bp, bp0);
    //     sum += -match(bp0, bp);
    // }
    // printf("ev = %.2f\n", sum / num_sample);

    printf("Training start\n");
    bp.train(1000000001, "blueprint_checkpoint.txt");
    printf("Training done\n");
}
