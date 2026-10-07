#include "../../04-Graphs/05-shortest_path.hpp"

static ulng seed = 20260927;
static lng checks = 0, cases = 0;
static string context;
static std::mt19937_64 rng;

string decimal(lll x) {
    if (!x) { return "0"; }
    bool negative = x < 0; if (negative) { x = -x; }
    string s;
    while (x) { s.push_back(char('0' + x % 10)); x /= 10; }
    if (negative) { s.push_back('-'); } reverse(s.begin(), s.end()); return s;}
void require(bool ok, const string &message) {
    ++checks;
    if (!ok) {
        cerr << "FAIL shortest paths seed=" << seed << " case=" << cases << " " << message << '\n' << context << '\n';
        std::exit(1);}}
void describe(const Graph &g, const vector<int> &sources, const string &label) {
    ++cases; std::ostringstream out;
    out << label << " n=" << g.n << " directed=" << g.directed << " sources=";
    for (int s : sources) { out << s << ','; }
    out << " edges=";
    for (auto e : g.edges) { out << '(' << e.u << ',' << e.v << ',' << e.w << ')'; }
    context = out.str();}

struct Expected {
    vector<lll> dist;
    vector<char> reachable, negative;
    explicit Expected(int n) : dist(n), reachable(n), negative(n) {}
};
struct FloydOracle {
    static constexpr lll INF = lll(1) << 120;
    vector<vector<lll>> d;
    explicit FloydOracle(const Graph &g, bool hops = false) : d(g.n, vector<lll>(g.n, INF)) {
        for (int u = 0; u < g.n; ++u) { d[u][u] = 0; }
        for (auto e : g.arcs) { d[e.from][e.to] = min(d[e.from][e.to], hops ? lll(1) : lll(e.w)); }
        for (int k = 0; k < g.n; ++k) {
            for (int u = 0; u < g.n; ++u) {
                for (int v = 0; v < g.n; ++v) {
                    if (d[u][k] != INF && d[k][v] != INF) { d[u][v] = min(d[u][v], d[u][k] + d[k][v]); }}}}}
    Expected from(const vector<int> &sources) const {
        int n = int(d.size()); Expected out(n);
        for (int v = 0; v < n; ++v) {
            for (int s : sources) {
                if (d[s][v] != INF) {
                    if (!out.reachable[v] || d[s][v] < out.dist[v]) { out.dist[v] = d[s][v]; }
                    out.reachable[v] = true;}
                for (int k = 0; k < n; ++k) {
                    if (d[s][k] != INF && d[k][k] < 0 && d[k][v] != INF) { out.negative[v] = true; }}}}
        return out;}
};

bool acyclicOracle(const Graph &g) {
    vector<vector<char>> reach(g.n, vector<char>(g.n));
    for (auto e : g.arcs) { reach[e.from][e.to] = true; }
    for (int k = 0; k < g.n; ++k) {
        for (int u = 0; u < g.n; ++u) {
            for (int v = 0; v < g.n; ++v) { reach[u][v] |= reach[u][k] && reach[k][v]; }}}
    for (int u = 0; u < g.n; ++u) { if (reach[u][u]) { return false; }} return true;}
Expected longestOracle(const Graph &g, const vector<int> &sources) {
    Expected out(g.n);
    auto visit = [&](auto &&visit, int u, uint used, lll d) -> void {
        if (!out.reachable[u] || d > out.dist[u]) { out.dist[u] = d; }
        out.reachable[u] = true;
        for (int a : g[u]) {
            auto e = g.arcs[a];
            if (!(used >> e.to & 1)) { visit(visit, e.to, used | uint(1) << e.to, d + e.w); }}};
    for (int s : sources) { visit(visit, s, uint(1) << s, 0); }
    return out;}

template<class G>
void validateCycle(const G &g, const CycleWitness &cycle, bool negative) {
    require(!cycle.arcs.empty(), "expected cycle, actual empty");
    require(cycle.vertices.size() == cycle.arcs.size() + 1, "cycle sizes");
    require(cycle.vertices.front() == cycle.vertices.back(), "cycle not closed");
    vector<char> used(g.n); lll weight = 0;
    for (int i = 0; i < int(cycle.arcs.size()); ++i) {
        int u = cycle.vertices[i], v = cycle.vertices[i + 1], a = cycle.arcs[i];
        require(0 <= u && u < g.n && !used[u], "cycle vertices invalid/repeated"); used[u] = true;
        require(0 <= a && a < int(g.arcs.size()), "cycle arc index");
        auto e = g.arcs[a]; require(e.from == u && e.to == v, "cycle arc orientation"); weight += e.w;}
    if (negative) { require(weight < 0, "cycle expected negative, actual=" + decimal(weight)); }}
template<class G>
void validateResult(const G &g, const vector<int> &sources, const ShortestPathResult &out,
                    const Expected &expected, bool hops = false) {
    require(int(out.dist.size()) == g.n && int(out.reachable.size()) == g.n && int(out.negative.size()) == g.n
            && int(out.parent.size()) == g.n && int(out.parent_arc.size()) == g.n, "result dimensions");
    bool has_negative = false;
    for (int v = 0; v < g.n; ++v) {
        require(out.reachable[v] == expected.reachable[v], "reachable vertex=" + std::to_string(v));
        require(out.negative[v] == expected.negative[v], "negative vertex=" + std::to_string(v));
        bool finite = expected.reachable[v] && !expected.negative[v];
        require(out.finite(v) == finite, "finite status"); has_negative |= expected.negative[v];
        if (finite) {
            require(out.dist[v] == expected.dist[v], "distance vertex=" + std::to_string(v)
                    + " expected=" + decimal(expected.dist[v]) + " actual=" + decimal(out.dist[v]));}
        if (!out.reachable[v]) { require(out.parent[v] == -1 && out.parent_arc[v] == -1, "unreachable parents"); }
        auto path = out.path(g, v);
        require(path.exists == finite, "path existence");
        if (!finite) { require(path.vertices.empty() && path.arcs.empty(), "absent path storage"); continue; }
        require(path.vertices.size() == path.arcs.size() + 1 && int(path.vertices.size()) <= g.n, "path dimensions");
        require(path.vertices.back() == v, "path destination");
        require(std::find(sources.begin(), sources.end(), path.vertices.front()) != sources.end(), "path origin source");
        require(out.parent[path.vertices.front()] == -1, "path origin parent");
        vector<char> seen(g.n); lll weight = 0;
        for (int u : path.vertices) { require(0 <= u && u < g.n && !seen[u], "path repeated vertex"); seen[u] = true; }
        for (int i = 0; i < int(path.arcs.size()); ++i) {
            int a = path.arcs[i], u = path.vertices[i], w = path.vertices[i + 1];
            require(0 <= a && a < int(g.arcs.size()), "path arc index");
            auto e = g.arcs[a]; require(e.from == u && e.to == w, "path orientation");
            require(out.parent[w] == u && out.parent_arc[w] == a, "path parents"); weight += hops ? lll(1) : lll(e.w);}
        require(weight == expected.dist[v], "path weight expected=" + decimal(expected.dist[v]) + " actual=" + decimal(weight));}
    require(!out.negative_cycle.arcs.empty() == has_negative, "negative-cycle existence");
    if (has_negative) {
        validateCycle(g, out.negative_cycle, true);
        for (int v : out.negative_cycle.vertices) { require(out.negative[v] && out.reachable[v], "cycle reachability"); }}
    else { require(out.negative_cycle.vertices.empty(), "absent cycle vertices"); }}

void checkGraph(const Graph &g, const vector<int> &sources, bool variants = true) {
    describe(g, sources, "oracle"); FloydOracle oracle(g), hops(g, true); auto expected = oracle.from(sources);
    validateResult(g, sources, bellmanFord(g, sources), expected);
    validateResult(g, sources, bfsShortestPaths(g, sources), hops.from(sources), true);
    bool nonnegative = true, binary = true;
    for (auto e : g.arcs) { nonnegative &= e.w >= 0; binary &= e.w == 0 || e.w == 1; }
    if (nonnegative) {
        validateResult(g, sources, dijkstra(g, sources), expected);
        validateResult(g, sources, dijkstraDense(DenseGraph(g), sources), expected);}
    if (binary) { validateResult(g, sources, zeroOneBfs(g, sources), expected); }
    if (variants) {
        CsrGraph csr(g); validateResult(csr, sources, bellmanFord(csr, sources), expected);
        validateResult(csr, sources, bfsShortestPaths(csr, sources), hops.from(sources), true);
        if (nonnegative) { validateResult(csr, sources, dijkstra(csr, sources), expected); }
        if (binary) { validateResult(csr, sources, zeroOneBfs(csr, sources), expected); }
        vector<int> all(g.n); iota(all.begin(), all.end(), 0); auto global = oracle.from(all);
        bool negative = std::find(global.negative.begin(), global.negative.end(), char(true)) != global.negative.end();
        auto cycle = findNegativeCycle(csr); require(!cycle.arcs.empty() == negative, "global cycle existence");
        if (negative) { validateCycle(csr, cycle, true); }
        if (sources.size() == 1) {
            int s = sources[0]; validateResult(g, sources, bellmanFord(g, s), expected);
            validateResult(g, sources, bfsShortestPaths(g, s), hops.from(sources), true);
            if (nonnegative) {
                validateResult(g, sources, dijkstra(g, s), expected);
                validateResult(g, sources, dijkstraDense(DenseGraph(g), s), expected);}
            if (binary) { validateResult(g, sources, zeroOneBfs(g, s), expected); }}}
    if (g.directed) {
        bool acyclic = acyclicOracle(g);
        auto shortest = dagShortestPaths(g, sources), longest = dagLongestPaths(g, sources);
        require(shortest.acyclic == acyclic && longest.acyclic == acyclic, "DAG acyclic status");
        if (acyclic) {
            require(shortest.cycle.arcs.empty() && longest.cycle.arcs.empty(), "DAG cycle should be absent");
            validateResult(g, sources, shortest.paths, expected);
            auto high = longestOracle(g, sources); validateResult(g, sources, longest.paths, high);
            if (variants) {
                CsrGraph csr(g); validateResult(csr, sources, dagShortestPaths(csr, sources).paths, expected);
                validateResult(csr, sources, dagShortestPaths(csr, sources, true).paths, high);
                if (sources.size() == 1) {
                    validateResult(csr, sources, dagShortestPaths(csr, sources[0]).paths, expected);
                    validateResult(csr, sources, dagLongestPaths(csr, sources[0]).paths, high);}}}
        else {
            validateCycle(g, shortest.cycle, false); validateCycle(g, longest.cycle, false);
            for (int v = 0; v < g.n; ++v) { require(!shortest.paths.reachable[v] && !longest.paths.reachable[v], "failed DAG paths"); }}}}

void fixtures() {
    for (bool directed : {false, true}) {
        checkGraph(Graph(0, directed), {}); checkGraph(Graph(1, directed), {}); checkGraph(Graph(1, directed), {0, 0});
        for (lng w : {std::numeric_limits<lng>::min(), lng(-1), lng(0), lng(1), std::numeric_limits<lng>::max()}) {
            Graph loop(1, directed); loop.addEdge(0, 0, w); checkGraph(loop, {0}); checkGraph(loop, {});}}
    Graph disconnected(7, true);
    disconnected.addEdge(0, 1, 5); disconnected.addEdge(1, 2, 0);
    disconnected.addEdge(3, 4, -2); disconnected.addEdge(4, 3, 1); disconnected.addEdge(4, 5, 4);
    checkGraph(disconnected, {0}); checkGraph(disconnected, {0, 3}); checkGraph(disconnected, {});
    disconnected.addEdge(2, 3, 0); checkGraph(disconnected, {0});
    Graph multiple(11, true);
    multiple.addEdge(0, 1, 1); multiple.addEdge(1, 2, -2); multiple.addEdge(2, 1, 1); multiple.addEdge(2, 3, 0);
    multiple.addEdge(0, 4, 1); multiple.addEdge(4, 5, -3); multiple.addEdge(5, 4, 2); multiple.addEdge(5, 6, 0);
    multiple.addEdge(0, 7, 8); multiple.addEdge(8, 9, -1); multiple.addEdge(9, 8, 0);
    checkGraph(multiple, {0});
    Graph extremes(6, true);
    extremes.addEdge(0, 1, std::numeric_limits<lng>::max()); extremes.addEdge(1, 2, std::numeric_limits<lng>::max());
    extremes.addEdge(3, 4, std::numeric_limits<lng>::min()); extremes.addEdge(4, 5, std::numeric_limits<lng>::min());
    checkGraph(extremes, {0, 3}); checkGraph(extremes, {0});
    extremes.addEdge(5, 3, 0); checkGraph(extremes, {0, 3});
    Graph nonnegative(4, true);
    nonnegative.addEdge(0, 1, std::numeric_limits<lng>::max());
    nonnegative.addEdge(1, 2, std::numeric_limits<lng>::max());
    nonnegative.addEdge(2, 3, std::numeric_limits<lng>::max()); checkGraph(nonnegative, {0});
    Graph decrease(4, true);
    decrease.addEdge(0, 1, 1); decrease.addEdge(0, 2, 0); decrease.addEdge(2, 1, 0);
    decrease.addEdge(1, 3, 0); decrease.addEdge(3, 2, 0); checkGraph(decrease, {0, 0});
    Graph improved_source(3, true); improved_source.addEdge(0, 1, -1); improved_source.addEdge(1, 2, -1);
    checkGraph(improved_source, {0, 1, 2});
    Graph parallel(3, false);
    for (int w = 20; w >= 0; --w) { parallel.addEdge(0, 1, w); }
    parallel.addEdge(1, 2, 0); checkGraph(parallel, {0});
    auto original = dijkstra(parallel, 0); auto copied = original; auto moved = std::move(copied);
    validateResult(parallel, {0}, moved, FloydOracle(parallel).from({0}));
    require(original.dist == moved.dist && original.parent == moved.parent, "copy/move result");
    cout << "PASS fixtures: extremes, unreachable/reachable cycles, loops, parallel arcs, stale queues, copy/move\n";}

void exhaustive(const string &mode) {
    int n = mode == "quick" ? 2 : 3, slots = n * (n - 1), graphs = 1;
    for (int i = 0; i < slots; ++i) { graphs *= 4; }
    for (int code = 0; code < graphs; ++code) {
        Graph g(n, true); int x = code;
        for (int u = 0; u < n; ++u) {
            for (int v = 0; v < n; ++v) {
                if (u == v) { continue; } int w = x % 4; x /= 4;
                if (w) { g.addEdge(u, v, w - 2); }}}
        for (int mask = 0; mask < (1 << n); ++mask) {
            vector<int> sources;
            for (int u = 0; u < n; ++u) { if (mask >> u & 1) { sources.push_back(u); }}
            checkGraph(g, sources, code % 97 == 0);}}
    cout << "PASS exhaustive signed digraphs=" << graphs << " source_subsets=" << (1 << n) << '\n';
    if (mode == "stress") {
        for (int code = 0; code < 19683; ++code) {
            Graph g(3, true); int x = code;
            for (int u = 0; u < 3; ++u) {
                for (int v = 0; v < 3; ++v) { int w = x % 3; x /= 3; if (w) { g.addEdge(u, v, 2 * w - 3); }}}
            checkGraph(g, {code % 3}, code % 101 == 0);}
        cout << "PASS stress exhaustive loops digraphs=19683\n";}}

void randomTests(const string &mode) {
    int count = mode == "quick" ? 100 : mode == "full" ? 1000 : 5000;
    for (int it = 0; it < count; ++it) {
        int n = 1 + int(rng() % 8); Graph g(n, bool(rng() % 2));
        int m = int(rng() % 35), domain = it % 4;
        for (int j = 0; j < m; ++j) {
            lng w = domain == 0 ? lng(rng() % 21) - 10 : domain == 1 ? lng(rng() % 100) : lng(rng() % 2);
            if (domain == 3) {
                static const array<lng, 7> weights = {std::numeric_limits<lng>::min(), std::numeric_limits<lng>::max(), -1, 0, 1, INF64, -INF64};
                w = weights[rng() % weights.size()];}
            g.addEdge(int(rng() % n), int(rng() % n), w);}
        vector<int> sources; int k = int(rng() % (n + 3));
        for (int j = 0; j < k; ++j) { sources.push_back(int(rng() % n)); }
        checkGraph(g, sources);
        Graph dag(n, true);
        for (int u = 0; u < n; ++u) {
            for (int v = u + 1; v < n; ++v) { if (rng() % 3 == 0) { dag.addEdge(u, v, lng(rng() % 41) - 20); }}}
        checkGraph(dag, sources);}
    cout << "PASS random multigraphs=" << count << " signed DAGs=" << count << '\n';}

void largeTests(const string &mode) {
    int n = mode == "quick" ? 2000 : mode == "full" ? 100000 : 300000;
    Graph path(n, true);
    for (int u = 1; u < n; ++u) { path.addEdge(u - 1, u, std::numeric_limits<lng>::max()); }
    ++cases; context = "large exact path n=" + std::to_string(n);
    auto out = dijkstra(path, 0);
    require(out.dist.back() == lll(n - 1) * std::numeric_limits<lng>::max(), "large path distance");
    require(int(out.path(path, n - 1).arcs.size()) == n - 1, "large iterative recovery");
    auto longest = dagLongestPaths(path, 0);
    require(longest.acyclic && longest.paths.dist.back() == out.dist.back(), "large DAG longest");
    auto bf = bellmanFord(path, 0); require(bf.dist.back() == out.dist.back(), "large BF early exit");
    Graph binary(n, true);
    for (int u = 1; u < n; ++u) { binary.addEdge(u - 1, u, u % 2); }
    auto zo = zeroOneBfs(binary, 0); require(zo.dist.back() == n / 2, "large 01 chain");
    Graph reverse_path(400, true);
    for (int u = 399; u > 0; --u) { reverse_path.addEdge(u - 1, u, -1); }
    auto signed_path = bellmanFord(reverse_path, 0); require(signed_path.dist.back() == -399, "adversarial BF pass count");
    cout << "PASS large exact/deep paths n=" << n << " reverse-relaxation path=400\n";}

void legacy() {
    context = "legacy Dijkstra n+1 and repeated runs"; ++cases;
    Dijkstra solver(3); vector<vector<pair<int, lng>>> adj(4);
    adj[0] = {{1, 3}, {2, 9}}; adj[1] = {{2, 2}}; adj[2] = {{3, 1}};
    solver.runDijkstra({0, 0}, adj); require(solver.dis == vector<lng>({0, 3, 5, 6}), "legacy weighted");
    require(solver.is_proc == vector<bool>(4, true) && solver.unproc.empty(), "legacy processed/heap");
    solver.unproc.push({1, 0}); solver.runDijkstra({3}, adj);
    require(solver.dis == vector<lng>({INF64, INF64, INF64, 0}), "legacy reset");
    require(solver.is_proc == vector<bool>({false, false, false, true}) && solver.unproc.empty(), "legacy reset flags");
    solver.runDijkstra({}, adj); require(solver.dis == vector<lng>(4, INF64), "legacy empty sources");
    adj[0] = {{1, INF64 - 1}}; adj[1].clear(); adj[2].clear(); solver.runDijkstra({0}, adj);
    require(solver.dis[1] == INF64 - 1, "legacy finite boundary");
    vector<vector<int>> unweighted = {{1, 2}, {2}, {3}, {}};
    solver.runDijkstra({0}, unweighted); require(solver.dis == vector<lng>({0, 1, 1, 2}), "legacy unweighted overload");
    Dijkstra zero(0); zero.runDijkstra({0}, vector<vector<int>>(1)); require(zero.dis == vector<lng>{0}, "legacy max-label zero");
    Graph direct(4, true); direct.addEdge(1, 2, 7); solver.runGraph({1}, direct);
    require(solver.dis == vector<lng>({INF64, 0, 7, INF64}), "legacy runGraph");
    ShortestPathResult helper(2, {0}); GraphArc edge{0, 1, 0, -1, 3};
    require(helper.relax(edge, 0) && helper.dist[1] == 3 && !helper.relax(edge, 0), "relax helper minimum");
    edge.w = 5; require(helper.relax(edge, 0, true) && helper.dist[1] == 5, "relax helper maximum");
    auto copied = solver; auto moved = std::move(copied); require(moved.dis == solver.dis, "legacy copy/move");
    cout << "PASS legacy wrapper: both overloads, zero label, reset, finite boundary, copy/move\n";}

void invalid(const string &which) {
    Graph g(2, true); g.addEdge(0, 1, 2);
    if (which == "source") { (void)bellmanFord(g, 2); }
    if (which == "zero-one") { (void)zeroOneBfs(g, 0); }
    if (which == "dijkstra") { g.addEdge(1, 1, -1); (void)dijkstra(g, 0); }
    if (which == "dense") { g.addEdge(1, 1, -1); (void)dijkstraDense(DenseGraph(g), 0); }
    if (which == "dense-maximum") { (void)dijkstraDense(DenseGraph(g, true), 0); }
    if (which == "dag-undirected") { (void)dagShortestPaths(Graph(1), 0); }
    if (which == "path-index") { (void)dijkstra(g, 0).path(g, 2); }
    if (which == "legacy-size") { Dijkstra bad(INT_MAX); }
    if (which == "legacy-limit") { Dijkstra bad(1); vector<vector<pair<int, lng>>> adj(2); adj[0] = {{1, INF64}}; bad.runDijkstra({0}, adj); }
    if (which == "legacy-source") { Dijkstra bad(1); bad.runDijkstra({2}, vector<vector<int>>(2)); }
    if (which == "legacy-adjacency") { Dijkstra bad(1); bad.runDijkstra({0}, vector<vector<int>>(1)); }
    if (which == "legacy-negative") { Dijkstra bad(1); vector<vector<pair<int, lng>>> adj(2); adj[1] = {{1, -1}}; bad.runDijkstra({0}, adj); }
    if (which == "result-size") { ShortestPathResult bad(-1); }
    if (which == "relax-range") { ShortestPathResult bad(2, {0}); bad.relax({0, 2, 0, -1, 0}, 0); }
    if (which == "relax-unreached") { ShortestPathResult bad(2); bad.relax({0, 1, 0, -1, 0}, 0); }
    if (which == "relax-arc") { ShortestPathResult bad(2, {0}); bad.relax({0, 1, 0, -1, 0}, -1); }
    std::exit(2);}

int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--invalid") { invalid(argv[++i]); }
        if (arg == "--mode") { mode = argv[++i]; }
        if (arg == "--seed") { seed = std::stoull(argv[++i]); }}
    rng.seed(seed);
    fixtures(); exhaustive(mode); randomTests(mode); largeTests(mode); legacy();
    cout << "PASS shortest paths mode=" << mode << " seed=" << seed << " cases=" << cases << " checks=" << checks << '\n';}
