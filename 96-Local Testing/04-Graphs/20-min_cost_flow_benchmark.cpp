#include "../../04-Graphs/20-min_cost_flow.hpp"

namespace {
    struct E { int u, v; lng cap, cost; };
    struct Net { int n, s, t; vector<E> es; };
    std::mt19937_64 rng;
    lng rand(lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }

    // ACL-style reference: nonnegative costs, one path per Dijkstra stopping at t, duals moved for settled vertices only.
    // ZERO keeps ACL's stack for zero reduced-cost arcs; DENSE scans all vertices instead of a heap (O(n^2) per phase).
    template<bool ZERO, bool DENSE>
    struct Reference {
        struct A { int to, rev; lng cap, cost; };
        int n;
        vector<vector<A>> g;
        explicit Reference(int N) : n(N), g(N) {}
        void addEdge(int u, int v, lng cap, lng cost) {
            int k = int(g[u].size()), r = int(g[v].size()) + (u == v);
            g[u].push_back({v, r, cap, cost}); g[v].push_back({u, k, 0, -cost});}
        pair<lng, lng> flow(int s, int t) {
            vector<lng> dual(n), dist(n);
            vector<int> pe(n), low;
            vector<char> vis(n);
            vector<pair<lng, int>> heap;
            lng flow = 0, cost = 0;
            const lng INF = std::numeric_limits<lng>::max();
            while (true) {
                fill(dist.begin(), dist.end(), INF); fill(vis.begin(), vis.end(), 0);
                dist[s] = 0; heap.assign(1, {0, s}); low.clear();
                while (true) {
                    int u = -1;
                    if (DENSE) {
                        for (int v = 0; v < n; ++v) {
                            if (!vis[v] && dist[v] < INF && (u < 0 || dist[v] < dist[u])) { u = v; }}
                        if (u < 0) { break; }}
                    else if (!low.empty()) { u = low.back(); low.pop_back(); }
                    else if (!heap.empty()) { std::pop_heap(heap.begin(), heap.end(), std::greater<>()); u = heap.back().second; heap.pop_back(); }
                    else { break; }
                    if (vis[u]) { continue; }
                    vis[u] = 1;
                    if (u == t) { break; }
                    for (auto &a : g[u]) {
                        if (a.cap == 0 || vis[a.to]) { continue; }
                        lng d = dist[u] + a.cost + dual[u] - dual[a.to];
                        if (d >= dist[a.to]) { continue; }
                        dist[a.to] = d; pe[a.to] = a.rev;
                        if (DENSE) { continue; }
                        if (ZERO && d == dist[u]) { low.push_back(a.to); }
                        else { heap.emplace_back(d, a.to); std::push_heap(heap.begin(), heap.end(), std::greater<>()); }}}
                if (!vis[t]) { return {flow, cost}; }
                for (int v = 0; v < n; ++v) {
                    if (vis[v]) { dual[v] += dist[v] - dist[t]; }}
                lng f = INF;
                for (int v = t; v != s; v = g[v][pe[v]].to) { auto &b = g[v][pe[v]]; f = min(f, g[b.to][b.rev].cap); }
                for (int v = t; v != s; v = g[v][pe[v]].to) { auto &b = g[v][pe[v]]; b.cap += f; g[b.to][b.rev].cap -= f; }
                flow += f; cost += f * (dual[t] - dual[s]);}}
    };

    Net sparse(int n, int m) {
        Net net{n, 0, n - 1, {}};
        for (int i = 0; i < m; ++i) { net.es.push_back({int(rng() % n), int(rng() % n), rand(1, 100), rand(0, 1000)}); }
        return net;}
    Net bipartite(int k) {
        Net net{2 * k + 2, 2 * k, 2 * k + 1, {}};
        for (int i = 0; i < k; ++i) { net.es.push_back({2 * k, i, 1, 0}); net.es.push_back({k + i, 2 * k + 1, 1, 0}); }
        for (int i = 0; i < k; ++i) {
            for (int j = 0; j < k; ++j) { net.es.push_back({i, k + j, 1, rand(0, 1000000)}); }}
        return net;}
    Net grid(int w) {
        Net net{w * w, 0, w * w - 1, {}};
        for (int r = 0; r < w; ++r) {
            for (int c = 0; c < w; ++c) {
                int v = r * w + c;
                if (c + 1 < w) { net.es.push_back({v, v + 1, rand(1, 10), rand(0, 1000)}); net.es.push_back({v + 1, v, rand(1, 10), rand(0, 1000)}); }
                if (r + 1 < w) { net.es.push_back({v, v + w, rand(1, 10), rand(0, 1000)}); net.es.push_back({v + w, v, rand(1, 10), rand(0, 1000)}); }}}
        return net;}
    Net dense(int n) {
        Net net{n, 0, n - 1, {}};
        for (int u = 0; u < n; ++u) {
            for (int v = 0; v < n; ++v) { if (u != v && rng() % 2) { net.es.push_back({u, v, rand(1, 10), rand(0, 1000)}); }}}
        return net;}

    template<class F>
    double time(F &&f) {
        auto start = std::chrono::steady_clock::now();
        f();
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();}
    void verify(bool ok, const string &what) {
        if (!ok) { cerr << "FAIL benchmark " << what << '\n'; std::exit(1); }}

    // Library, ACL-style reference, reference without the zero reduced-cost stack, dense-scan reference.
    void compareEngines(const string &shape, const Net &net, bool withDense) {
        auto lib = [&] { MinCostFlow<lng, lng> mf(net.n); for (auto e : net.es) { mf.addEdge(e.u, e.v, e.cap, e.cost); } auto r = mf.flow(net.s, net.t); return pair<lng, lng>{r.first, lng(r.second)}; };
        auto ref = [&]<bool Z, bool D>() { Reference<Z, D> mf(net.n); for (auto e : net.es) { mf.addEdge(e.u, e.v, e.cap, e.cost); } return mf.flow(net.s, net.t); };
        auto want = lib();
        verify(ref.template operator()<true, false>() == want && ref.template operator()<false, false>() == want && (!withDense || ref.template operator()<false, true>() == want), shape);
        for (int run = 0; run < 5; ++run) {
            double a = time(lib), b = time([&] { ref.template operator()<true, false>(); }), c = time([&] { ref.template operator()<false, false>(); });
            double d = withDense ? time([&] { ref.template operator()<false, true>(); }) : -1;
            cout << "engines " << shape << ' ' << net.n << ' ' << net.es.size() << ' ' << run << ' ' << a << ' ' << b << ' ' << c << ' ' << d << ' ' << want.first << ' ' << want.second << std::endl;}}
    // x^2 convex costs: one addConvexEdge of p unit pieces (costs 1, 3, 5, ...) against p parallel addEdge arcs.
    void compareConvex(int n, int m, int p) {
        vector<array<int, 2>> ends(m);
        for (auto &e : ends) { e = {int(rng() % n), int(rng() % n)}; }
        vector<pair<lng, lng>> pieces;
        for (int i = 0; i < p; ++i) { pieces.emplace_back(1, 2 * i + 1); }
        auto convex = [&] { MinCostFlow<lng, lng> mf(n); for (auto [u, v] : ends) { mf.addConvexEdge(u, v, pieces); } return mf.flow(0, n - 1); };
        auto parallel = [&] { MinCostFlow<lng, lng> mf(n); for (auto [u, v] : ends) { for (auto [len, c] : pieces) { mf.addEdge(u, v, len, c); }} return mf.flow(0, n - 1); };
        auto want = convex();
        verify(parallel() == want, "convex");
        for (int run = 0; run < 5; ++run) {
            cout << "convex x^2 " << n << ' ' << m << ' ' << run << ' ' << time(convex) << ' ' << time(parallel) << ' ' << p << ' ' << want.first << ' ' << lng(want.second) << std::endl;}}
    // Negative-cost DAG labelled against its order (interval scheduling): Kahn-order initialisation against the same graph labelled in order.
    void compareNegative(int n) {
        vector<E> es;
        for (int i = 0; i + 1 < n; ++i) { es.push_back({i, i + 1, 10, 0}); }
        for (int i = 0; i < n; ++i) { int l = int(rng() % (n - 1)), len = int(rand(1, 50)); es.push_back({l, min(n - 1, l + len), 1, -rand(1, 1000000)}); }
        auto run = [&](bool reversed) {
            MinCostFlow<lng, lng> mf(n);
            for (auto e : es) { mf.addEdge(reversed ? n - 1 - e.u : e.u, reversed ? n - 1 - e.v : e.v, e.cap, e.cost); }
            auto r = mf.flow(reversed ? n - 1 : 0, reversed ? 0 : n - 1);
            return pair<lng, lng>{r.first, lng(r.second)};};
        auto want = run(false);
        verify(run(true) == want, "negative DAG");
        for (int r = 0; r < 5; ++r) { cout << "negative dag " << n << ' ' << es.size() << ' ' << r << ' ' << time([&] { run(false); }) << ' ' << time([&] { run(true); }) << ' ' << want.first << ' ' << want.second << std::endl; }}
} // namespace

int main(int argc, char **argv) {
    rng.seed(argc > 1 ? std::stoull(argv[1]) : 20261013);
    compareEngines("small", sparse(50, 300), true);
    compareEngines("dense", dense(300), true);
    compareEngines("bipartite", bipartite(300), true);
    compareEngines("grid", grid(100), false);
    compareEngines("sparse", sparse(5000, 50000), false);
    compareConvex(200, 1000, 50);
    compareNegative(200000);}
