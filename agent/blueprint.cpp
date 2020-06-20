#include "blueprint.h"
#include "random.h"
#include <cstring>
#include <chrono>

void Blueprint::init() {
    decay_iter = 0;
    for (int s = 0; s < ABS_SIZE; ++s) {
        num_visit[s] = 0;
        for (int a = 0; a < ACTION_SIZE; ++a) {
            regret[s][a] = 0;
            sigma[s][a] = 1. / ACTION_SIZE;
            sum_sigma[s][a] = 1. / ACTION_SIZE;
        }
    }
}

void Blueprint::decayRegret() {
    ++decay_iter;
    double coef = (double)decay_iter / (decay_iter + 1);
    for (int s = 0; s < ABS_SIZE; ++s) {
        if (num_visit[s] == 0)
            continue;
        for (int a = 0; a < ACTION_SIZE; ++a) {
            regret[s][a] *= coef;
            sum_sigma[s][a] *= coef;
        }
    }
}

void Blueprint::train(int num_iter, std::string save_file) {
    cnt = cur_cnt = 0;
    auto last_time = std::chrono::high_resolution_clock::now();

    for (iter = 0; iter < num_iter; ++iter) {
        sample_player = iter % 2;
        GameState g;
        mccfr(g);

        if (iter % 100 == 0) {
            auto cur_time = std::chrono::high_resolution_clock::now();
            double duration = std::chrono::duration_cast<std::chrono::seconds>(cur_time - last_time).count();
            printf("Iteration %d/%d\n", iter, num_iter);
            printf("\tNodes (this period)   %lld\n", cur_cnt);
            printf("\tNodes (overall)       %d/%d\n", cnt, ABS_SIZE);
            printf("\tSpeed (nodes per sec) %.2f\n", cur_cnt / duration);
            cur_cnt = 0;
            last_time = cur_time;
        }

        if (iter % 500 == 0 && iter > 0) {
            printf("Saving blueprint...\n");
            save(save_file);
            printf("Saving success.\n");
        }

        if (iter % 500 == 0 && iter > 0) {
            printf("Clearing EHS...\n");
            clearEHS();
            printf("Clearing success.\n");
        }

        if (iter % 2000 == 0 && iter > 0) {
            printf("Decaying regret (%d)...\n", decay_iter);
            decayRegret();
            printf("Decaying success.\n");
        }
    }
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

    if (num_visit[infoset] == 0)
        ++cnt;
    ++num_visit[infoset];

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

Blueprint bp;

int main() {
    srand(time(NULL));

    printf("Initialization start\n");
    bp.init();
    initAbstraction("");
    printf("Initialization done\n");
    printf("Occupied space: %ld MB\n", sizeof(bp) / 1024 / 1024);

    bp.load("blueprint_checkpoint.txt");

    printf("Training start\n");
    bp.train(1000000001, "blueprint_checkpoint.txt");
    printf("Training done\n");
}
