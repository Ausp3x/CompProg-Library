#include "../../04-Graphs/008-scc.hpp"

namespace {
    ulng seed = 20260927;
    string mode = "full", context;
    lng checks = 0, cases = 0;
    vector<lng> weight;
    void check(bool ok, const string &operation) {
        ++checks;
        if (!ok) { throw std::runtime_error(context + " operation=" + operation + " expected=true actual=false"); }}
    string show(const Graph &g) {
        std::ostringstream out; out << "n=" << g.n << " edges=";
        for (auto e : g.edges) { out << '(' << e.u << ',' << e.v << ',' << e.w << ')'; }
        return out.str();}
    vector<vector<uint8_t>> closure(const Graph &g) {
        vector<vector<uint8_t>> r(g.n, vector<uint8_t>(g.n));
        for (int u = 0; u < g.n; ++u) { r[u][u] = true; }
        for (auto e : g.edges) { r[e.u][e.v] = true; }
        for (int k = 0; k < g.n; ++k) { for (int i = 0; i < g.n; ++i) {
            for (int j = 0; j < g.n; ++j) { r[i][j] |= r[i][k] && r[k][j]; }}}
        return r;}
    template<class G>
    void result(const G &g, const SccResult &r, const vector<vector<uint8_t>> &reach) {
        check(int(r.component.size()) == g.n, "component domain");
        int k = int(r.groups.size()); vector<int> count(g.n);
        for (int c = 0; c < k; ++c) {
            check(!r.groups[c].empty(), "nonempty group");
            for (int u : r.groups[c]) {
                check(0 <= u && u < g.n, "group vertex domain");
                check(r.component[u] == c && ++count[u] == 1, "partition and group label");}}
        for (int u = 0; u < g.n; ++u) {
            check(count[u] == 1, "partition covers every vertex");
            for (int v = 0; v < g.n; ++v) {
                check((r.component[u] == r.component[v]) == bool(reach[u][v] && reach[v][u]),
                      "SCC iff mutually reachable (Floyd closure)");}}
        set<pair<int, int>> expected, actual;
        for (auto e : g.edges) {
            int a = r.component[e.u], b = r.component[e.v];
            if (a != b) { check(a < b, "source-to-sink component order"); expected.emplace(a, b); }}
        auto dag = condensationDag(g, r);
        check(dag.n == k && dag.directed, "condensation size and direction");
        for (auto e : dag.edges) {
            check(e.u < e.v && e.w == 1, "condensation ordered unit edges");
            check(actual.emplace(e.u, e.v).second, "no parallel condensation edges");}
        check(actual == expected, "exact intercomponent edge set");
        auto condensed = closure(dag);
        for (int u = 0; u < g.n; ++u) { for (int v = 0; v < g.n; ++v) {
            check(reach[u][v] == condensed[r.component[u]][r.component[v]], "condensation preserves reachability");}}
        // Sources and sinks from the closure: a sink reaches nothing outside, a source is reached from nothing outside.
        vector<uint8_t> source(k, 1), sink(k, 1);
        bool semi = true;
        for (int u = 0; u < g.n; ++u) { for (int v = 0; v < g.n; ++v) {
            semi = semi && (reach[u][v] || reach[v][u]);
            if (reach[u][v] && r.component[u] != r.component[v]) { sink[r.component[u]] = 0; source[r.component[v]] = 0; }}}
        vector<int> attracting;
        for (int c = 0; c < k; ++c) { if (sink[c]) { attracting.push_back(c); }}
        check(attractingComponents(g, r) == attracting, "attracting components (closure)");
        check(isSemiconnected(g, r) == semi, "semiconnected iff every pair comparable (closure)");
        auto added = strongConnectivityAugmentation(g, r);
        if (k >= 2) { check(!reach[added[0].first][added[0].second], "first augmentation arc is unreachable (CSES 1682 witness)"); }
        int sources = int(std::count(source.begin(), source.end(), 1)), sinks = int(attracting.size());
        check(int(added.size()) == (k <= 1 ? 0 : max(sources, sinks)), "augmentation size max(sources, sinks)");
        Graph augmented(g.n, true);
        for (auto e : g.edges) { augmented.addEdge(e.u, e.v); }
        for (auto [u, v] : added) {
            check(0 <= u && u < g.n && 0 <= v && v < g.n && u != v, "augmentation arc domain, no loops");
            check(r.groups[r.component[u]][0] == u && r.groups[r.component[v]][0] == v, "augmentation between representatives");
            augmented.addEdge(u, v);}
        auto strong = closure(augmented);
        for (int u = 0; u < g.n; ++u) { for (int v = 0; v < g.n; ++v) { check(strong[u][v], "augmented graph strongly connected (closure)"); }}
        // Heaviest chain of components ordered by reachability, memoized over the closure.
        vector<lll> mass(k), best(k, -1);
        for (int u = 0; u < g.n; ++u) { mass[r.component[u]] += weight[u]; }
        auto chain = [&](auto &self, int c) -> lll {
            if (best[c] >= 0) { return best[c]; }
            lll tail = 0;
            for (int d = 0; d < k; ++d) {
                if (d != c && reach[r.groups[c][0]][r.groups[d][0]]) { tail = max(tail, self(self, d)); }}
            return best[c] = mass[c] + tail;};
        lll value = 0;
        for (int c = 0; c < k; ++c) { value = max(value, chain(chain, c)); }
        auto [total, walk] = maximumWeightWalk(g, r, weight);
        check(total == value, "walk value (closure chains)");
        check(walk.empty() == (g.n == 0), "walk empty iff n == 0");
        lll sum = 0; vector<uint8_t> used(k);
        for (int i = 0; i < int(walk.size()); ++i) {
            int c = walk[i];
            check(0 <= c && c < k && !used[c], "walk component domain, distinct");
            used[c] = 1;
            for (int u : r.groups[c]) { sum += weight[u]; }
            if (i) { check(expected.count({walk[i - 1], c}) == 1, "consecutive walk components joined by an arc"); }}
        check(sum == value, "walk members sum to value");
        // Documented witness: end at the least ID of the heaviest walk ending there; step back to the least-ID in-neighbour of heaviest positive value.
        vector<lll> ending(k, -1);
        auto into = [&](auto &self, int c) -> lll {
            if (ending[c] >= 0) { return ending[c]; }
            lll head = 0;
            for (int d = 0; d < k; ++d) {
                if (d != c && reach[r.groups[d][0]][r.groups[c][0]]) { head = max(head, self(self, d)); }}
            return ending[c] = mass[c] + head;};
        vector<int> witness;
        for (int c = 0; c < k; ++c) { if (into(into, c) > (witness.empty() ? -1 : ending[witness[0]])) { witness = {c}; }}
        while (!witness.empty()) {
            int c = witness.back(), pred = -1;
            for (int d = 0; d < k; ++d) {
                if (expected.count({d, c}) && into(into, d) > 0 && (pred == -1 || ending[d] > ending[pred])) { pred = d; }}
            if (pred == -1) { break; }
            witness.push_back(pred);}
        reverse(witness.begin(), witness.end());
        check(walk == witness, "walk exact documented witness");}
    // Minimum realization by exhaustive search over all loopless arc subsets, n <= 4: closure mask -> fewest arcs.
    template<class G>
    int exhaustiveRealization(const G &g) {
        static array<vector<pair<int, int>>, 5> table;
        int n = g.n; auto &t = table[n];
        if (t.empty()) {
            vector<int> bits;
            for (int u = 0; u < n; ++u) { for (int v = 0; v < n; ++v) { if (u != v) { bits.push_back(u * n + v); }}}
            map<int, int> best;
            for (int s = 0; s < (1 << int(bits.size())); ++s) {
                int m = 0;
                for (int i = 0; i < int(bits.size()); ++i) { if ((s >> i) & 1) { m |= 1 << bits[i]; }}
                for (int x = 0; x < n; ++x) { for (int i = 0; i < n; ++i) { for (int j = 0; j < n; ++j) {
                    if ((m >> (i * n + x) & 1) && (m >> (x * n + j) & 1)) { m |= 1 << (i * n + j); }}}}
                int cost = __builtin_popcount(unsigned(s));
                auto [it, fresh] = best.emplace(m, cost);
                if (!fresh) { it->second = min(it->second, cost); }}
            t.assign(best.begin(), best.end());}
        int q = 0, res = INT_MAX;
        for (auto e : g.edges) { if (e.u != e.v) { q |= 1 << (e.u * n + e.v); }}
        for (auto [m, cost] : t) { if ((m & q) == q) { res = min(res, cost); }}
        return res;}
    // Weak components of the requests by label propagation: label = least vertex of the component.
    template<class G>
    vector<int> weakLabels(const G &g) {
        vector<int> label(g.n); iota(label.begin(), label.end(), 0);
        for (bool changed = true; changed;) {
            changed = false;
            for (auto e : g.edges) {
                int low = min(label[e.u], label[e.v]);
                if (label[e.u] != low || label[e.v] != low) { label[e.u] = label[e.v] = low; changed = true; }}}
        return label;}
    vector<uint8_t> cyclicLabels(const vector<int> &label, const vector<vector<uint8_t>> &reach) {
        int n = int(label.size()); vector<uint8_t> cyclic(n);
        for (int u = 0; u < n; ++u) { for (int v = 0; v < n; ++v) { if (u != v && reach[u][v] && reach[v][u]) { cyclic[label[u]] = 1; }}}
        return cyclic;}
    // Per weak component of the requests: size - 1, or size when it holds a directed cycle of length >= 2.
    template<class G>
    int realizationCount(const G &g, const vector<vector<uint8_t>> &reach) {
        auto label = weakLabels(g); auto cyclic = cyclicLabels(label, reach); vector<int> size(g.n);
        for (int u = 0; u < g.n; ++u) { ++size[label[u]]; }
        int res = 0;
        for (int u = 0; u < g.n; ++u) { if (size[u]) { res += size[u] - 1 + cyclic[u]; }}
        return res;}
    // Documented order: each weak component's vertices chained in tarjanScc group order, then (last, first) per cyclic component by first vertex.
    template<class G>
    vector<pair<int, int>> realizationOrder(const G &g, const vector<vector<uint8_t>> &reach) {
        auto label = weakLabels(g); auto cyclic = cyclicLabels(label, reach);
        vector<int> first(g.n, -1), last(g.n, -1); vector<pair<int, int>> res, closing;
        for (const auto &group : tarjanScc(g).groups) { for (int u : group) {
            int c = label[u];
            if (last[c] == -1) { first[c] = u; }
            else { res.push_back({last[c], u}); }
            last[c] = u;}}
        for (int c = 0; c < g.n; ++c) { if (cyclic[c]) { closing.push_back({first[c], last[c]}); }}
        sort(closing.begin(), closing.end());
        for (auto [u, v] : closing) { res.push_back({v, u}); }
        return res;}
    template<class G>
    void graphCase(const G &g, const vector<vector<uint8_t>> &reach) {
        result(g, tarjanScc(g), reach); result(g, kosarajuScc(g), reach);
        auto arcs = reachabilityRealization(g); Graph h(g.n, true);
        for (auto [u, v] : arcs) {
            check(0 <= u && u < g.n && 0 <= v && v < g.n && u != v, "realization arc domain, no loops");
            h.addEdge(u, v);}
        auto got = closure(h);
        for (auto e : g.edges) { check(got[e.u][e.v], "every request realized (closure)"); }
        int count = realizationCount(g, reach);
        if (g.n <= 4) { check(exhaustiveRealization(g) == count, "count formula equals exhaustive minimum"); }
        check(int(arcs.size()) == count, "realization minimum");
        check(arcs == realizationOrder(g, reach), "realization exact documented order");}
    void runCase(const Graph &g) {
        ++cases; context = show(g); auto reach = closure(g);
        std::mt19937_64 rng(seed ^ ulng(cases) * 0x9E3779B97F4A7C15ULL); weight.assign(g.n, 0);
        for (lng &x : weight) {
            ulng kind = rng() % 5;
            x = kind == 0 ? 0 : kind == 1 ? lng(rng() % 4) : kind == 2 ? lng(1) << 62 : kind == 3 ? std::numeric_limits<lng>::max()
                : lng(rng() % ((ulng(1) << 62) + 1));}
        graphCase(g, reach); graphCase(CsrGraph(g), reach);
        check(show(g) == context, "graph unchanged");}
    void exhaustive() {
        for (bool loops : {false, true}) {
            int bound = loops ? (mode == "quick" ? 2 : mode == "full" ? 3 : 4) : (mode == "quick" ? 3 : 4);
            for (int n = 0; n <= bound; ++n) {
                vector<pair<int, int>> possible;
                for (int u = 0; u < n; ++u) { for (int v = 0; v < n; ++v) {
                    if (loops || u != v) { possible.emplace_back(u, v); }}}
                for (int mask = 0; mask < (1 << int(possible.size())); ++mask) {
                    Graph g(n, true);
                    for (int i = 0; i < int(possible.size()); ++i) {
                        if ((mask >> i) & 1) { g.addEdge(possible[i].first, possible[i].second); }}
                    runCase(g);}}}
        for (int mask = 0; mask < 81; ++mask) {
            int value = mask; Graph g(2, true);
            for (int u = 0; u < 2; ++u) { for (int v = 0; v < 2; ++v) {
                for (int i = 0; i < value % 3; ++i) { g.addEdge(u, v); } value /= 3;}}
            runCase(g);}
        cout << "PASS exhaustive digraphs and multiplicity-2 graphs cases=" << cases << '\n';}
    void randomCases() {
        std::mt19937_64 rng(seed);
        int rounds = mode == "quick" ? 150 : mode == "full" ? 1800 : 10000;
        for (int trial = 0; trial < rounds; ++trial) {
            int n = 1 + int(rng() % 12), m = int(rng() % 70); Graph g(n, true);
            for (int i = 0; i < m; ++i) {
                g.addEdge(int(rng() % n), int(rng() % n),
                    i % 2 ? std::numeric_limits<lng>::min() : std::numeric_limits<lng>::max());}
            runCase(g);
            if (trial % 8 == 0) {
                runCase(g.reverse());
                vector<int> permutation(n); iota(permutation.begin(), permutation.end(), 0);
                std::shuffle(permutation.begin(), permutation.end(), rng); Graph relabeled(n, true);
                for (auto e : g.edges) { relabeled.addEdge(permutation[e.u], permutation[e.v], 0); }
                runCase(relabeled);}}
        cout << "PASS random multigraph closure, reversal/relabeling and extreme ignored weights rounds=" << rounds << '\n';}
    template<class G>
    void largeCase(const G &g, bool cycle) {
        vector<lng> w(g.n); lll total = 0;
        for (int u = 0; u < g.n; ++u) { w[u] = (lng(1) << 62) - u; total += w[u]; }
        for (int method = 0; method < 2; ++method) {
            auto r = method ? kosarajuScc(g) : tarjanScc(g);
            check(int(r.groups.size()) == (cycle ? 1 : g.n), "large partition size");
            for (int u = 0; u < g.n; ++u) { check(r.component[u] == (cycle ? 0 : u), "large component order"); }
            auto dag = condensationDag(g, r);
            check(int(dag.edges.size()) == (cycle ? 0 : g.n - 1), "large condensation size");
            check(attractingComponents(g, r) == vector<int>{cycle ? 0 : g.n - 1}, "large attracting component");
            check(isSemiconnected(g, r), "large semiconnected");
            check(strongConnectivityAugmentation(g, r) == (cycle ? vector<pair<int, int>>{} : vector<pair<int, int>>{{g.n - 1, 0}}),
                  "large augmentation closes the chain");
            auto [value, walk] = maximumWeightWalk(g, r, w);
            check(value == total && walk.size() == r.groups.size(), "large walk takes every vertex");
            for (int i = 0; i < int(walk.size()); ++i) { check(walk[i] == i, "large walk order"); }}
        auto arcs = reachabilityRealization(g);
        check(int(arcs.size()) == (cycle ? g.n : g.n - 1), "large realization size");
        // Chain: the only minimum realization is the chain. Cycle: n arcs with distinct tails tracing one Hamiltonian cycle.
        vector<int> succ(g.n, -1);
        for (auto [u, v] : arcs) {
            check(0 <= u && u < g.n && 0 <= v && v < g.n && u != v && succ[u] == -1, "large realization arcs");
            succ[u] = v;}
        vector<uint8_t> seen(g.n); int u = 0;
        for (int i = 0; i < (cycle ? g.n : g.n - 1); ++i) {
            check(!seen[u] && (cycle || succ[u] == u + 1), "large realization traces the chain or a Hamiltonian cycle");
            seen[u] = 1; u = succ[u];}
        check(u == (cycle ? 0 : g.n - 1), "large realization closes");}
    void boundaries() {
        int n = mode == "quick" ? 20000 : mode == "full" ? 200000 : 500000;
        Graph g(n, true);
        for (int u = 1; u < n; ++u) { g.addEdge(u - 1, u); }
        context = "chain n=" + std::to_string(n); largeCase(g, false); largeCase(CsrGraph(g), false);
        g.addEdge(n - 1, 0); context = "cycle n=" + std::to_string(n);
        largeCase(g, true); largeCase(CsrGraph(g), true);
        Graph fixture(6, true);
        fixture.addEdge(0, 1); fixture.addEdge(1, 0); fixture.addEdge(2, 0);
        fixture.addEdge(2, 3); fixture.addEdge(3, 4); fixture.addEdge(4, 2); fixture.addEdge(3, 1);
        runCase(fixture); auto copy = tarjanScc(fixture); auto moved = std::move(copy);
        result(fixture, moved, closure(fixture));
        CsrGraph snapshot(fixture); auto reach = closure(fixture);
        fixture.addEdge(0, 4); runCase(fixture); graphCase(snapshot, reach);
        // Two cyclic weak components whose closing arcs sort differently by first and by last vertex.
        Graph twin(4, true);
        twin.addEdge(0, 3); twin.addEdge(3, 0); twin.addEdge(1, 2); twin.addEdge(2, 1);
        runCase(twin);
        cout << "PASS stack-safe chains/cycles n=" << n << " with sink/semiconnected/augmentation/walk/realization, cross-edge regression, result ownership and graph mutation\n";}
    void invalid(const string &name) {
        Graph g(2);
        if (name == "tarjan-undirected") { tarjanScc(g); }
        else if (name == "kosaraju-undirected") { kosarajuScc(g); }
        else if (name == "condensation-undirected") { condensationDag(g, SccResult{vector<int>(2), {{0, 1}}}); }
        else if (name == "condensation-size") { condensationDag(Graph(2, true), SccResult{}); }
        else if (name == "attracting-undirected") { attractingComponents(g, SccResult{vector<int>(2), {{0, 1}}}); }
        else if (name == "attracting-size") { attractingComponents(Graph(2, true), SccResult{}); }
        else if (name == "semi-undirected") { isSemiconnected(g, SccResult{vector<int>(2), {{0, 1}}}); }
        else if (name == "semi-size") { isSemiconnected(Graph(2, true), SccResult{}); }
        else if (name == "augment-undirected") { strongConnectivityAugmentation(g, SccResult{vector<int>(2), {{0, 1}}}); }
        else if (name == "augment-size") { strongConnectivityAugmentation(Graph(2, true), SccResult{}); }
        else if (name == "walk-negative") { Graph d(2, true); maximumWeightWalk(d, tarjanScc(d), {0, -1}); }
        else if (name == "walk-size") { Graph d(2, true); maximumWeightWalk(d, tarjanScc(d), {0}); }
        else if (name == "realize-undirected") { reachabilityRealization(g); }
        else if (name == "condensation-size-large") { condensationDag(Graph(2, true), SccResult{vector<int>(3), {{0, 1}}}); }
        else if (name == "attracting-size-large") { attractingComponents(Graph(2, true), SccResult{vector<int>(3), {{0, 1}}}); }
        else if (name == "semi-size-large") { isSemiconnected(Graph(2, true), SccResult{vector<int>(3), {{0, 1}}}); }
        else if (name == "walk-size-large") { Graph d(2, true); maximumWeightWalk(d, tarjanScc(d), {0, 0, 0}); }
        else { throw std::runtime_error("unknown invalid probe " + name); }}
} // namespace

int main(int argc, char **argv) {
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); return 2; }}
        exhaustive(); randomCases(); boundaries();
        cout << "PASS SCC seed=" << seed << " mode=" << mode << " cases=" << cases << " checks=" << checks << '\n';} catch (const std::exception &e) {
        cerr << "FAIL SCC seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1;}}
