#include "../../04-Graphs/10-euleriantrail.hpp"

namespace {
    ulng seed = 20260927;
    string mode = "full", context;
    lng checks = 0, cases = 0;
    void check(bool ok, const string &operation) {
        ++checks;
        if (!ok) { throw std::runtime_error(context + " operation=" + operation + " expected=true actual=false"); }}
    string show(const Graph &g) {
        std::ostringstream out; out << "n=" << g.n << " directed=" << g.directed << " edges=";
        for (auto e : g.edges) { out << '(' << e.u << ',' << e.v << ',' << e.w << ')'; }
        return out.str(); }
    uint oracle(const Graph &g) {
        // dp[mask][v] is the set of possible starting vertices of trails
        // ending at v and using exactly mask. No degree/connectivity theorem.
        int m = int(g.edges.size());
        vector<vector<uint>> dp(1 << m, vector<uint>(g.n));
        for (int u = 0; u < g.n; ++u) { dp[0][u] = 1U << u; }
        for (int mask = 0; mask < (1 << m); ++mask) {
            for (int id = 0; id < m; ++id) {
                if ((mask >> id) & 1) { continue; }
                auto e = g.edges[id]; int next = mask | (1 << id);
                dp[next][e.v] |= dp[mask][e.u];
                if (!g.directed) { dp[next][e.u] |= dp[mask][e.v]; }}}
        uint starts = 0; for (uint bits : dp.back()) { starts |= bits; } return starts; }
    template<class G>
    void result(const G &g, const EulerTrailResult &r, int start, bool exists) {
        check(r.exists == exists, "edge-subset path existence start=" + std::to_string(start));
        if (!exists) {
            check(r.vertices.empty() && r.edges.empty() && r.arcs.empty(), "failure empty witnesses"); return; }
        check(r.edges.size() == g.edges.size() && r.arcs.size() == g.edges.size(), "edge witness lengths");
        check(r.vertices.size() == g.edges.size() + (g.n != 0), "vertex witness length");
        if (!g.n) { return; }
        if (start != -1) { check(r.vertices.front() == start, "requested start"); }
        if (g.edges.empty() && start == -1) { check(r.vertices == vector<int>{0}, "edgeless automatic start"); }
        vector<uint8_t> used(g.edges.size());
        for (int i = 0; i < int(r.edges.size()); ++i) {
            int id = r.edges[i], a = r.arcs[i];
            check(0 <= id && id < int(g.edges.size()) && !used[id], "each logical edge once"); used[id] = true;
            check(0 <= a && a < int(g.arcs.size()), "arc ID domain");
            auto e = g.arcs[a];
            check(e.id == id && e.from == r.vertices[i] && e.to == r.vertices[i + 1], "oriented arc/vertex certificate"); }}
    template<class G>
    void graphCase(const G &g, uint starts) {
        result(g, eulerianTrail(g), -1, !g.n || starts);
        for (int start = 0; start < g.n; ++start) {
            result(g, eulerianTrail(g, start), start, (starts >> start) & 1); }}
    void runCase(const Graph &g) {
        ++cases; context = show(g); uint starts = oracle(g);
        graphCase(g, starts); graphCase(CsrGraph(g), starts);
        check(show(g) == context, "graph unchanged"); }
    void exhaustive() {
        for (bool directed : {false, true}) {
            int bound = directed ? (mode == "quick" ? 2 : 3) : (mode == "quick" ? 3 : 4);
            for (int n = 0; n <= bound; ++n) {
                vector<pair<int, int>> possible;
                for (int u = 0; u < n; ++u) { for (int v = directed ? 0 : u; v < n; ++v) {
                    possible.emplace_back(u, v); }}
                for (int mask = 0; mask < (1 << int(possible.size())); ++mask) {
                    Graph g(n, directed);
                    for (int i = 0; i < int(possible.size()); ++i) {
                        if ((mask >> i) & 1) { g.addEdge(possible[i].first, possible[i].second); }}
                    runCase(g); }}
            for (int mask = 0; mask < (directed ? 81 : 27); ++mask) {
                Graph g(2, directed); int value = mask;
                for (int u = 0; u < 2; ++u) { for (int v = directed ? 0 : u; v < 2; ++v) {
                    for (int i = 0; i < value % 3; ++i) { g.addEdge(u, v); } value /= 3; }}
                runCase(g); }}
        cout << "PASS exhaustive looped directed/undirected graphs and multiplicity-2 graphs cases=" << cases << '\n'; }
    void randomCases() {
        std::mt19937_64 rng(seed);
        int rounds = mode == "quick" ? 120 : mode == "full" ? 1500 : 10000;
        for (int trial = 0; trial < rounds; ++trial) {
            int n = 1 + int(rng() % 8), m = int(rng() % 11); Graph g(n, rng() & 1);
            int last = int(rng() % n);
            for (int i = 0; i < m; ++i) {
                int u = trial % 3 == 0 ? last : int(rng() % n), v = int(rng() % n);
                g.addEdge(u, v, i % 2 ? std::numeric_limits<lng>::min() : std::numeric_limits<lng>::max()); last = v; }
            runCase(g);
            if (trial % 10 == 0) { runCase(g.reverse()); }}
        cout << "PASS random multigraphs/generated trails, reversal and extreme ignored weights rounds=" << rounds << '\n'; }
    template<class G>
    void largeCase(const G &g, bool cycle) {
        result(g, eulerianTrail(g), -1, true);
        result(g, eulerianTrail(g, 0), 0, true);
        result(g, eulerianTrail(g, g.n - 1), g.n - 1, cycle || !g.directed); }
    void boundaries() {
        int n = mode == "quick" ? 20000 : mode == "full" ? 200000 : 500000;
        for (bool directed : {false, true}) {
            Graph g(n, directed);
            for (int u = 1; u < n; ++u) { g.addEdge(u - 1, u); }
            context = "chain n=" + std::to_string(n) + " directed=" + std::to_string(directed);
            largeCase(g, false); largeCase(CsrGraph(g), false);
            g.addEdge(n - 1, 0); context = "cycle n=" + std::to_string(n) + " directed=" + std::to_string(directed);
            largeCase(g, true); largeCase(CsrGraph(g), true);
            g.addEdge(0, 0); result(g, eulerianTrail(g), -1, true);
            Graph disconnected(n + 2, directed);
            for (auto e : g.edges) { disconnected.addEdge(e.u, e.v, e.w); }
            disconnected.addEdge(n, n + 1); disconnected.addEdge(n + 1, n);
            context = "two edge-bearing components with valid degrees";
            result(disconnected, eulerianTrail(disconnected), -1, false); }
        Graph fixture(6); fixture.addEdge(4, 4); fixture.addEdge(4, 5); fixture.addEdge(5, 4);
        runCase(fixture); auto copy = eulerianTrail(fixture); auto moved = std::move(copy);
        result(fixture, moved, -1, true); CsrGraph snapshot(fixture);
        fixture.addEdge(1, 2); runCase(fixture);
        result(snapshot, eulerianTrail(snapshot), -1, true);
        cout << "PASS stack-safe chains/cycles n=" << n << ", loop/isolates, disconnected balanced graphs and snapshot ownership\n"; }
    void invalid(const string &name) {
        if (name == "negative-start") { eulerianTrail(Graph(2), -2); }
        else if (name == "high-start") { eulerianTrail(Graph(2), 2); }
        else if (name == "empty-start") { eulerianTrail(Graph(0), 0); }
        else { throw std::runtime_error("unknown invalid probe " + name); }}
}

int main(int argc, char **argv) {
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); return 2; }}
        exhaustive(); randomCases(); boundaries();
        cout << "PASS Euler seed=" << seed << " mode=" << mode << " cases=" << cases << " checks=" << checks << '\n';
    } catch (const std::exception &e) {
        cerr << "FAIL Euler seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1; }}
