#include "../../04-Graphs/08-scc.hpp"

namespace {
    ulng seed = 20260927;
    string mode = "full", context;
    lng checks = 0, cases = 0;
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
            check(reach[u][v] == condensed[r.component[u]][r.component[v]], "condensation preserves reachability");}}}
    template<class G>
    void graphCase(const G &g, const vector<vector<uint8_t>> &reach) {
        result(g, tarjanScc(g), reach); result(g, kosarajuScc(g), reach);}
    void runCase(const Graph &g) {
        ++cases; context = show(g); auto reach = closure(g);
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
        for (int method = 0; method < 2; ++method) {
            auto r = method ? kosarajuScc(g) : tarjanScc(g);
            check(int(r.groups.size()) == (cycle ? 1 : g.n), "large partition size");
            for (int u = 0; u < g.n; ++u) { check(r.component[u] == (cycle ? 0 : u), "large component order"); }
            auto dag = condensationDag(g, r);
            check(int(dag.edges.size()) == (cycle ? 0 : g.n - 1), "large condensation size");}}
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
        cout << "PASS stack-safe chains/cycles n=" << n << ", cross-edge regression, result ownership and graph mutation\n";}
    void invalid(const string &name) {
        Graph g(2);
        if (name == "tarjan-undirected") { tarjanScc(g); }
        else if (name == "kosaraju-undirected") { kosarajuScc(g); }
        else if (name == "condensation-undirected") { condensationDag(g, SccResult{vector<int>(2), {{0, 1}}}); }
        else if (name == "condensation-size") { condensationDag(Graph(2, true), SccResult{}); }
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
