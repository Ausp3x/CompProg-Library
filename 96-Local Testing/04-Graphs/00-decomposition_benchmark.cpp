#include "../../04-Graphs/07-lca.hpp"
#include "../../04-Graphs/08-scc.hpp"

// Timings include allocation; correctness checks stay outside the timed region.
int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20260927;
    std::mt19937_64 rng(seed);
    auto measure = [] (auto &&f) -> double {
        auto start = std::chrono::steady_clock::now(); f();
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    };
    for (int n : {32, 2000, 100000}) {
        for (string shape : {"chain", "star", "random", "forest"}) {
            Graph g(n); vector<pair<int, int>> queries;
            for (int v = 1; v < n; ++v) {
                if (shape == "forest" && v % 32 == 0) { continue; }
                int u = shape == "star" ? 0 : shape == "random" ? int(rng() % v) : v - 1;
                g.addEdge(u, v); }
            int q = max(1000, 4 * n);
            for (int i = 0; i < q; ++i) { queries.push_back({int(rng() % n), int(rng() % n)}); }
            LCA binary(g); EulerLCA euler(g);
            vector<int> expected;
            for (auto [u, v] : queries) {
                int a = binary.getLCA(u, v);
                if (a != euler.getLCA(u, v)) { std::abort(); } expected.push_back(a); }
            for (int run = 0; run < 5; ++run) {
                std::optional<LCA> a; std::optional<EulerLCA> b;
                double sa = measure([&] { a.emplace(g); });
                double sb = measure([&] { b.emplace(g); });
                vector<int> x(q), y(q);
                double qa = measure([&] { for (int i = 0; i < q; ++i) {
                    x[i] = a->getLCA(queries[i].first, queries[i].second); }});
                double qb = measure([&] { for (int i = 0; i < q; ++i) {
                    y[i] = b->getLCA(queries[i].first, queries[i].second); }});
                if (x != expected || y != expected) { std::abort(); }
                lng checksum = accumulate(x.begin(), x.end(), lng(0));
                std::cout << "lca " << n << ' ' << g.edges.size() << ' ' << q << ' ' << shape
                          << ' ' << run << ' ' << sa << ' ' << sb << ' ' << qa << ' ' << qb << ' ' << checksum << '\n'; }}}
    for (int n : {32, 2000, 100000}) {
        for (string shape : {"chain", "cycle", "random", "blocks"}) {
            Graph g(n, true);
            for (int v = 1; v < n; ++v) { g.addEdge(v - 1, v); }
            if (shape == "cycle") { g.addEdge(n - 1, 0); }
            if (shape == "random") {
                for (int i = 0; i < 4 * n; ++i) { g.addEdge(int(rng() % n), int(rng() % n)); }}
            if (shape == "blocks") {
                for (int v = 0; v < n; v += 32) { g.addEdge(min(v + 31, n - 1), v); }}
            auto canonical = [&] (const SccResult &s) -> vector<int> {
                vector<int> smallest(s.groups.size(), n), result(n);
                for (int u = 0; u < n; ++u) { smallest[s.component[u]] = min(smallest[s.component[u]], u); }
                for (int u = 0; u < n; ++u) { result[u] = smallest[s.component[u]]; }
                return result;
            };
            auto expected = canonical(tarjanScc(g));
            if (expected != canonical(kosarajuScc(g))) { std::abort(); }
            for (int run = 0; run < 5; ++run) {
                SccResult a, b;
                double sa = measure([&] { a = tarjanScc(g); });
                double sb = measure([&] { b = kosarajuScc(g); });
                if (canonical(a) != expected || canonical(b) != expected) { std::abort(); }
                lng checksum = accumulate(expected.begin(), expected.end(), lng(0));
                std::cout << "scc " << n << ' ' << g.edges.size() << " 0 " << shape << ' ' << run
                          << ' ' << sa << ' ' << sb << " 0 0 " << checksum << '\n'; }}}
}
