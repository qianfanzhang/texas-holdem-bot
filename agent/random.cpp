#include <random>
#include "random.h"

struct RandomGenerator {
    std::random_device device;
    std::mt19937 rng;
    RandomGenerator() {
        rng = std::mt19937(device());
    }
};

static RandomGenerator rng;

int Random::randint(int l, int r) {
    std::uniform_int_distribution<int> dist(l, r);
    return dist(rng.rng);
}

bool Random::decide(float p) {
    std::discrete_distribution<int> dist({1 - p, p});
    return dist(rng.rng);
}

int Random::choice(int n, const float p[]) {
    std::discrete_distribution<int> dist(p, p + n);
    return dist(rng.rng);
}
