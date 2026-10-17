#include "../../04-Graphs/31-path_cover.hpp"

namespace {
    ulng seed = 20260927;
    string mode = "full", context;
    lng checks = 0, cases = 0;
    void check(bool ok, const string &operation) {
        ++checks;
        if (!ok) { throw std::runtime_error(context + " operation=" + operation + " expected=true actual=false"); }}
    string show(const Graph &g) {
        std::ostringstream out; out << "n=" << g.n << " arcs=";
        for (auto e : g.edges) { out << e.u << "->" << e.v << ' '; }
        return out.str();}
    Graph randomDag(std::mt19937_64 &rng, int n, int m) {
        vector<int> perm(n); iota(perm.begin(), perm.end(), 0); std::shuffle(perm.begin(), perm.end(), rng);
        Graph g(n, true);
        for (int i = 0; i < m && n > 1; ++i) {
            int a = int(rng() % n), b = int(rng() % n);
            if (a == b) { continue; }
            if (a > b) { swap(a, b); }
            g.addEdge(perm[a], perm[b]);
            if (rng() % 6 == 0) { g.addEdge(perm[a], perm[b]); }}
        return g;}
    // Strict reachability by BFS from every vertex (independent of the bitset closure).
    vector<vector<char>> closure(const Graph &g) {
        vector<vector<char>> r(g.n, vector<char>(g.n));
        for (int s = 0; s < g.n; ++s) {
            vector<int> q{s};
            for (int i = 0; i < int(q.size()); ++i) {
                for (int a : g[q[i]]) {
                    int y = g.arcs[a].to;
                    if (!r[s][y]) { r[s][y] = 1; q.push_back(y); }}}}
        return r;}
    void validatePartition(int n, const vector<vector<int>> &ps, const string &name) {
        vector<int> seen(n);
        for (auto &p : ps) { check(!p.empty(), name + " nonempty"); for (int v : p) { check(0 <= v && v < n && !seen[v]++, name + " partition"); }}
        check(std::count(seen.begin(), seen.end(), 1) == n, name + " covers every vertex");
        for (int i = 1; i < int(ps.size()); ++i) { check(ps[i - 1][0] < ps[i][0], name + " ordered by first vertex"); }}
    void validateAntichain(const vector<vector<char>> &r, const vector<int> &a, const string &name) {
        for (int i = 0; i < int(a.size()); ++i) {
            check(i == 0 || a[i - 1] < a[i], name + " ascending");
            for (int x : a) { check(!r[a[i]][x], name + " pairwise incomparable"); }}}
    void runCase(const Graph &g, std::mt19937_64 &rng) {
        ++cases; context = show(g); int n = g.n; auto r = closure(g);
        vector<vector<char>> arc(n, vector<char>(n));
        for (auto e : g.edges) { arc[e.u][e.v] = 1; }
        // Brute path cover: each vertex picks an out-neighbour successor or none, successors distinct.
        int best = 0; vector<char> taken(n);
        auto rec = [&](auto &&self, int u, int got) -> void {
            if (got + (n - u) <= best) { return; }
            if (u == n) { best = got; return; }
            self(self, u + 1, got);
            for (int v = 0; v < n; ++v) {
                if (arc[u][v] && !taken[v]) { taken[v] = 1; self(self, u + 1, got + 1); taken[v] = 0; }}};
        rec(rec, 0, 0);
        auto paths = dagMinimumPathCover(g);
        validatePartition(n, paths, "dagMinimumPathCover");
        for (auto &p : paths) { for (int i = 1; i < int(p.size()); ++i) { check(arc[p[i - 1]][p[i]], "dagMinimumPathCover follows arcs"); }}
        check(int(paths.size()) == n - best, "dagMinimumPathCover size equals brute");
        vector<lng> w(n);
        for (auto &x : w) { x = rng() % 4 == 0 ? 0 : rng() % 7 == 0 ? lng(rng() >> 2) : lng(rng() % 10); }
        int width = 0; lll heavy = 0;
        for (int mask = 0; mask < (1 << n); ++mask) {
            bool ok = true; lll s = 0;
            for (int i = 0; i < n && ok; ++i) {
                if (!(mask >> i & 1)) { continue; }
                s += w[i];
                for (int j = 0; j < n; ++j) { if ((mask >> j & 1) && r[i][j]) { ok = false; }}}
            if (ok) { width = max(width, __builtin_popcount(uint(mask))); heavy = max(heavy, s); }}
        auto cc = dagMinimumChainCover(g);
        validatePartition(n, cc.chains, "dagMinimumChainCover");
        for (auto &p : cc.chains) { for (int i = 1; i < int(p.size()); ++i) { check(r[p[i - 1]][p[i]], "chain in reachability order"); }}
        check(int(cc.chains.size()) == width && int(cc.antichain.size()) == width, "chain count and antichain size equal the brute width");
        validateAntichain(r, cc.antichain, "ChainCoverResult::antichain");
        auto wa = maximumWeightedAntichain(g, w);
        validateAntichain(r, wa.antichain, "maximumWeightedAntichain");
        lll s = 0;
        for (int v : wa.antichain) { s += w[v]; }
        check(s == wa.weight && wa.weight == heavy, "maximumWeightedAntichain weight equals brute");
        CsrGraph c(g);
        check(dagMinimumPathCover(c) == paths && dagMinimumChainCover(c).chains == cc.chains && maximumWeightedAntichain(c, w).weight == wa.weight, "CSR agrees");}
    void exhaustive() {
        int bound = mode == "quick" ? 3 : 4; std::mt19937_64 rng(seed ^ 0x11);
        for (int n = 0; n <= bound; ++n) {
            vector<pair<int, int>> pairs;
            for (int u = 0; u < n; ++u) { for (int v = 0; v < n; ++v) { if (u != v) { pairs.push_back({u, v}); }}}
            for (int mask = 0; mask < (1 << pairs.size()); ++mask) {
                Graph g(n, true);
                for (int i = 0; i < int(pairs.size()); ++i) { if (mask >> i & 1) { g.addEdge(pairs[i].first, pairs[i].second); }}
                if (topologicalSort(g).acyclic) { runCase(g, rng); }}}
        cout << "PASS every labelled DAG n<=" << bound << " cases=" << cases << '\n';}
    void randomCases() {
        std::mt19937_64 rng(seed);
        int rounds = mode == "quick" ? 300 : mode == "full" ? 3000 : 15000;
        for (int t = 0; t < rounds; ++t) {
            int n = 1 + int(rng() % 10);
            runCase(randomDag(rng, n, int(rng() % (3 * n + 1))), rng);}
        cout << "PASS random DAG multigraphs n<=10 rounds=" << rounds << '\n';}
    // Tester-side Kuhn matching on the arcs: minimum path cover is n minus it.
    int kuhn(const Graph &g) {
        int n = g.n, res = 0; vector<int> mate(n, -1), seen(n, -1);
        auto aug = [&](auto &&self, int u, int stamp) -> bool {
            for (int a : g[u]) {
                int v = g.arcs[a].to;
                if (seen[v] == stamp) { continue; }
                seen[v] = stamp;
                if (mate[v] == -1 || self(self, mate[v], stamp)) { mate[v] = u; return true; }}
            return false;};
        for (int u = 0; u < n; ++u) { res += aug(aug, u, u); }
        return res;}
    void large() {
        std::mt19937_64 rng(seed ^ 0x2f);
        int s = mode == "quick" ? 300 : mode == "full" ? 2000 : 4000;
        for (int shape = 0; shape < 4; ++shape) {
            Graph g(s, true);
            if (shape == 0) { g = randomDag(rng, s, 4 * s); }
            else if (shape == 1) { for (int u = 0; u < s; ++u) { for (int v = u + 1; v < s; ++v) { if (rng() % 10 < 3) { g.addEdge(u, v); }}}}
            else if (shape == 2) { int k = 1; while ((k + 1) * (k + 1) <= s) { ++k; } for (int v = 0; v < s; ++v) { if (v % k + 1 < k && v + 1 < s) { g.addEdge(v, v + 1); } if (v + k < s) { g.addEdge(v, v + k); }}}
            else { for (int v = 0; v + 2 < s; ++v) { g.addEdge(v, v + 2); if (rng() % 50 == 0) { g.addEdge(v, v + 1); }}}
            context = "large shape=" + std::to_string(shape) + " n=" + std::to_string(s);
            auto cc = dagMinimumChainCover(g);
            validatePartition(s, cc.chains, "large chains");
            for (auto &p : cc.chains) {
                for (int i = 1; i < int(p.size()); ++i) {
                    vector<char> vis(s); vector<int> q{p[i - 1]}; vis[p[i - 1]] = 1; bool hit = false;
                    for (int j = 0; j < int(q.size()) && !hit; ++j) {
                        for (int a : g[q[j]]) { int y = g.arcs[a].to; if (!vis[y]) { vis[y] = 1; q.push_back(y); hit |= y == p[i]; }}}
                    check(hit, "large chain step reachable");}}
            vector<char> inA(s);
            for (int v : cc.antichain) { inA[v] = 1; }
            for (int v : cc.antichain) {
                vector<char> vis(s); vector<int> q{v}; vis[v] = 1;
                for (int j = 0; j < int(q.size()); ++j) {
                    for (int a : g[q[j]]) { int y = g.arcs[a].to; if (!vis[y]) { vis[y] = 1; q.push_back(y); check(!inA[y], "large antichain incomparable"); }}}}
            check(cc.antichain.size() == cc.chains.size(), "large Dilworth certificate sizes match");
            auto wa = maximumWeightedAntichain(g, vector<lng>(s, 1));
            check(wa.weight == lll(cc.chains.size()), "unit-weight flow antichain equals bitset chain count");
            auto paths = dagMinimumPathCover(g);
            check(int(paths.size()) == s - kuhn(g), "large path cover versus Kuhn");}
        int n = mode == "quick" ? 5000 : mode == "full" ? 200000 : 400000;
        Graph g = randomDag(rng, n, 3 * n); context = "path cover n=" + std::to_string(n);
        auto paths = dagMinimumPathCover(g);
        vector<int> seen(n);
        for (auto &p : paths) {
            for (int i = 0; i < int(p.size()); ++i) {
                check(!seen[p[i]]++, "huge path partition");
                if (i) { check(std::ranges::any_of(g[p[i - 1]], [&](int a) { return g.arcs[a].to == p[i]; }), "huge path follows arcs"); }}}
        check(std::count(seen.begin(), seen.end(), 1) == n, "huge path cover covers every vertex");
        vector<lng> big(min(n, 2000), LLONG_MAX); Graph anti(int(big.size()), true);
        auto wa = maximumWeightedAntichain(anti, big);
        check(wa.weight == lll(LLONG_MAX) * lll(big.size()) && int(wa.antichain.size()) == anti.n, "edgeless full-range weights");
        cout << "PASS large chain covers n=" << s << " (random, dense, grid, two chains) with certificates, path cover n=" << n << '\n';}
    void invalid(const string &name) {
        Graph cyc(2, true); cyc.addEdge(0, 1); cyc.addEdge(1, 0);
        if (name == "undirected") { dagMinimumPathCover(Graph(2)); }
        else if (name == "path-cyclic") { dagMinimumPathCover(cyc); }
        else if (name == "chain-cyclic") { dagMinimumChainCover(cyc); }
        else if (name == "weighted-cyclic") { maximumWeightedAntichain(cyc, {1, 1}); }
        else if (name == "weight-size") { maximumWeightedAntichain(Graph(2, true), {1}); }
        else if (name == "weight-negative") { maximumWeightedAntichain(Graph(2, true), {1, -1}); }
        else { throw std::runtime_error("unknown invalid probe " + name); }}
} // namespace

int main(int argc, char **argv) {
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); return 2; }}
        exhaustive(); randomCases(); large();
        cout << "PASS path_cover seed=" << seed << " mode=" << mode << " cases=" << cases << " checks=" << checks << '\n';}
    catch (const std::exception &e) {
        cerr << "FAIL path_cover seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1;}}
