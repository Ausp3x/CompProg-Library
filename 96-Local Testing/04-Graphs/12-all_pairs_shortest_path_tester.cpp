#include "../../04-Graphs/12-all_pairs_shortest_path.hpp"

using Matrix = vector<vector<lll>>;
static ulng seed = 20260927;
static lng checks = 0, cases = 0;
static string context;
static const Graph *current = nullptr;
static std::mt19937_64 rng;
static const lng LMIN = std::numeric_limits<lng>::min(), LMAX = std::numeric_limits<lng>::max();

string decimal(lll x) {
    if (x == PATH_INF) { return "INF"; }
    if (x == -PATH_INF) { return "-INF"; }
    if (!x) { return "0"; }
    bool negative = x < 0; if (negative) { x = -x; }
    string s;
    while (x) { s.push_back(char('0' + x % 10)); x /= 10; }
    if (negative) { s.push_back('-'); }
    reverse(s.begin(), s.end()); return s;}
// The message is a string or, on hot paths, a callable built only on failure; the graph is printed only on failure.
template<class F>
void require(bool ok, const F &message) {
    ++checks;
    if (ok) { return; }
    cerr << "FAIL all-pairs shortest paths seed=" << seed << " case=" << cases << " ";
    if constexpr (std::is_invocable_v<F>) { cerr << message(); } else { cerr << message; }
    cerr << '\n' << context;
    if (current) {
        cerr << " n=" << current->n << " directed=" << current->directed << " edges=";
        for (auto e : current->edges) { cerr << '(' << e.u << ',' << e.v << ',' << e.w << ')'; }}
    cerr << '\n'; std::exit(1);}
void describe(const Graph &g, const string &label) { ++cases; context = label; current = &g; }
string at(int u, int v) { return " u=" + std::to_string(u) + " v=" + std::to_string(v); }
void requireMatrix(const Matrix &actual, const Matrix &expected, const string &name) {
    require(actual.size() == expected.size(), name + " rows");
    for (int u = 0; u < int(expected.size()); ++u) {
        require(actual[u].size() == expected[u].size(), name + " columns");
        for (int v = 0; v < int(expected.size()); ++v) {
            require(actual[u][v] == expected[u][v], [&] { return name + at(u, v) + " expected=" + decimal(expected[u][v]) + " actual=" + decimal(actual[u][v]); });}}}

// Bellman-Ford per source: n - 1 passes, then every arc still relaxable seeds an explicit negative-infinite closure.
Matrix bellmanOracle(const Graph &g) {
    int n = g.n; Matrix res(n, vector<lll>(n, PATH_INF));
    for (int s = 0; s < n; ++s) {
        auto &d = res[s]; d[s] = 0;
        for (int pass = 0; pass + 1 < n; ++pass) {
            for (auto e : g.arcs) { if (d[e.from] != PATH_INF) { d[e.to] = min(d[e.to], d[e.from] + e.w); }}}
        vector<char> negative(n); vector<int> stack;
        for (auto e : g.arcs) {
            if (d[e.from] != PATH_INF && d[e.from] + e.w < d[e.to] && !negative[e.to]) { negative[e.to] = true; stack.push_back(e.to); }}
        while (!stack.empty()) {
            int u = stack.back(); stack.pop_back();
            for (auto e : g.arcs) { if (e.from == u && !negative[e.to]) { negative[e.to] = true; stack.push_back(e.to); }}}
        for (int v = 0; v < n; ++v) { if (negative[v]) { d[v] = -PATH_INF; }}}
    return res;}

// walks[k][u][v] is the least weight of a walk with exactly k arcs, by DP over walk length.
vector<Matrix> walkOracle(const Graph &g, int kmax) {
    int n = g.n; vector<Matrix> walks(1, Matrix(n, vector<lll>(n, PATH_INF)));
    for (int u = 0; u < n; ++u) { walks[0][u][u] = 0; }
    for (int k = 1; k <= kmax; ++k) {
        Matrix next(n, vector<lll>(n, PATH_INF));
        for (int u = 0; u < n; ++u) {
            for (auto e : g.arcs) {
                if (walks.back()[u][e.from] != PATH_INF) { next[u][e.to] = min(next[u][e.to], walks.back()[u][e.from] + e.w); }}}
        walks.push_back(next);}
    return walks;}

// Incremental threshold reachability over arcs sorted by weight: v first becomes reachable from s at its bottleneck.
Matrix thresholdOracle(const Graph &g, bool maximum) {
    int n = g.n; Matrix res(n, vector<lll>(n, maximum ? -PATH_INF : PATH_INF));
    vector<int> order(g.arcs.size()); iota(order.begin(), order.end(), 0);
    sort(order.begin(), order.end(), [&](int a, int b) { return maximum ? g.arcs[a].w > g.arcs[b].w : g.arcs[a].w < g.arcs[b].w; });
    for (int s = 0; s < n; ++s) {
        vector<char> seen(n); seen[s] = true; res[s][s] = maximum ? PATH_INF : -PATH_INF;
        for (int a : order) {
            lng t = g.arcs[a].w;
            if (!seen[g.arcs[a].from] || seen[g.arcs[a].to]) { continue; }
            vector<int> stack = {g.arcs[a].to}; seen[g.arcs[a].to] = true; res[s][g.arcs[a].to] = t;
            while (!stack.empty()) {
                int u = stack.back(); stack.pop_back();
                for (int b : g[u]) {
                    auto e = g.arcs[b];
                    if (!seen[e.to] && (maximum ? e.w >= t : e.w <= t)) { seen[e.to] = true; res[s][e.to] = t; stack.push_back(e.to); }}}}}
    return res;}

// Second bottleneck oracle for tiny graphs: (min, max) Floyd closure.
Matrix bottleneckFloyd(const Graph &g, bool maximum) {
    int n = g.n; lll worst = maximum ? -PATH_INF : PATH_INF;
    auto better = [&](lll a, lll b) { return maximum ? a > b : a < b; };
    Matrix d(n, vector<lll>(n, worst));
    for (auto e : g.arcs) { if (better(e.w, d[e.from][e.to])) { d[e.from][e.to] = e.w; }}
    for (int u = 0; u < n; ++u) { d[u][u] = -worst; }
    for (int k = 0; k < n; ++k) {
        for (int u = 0; u < n; ++u) {
            for (int v = 0; v < n; ++v) {
                lll c = better(d[u][k], d[k][v]) ? d[k][v] : d[u][k];
                if (better(c, d[u][v])) { d[u][v] = c; }}}}
    return d;}

// Validates the Bellman-Ford oracle against walk enumeration when every |w| <= big: K arcs suffice to expose a negative cycle.
void validateOracle(const Graph &g, const Matrix &expected, lng big) {
    int n = g.n, kmax = 2 * (n - 1) + n * int(3 * (n - 1) * big + 1);
    auto walks = walkOracle(g, max(kmax, 0));
    for (int u = 0; u < n; ++u) {
        for (int v = 0; v < n; ++v) {
            lll simple = PATH_INF, any = PATH_INF;
            for (int k = 0; k <= kmax; ++k) {
                any = min(any, walks[k][u][v]);
                if (k < n) { simple = any; }}
            lll want = any < simple ? -PATH_INF : simple;
            require(expected[u][v] == want, [&] { return "oracle self-check" + at(u, v) + " bellman=" + decimal(expected[u][v]) + " walks=" + decimal(want); });}}
    requireMatrix(thresholdOracle(g, false), bottleneckFloyd(g, false), "minimax oracle self-check");
    requireMatrix(thresholdOracle(g, true), bottleneckFloyd(g, true), "maximin oracle self-check");}

template<class G>
void validateApsp(const G &g, const AllPairsResult &res, const Matrix &expected, const string &name) {
    int n = g.n;
    require(int(res.dist.size()) == n && int(res.parent_arc.size()) == n, name + " dimensions");
    for (int u = 0; u < n; ++u) {
        require(int(res.dist[u].size()) == n && int(res.parent_arc[u].size()) == n, name + " row dimensions");
        for (int v = 0; v < n; ++v) {
            lll d = res.dist[u][v];
            require(d == expected[u][v], [&] { return name + " dist" + at(u, v) + " expected=" + decimal(expected[u][v]) + " actual=" + decimal(d); });
            bool finite = d != PATH_INF && d != -PATH_INF;
            require(res.finite(u, v) == finite, [&] { return name + " finite" + at(u, v); });
            int a = res.parent_arc[u][v];
            if (!finite || u == v) { require(a == -1, [&] { return name + " parent_arc should be -1" + at(u, v); }); }
            else { require(0 <= a && a < int(g.arcs.size()) && g.arcs[a].to == v, [&] { return name + " parent_arc" + at(u, v); }); }
            auto path = res.path(g, u, v);
            require(path.exists == finite, [&] { return name + " path existence" + at(u, v); });
            if (!finite) { require(path.vertices.empty() && path.arcs.empty(), [&] { return name + " absent path storage" + at(u, v); }); continue; }
            require(path.vertices.size() == path.arcs.size() + 1 && int(path.vertices.size()) <= n, [&] { return name + " path dimensions" + at(u, v); });
            require(path.vertices.front() == u && path.vertices.back() == v, [&] { return name + " path endpoints" + at(u, v); });
            require(u != v || path.arcs.empty(), [&] { return name + " diagonal path has arcs" + at(u, v); });
            vector<char> seen(n); lll weight = 0;
            for (int x : path.vertices) { require(0 <= x && x < n && !seen[x], [&] { return name + " path repeats a vertex" + at(u, v); }); seen[x] = true; }
            for (int i = 0; i < int(path.arcs.size()); ++i) {
                int b = path.arcs[i];
                require(0 <= b && b < int(g.arcs.size()), [&] { return name + " path arc index" + at(u, v); });
                require(g.arcs[b].from == path.vertices[i] && g.arcs[b].to == path.vertices[i + 1], [&] { return name + " path orientation" + at(u, v); });
                weight += g.arcs[b].w;}
            require(u == v || path.arcs.back() == a, [&] { return name + " path last arc" + at(u, v); });
            require(weight == d, [&] { return name + " path weight" + at(u, v) + " expected=" + decimal(d) + " actual=" + decimal(weight); });}}}

void checkEccentricities(const Matrix &d) {
    int n = int(d.size()); vector<lll> ecc(n);
    for (int u = 0; u < n; ++u) { ecc[u] = *std::max_element(d[u].begin(), d[u].end()); }
    lll radius = *std::min_element(ecc.begin(), ecc.end()), diameter = *std::max_element(ecc.begin(), ecc.end());
    vector<int> center, periphery;
    for (int u = 0; u < n; ++u) {
        if (ecc[u] == radius) { center.push_back(u); }
        if (ecc[u] == diameter) { periphery.push_back(u); }}
    auto out = eccentricities(d);
    require(out.ecc == ecc, "eccentricities ecc");
    require(out.radius == radius && out.diameter == diameter, "eccentricities radius=" + decimal(out.radius) + " expected=" + decimal(radius)
            + " diameter=" + decimal(out.diameter) + " expected=" + decimal(diameter));
    require(out.center == center && out.periphery == periphery, "eccentricities center/periphery");}

template<class G>
void checkOn(const G &g, const Matrix &expected, const Matrix &low, const Matrix &high, const vector<Matrix> &walks, const vector<int> &ks) {
    validateApsp(g, floydWarshall(g), expected, "floydWarshall");
    validateApsp(g, johnson(g), expected, "johnson");
    requireMatrix(minimaxFloydWarshall(g), low, "minimaxFloydWarshall");
    requireMatrix(minimaxFloydWarshall(g, true), high, "minimaxFloydWarshall maximum");
    for (int k : ks) { requireMatrix(fixedLengthShortestPath(g, k), walks[k], "fixedLengthShortestPath k=" + std::to_string(k)); }}

// Every operation on g (and on CsrGraph(g) when csr) against the oracles; big > 0 also self-checks the oracles.
void checkGraph(const Graph &g, const string &label, bool csr, const vector<int> &ks, lng big = 0) {
    describe(g, label);
    auto expected = bellmanOracle(g), low = thresholdOracle(g, false), high = thresholdOracle(g, true);
    if (big > 0) { validateOracle(g, expected, big); }
    auto walks = walkOracle(g, ks.empty() ? 0 : *std::max_element(ks.begin(), ks.end()));
    checkOn(g, expected, low, high, walks, ks);
    if (csr) { checkOn(CsrGraph(g), expected, low, high, walks, ks); }
    bool negative = false;
    for (const auto &row : expected) { negative |= std::find(row.begin(), row.end(), -PATH_INF) != row.end(); }
    if (g.n >= 1 && !negative) { checkEccentricities(expected); }}
vector<int> upTo(int k) { vector<int> ks(k + 1); iota(ks.begin(), ks.end(), 0); return ks; }

void fixtures() {
    for (bool directed : {false, true}) {
        checkGraph(Graph(0, directed), "empty", true, upTo(3)); checkGraph(Graph(1, directed), "single", true, upTo(3));
        for (lng w : {LMIN, lng(-1), lng(0), lng(1), LMAX}) {
            Graph loop(1, directed); loop.addEdge(0, 0, w); checkGraph(loop, "loop", true, upTo(4));
            Graph two(2, directed); two.addEdge(0, 0, w); two.addEdge(0, 1, 3); checkGraph(two, "loop then arc", true, upTo(4));}}
    Graph negative_edge(5, false);
    negative_edge.addEdge(0, 1, -1); negative_edge.addEdge(3, 4, 7); negative_edge.addEdge(3, 4, 2);
    checkGraph(negative_edge, "undirected negative edge", true, upTo(4));
    Graph chain(6, true);
    for (int u = 0; u < 5; ++u) { chain.addEdge(u, u + 1, u < 3 ? LMAX : LMIN); }
    chain.addEdge(0, 5, LMIN); chain.addEdge(5, 0, LMAX); chain.addEdge(2, 2, LMAX);
    checkGraph(chain, "full-range chain", true, upTo(7));
    Graph reach(7, true);
    reach.addEdge(0, 1, 4); reach.addEdge(1, 2, -1); reach.addEdge(2, 1, 0); reach.addEdge(3, 0, 5);
    reach.addEdge(2, 4, 9); reach.addEdge(5, 6, -3); reach.addEdge(6, 5, 3); reach.addEdge(4, 6, 1);
    checkGraph(reach, "negative cycle reachability", true, upTo(8));
    Graph zero(4, true);
    zero.addEdge(0, 1, 0); zero.addEdge(1, 0, 0); zero.addEdge(1, 2, -1); zero.addEdge(2, 3, 0); zero.addEdge(3, 2, 1); zero.addEdge(1, 1, 0);
    checkGraph(zero, "zero cycles", true, upTo(6));
    Graph parallel(3, false);
    for (int w = 20; w >= 0; --w) { parallel.addEdge(0, 1, w); }
    parallel.addEdge(1, 2, 5); parallel.addEdge(2, 1, 5); parallel.addEdge(2, 2, 0);
    checkGraph(parallel, "parallel undirected", true, upTo(4));
    Graph potential(4, true);
    potential.addEdge(0, 1, -5); potential.addEdge(1, 2, 3); potential.addEdge(2, 0, 3); potential.addEdge(2, 3, -4); potential.addEdge(3, 1, 9);
    checkGraph(potential, "johnson reweighting", true, upTo(6));
    // Without valid potentials Dijkstra improves vertex k 2^k times here, so a johnson that loses its reweighting times out.
    const int k = 40; Graph ladder(2 * k + 1, true);
    for (int j = 1; j <= k; ++j) {
        lng gain = lng(1) << (k - j), wide = lng(k + 1 - j) << (k + 2);
        ladder.addEdge(j - 1, j, 0); ladder.addEdge(j - 1, k + j, wide); ladder.addEdge(k + j, j, -wide - gain);}
    checkGraph(ladder, "johnson exponential ladder", false, {0, 1});
    auto first = floydWarshall(potential), second = floydWarshall(potential); auto copied = first; auto moved = std::move(copied);
    require(first.dist == second.dist && first.parent_arc == second.parent_arc && moved.dist == first.dist, "repeated/copied results");
    require(AllPairsResult().dist.empty() && AllPairsResult(2).dist == Matrix({{0, PATH_INF}, {PATH_INF, 0}}), "AllPairsResult constructor");
    cout << "PASS fixtures: n=0/1, full-range loops and chains, undirected negative edge, reachability, zero cycles, parallel arcs, exponential ladder, copies\n";}

void fixedLengthFixtures() {
    context = "fixed length: large k"; current = nullptr; ++cases;
    for (lng w : {lng(-1), lng(1), lng(7)}) {
        for (lng k : {lng(1000000000000), lng(999999999999), lng(5)}) {
            Graph cycle(3, true);
            for (int u = 0; u < 3; ++u) { cycle.addEdge(u, (u + 1) % 3, w); }
            auto res = fixedLengthShortestPath(cycle, k); Matrix want(3, vector<lll>(3, PATH_INF));
            for (int u = 0; u < 3; ++u) {
                for (int v = 0; v < 3; ++v) { if ((v - u + 3) % 3 == k % 3) { want[u][v] = lll(k) * w; }}}
            requireMatrix(res, want, "directed 3-cycle k=" + std::to_string(k));
            requireMatrix(fixedLengthShortestPath(CsrGraph(cycle), k), want, "directed 3-cycle csr");}}
    lng k = 999999999999;
    Graph loops(2, true); loops.addEdge(0, 0, -1); loops.addEdge(0, 1, 5); loops.addEdge(1, 1, 2);
    requireMatrix(fixedLengthShortestPath(loops, k), Matrix({{-lll(k), 6 - lll(k)}, {PATH_INF, 2 * lll(k)}}), "two loops");
    for (lng w : {LMIN, LMAX}) {
        Graph g(1, true); g.addEdge(0, 0, w); lng edge = (lng(1) << 37) - 1;
        requireMatrix(fixedLengthShortestPath(g, edge), Matrix({{lll(edge) * w}}), "k at the 2^100 bound w=" + decimal(w));}
    // k = floor(2^100 / w) lies inside the domain: k * w = 2^100 - 2^36.
    Graph tight(1, true); tight.addEdge(0, 0, 3 * (lng(1) << 36)); lng tk = 6148914691236517205;
    requireMatrix(fixedLengthShortestPath(tight, tk), Matrix({{(lll(1) << 100) - (lll(1) << 36)}}), "k * w just below 2^100");
    Graph zero(2, true); zero.addEdge(0, 0, 0); zero.addEdge(0, 1, 0);
    requireMatrix(fixedLengthShortestPath(zero, LMAX), Matrix({{0, 0}, {PATH_INF, PATH_INF}}), "zero weights k=INT64_MAX");
    requireMatrix(fixedLengthShortestPath(zero, 0), Matrix({{0, PATH_INF}, {PATH_INF, 0}}), "k=0 identity");
    requireMatrix(fixedLengthShortestPath(Graph(2, false), LMAX), Matrix(2, vector<lll>(2, PATH_INF)), "no arcs k=INT64_MAX");
    require(fixedLengthShortestPath(Graph(0, true), LMAX).empty(), "n=0 k=INT64_MAX");
    Graph undirected(3, false); undirected.addEdge(0, 1, -2); undirected.addEdge(1, 2, 3);
    auto walks = walkOracle(undirected, 41);
    for (int j : {17, 40, 41}) { requireMatrix(fixedLengthShortestPath(undirected, j), walks[j], "undirected k=" + std::to_string(j)); }
    cout << "PASS fixed-length fixtures: k up to 10^12, INT64_MAX with zero weights, the 2^100 bound with INT64_MIN/MAX\n";}

void eccentricityFixtures() {
    context = "eccentricities fixtures"; current = nullptr; ++cases;
    auto run = [&](const Graph &g) { return eccentricities(floydWarshall(g).dist); };
    Graph path(3, false); path.addEdge(0, 1, 1); path.addEdge(1, 2, 1);
    auto e = run(path);
    require(e.ecc == vector<lll>({2, 1, 2}) && e.radius == 1 && e.diameter == 2, "path ecc");
    require(e.center == vector<int>({1}) && e.periphery == vector<int>({0, 2}), "path center/periphery");
    auto lone = run(Graph(1, true));
    require(lone.ecc == vector<lll>({0}) && lone.radius == 0 && lone.diameter == 0 && lone.center == vector<int>({0}) && lone.periphery == vector<int>({0}), "n=1");
    auto split = run(Graph(3, false));
    require(split.radius == PATH_INF && split.diameter == PATH_INF && split.center == vector<int>({0, 1, 2}) && split.periphery == split.center, "disconnected");
    Graph arc(2, true); arc.addEdge(0, 1, -5);
    auto directed = run(arc);
    require(directed.ecc == vector<lll>({0, PATH_INF}) && directed.radius == 0 && directed.diameter == PATH_INF, "directed out-eccentricity");
    require(directed.center == vector<int>({0}) && directed.periphery == vector<int>({1}), "directed center/periphery");
    Graph big(3, true); big.addEdge(0, 1, LMAX); big.addEdge(1, 2, LMAX); big.addEdge(2, 0, LMAX);
    auto wide = run(big);
    require(wide.radius == 2 * lll(LMAX) && wide.diameter == 2 * lll(LMAX) && wide.center.size() == 3, "full-range ecc");
    checkEccentricities(Matrix({{0, -3, 4}, {-1, 0, -2}, {5, PATH_INF, 0}}));
    auto below = eccentricities(Matrix({{-5}}));
    require(below.ecc == vector<lll>({-5}) && below.radius == -5 && below.diameter == -5, "negative diagonal ecc");
    checkEccentricities(Matrix({{-5, -7}, {-9, -6}}));
    cout << "PASS eccentricities fixtures: path, n=1, disconnected, directed, full range, negative entries\n";}

// Every graph on n vertices whose slots (ordered pairs or unordered for undirected, loops when asked) take a weight or are absent.
void exhaustive(int n, bool directed, bool loops, const vector<lng> &weights, int csr_every, const vector<int> &ks) {
    vector<pair<int, int>> slots;
    for (int u = 0; u < n; ++u) {
        for (int v = directed ? 0 : u; v < n; ++v) { if (u != v || loops) { slots.push_back({u, v}); }}}
    int base = int(weights.size()) + 1, graphs = 1; lng big = 0;
    for (lng w : weights) { big = max(big, abs(w)); }
    for (int i = 0; i < int(slots.size()); ++i) { graphs *= base; }
    for (int code = 0; code < graphs; ++code) {
        Graph g(n, directed);
        for (int i = 0, x = code; i < int(slots.size()); ++i, x /= base) {
            if (x % base) { g.addEdge(slots[i].first, slots[i].second, weights[x % base - 1]); }}
        checkGraph(g, "exhaustive", code % csr_every == 0, ks, big);}
    cout << "PASS exhaustive n=" << n << " directed=" << directed << " loops=" << loops << " weights=" << weights.size() << " graphs=" << graphs << '\n';}

lng randomWeight(int domain) {
    static const array<lng, 8> extremes = {LMIN, LMAX, -(lng(1) << 62), lng(1) << 62, -1, 0, 1, LMIN + 1};
    static const array<lng, 4> wide = {0, 1, lng(1) << 62, LMAX};
    if (domain == 0) { return lng(rng() % 7) - 3; }
    if (domain == 1) { return lng(rng() % 21); }
    if (domain == 2) { return extremes[rng() % extremes.size()]; }
    return wide[rng() % wide.size()];}
Graph randomGraph(int n, bool directed, int m, int domain) {
    Graph g(n, directed);
    if (n == 0) { return g; }
    vector<lng> h(n);
    for (auto &x : h) { x = lng(rng() % 101) - 50; }
    for (int j = 0; j < m; ++j) {
        int u = int(rng() % ulng(n)), v = int(rng() % ulng(n));
        // Domain 4 has reduced costs >= 0 under potential h: negative arcs, no negative cycle.
        g.addEdge(u, v, domain == 4 ? h[u] - h[v] + lng(rng() % 4) : randomWeight(domain));}
    return g;}

void randomTests(const string &mode) {
    int count = mode == "quick" ? 150 : mode == "full" ? 2000 : 10000;
    for (int it = 0; it < count; ++it) {
        int n = int(rng() % 9), domain = it % 5; bool directed = domain == 4 || rng() % 2;
        checkGraph(randomGraph(n, directed, int(rng() % ulng(3 * n + 3)), domain), "random", it % 2 == 0, upTo(n + 2));}
    int medium = mode == "quick" ? 4 : mode == "full" ? 40 : 200;
    for (int it = 0; it < medium; ++it) {
        int n = 10 + int(rng() % 31), domain = it % 5; bool directed = domain == 4 || rng() % 2;
        checkGraph(randomGraph(n, directed, int(rng() % ulng(4 * n)), domain), "random medium", it % 4 == 0, {0, 1, 2, 3, n});}
    cout << "PASS random graphs n<=8: " << count << ", n in [10,40]: " << medium << '\n';}

// Word-boundary sizes for the bitset closure; APSP and fixed length on the same graphs.
void boundaryTests(const string &mode) {
    vector<int> sizes = mode == "quick" ? vector<int>{64, 65} : vector<int>{63, 64, 65, 127, 128, 129, 200};
    int rounds = mode == "stress" ? 2 : 1;
    for (int n : sizes) {
        for (int round = 0; round < rounds; ++round) {
            for (int domain : {0, 2, 4}) {
                bool directed = domain == 4 || rng() % 2;
                checkGraph(randomGraph(n, directed, 2 * n, domain), "boundary random", domain == 0, {0, 1, 2});}
            Graph chain(n, true), reversed(n, false);
            for (int u = 0; u + 1 < n; ++u) {
                chain.addEdge(u, u + 1, lng(rng() % 2001) - 1000);
                reversed.addEdge(n - 1 - u, n - 2 - u, randomWeight(2));}
            chain.addEdge(n - 1, 0, LMAX);
            checkGraph(chain, "boundary chain", true, {0, 1});
            checkGraph(reversed, "boundary reversed undirected chain", false, {0, 1});}}
    cout << "PASS word-boundary graphs sizes=" << sizes.size() << " rounds=" << rounds << '\n';}

// A size at which rows of n + 127 words instead of ceil(n / 64) cost the closure its word-parallel factor.
void minimaxScaleFixture() {
    int n = 4000; Graph g(n, true);
    for (int i = 0; i + 1 < n; ++i) { g.addEdge(i, i + 1, i); }
    describe(g, "minimax long path");
    require(minimaxFloydWarshall(g)[0][n - 1] == n - 2, "minimax closure on a long path");}

void invalid(const string &which) {
    Graph g(2, true); g.addEdge(0, 1, 2);
    if (which == "ecc-non-square") { (void)eccentricities(Matrix({{0, 1}, {0}})); }
    if (which == "finite-range") { (void)floydWarshall(g).finite(0, 2); }
    if (which == "finite-row") { (void)floydWarshall(g).finite(2, 0); }
    if (which == "finite-row-far") { (void)floydWarshall(g).finite(3, 0); }
    if (which == "finite-column-far") { (void)floydWarshall(g).finite(0, 3); }
    if (which == "fixed-negative-k") { (void)fixedLengthShortestPath(g, -1); }
    if (which == "fixed-overflow") { Graph h(1, true); h.addEdge(0, 0, LMIN); (void)fixedLengthShortestPath(h, lng(1) << 37); }
    if (which == "fixed-overflow-far") { Graph h(1, true); h.addEdge(0, 0, LMIN); (void)fixedLengthShortestPath(h, lng(1) << 38); }
    if (which == "ecc-empty") { (void)eccentricities(Matrix()); }
    if (which == "ecc-negative-infinite") { g.addEdge(1, 1, -1); (void)eccentricities(floydWarshall(g).dist); }
    if (which == "path-size") { (void)floydWarshall(g).path(Graph(3, true), 0, 1); }
    std::exit(2);}

int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--invalid") { invalid(argv[++i]); }
        if (arg == "--mode") { mode = argv[++i]; }
        if (arg == "--seed") { seed = std::stoull(argv[++i]); }}
    rng.seed(seed);
    fixtures(); fixedLengthFixtures(); eccentricityFixtures();
    const vector<lng> signed3 = {-1, 0, 1}, signed2 = {-1, 1};
    if (mode == "quick") { exhaustive(2, true, true, signed3, 3, upTo(4)); exhaustive(2, false, true, signed3, 3, upTo(4)); }
    else {
        exhaustive(2, true, true, signed3, 1, upTo(6)); exhaustive(2, false, true, signed3, 1, upTo(6));
        exhaustive(3, true, false, signed3, 5, upTo(5)); exhaustive(3, false, true, signed3, 5, upTo(5));
        if (mode == "full") { exhaustive(3, true, true, signed2, 7, upTo(4)); }
        else { exhaustive(3, true, true, signed3, 11, upTo(4)); }}
    randomTests(mode); boundaryTests(mode);
#ifdef NDEBUG
    if (mode != "quick") { minimaxScaleFixture(); }
#endif
    cout << "PASS all-pairs shortest paths mode=" << mode << " seed=" << seed << " cases=" << cases << " checks=" << checks << '\n';}
