#include "../../04-Graphs/16-centroiddecomposition.hpp"

// Lines: "square t naive_us ntt_us" and "tree shape n op ms checksum"; every output is verified.
namespace {
    double now() { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
    vector<lng> naiveSquare(const vector<lng> &h) {
        int top = int(h.size()) - 1;
        vector<lng> sq(2 * top + 1);
        for (int i = 0; i <= top; ++i) {
            sq[2 * i] += h[i] * h[i];
            for (int j = i + 1; j <= top; ++j) { sq[i + j] += 2 * h[i] * h[j]; }}
        return sq;}
    Graph shaped(int n, int shape, std::mt19937_64 &rng) {
        Graph g(n);
        for (int i = 1; i < n; ++i) {
            int p = shape == 0 ? int(rng() % ulng(i)) : shape == 1 ? i - 1 : shape == 2 ? 0 : shape == 3 ? (i % 2 ? i - 1 : max(0, i - 2)) : (i - 1) / 2;
            g.addEdge(p, i);}
        return g;}
    struct SumMonoid { using S = lng; static S op(S a, S b) { return a + b; } static S e() { return 0; } };
} // namespace

int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 1;
    std::mt19937_64 rng(seed);
    for (int t : {256, 512, 640, 768, 800, 896, 1024, 1536}) {
        vector<lng> h(t);
        for (auto &x : h) { x = lng(rng() % 1000); }
        int reps = max(3, 3000000 / (t * t / 2 + 1));
        double t0 = now();
        vector<lng> a, b;
        for (int r = 0; r < reps; ++r) { a = naiveSquare(h); }
        double t1 = now();
        for (int r = 0; r < reps; ++r) { b = convolutionLong(h, h); }
        double t2 = now();
        if (a != b) { cerr << "square mismatch t=" << t << '\n'; return 1; }
        printf("square %d %.2f %.2f\n", t, 1000 * (t1 - t0) / reps, 1000 * (t2 - t1) / reps);}
    const char *names[] = {"random", "path", "star", "caterpillar", "binary"};
    int n = 200000, q = 200000;
    for (int shape = 0; shape < 5; ++shape) {
        Graph g = shaped(n, shape, rng);
        double t0 = now();
        CentroidDecomposition<int> cd(g);
        double t1 = now();
        auto f = treeDistanceFrequency(g);
        double t2 = now();
        lng total = 0;
        for (lng x : f) { total += x; }
        if (total != lng(n) * (n - 1) / 2) { cerr << "frequency total\n"; return 1; }
        ContourSum<lng> cs(g, vector<lng>(n, 1));
        ContourFold<SumMonoid> cf(g, vector<lng>(n, 1));
        vector<pair<int, int>> qs(q);
        for (auto &[v, l] : qs) { v = int(rng() % ulng(n)); l = int(rng() % 8); }
        double t3 = now();
        lng sa = 0, sb = 0;
        for (auto [v, l] : qs) { sa += cs.rangeContourSum(v, l, l + 4); }
        double t4 = now();
        for (auto [v, l] : qs) { sb += cf.rangeContourFold(v, l, l + 4); }
        double t5 = now();
        if (sa != sb) { cerr << "contour mismatch\n"; return 1; }
        printf("tree %s %d build %.2f %d\n", names[shape], n, t1 - t0, cd.order[0]);
        printf("tree %s %d frequency %.2f %lld\n", names[shape], n, t2 - t1, (long long)f[1]);
        printf("tree %s %d contourSum %.2f %lld\n", names[shape], n, t4 - t3, (long long)sa);
        printf("tree %s %d contourFold %.2f %lld\n", names[shape], n, t5 - t4, (long long)sb);}}
