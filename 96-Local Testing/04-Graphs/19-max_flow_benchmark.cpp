#include "../../04-Graphs/19-max_flow.hpp"

namespace {
    struct E { int u, v; lng cap, rcap; };
    struct Net { int n, s, t; vector<E> es; };

    // ACL-style reference: BFS from s, recursive multi-push DFS from t over reverse arcs.
    struct AclStyle {
        struct A { int to, rev; lng cap; };
        int n;
        vector<vector<A>> g;
        vector<int> level, iter;

        explicit AclStyle(int N) : n(N), g(N), level(N), iter(N) {}
        void addEdge(int u, int v, lng c, lng rc) {
            int k = int(g[u].size()), r = int(g[v].size()) + (u == v);
            g[u].push_back({v, r, c}); g[v].push_back({u, k, rc});}
        lng dfs(int v, int s, lng up) {
            if (v == s) { return up; }
            lng res = 0; int lv = level[v];
            for (int &i = iter[v]; i < int(g[v].size()); ++i) {
                A &e = g[v][i];
                if (lv <= level[e.to] || g[e.to][e.rev].cap == 0) { continue; }
                lng d = dfs(e.to, s, min(up - res, g[e.to][e.rev].cap));
                if (d <= 0) { continue; }
                g[v][i].cap += d; g[e.to][e.rev].cap -= d; res += d;
                if (res == up) { return res; }}
            level[v] = n;
            return res;}
        lng flow(int s, int t) {
            lng res = 0;
            while (true) {
                fill(level.begin(), level.end(), -1); level[s] = 0;
                vector<int> q{s};
                for (int i = 0; i < int(q.size()) && level[t] < 0; ++i) {
                    for (auto &e : g[q[i]]) {
                        if (e.cap > 0 && level[e.to] < 0) { level[e.to] = level[q[i]] + 1; q.push_back(e.to); }}}
                if (level[t] < 0) { return res; }
                fill(iter.begin(), iter.end(), 0);
                lng f = dfs(t, s, std::numeric_limits<lng>::max() - res);
                if (f == 0) { return res; }
                res += f;}}
    };

    Net make(const string &shape, std::mt19937_64 &rng) {
        auto cap = [&](lng hi) { return 1 + lng(rng() % ulng(hi)); };
        Net net{0, 0, 0, {}};
        if (shape == "sparse") {
            net.n = 20000; net.t = net.n - 1;
            for (int i = 0; i < 5 * net.n; ++i) { net.es.push_back({int(rng() % net.n), int(rng() % net.n), cap(1000000000), 0}); }}
        else if (shape == "bipartite") {
            int k = 50000; net.n = 2 * k + 2; net.s = 2 * k; net.t = 2 * k + 1;
            for (int i = 0; i < k; ++i) { net.es.push_back({net.s, i, 1, 0}); net.es.push_back({k + i, net.t, 1, 0}); }
            for (int i = 0; i < 4 * k; ++i) { net.es.push_back({int(rng() % k), k + int(rng() % k), 1, 0}); }}
        else if (shape == "dense") {
            net.n = 400; net.t = net.n - 1;
            for (int u = 0; u < net.n; ++u) {
                for (int v = 0; v < net.n; ++v) {
                    if (u != v && rng() % 2) { net.es.push_back({u, v, cap(1000000), 0}); }}}}
        else if (shape == "grid") {
            int w = 300; net.n = w * w; net.t = net.n - 1;
            for (int x = 0; x < w; ++x) {
                for (int y = 0; y < w; ++y) {
                    lng c = cap(1000000), d = cap(1000000);
                    if (x + 1 < w) { net.es.push_back({x * w + y, (x + 1) * w + y, c, c}); }
                    if (y + 1 < w) { net.es.push_back({x * w + y, x * w + y + 1, d, d}); }}}}
        else if (shape == "chain") {
            net.n = 20000; net.t = net.n - 1;
            for (int u = 0; u + 1 < net.n; ++u) {
                net.es.push_back({u, u + 1, cap(1000000000), 0});
                if (u % 3 == 0) { net.es.push_back({u, int(rng() % net.n), cap(1000), 0}); }}}
        else {
            net.n = 50; net.t = net.n - 1;
            for (int i = 0; i < 300; ++i) { net.es.push_back({int(rng() % net.n), int(rng() % net.n), cap(100), 0}); }}
        return net;}
} // namespace

int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20260927;
    std::mt19937_64 rng(seed);
    for (string shape : {"small", "sparse", "bipartite", "dense", "grid", "chain"}) {
        Net net = make(shape, rng);
        int repeats = shape == "small" ? 2000 : 1;
        auto build = [&] {
            MaxFlow<lng> mf(net.n);
            for (auto e : net.es) { mf.addEdge(e.u, e.v, e.cap, e.rcap); }
            return mf;};
        lng expected = build().flow(net.s, net.t);
        {
            auto mf = build(); mf.flow(net.s, net.t); auto side = mf.minCut(net.s); lll cut = 0;
            for (auto e : net.es) { cut += side[e.u] && !side[e.v] ? e.cap : side[e.v] && !side[e.u] ? e.rcap : 0; }
            if (cut != expected) { std::abort(); }}
        auto measure = [&](auto &&run) {
            lng total = 0; auto start = std::chrono::steady_clock::now();
            for (int k = 0; k < repeats; ++k) {
                lng v = run();
                if (v != expected) { std::abort(); }
                total += v;}
            double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
            return pair{ms / repeats, total};};
        auto dinic = [&] { auto mf = build(); return mf.flow(net.s, net.t); };
        auto scaling = [&] { auto mf = build(); return mf.capacityScalingDinic(net.s, net.t); };
        auto pushRelabel = [&] { auto mf = build(); return mf.pushRelabel(net.s, net.t); };
        auto acl = [&] {
            AclStyle mf(net.n);
            for (auto e : net.es) { mf.addEdge(e.u, e.v, e.cap, e.rcap); }
            return mf.flow(net.s, net.t);};
        dinic(); scaling(); pushRelabel(); acl();
        for (int run = 0; run < 5; ++run) {
            auto a = measure(dinic); auto b = measure(scaling); auto c = measure(pushRelabel); auto d = measure(acl);
            cout << shape << ' ' << net.n << ' ' << net.es.size() << ' ' << run << ' ' << repeats << ' ' << a.first << ' '
                 << b.first << ' ' << c.first << ' ' << d.first << ' ' << expected << '\n';}}}
