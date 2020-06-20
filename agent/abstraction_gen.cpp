#include <cmath>
#include <string>
#include <utility>
#include "cards.h"
#include "abstraction.h"

const int SIZE = 1000005;
const int C_SIZE = 1005;

PEHS points[SIZE];
PEHS centroids[C_SIZE];
int num_centroids;

std::pair<double, int> minCentroidDist(const PEHS& p) {
    double min_dist = 1000;
    int min_pos = -1;
    for (int i = 0; i < num_centroids; ++i) {
        double d = p.distance2(centroids[i]);
        if (d < min_dist) {
            min_dist = d;
            min_pos = i;
        }
    }
    assert(min_pos != -1);
    return std::make_pair(min_dist, min_pos);
}

void generate(int size, int num_boardcard, std::string file_name, int sample_size, int num_iter = 5000) {
    printf("Generate %d %d %s %d %d\n", size, num_boardcard, file_name.c_str(), sample_size, num_iter);

    for (int i = 0; i < sample_size; ++i) {
        CardState s;
        if (i % 1000 == 0)
            printf("%s sampling %d/%d\n", file_name.c_str(), i, sample_size);
        s.sampleHoleCard(0, 2);
        s.sampleBoardCard(num_boardcard);
        points[i] = getPEHS(s, 0, 1000, 200);
    }

    printf("Sampling finished\n");

    double p[SIZE];

    num_centroids = 0;
    centroids[num_centroids++] = points[0];
    for (int i = 1; i < size; ++i) {
        double sum = 0;
        for (int j = 0; j < sample_size; ++j) {
            p[j] = minCentroidDist(points[j]).first;
            sum += p[j];
        }
        sum *= rand() / RAND_MAX;
        for (int j = 0; j < sample_size; ++j) {
            sum -= p[j];
            if (sum < 0) {
                centroids[num_centroids++] = points[j];
                break;
            }
        }
    }

    printf("Kmeans++ finished\n");

    PEHS sum[C_SIZE];
    int cnt[C_SIZE];

    for (int i = 0; i < num_iter; ++i) {
        memset(sum, 0, sizeof(sum));
        memset(cnt, 0, sizeof(cnt));
        double cost = 0;
        for (int j = 0; j < sample_size; ++j) {
            auto t = minCentroidDist(points[j]);
            double d = t.first;
            int c = t.second;
            cost += d;
            sum[c] += points[j];
            cnt[c] += 1;
        }
        if (i % 1000 == 0)
            printf("Iteration %d/%d, cost=%.3f\n", i, num_iter, cost);
        for (int j = 0; j < num_centroids; ++j) {
            if (cnt[j] > 0)
                centroids[j] = sum[j] / cnt[j];
            // else
                // printf("WARNING: unused centroid");
        }
    }

    FILE* f = fopen(file_name.c_str(), "w");
    fprintf(f, "%d\n", num_centroids);
    for (int i = 0; i < num_centroids; ++i) {
        for (int j = 0; j < PEHS::BUCKET_SIZE; ++j)
            fprintf(f, "%.10f ", centroids[i].x[j]);
        fprintf(f, "\n");
    }
    fclose(f);

    printf("Saved.\n");

    for (int i = 0; i < 10; ++i) {
        CardState s;
        s.sampleHoleCard(0, 2);
        s.sampleBoardCard(num_boardcard);
        PEHS ps = getPEHS(s, 0, 500, 100);
        auto t = minCentroidDist(ps);
        ps.print();
        centroids[t.second].print();
        printf("Dist: %.3f %.3f\n", t.first, ps.distance(centroids[t.second]));
    }
}

int main() {
    srand(time(NULL));

    generate(100, 0, "preflop.abs", 50000);
    // clearEHS();
    generate(100, 3, "flop.abs", 100000);
    // clearEHS();
    generate(100, 4, "turn.abs", 150000);
    // clearEHS();
    generate(100, 5, "river.abs", 300000);
}
