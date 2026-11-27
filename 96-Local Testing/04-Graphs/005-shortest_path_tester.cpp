#include "../../04-Graphs/005-shortest_path.hpp"

static ulng seed = 20260927;
// Debug-mode push_heap and pop_heap verify the whole heap, so the checked build shrinks the Voronoi adversaries.
#ifdef _GLIBCXX_DEBUG
constexpr bool CHECKED = true;
#else
constexpr bool CHECKED = false;
#endif
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

// Cells: the smallest center at minimum Floyd distance; tree arcs stay inside one cell and are tight.
template<class G>
void checkVoronoi(const G &g, const vector<int> &centers, const FloydOracle &oracle) {
    auto res = voronoiCells(g, centers); validateResult(g, centers, res.paths, oracle.from(centers));
    require(int(res.center.size()) == g.n, "voronoi dimensions");
    for (int v = 0; v < g.n; ++v) {
        int best = -1;
        for (int c : centers) {
            lll d = oracle.d[c][v];
            if (d != FloydOracle::INF && (best == -1 || d < oracle.d[best][v] || (d == oracle.d[best][v] && c < best))) { best = c; }}
        require(res.center[v] == best, "voronoi center vertex=" + std::to_string(v) + " expected=" + std::to_string(best)
                + " actual=" + std::to_string(res.center[v]));
        if (best == -1) { continue; }
        int p = res.paths.parent[v];
        require(p != -1 || v == best, "voronoi root is not its own center");
        if (p != -1) {
            auto e = g.arcs[res.paths.parent_arc[v]];
            require(res.center[p] == best && e.from == p && e.to == v && res.paths.dist[p] + e.w == res.paths.dist[v],
                    "voronoi tree arc vertex=" + std::to_string(v));}
        require(res.paths.path(g, v).vertices.front() == best, "voronoi path origin vertex=" + std::to_string(v));}}

// Ball: Floyd membership, nondecreasing distance order from the center, inducedSubgraph and brute-force edge set.
template<class G>
void checkEgo(const G &g, const FloydOracle &oracle) {
    for (int c = 0; c < g.n; ++c) {
        vector<lll> radii = {0, lll(1) << 100};
        for (int v = 0; v < g.n; ++v) {
            lll d = oracle.d[c][v];
            if (d != FloydOracle::INF) { radii.push_back(d); if (d) { radii.push_back(d - 1); }}}
        for (lll r : radii) {
            auto sub = egoGraphWeighted(g, c, r); const auto &vs = sub.vertex; string at = " center=" + std::to_string(c) + " radius=" + decimal(r);
            require(!vs.empty() && vs[0] == c, "ego center first" + at);
            vector<char> in(g.n);
            for (int i = 0; i < int(vs.size()); ++i) {
                int v = vs[i]; require(0 <= v && v < g.n && !in[v], "ego vertex invalid/repeated" + at); in[v] = true;
                require(i == 0 || oracle.d[c][vs[i - 1]] <= oracle.d[c][v], "ego distance order" + at);}
            for (int v = 0; v < g.n; ++v) { require(bool(in[v]) == (oracle.d[c][v] <= r), "ego membership vertex=" + std::to_string(v) + at); }
            auto induced = inducedSubgraph(g, vs); vector<int> brute, ids = sub.edge;
            for (int id = 0; id < int(g.edges.size()); ++id) { if (in[g.edges[id].u] && in[g.edges[id].v]) { brute.push_back(id); }}
            sort(ids.begin(), ids.end());
            require(sub.edge == induced.edge && ids == brute, "ego edge set" + at);
            require(sub.graph.n == int(vs.size()) && sub.graph.directed == g.directed && sub.graph.edges.size() == sub.edge.size()
                    && sub.graph.arcs.size() == induced.graph.arcs.size(), "ego subgraph dimensions" + at);
            for (int i = 0; i < int(sub.edge.size()); ++i) {
                auto e = sub.graph.edges[i], f = g.edges[sub.edge[i]];
                require(vs[e.u] == f.u && vs[e.v] == f.v && e.w == f.w, "ego edge mapping" + at);}}}}

// AOE oracle: path maxima by enumerating every path of the DAG (longestOracle) from in-degree-0 vertices and to out-degree-0 ones.
void checkCritical(const Graph &g, bool acyclic, bool variants) {
    vector<lll> early(g.n), latest(g.n); vector<int> critical; lll length = 0;
    if (acyclic) {
        vector<int> in(g.n), out(g.n), starts;
        for (auto e : g.arcs) { ++out[e.from]; ++in[e.to]; }
        for (int v = 0; v < g.n; ++v) { if (!in[v]) { starts.push_back(v); }}
        auto into = longestOracle(g, starts);
        for (int v = 0; v < g.n; ++v) {
            require(into.reachable[v], "critical oracle: vertex unreachable from in-degree-0 vertices");
            early[v] = into.dist[v];}
        bool sink = false;
        for (int v = 0; v < g.n; ++v) { if (!out[v] && (!sink || early[v] > length)) { length = early[v]; sink = true; }}
        for (int v = 0; v < g.n; ++v) {
            auto from = longestOracle(g, {v}); bool found = false; lll tail = 0;
            for (int t = 0; t < g.n; ++t) {
                if (!out[t] && from.reachable[t] && (!found || from.dist[t] > tail)) { tail = from.dist[t]; found = true; }}
            require(found, "critical oracle: no out-degree-0 vertex reachable"); latest[v] = length - tail;}
        for (int a = 0; a < int(g.arcs.size()); ++a) {
            auto e = g.arcs[a]; if (early[e.from] + e.w == latest[e.to]) { critical.push_back(a); }}}
    auto validate = [&](const auto &h, const CriticalPathResult &r) {
        require(r.acyclic == acyclic, "critical acyclic status");
        if (!acyclic) {
            validateCycle(h, r.cycle, false);
            require(r.length == 0 && r.earliest.empty() && r.latest.empty() && r.critical.empty(), "critical fields on a cycle");
            return;}
        require(r.cycle.arcs.empty() && r.cycle.vertices.empty(), "critical cycle should be absent");
        require(r.length == length, "critical length expected=" + decimal(length) + " actual=" + decimal(r.length));
        require(r.earliest == early && r.latest == latest, "critical earliest/latest");
        require(r.critical == critical, "critical arcs");};
    validate(g, criticalPath(g));
    if (variants) { CsrGraph csr(g); validate(csr, criticalPath(csr)); }}

// Whole-graph longest path: enumerated simple-path maxima from every start; witness weight and smallest end.
void checkDagLongest(const Graph &g, bool acyclic, bool variants) {
    vector<int> all(g.n); iota(all.begin(), all.end(), 0);
    auto best = acyclic ? longestOracle(g, all) : Expected(g.n); lll length = 0; int end = -1;
    for (int v = 0; v < g.n && acyclic; ++v) { if (end == -1 || best.dist[v] > length) { length = best.dist[v]; end = v; }}
    auto validate = [&](const auto &h, const DagLongestPathResult &r) {
        require(r.acyclic == acyclic, "dag longest acyclic status");
        if (!acyclic) {
            validateCycle(h, r.cycle, false);
            require(r.length == 0 && !r.path.exists && r.path.vertices.empty(), "dag longest fields on a cycle");
            return;}
        require(r.cycle.arcs.empty() && r.cycle.vertices.empty(), "dag longest cycle should be absent");
        require(r.length == length, "dag longest length expected=" + decimal(length) + " actual=" + decimal(r.length));
        require(r.path.exists == (g.n > 0), "dag longest path existence");
        if (!g.n) { require(r.path.vertices.empty() && r.path.arcs.empty(), "dag longest empty path"); return; }
        const auto &p = r.path; lll weight = 0;
        require(p.vertices.size() == p.arcs.size() + 1 && p.vertices.back() == end, "dag longest witness end expected="
                + std::to_string(end) + " actual=" + std::to_string(p.vertices.back()));
        for (int i = 0; i < int(p.arcs.size()); ++i) {
            int a = p.arcs[i]; require(0 <= a && a < int(h.arcs.size()), "dag longest arc index");
            auto e = h.arcs[a]; require(e.from == p.vertices[i] && e.to == p.vertices[i + 1], "dag longest orientation"); weight += e.w;}
        require(weight == length, "dag longest witness weight=" + decimal(weight));};
    validate(g, dagLongestPath(g));
    if (variants) { CsrGraph csr(g); validate(csr, dagLongestPath(csr)); }}

// The dispatcher's result equals the engine its weights select, field by field.
template<class G>
void checkDispatch(const G &g, const vector<int> &sources, const Expected &expected, bool nonnegative, bool binary) {
    auto any = shortestPaths(g, sources); validateResult(g, sources, any, expected);
    auto engine = !nonnegative ? bellmanFord(g, sources) : binary ? zeroOneBfs(g, sources) : dijkstra(g, sources);
    require(any.reachable == engine.reachable && any.negative == engine.negative && any.parent == engine.parent
            && any.parent_arc == engine.parent_arc, "shortestPaths dispatch fields");
    for (int v = 0; v < g.n; ++v) { require(!any.finite(v) || any.dist[v] == engine.dist[v], "shortestPaths dispatch distance"); }
    require(any.negative_cycle.arcs == engine.negative_cycle.arcs && any.negative_cycle.vertices == engine.negative_cycle.vertices, "shortestPaths dispatch cycle");}

void checkGraph(const Graph &g, const vector<int> &sources, bool variants = true) {
    describe(g, sources, "oracle"); FloydOracle oracle(g), hops(g, true); auto expected = oracle.from(sources);
    validateResult(g, sources, bellmanFord(g, sources), expected);
    validateResult(g, sources, bfsShortestPaths(g, sources), hops.from(sources), true);
    bool nonnegative = true, binary = true; lng c = 0;
    for (auto e : g.arcs) {
        nonnegative &= e.w >= 0;
        if (e.w) { c = c ? c : e.w; binary &= e.w == c && c > 0; }}
    if (nonnegative) {
        validateResult(g, sources, dijkstra(g, sources), expected);
        validateResult(g, sources, dijkstraDense(DenseGraph(g), sources), expected);
        checkVoronoi(g, sources, oracle); checkEgo(g, oracle);}
    if (binary) { validateResult(g, sources, zeroOneBfs(g, sources), expected); }
    checkDispatch(g, sources, expected, nonnegative, binary);
    vector<int> all(g.n); iota(all.begin(), all.end(), 0); auto global = oracle.from(all);
    bool negative = std::find(global.negative.begin(), global.negative.end(), char(true)) != global.negative.end();
    auto witness = findNegativeCycle(g); require(!witness.arcs.empty() == negative, "global cycle existence");
    if (negative) { validateCycle(g, witness, true); }
    if (variants) {
        CsrGraph csr(g); validateResult(csr, sources, bellmanFord(csr, sources), expected);
        validateResult(csr, sources, bfsShortestPaths(csr, sources), hops.from(sources), true);
        if (nonnegative) {
            validateResult(csr, sources, dijkstra(csr, sources), expected);
            checkVoronoi(csr, sources, oracle); checkVoronoi(g, all, oracle); checkEgo(csr, oracle);}
        if (binary) { validateResult(csr, sources, zeroOneBfs(csr, sources), expected); }
        checkDispatch(csr, sources, expected, nonnegative, binary);
        auto cycle = findNegativeCycle(csr); require(!cycle.arcs.empty() == negative, "global cycle existence on CSR");
        if (negative) { validateCycle(csr, cycle, true); }}
    if (g.directed) {
        bool acyclic = acyclicOracle(g); checkCritical(g, acyclic, variants); checkDagLongest(g, acyclic, variants);
        auto shortest = dagShortestPaths(g, sources), longest = dagLongestPathsFrom(g, sources);
        require(shortest.acyclic == acyclic && longest.acyclic == acyclic, "DAG acyclic status");
        if (acyclic) {
            require(shortest.cycle.arcs.empty() && longest.cycle.arcs.empty(), "DAG cycle should be absent");
            validateResult(g, sources, shortest.paths, expected);
            auto high = longestOracle(g, sources); validateResult(g, sources, longest.paths, high);
            if (variants) {
                CsrGraph csr(g); validateResult(csr, sources, dagShortestPaths(csr, sources).paths, expected);
                validateResult(csr, sources, dagLongestPathsFrom(csr, sources).paths, high);}}
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
    auto original = dijkstra(parallel, {0}); auto copied = original; auto moved = std::move(copied);
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

// zeroOneBfs on {0, c} multigraphs, c up to LLONG_MAX so distances pass 2^63; checkGraph adds voronoi and ego.
void zeroOneTests(const string &mode) {
    int count = mode == "quick" ? 100 : mode == "full" ? 1000 : 5000;
    static const array<lng, 5> weights = {1, 7, lng(1e18), lng(4e18), std::numeric_limits<lng>::max()};
    for (int it = 0; it < count; ++it) {
        int n = 1 + int(rng() % 8), m = int(rng() % 30); lng c = weights[it % weights.size()]; Graph g(n, bool(rng() % 2));
        for (int j = 0; j < m; ++j) { g.addEdge(int(rng() % n), int(rng() % n), rng() % 3 ? c : 0); }
        vector<int> sources; int k = int(rng() % (n + 3));
        for (int j = 0; j < k; ++j) { sources.push_back(int(rng() % n)); }
        checkGraph(g, sources);}
    cout << "PASS random {0, c} multigraphs=" << count << " c in {1, 7, 1e18, 4e18, LLONG_MAX}\n";}

// Known answers for voronoiCells, egoGraphWeighted and criticalPath, each also through checkGraph's oracles.
void newOperationFixtures() {
    Graph aoe(9, true);
    for (auto [u, v, w] : vector<tuple<int, int, lng>>{{0, 1, 6}, {0, 2, 4}, {0, 3, 5}, {1, 4, 1}, {2, 4, 1}, {3, 5, 2},
                                                        {4, 6, 9}, {4, 7, 7}, {5, 7, 4}, {6, 8, 2}, {7, 8, 4}}) { aoe.addEdge(u, v, w); }
    checkGraph(aoe, {0}); auto cp = criticalPath(aoe); ++cases; context = "classic AOE network";
    require(cp.acyclic && cp.length == 18 && cp.critical == vector<int>({0, 3, 6, 7, 9, 10}), "classic AOE critical arcs");
    require(cp.earliest == vector<lll>({0, 6, 4, 5, 7, 7, 16, 14, 18}) && cp.latest == vector<lll>({0, 6, 6, 8, 7, 10, 16, 14, 18}), "classic AOE times");
    Graph shape(7, true);
    shape.addEdge(0, 2, 3); shape.addEdge(1, 2, -4); shape.addEdge(2, 3, 5); shape.addEdge(2, 3, 5); shape.addEdge(2, 3, -1);
    shape.addEdge(2, 4, -7); shape.addEdge(5, 4, std::numeric_limits<lng>::max()); checkGraph(shape, {}); checkGraph(shape, {0, 1});
    cp = criticalPath(Graph(0, true)); ++cases; context = "critical n=0";
    require(cp.acyclic && cp.length == 0 && cp.earliest.empty() && cp.latest.empty() && cp.critical.empty(), "critical n=0");
    cp = criticalPath(Graph(3, true)); ++cases; context = "critical isolated";
    require(cp.acyclic && cp.length == 0 && cp.earliest == vector<lll>(3) && cp.latest == vector<lll>(3) && cp.critical.empty(), "critical isolated");
    Graph negative(2, true); negative.addEdge(0, 1, -5); cp = criticalPath(negative); ++cases; context = "critical negative arc";
    require(cp.length == -5 && cp.earliest == vector<lll>({0, -5}) && cp.latest == vector<lll>({0, -5}) && cp.critical == vector<int>({0}), "critical negative arc");
    Graph cyclic(3, true); cyclic.addEdge(0, 1, 1); cyclic.addEdge(1, 2, 1); cyclic.addEdge(2, 1, 1); checkGraph(cyclic, {0});
    Graph ties(6, false);
    ties.addEdge(0, 1, 2); ties.addEdge(1, 2, 2); ties.addEdge(2, 3, 2); ties.addEdge(3, 4, 0); ties.addEdge(4, 5, 0); ties.addEdge(0, 5, 4);
    for (const vector<int> &centers : vector<vector<int>>{{3, 0}, {4, 3}, {0, 2, 2}, {}, {0, 1, 2, 3, 4, 5}, {5, 0}}) { checkGraph(ties, centers); }
    auto vor = voronoiCells(ties, {4, 3}); ++cases; context = "voronoi zero-distance centers";
    require(vor.center == vector<int>({3, 3, 3, 3, 3, 3}) && vor.paths.parent[4] == 3, "zero-distance center joins the smaller center");
    vor = voronoiCells(ties, {0, 3}); context = "voronoi tie at vertex 1/2";
    require(vor.center == vector<int>({0, 0, 3, 3, 3, 3}), "voronoi ties go to the smallest center");
    vor = voronoiCells(ties, {}); context = "voronoi empty";
    require(vor.center == vector<int>(6, -1) && vor.paths.reachable == vector<char>(6), "voronoi empty centers");
    auto ego = egoGraphWeighted(ties, 0, 4); context = "ego inclusive radius";
    vector<int> ball = ego.vertex; sort(ball.begin(), ball.end());
    require(ego.vertex[0] == 0 && ball == vector<int>({0, 1, 2, 3, 4, 5}) && ego.graph.edges.size() == 6, "ego radius equal to a distance is inclusive");
    ego = egoGraphWeighted(ties, 0, 3); context = "ego radius below a distance";
    require(ego.vertex == vector<int>({0, 1}) && ego.edge == vector<int>({0}), "ego radius 3");
    ego = egoGraphWeighted(ties, 2, 0); context = "ego radius 0";
    require(ego.vertex == vector<int>({2}) && ego.edge.empty() && ego.graph.n == 1, "ego radius 0");
    cout << "PASS new-operation fixtures: classic AOE, parallel/negative/isolated activities, n=0, voronoi ties and zero-distance centers, ego radii\n";}

void largeTests(const string &mode) {
    int n = mode == "quick" ? 2000 : mode == "full" ? 100000 : 300000;
    Graph path(n, true);
    for (int u = 1; u < n; ++u) { path.addEdge(u - 1, u, std::numeric_limits<lng>::max()); }
    ++cases; context = "large exact path n=" + std::to_string(n);
    auto out = dijkstra(path, {0});
    require(out.dist.back() == lll(n - 1) * std::numeric_limits<lng>::max(), "large path distance");
    require(int(out.path(path, n - 1).arcs.size()) == n - 1, "large iterative recovery");
    auto longest = dagLongestPathsFrom(path, {0});
    require(longest.acyclic && longest.paths.dist.back() == out.dist.back(), "large DAG longest");
    auto bf = bellmanFord(path, {0}); require(bf.dist.back() == out.dist.back(), "large BF early exit");
    Graph binary(n, true);
    for (int u = 1; u < n; ++u) { binary.addEdge(u - 1, u, u % 2); }
    auto zo = zeroOneBfs(binary, {0}); require(zo.dist.back() == n / 2, "large 01 chain");
    Graph huge(n, false); lng c = std::numeric_limits<lng>::max();
    for (int u = 1; u < n; ++u) { huge.addEdge(u - 1, u, u % 2 ? c : 0); }
    context = "large {0, LLONG_MAX} chain, voronoi, ego n=" + std::to_string(n);
    zo = zeroOneBfs(huge, {0}); require(zo.dist.back() == lll(n / 2) * c && shortestPaths(huge, {0}).dist == zo.dist, "large {0, c} chain");
    auto cells = voronoiCells(huge, {0, n - 1}); int mid = (n - 1) / 2;
    require(cells.center[0] == 0 && cells.center[n - 1] == n - 1 && cells.paths.dist[mid] == min(zo.dist[mid], zo.dist.back() - zo.dist[mid]), "large voronoi");
    auto ball = egoGraphWeighted(huge, 0, lll(10) * c);
    require(int(ball.vertex.size()) == 21 && *std::max_element(ball.vertex.begin(), ball.vertex.end()) == 20 && int(ball.edge.size()) == 20, "large ego ball");
    context = "large critical chain n=" + std::to_string(n);
    auto cp = criticalPath(path);
    require(cp.acyclic && cp.length == out.dist.back() && cp.earliest == out.dist && cp.latest == out.dist && int(cp.critical.size()) == n - 1, "large critical chain");
    auto whole = dagLongestPath(path);
    require(whole.acyclic && whole.length == out.dist.back() && int(whole.path.arcs.size()) == n - 1, "large dag longest path");
    // Label-correcting orders (every arc to one end of the deque, or the ends swapped) are superlinear here.
    int K = mode == "quick" ? 2000 : 100000; context = "0-1 BFS deque-order adversaries K=L=" + std::to_string(K);
    Graph lifo(2 * K + 3 + K, true); int x = 2 * K + 2;  // y_i -> q_i (0) before y_i -> y_{i+1} (1); q_i -> x (0); x -> K tails (1)
    for (int i = 0; i <= K; ++i) { lifo.addEdge(i, K + 1 + i, 0); if (i < K) { lifo.addEdge(i, i + 1, 1); }}
    for (int i = 0; i <= K; ++i) { lifo.addEdge(K + 1 + i, x, 0); }
    for (int j = 0; j < K; ++j) { lifo.addEdge(x, x + 1 + j, 1); }
    auto lifo_out = zeroOneBfs(lifo, {0});
    for (int v = 0; v < lifo.n; ++v) {
        lll want = v <= K ? v : v <= 2 * K + 1 ? v - K - 1 : v == x ? 0 : 1;
        require(lifo_out.dist[v] == want, "LIFO adversary distance");}
    Graph fifo(4 * K + 3, true); int z = K + 1; x = z + 2 * K + 1;  // y_a -> z_{2a} (0), zero chain z -> x, x -> K tails
    for (int a = 0; a < K; ++a) { fifo.addEdge(a, a + 1, 1); }
    for (int a = 0; a <= K; ++a) { fifo.addEdge(a, z + 2 * a, 0); }
    for (int e = 0; e < 2 * K; ++e) { fifo.addEdge(z + e, z + e + 1, 0); }
    fifo.addEdge(z + 2 * K, x, 0);
    for (int j = 0; j < K; ++j) { fifo.addEdge(x, x + 1 + j, 1); }
    auto fifo_out = zeroOneBfs(fifo, {0});
    for (int v = 0; v < fifo.n; ++v) { require(fifo_out.dist[v] == (v <= K ? v : v <= x ? 0 : 1), "FIFO adversary distance"); }
    // A hub improved k times before it settles, each stale label rescanning its k out-arcs: from one center, then by
    // ever smaller centers at one distance.
    int k = mode == "quick" || CHECKED ? 2000 : 200000; context = "voronoi stale-label adversaries k=" + std::to_string(k);
    Graph hub(2 * k + 2, true); int h = k + 1;  // chain 0..k (1), i -> h (4k - 2i), h -> k tails (1)
    for (int i = 1; i <= k; ++i) { hub.addEdge(i - 1, i, 1); hub.addEdge(i, h, 4 * lng(k) - 2 * i); }
    for (int j = 0; j < k; ++j) { hub.addEdge(h, h + 1 + j, 1); }
    auto hub_out = voronoiCells(hub, {0});
    for (int v = 0; v < hub.n; ++v) {
        require(hub_out.center[v] == 0 && hub_out.paths.dist[v] == (v <= k ? v : v == h ? 3 * k : 3 * k + 1), "hub adversary");}
    Graph ties(3 * k + 1, true); vector<int> centers(k); iota(centers.begin(), centers.end(), 0); h = 2 * k;
    for (int i = 0; i < k; ++i) { ties.addEdge(i, k + i, k - i); ties.addEdge(k + i, h, i); }  // every center reaches h at k
    for (int j = 0; j < k; ++j) { ties.addEdge(h, h + 1 + j, 1); }
    auto ties_out = voronoiCells(ties, centers);
    for (int v = 0; v < ties.n; ++v) {
        lll want = v < k ? 0 : v < h ? 2 * k - v : v == h ? k : k + 1;
        require(ties_out.paths.dist[v] == want && ties_out.center[v] == (v < k ? v : v < h ? v - k : 0), "ties adversary");}
    Graph reverse_path(400, true);
    for (int u = 399; u > 0; --u) { reverse_path.addEdge(u - 1, u, -1); }
    auto signed_path = bellmanFord(reverse_path, {0}); require(signed_path.dist.back() == -399, "adversarial BF pass count");
    cout << "PASS large exact/deep paths n=" << n << " ({0, LLONG_MAX} chain, voronoi, ego, critical, DAG longest) reverse-relaxation path=400"
        << " 0-1 BFS deque-order adversaries K=" << K << " voronoi stale-label adversaries k=" << k << '\n';}

// Adjacency lists through Graph::fromAdjacency, then dijkstra.
void adjacencyInputs() {
    context = "adjacency-list inputs, repeated runs"; ++cases;
    vector<vector<pair<int, lng>>> adj(4);
    adj[0] = {{1, 3}, {2, 9}}; adj[1] = {{2, 2}}; adj[2] = {{3, 1}};
    auto g = Graph::fromAdjacency(adj, true); FloydOracle oracle(g);
    for (const vector<int> &sources : vector<vector<int>>{{0, 0}, {3}, {}}) {
        validateResult(g, sources, dijkstra(g, sources), oracle.from(sources));}
    require(dijkstra(g, {0, 0}).dist == vector<lll>({0, 3, 5, 6}), "weighted adjacency distances");
    auto late = dijkstra(g, {3});
    require(late.reachable == vector<char>({false, false, false, true}), "fresh result per run");
    adj[0] = {{1, INF64 - 1}}; adj[1] = {{2, INF64}}; adj[2].clear();
    auto large = dijkstra(Graph::fromAdjacency(adj, true), {0});
    require(large.dist[1] == INF64 - 1 && large.dist[2] == lll(INF64) * 2 - 1 && large.finite(2), "no INF64 distance bound");
    vector<vector<int>> unweighted = {{1, 2}, {2}, {3}, {}};
    require(dijkstra(Graph::fromAdjacency(unweighted, true), {0}).dist == vector<lll>({0, 1, 1, 2}), "unweighted adjacency");
    require(dijkstra(Graph::fromAdjacency(vector<vector<int>>(1), true), {0}).dist == vector<lll>{0}, "one-vertex adjacency");
    Graph direct(4, true); direct.addEdge(1, 2, 7); auto run = dijkstra(direct, {1});
    require(run.reachable == vector<char>({false, true, true, false}) && run.dist[2] == 7, "graph input");
    ShortestPathResult helper(2, {0}); GraphArc edge{0, 1, 0, -1, 3};
    require(helper.relax(edge, 0) && helper.dist[1] == 3 && !helper.relax(edge, 0), "relax helper minimum");
    edge.w = 1; require(helper.relax(edge, 0) && helper.dist[1] == 1, "relax helper defaults to minimum");
    edge.w = 5; require(helper.relax(edge, 0, true) && helper.dist[1] == 5, "relax helper maximum");
    require(!helper.relax(edge, 0, true) && helper.dist[1] == 5, "relax helper maximum keeps an equal value");
    ShortestPathResult into_zero(2, {1}); GraphArc back{1, 0, 1, -1, 4};
    require(into_zero.relax(back, 1) && into_zero.dist[0] == 4 && into_zero.parent[0] == 1 && into_zero.parent_arc[0] == 1, "relax helper into vertex 0 by arc 1");
    DagPathResult dag_default; DagLongestPathResult longest_default; CriticalPathResult critical_default;
    require(!dag_default.acyclic && !longest_default.acyclic && longest_default.length == 0 && !critical_default.acyclic
        && critical_default.length == 0, "default-constructed results");
    cout << "PASS adjacency inputs: weighted and unweighted lists, one vertex, repeated runs, beyond INF64, relax helper\n";}

void invalid(const string &which) {
    Graph g(2, true); g.addEdge(0, 1, 2);
    if (which == "source") { (void)bellmanFord(g, {2}); }
    if (which == "source-past") { (void)bellmanFord(g, {3}); }
    if (which == "zero-one") { g.addEdge(1, 0, 3); (void)zeroOneBfs(g, {0}); }
    if (which == "zero-one-smaller") { Graph h(2, true); h.addEdge(0, 1, 3); h.addEdge(1, 0, 2); (void)zeroOneBfs(h, {0}); }
    if (which == "zero-one-negative") { Graph h(2, true); h.addEdge(0, 1, -1); (void)zeroOneBfs(h, {0}); }
    if (which == "voronoi-negative") { g.addEdge(1, 1, -1); (void)voronoiCells(g, {0}); }
    if (which == "voronoi-center") { (void)voronoiCells(g, {2}); }
    if (which == "ego-negative-radius") { (void)egoGraphWeighted(g, 0, -1); }
    if (which == "ego-center") { (void)egoGraphWeighted(g, 2, 0); }
    if (which == "ego-negative") { g.addEdge(1, 0, -1); (void)egoGraphWeighted(g, 0, 5); }
    if (which == "critical-undirected") { (void)criticalPath(Graph(1)); }
    if (which == "dag-longest-undirected") { (void)dagLongestPath(Graph(1)); }
    if (which == "dijkstra") { g.addEdge(1, 1, -1); (void)dijkstra(g, {0}); }
    if (which == "dense") { g.addEdge(1, 1, -1); (void)dijkstraDense(DenseGraph(g), {0}); }
    if (which == "dense-maximum") { (void)dijkstraDense(DenseGraph(g, true), {0}); }
    if (which == "dag-undirected") { (void)dagShortestPaths(Graph(1), {0}); }
    if (which == "finite-past") { (void)ShortestPathResult(2, {0}).finite(3); }
    if (which == "path-index") { (void)dijkstra(g, {0}).path(g, 2); }
    if (which == "path-small-graph") { (void)dijkstra(g, {0}).path(Graph(1, true), 0); }
    if (which == "path-large-graph") { (void)dijkstra(g, {0}).path(Graph(3, true), 0); }
    if (which == "result-size") { ShortestPathResult bad(-1); }
    if (which == "relax-range") { ShortestPathResult bad(2, {0}); bad.relax({0, 2, 0, -1, 0}, 0); }
    if (which == "relax-to-past") { ShortestPathResult bad(2, {0}); bad.relax({0, 3, 0, -1, 0}, 0); }
    if (which == "relax-from-end") { ShortestPathResult bad(2, {0}); bad.relax({2, 1, 0, -1, 0}, 0); }
    if (which == "relax-from-past") { ShortestPathResult bad(2, {0}); bad.relax({3, 1, 0, -1, 0}, 0); }
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
    fixtures(); newOperationFixtures(); exhaustive(mode); randomTests(mode); zeroOneTests(mode); largeTests(mode); adjacencyInputs();
    cout << "PASS shortest paths mode=" << mode << " seed=" << seed << " cases=" << cases << " checks=" << checks << '\n';}
