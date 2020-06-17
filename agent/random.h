#ifndef _MYRNG_H
#define _MYRNG_H

#include <random>

namespace Random {

inline int randint(int l, int r) {
    return rand() % (r - l + 1) + l;
}

inline int choice(int n, const double p[]) {
    double x = (double)rand() / RAND_MAX;
    for (int i = 0; i < n; ++i) {
        x -= p[i];
        if (x < 1e-6)
            return i;
    }
    abort();
}

}

#endif
