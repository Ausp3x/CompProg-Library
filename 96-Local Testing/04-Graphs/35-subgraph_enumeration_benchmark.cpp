#include "../../04-Graphs/35-subgraph_enumeration.hpp"

namespace {
    namespace sd = subgraph_enumeration_detail;

    lng choose3(lng n) { return n * (n - 1) * (n - 2) / 6; }

    Graph build(int n, vector<pair<int, int>> es) {
        Graph g(n);
        vector<int> deg(n);
        for (auto [u, v] : es) { ++deg[u]; ++deg[v]; }
        for (int u = 0; u < n; ++u) { g.adj[u].reserve(deg[u]); }
        g.edges.reserve(es.size()); g.arcs.reserve(2 * es.size());
        for (auto [u, v] : es) { g.addEdge(u, v); }
        return g;}

    // Erdos-Renyi G(n, p): every pair u < v independently with probability p.
    vector<pair<int, int>> gnp(int n, double p, std::mt19937_64 &rng) {
        vector<pair<int, int>> es;
        es.reserve(size_t(p * n * (n - 1) / 2 * 1.01 + 64));
        ulng limit = p >= 1 ? ~ulng(0) : ulng(p * 0x1p64);
        for (int u = 0; u < n; ++u) {
            for (int v = u + 1; v < n; ++v) {
                if (p >= 1 || rng() < limit) { es.push_back({u, v}); }}}
        return es;}

    // A c-clique on 0..c-1 plus len further vertices, isolated or on a path hanging off vertex 0.
    vector<pair<int, int>> cliquePlus(int c, int len, bool path) {
        vector<pair<int, int>> es;
        for (int u = 0; u < c; ++u) {
            for (int v = u + 1; v < c; ++v) { es.push_back({u, v}); }}
        for (int u = c - 1; path && u + 1 < c + len; ++u) { es.push_back({u == c - 1 ? 0 : u, u + 1}); }
        return es;}

    vector<pair<int, int>> bipartite(int a) {
        vector<pair<int, int>> es;
        for (int u = 0; u < a; ++u) {
            for (int v = 0; v < a; ++v) { es.push_back({u, a + v}); }}
        return es;}

    vector<pair<int, int>> randomSparse(int n, int m, std::mt19937_64 &rng) {
        vector<pair<int, int>> es;
        while (int(es.size()) < m) {
            int u = int(rng() % ulng(n)), v = int(rng() % ulng(n));
            if (u != v) { es.push_back({u, v}); }}
        return es;}

    // One warmup (checked, unprinted) and five printed samples; each section repeats k times so small graphs time reliably.
    void bench(const string &name, int n, vector<pair<int, int>> es, lng expected) {
        Graph g = build(n, std::move(es));
        lng m = lng(g.edges.size()), words = (n + 63) / 64;
        int k = int(std::clamp<lng>(4000000 / (n + m + 1), 1, 200));
        bool full = n * words <= 200000000;
        auto time = [&](auto &&f) {
            auto start = std::chrono::steady_clock::now();
            for (int r = 0; r < k; ++r) { f(); }
            return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() / k;};
        for (int rep = 0; rep <= 5; ++rep) {
            std::optional<sd::Ranked> h;
            lng probes = 0, scan = n * words, sparse = 0, dense = -1, total = 0;
            int r0 = 0;
            double build_full_ms = time([&] { h.emplace(g, true); });
            h.reset();
            double build_ms = time([&] { h.emplace(g); });
            for (int u = 0; u < h->n; ++u) {
                for (int i = h->split[u]; i < h->start[u + 1]; ++i) {
                    int v = h->to[i];
                    probes += h->start[v + 1] - h->split[v]; scan += words - v / 64;}}
            double start_ms = time([&] { r0 = sd::denseStart(*h); });
            double sparse_ms = time([&] { sparse = 0; sd::triangles(*h, [&](int, int, int, int, int, int) { ++sparse; }); });
            double dense_ms = full ? time([&] { dense = sd::bitsetTriangles(*h); }) : -1;
            h.reset();
            double count_ms = time([&] { total = countTriangles(g); });
            if (sparse != total || (full && dense != sparse) || (expected >= 0 && sparse != expected)) {
                cerr << "MISMATCH " << name << ' ' << sparse << ' ' << dense << ' ' << total << ' ' << expected << '\n';
                std::abort();}
            if (rep) {
                cout << name << ' ' << n << ' ' << m << ' ' << probes << ' ' << scan << ' ' << r0 << ' ' << rep << ' ' << k << ' ' << build_ms << ' '
                     << build_full_ms << ' ' << start_ms << ' ' << sparse_ms << ' ' << dense_ms << ' ' << count_ms << ' ' << sparse << std::endl;}}}
} // namespace

int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20261029;
    bool quick = argc > 2 && string(argv[2]) == "quick";
    ulng index = 0;
    auto next = [&] { return std::mt19937_64(seed + index++); };
    for (int n : {500, 1000, 2000, 4000, 8000, 16000, 32000}) {
        for (double p : {0.005, 0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0}) {
            auto rng = next();
            bool skip = (n == 32000 && p != 0.01 && p != 0.02) || (n < 32000 && p == 0.01);
            if (skip || (quick && n > 1000) || p * n * (n - 1) / 2 > 1.7e7) { continue; }
            bench(std::format("gnp-p{}", p), n, gnp(n, p, rng), p >= 1 ? choose3(n) : -1);}}
    if (!quick) {
        bench("clique2000+path10000", 12000, cliquePlus(2000, 10000, true), choose3(2000));
        bench("clique2000+path100000", 102000, cliquePlus(2000, 100000, true), choose3(2000));
        for (int len : {13384, 13385, 100000}) { bench(std::format("clique3000+isolated{}", len), 3000 + len, cliquePlus(3000, len, false), choose3(3000)); }
        bench("clique3000+path100000", 103000, cliquePlus(3000, 100000, true), choose3(3000));
        for (int a : {1000, 2000, 4000}) { bench(std::format("K{},{}", a, a), 2 * a, bipartite(a), 0); }}
    for (int n : {100000, 500000}) {
        auto rng = next();
        if (!quick || n == 100000) { bench("random-n=m", n, randomSparse(n, n, rng), -1); }}}
