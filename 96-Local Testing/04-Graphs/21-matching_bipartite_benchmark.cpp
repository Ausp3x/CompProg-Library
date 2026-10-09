#include "../../04-Graphs/21-matching_bipartite.hpp"

// Timings include construction and allocation; matching sizes are compared outside the timed region.
int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20260927;
    std::mt19937_64 rng(seed);
    auto measure = [](auto &&f) -> double {
        auto start = std::chrono::steady_clock::now(); f();
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();};
    struct Work { string shape; int n; vector<pair<int, int>> edges; bool dense; };
    vector<Work> works;
    auto random = [&](string shape, int n, lng m, bool dense) {
        Work w{shape, n, {}, dense};
        for (lng i = 0; i < m; ++i) { w.edges.emplace_back(int(rng() % n), int(rng() % n)); }
        works.push_back(std::move(w));};
    random("sparse-deg3", 100000, 300000, false);
    random("sparse-deg3", 5000, 15000, true);
    random("dense-p0.5", 2000, 2000000, true);
    random("dense-p0.05", 5000, 1250000, true);
    Work path{"reversed-path", 8000, {}, true};
    for (int i = 0; i < path.n; ++i) { path.edges.emplace_back(i, i); if (i + 1 < path.n) { path.edges.emplace_back(i + 1, i); } }
    std::reverse(path.edges.begin(), path.edges.end());
    works.push_back(path);
    for (auto &w : works) {
        for (int run = 0; run < 6; ++run) {
            int a = 0, b = 0, c = -1;
            double ta = measure([&] { BipartiteMatching m(w.n, w.n, w.edges); m.hopcroftKarp(); a = m.size; });
            double tb = measure([&] { BipartiteMatching m(w.n, w.n, w.edges); m.kuhn(); b = m.size; });
            double tc = w.dense ? measure([&] { c = bipartiteMatchingDense(w.n, w.n, w.edges).size; }) : -1;
            if (a != b || (w.dense && c != a)) { std::abort(); }
            if (run > 0) { cout << w.shape << ' ' << w.n << ' ' << w.edges.size() << ' ' << run << ' ' << ta << ' ' << tb << ' ' << tc << ' ' << a << '\n'; }}}}
