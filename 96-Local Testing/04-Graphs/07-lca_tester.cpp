#include "../../04-Graphs/07-lca.hpp"

static ulng seed = 20260927;
static lng checks = 0;
static string context;

string show(lll x) {
    if (!x) { return "0"; }
    bool neg = x < 0; ulll u = neg ? ulll(0) - ulll(x) : ulll(x); string s;
    while (u) { s += char('0' + u % 10); u /= 10; }
    if (neg) { s += '-'; } reverse(s.begin(), s.end()); return s;}
void require(bool ok, const string &op, const string &want = "true", const string &got = "false") {
    ++checks;
    if (!ok) { throw std::runtime_error(context + " operation=" + op + " expected=" + want + " actual=" + got); }}
void expect(int got, int want, const string &op) { require(got == want, op, std::to_string(want), std::to_string(got)); }

// BFS independently constructs rooted parent chains and unique undirected paths.
struct Oracle {
    vector<vector<pair<int, lng>>> adj;
    vector<int> parent, depth, component;
    explicit Oracle(const Graph &g, int root = -1) : adj(g.n), parent(g.n, -2), depth(g.n), component(g.n) {
        for (auto e : g.edges) { adj[e.u].push_back({e.v, e.w}); adj[e.v].push_back({e.u, e.w}); }
        auto bfs = [&](int r) {
            vector<int> q{r}; parent[r] = -1; component[r] = r;
            for (int i = 0; i < int(q.size()); ++i) {
                int u = q[i];
                for (auto [v, w] : adj[u]) {
                    (void)w;
                    if (parent[v] != -2) { continue; }
                    parent[v] = u; depth[v] = depth[u] + 1; component[v] = r; q.push_back(v);}}};
        if (root != -1) { bfs(root); }
        for (int u = 0; u < g.n; ++u) { if (parent[u] == -2) { bfs(u); }}}
    int ancestor(int u, lng k) const {
        while (k && u != -1) { --k; u = parent[u]; }
        return u;}
    int lca(int u, int v) const {
        set<int> seen;
        for (; u != -1; u = parent[u]) { seen.insert(u); }
        for (; v != -1 && !seen.count(v); v = parent[v]) {}
        return v;}
    pair<vector<int>, lll> path(int u, int v) const {
        vector<int> prev(adj.size(), -2), q{u}; vector<lll> weight(adj.size()); prev[u] = -1;
        for (int i = 0; i < int(q.size()); ++i) {
            int a = q[i];
            for (auto [b, w] : adj[a]) {
                if (prev[b] != -2) { continue; }
                prev[b] = a; weight[b] = weight[a] + lll(w); q.push_back(b);}}
        if (prev[v] == -2) { return {{}, 0}; }
        vector<int> result;
        for (int a = v; a != -1; a = prev[a]) { result.push_back(a); }
        reverse(result.begin(), result.end()); return {result, weight[v]};}
};

// Noncommutative string folds and path intersections against explicit BFS paths.
void paths(const Graph &g, int root, const LCA &binary, const EulerLCA &euler, const Oracle &oracle) {
    auto key = [](int u, int v) { return std::to_string(min(u, v)) + "-" + std::to_string(max(u, v)) + ","; };
    vector<string> label(g.n), edge(g.n);
    for (int u = 0; u < g.n; ++u) {
        label[u] = std::to_string(u) + ","; edge[u] = oracle.parent[u] < 0 ? "R" : key(u, oracle.parent[u]);}
    auto cat = [](const string &x, const string &y) { return x + y; };
    LCAFold vertex(g, label, string(), cat, root);
    LCAFold edges(CsrGraph(g), edge, string(), cat, root);
    vector<vector<vector<int>>> path(g.n, vector<vector<int>>(g.n));
    for (int u = 0; u < g.n; ++u) { for (int v = 0; v < g.n; ++v) { path[u][v] = oracle.path(u, v).first; }}
    bool folds = g.n <= 4 || g.n > 6 || root <= 0 || root == g.n - 1;
    for (int u = 0; u < g.n && folds; ++u) {
        for (int v = 0; v < g.n; ++v) {
            const auto &p = path[u][v]; string want = p.empty() ? "" : label[p[0]], ewant;
            for (int i = 1; i < int(p.size()); ++i) { want += label[p[i]]; ewant += key(p[i - 1], p[i]); }
            string pair = " u=" + std::to_string(u) + " v=" + std::to_string(v);
            auto [ok, got] = vertex.pathFold(u, v);
            require(ok == !p.empty() && got == want, "vertex pathFold" + pair, want, got);
            auto [eok, egot] = edges.pathFold(u, v, true);
            require(eok == !p.empty() && egot == ewant, "edge pathFold" + pair, ewant, egot);}}
    std::mt19937_64 rng(seed ^ ulng(g.n * 1000003 + root));
    int total = g.n * g.n * g.n * g.n, quads = g.n <= 5 && root == -1 ? total : !g.n ? 0 : root == -1 ? 100 : 10;
    for (int i = 0; i < quads; ++i) {
        int code = quads == total ? i : int(rng() % ulng(total)), a = code % g.n, b = code / g.n % g.n;
        int c = code / g.n / g.n % g.n, d = code / g.n / g.n / g.n;
        const auto &p = path[a][b], &q = path[c][d]; std::pair<int, int> want{-1, -1};
        vector<int> common;
        for (int x : q) { if (std::find(p.begin(), p.end(), x) != p.end()) { common.push_back(x); }}
        if (!common.empty()) { want = {common.front(), common.back()}; }
        string quad = " a=" + std::to_string(a) + " b=" + std::to_string(b) + " c=" + std::to_string(c) + " d=" + std::to_string(d);
        require(pathIntersection(binary, a, b, c, d) == want && pathIntersection(euler, a, b, c, d) == want,
            "pathIntersection" + quad, std::to_string(want.first) + "," + std::to_string(want.second));}}

void verify(const Graph &g, int root, bool exhaustive_reroot = true) {
    context = "n=" + std::to_string(g.n) + " root=" + std::to_string(root) + " edges=";
    for (auto e : g.edges) { context += "(" + std::to_string(e.u) + "," + std::to_string(e.v) + "," + show(e.w) + ")"; }
    LCA binary(g, root), csr_binary(CsrGraph(g), root); EulerLCA euler(g, root), csr_euler(CsrGraph(g), root);
    Oracle oracle(g, root); vector<pair<int, int>> queries; vector<int> answers;
    require(binary.parent == oracle.parent && binary.dep == oracle.depth && binary.component == oracle.component,
        "rooted metadata against BFS");
    require(euler.parent == binary.parent && euler.dep == binary.dep && euler.component == binary.component,
        "Euler rooted metadata");
    require(binary.parent == csr_binary.parent && euler.tour == csr_euler.tour, "CSR metadata parity");
    expect(binary.n, g.n, "n metadata"); expect(binary.timer, g.n, "timer covers forest");
    expect(binary.root, root == -1 && g.n ? 0 : root, "preferred root metadata");
    expect(int(euler.tour.size()), 2 * g.n + 1, "Euler virtual-root tour length");
    for (int i = 1; i < int(euler.tour.size()); ++i) {
        expect(abs(euler.height(i) - euler.height(i - 1)), 1, "FCB plus/minus-one invariant");}
    vector<Oracle> reroots;
    if (exhaustive_reroot) { for (int r = 0; r < g.n; ++r) { reroots.emplace_back(g, r); }}
    for (int u = 0; u < g.n; ++u) {
        for (int k = 0; k <= oracle.depth[u] + 1; ++k) {
            expect(binary.kthAncestor(u, k), oracle.ancestor(u, k), "kthAncestor u=" + std::to_string(u) + " k=" + std::to_string(k));
            expect(binary.getKthAncestor(u, k), oracle.ancestor(u, k), "legacy getKthAncestor");}
        expect(binary.getKthAncestor(u, INT_MIN), u, "legacy negative ancestor is identity");
        expect(binary.kthAncestor(u, std::numeric_limits<lng>::max()), -1, "huge missing ancestor");
        for (int v = 0; v < g.n; ++v) {
            string pair = " u=" + std::to_string(u) + " v=" + std::to_string(v);
            int want = oracle.lca(u, v); queries.push_back({u, v}); answers.push_back(want);
            expect(binary.getLCA(u, v), want, "binary LCA" + pair);
            expect(euler.getLCA(u, v), want, "Euler LCA" + pair);
            expect(csr_binary.getLCA(u, v), want, "CSR binary LCA" + pair);
            expect(csr_euler.getLCA(u, v), want, "CSR Euler LCA" + pair);
            require(binary.isAncestor(u, v) == (want == u) && euler.isAncestor(u, v) == (want == u), "ancestor relation" + pair);
            require((binary.component[u] == binary.component[v] && binary.t_in[u] <= binary.t_in[v] && binary.t_in[v] <= binary.t_out[u])
                == (want == u), "inclusive subtree interval" + pair);
            auto [path, weight] = oracle.path(u, v); int length = path.empty() ? -1 : int(path.size()) - 1;
            expect(binary.distance(u, v), length, "binary edge distance" + pair);
            expect(euler.distance(u, v), length, "Euler edge distance" + pair);
            lll out = 123456789;
            require(binary.weightedDistance(u, v, out) == !path.empty(), "binary weighted connection status" + pair);
            require(out == (path.empty() ? lll(123456789) : weight), "binary exact weight/unchanged missing", show(weight), show(out));
            out = 123456789;
            require(euler.weightedDistance(u, v, out) == !path.empty(), "Euler weighted connection status" + pair);
            require(out == (path.empty() ? lll(123456789) : weight), "Euler exact weight/unchanged missing", show(weight), show(out));
            for (int k = 0; k <= int(path.size()); ++k) {
                expect(binary.jump(u, v, k), k < int(path.size()) ? path[k] : -1, "path jump" + pair + " k=" + std::to_string(k));}
            expect(binary.jump(u, v, std::numeric_limits<lng>::max()), -1, "huge missing path jump");
            if (exhaustive_reroot) { for (int r = 0; r < g.n; ++r) {
                int expected = oracle.component[u] == oracle.component[r] ? reroots[r].lca(u, v) : -1;
                expect(binary.rerootedLCA(u, v, r), expected, "binary reroot" + pair + " r=" + std::to_string(r));
                expect(euler.rerootedLCA(u, v, r), expected, "Euler reroot" + pair + " r=" + std::to_string(r));}}}}
    paths(g, root, binary, euler, oracle);
    require(offlineLCA(g, queries, root) == answers && offlineLCA(CsrGraph(g), queries, root) == answers,
        "offline Tarjan all pairs vs independent ancestor intersections");
    reverse(queries.begin(), queries.end()); reverse(answers.begin(), answers.end());
    if (!queries.empty()) { queries.push_back(queries[0]); answers.push_back(answers[0]); }
    require(offlineLCA(g, queries, root) == answers, "offline reordered/duplicated queries");
    require(offlineLCA(g, {}, root).empty(), "offline empty query batch");
    LCA copied = binary; EulerLCA ecopied = euler;
    LCA moved = std::move(copied); EulerLCA emoved = std::move(ecopied);
    copied = moved; ecopied = emoved;
    if (g.n) {
        expect(copied.getLCA(0, g.n - 1), oracle.lca(0, g.n - 1), "binary copy/move assignment");
        expect(ecopied.getLCA(0, g.n - 1), oracle.lca(0, g.n - 1), "Euler copy/move assignment");}}

void exhaustive(const string &mode) {
    int bound = mode == "quick" ? 4 : 6, cases = 0;
    for (int n = 0; n <= bound; ++n) {
        vector<pair<int, int>> slots;
        for (int u = 0; u < n; ++u) { for (int v = u + 1; v < n; ++v) { slots.push_back({u, v}); }}
        for (uint mask = 0; mask < (1U << slots.size()); ++mask) {
            vector<vector<bool>> reach(n, vector<bool>(n)); Graph g(n); bool forest = true;
            for (int i = 0; i < int(slots.size()); ++i) { if (mask >> i & 1U) {
                auto [u, v] = slots[i]; reach[u][v] = reach[v][u] = true; g.addEdge(u, v);}}
            // A simple component is acyclic iff edges = vertices-1.
            for (int k = 0; k < n; ++k) { for (int u = 0; u < n; ++u) { for (int v = 0; v < n; ++v) {
                reach[u][v] = reach[u][v] || (reach[u][k] && reach[k][v]);}}}
            int components = 0;
            for (int u = 0; u < n; ++u) {
                bool first = true; for (int v = 0; v < u; ++v) { first = first && !reach[u][v]; }
                components += first;}
            forest = int(g.edges.size()) == n - components;
            if (!forest) { continue; }
            for (int root = -1; root < n; ++root) { verify(g, root); ++cases; }}}
    std::cout << "PASS exhaustive labeled forests through n=" << bound << " root-cases=" << cases << '\n';}
void legacy() {
    context = "legacy adjacency constructors";
    vector<vector<int>> zero{{1}, {0, 2}, {1}}, one{{}, {2}, {1, 3}, {2}};
    LCA a(3, 0, zero), b(3, 1, one), empty(0, -1, {});
    expect(a.getLCA(0, 2), 0, "zero-based constructor"); expect(a.getKthAncestor(2, 2), 0, "zero-based kth ancestor");
    expect(b.getLCA(2, 3), 2, "one-based constructor"); expect(b.getKthAncestor(3, 2), 1, "one-based kth ancestor");
    expect(b.getLCA(0, 3), -1, "one-based unused vertex 0 isolated"); expect(b.n, 4, "legacy actual adjacency size");
    expect(empty.n, 0, "empty legacy constructor");
    Graph g(3); g.addEdge(0, 1); LCA snapshot(g); EulerLCA esnapshot(g); g.addEdge(1, 2);
    expect(snapshot.getLCA(0, 2), -1, "binary owning snapshot after input mutation");
    expect(esnapshot.getLCA(0, 2), -1, "Euler owning snapshot after input mutation");
    snapshot = LCA(g); esnapshot = EulerLCA(g);
    expect(snapshot.getLCA(0, 2), 0, "binary rebuild by assignment"); expect(esnapshot.getLCA(0, 2), 0, "Euler rebuild by assignment");
    std::cout << "PASS legacy labels, snapshot lifetime and rebuild\n";}
void randomized(const string &mode) {
    std::mt19937_64 rng(seed); int count = mode == "quick" ? 40 : mode == "full" ? 350 : 2500;
    array<lng, 7> weights{std::numeric_limits<lng>::min(), std::numeric_limits<lng>::max(), -100, -1, 0, 1, 100};
    for (int i = 0; i < count; ++i) {
        int n = 1 + int(rng() % 18); Graph g(n); vector<int> perm(n); iota(perm.begin(), perm.end(), 0);
        std::shuffle(perm.begin(), perm.end(), rng);
        for (int u = 1; u < n; ++u) { if (rng() % 5) { g.addEdge(perm[u], perm[rng() % u], weights[rng() % weights.size()]); }}
        verify(g, int(rng() % (n + 1)) - 1);}
    std::cout << "PASS seeded weighted forests count=" << count << '\n';}
void shapes(const string &mode) {
    std::mt19937_64 rng(seed + 1);
    for (int n : {7, 8, 15, 16, 31, 32, 63, 64, 127, 128, 255, 256, 511, 512, 1023, 1024}) {
        for (int shape = 0; shape < 5; ++shape) {
            Graph g(n);
            for (int u = 1; u < n; ++u) {
                if (shape == 4 && u % 7 == 0) { continue; }
                int p = shape == 0 ? u - 1 : shape == 1 ? 0 : shape == 2 ? (u - 1) / 2 : int(rng() % u);
                g.addEdge(u, p, u % 2 ? std::numeric_limits<lng>::max() : std::numeric_limits<lng>::min());}
            LCA binary(g); EulerLCA euler(g); Oracle oracle(g);
            context = "FCB boundary n=" + std::to_string(n) + " shape=" + std::to_string(shape);
            vector<pair<int, int>> queries; vector<int> answers;
            for (int i = 0; i < 1000; ++i) {
                int u = int(rng() % n), v = int(rng() % n), want = oracle.lca(u, v);
                expect(binary.getLCA(u, v), want, "binary boundary oracle"); expect(euler.getLCA(u, v), want, "Euler boundary oracle");
                queries.push_back({u, v}); answers.push_back(want);}
            require(offlineLCA(g, queries) == answers, "Tarjan boundary oracle");}}
    int n = mode == "quick" ? 2000 : mode == "full" ? 200000 : 500000;
    Graph g(n); for (int u = 1; u < n; ++u) { g.addEdge(u - 1, u, std::numeric_limits<lng>::max()); }
    LCA binary(g); EulerLCA euler(g); context = "deep chain n=" + std::to_string(n);
    for (int i = 0; i < 20000; ++i) {
        int u = int(rng() % n), v = int(rng() % n), r = int(rng() % n);
        expect(binary.getLCA(u, v), min(u, v), "deep binary LCA"); expect(euler.getLCA(u, v), min(u, v), "deep Euler LCA");
        array<int, 3> points{u, v, r}; sort(points.begin(), points.end());
        expect(binary.rerootedLCA(u, v, r), points[1], "deep binary reroot median");
        expect(euler.rerootedLCA(u, v, r), points[1], "deep Euler reroot median");}
    lll out = 0; require(binary.weightedDistance(0, n - 1, out) && out == lll(n - 1) * std::numeric_limits<lng>::max(), "deep exact wide sum");
    expect(binary.jump(n - 1, 0, n - 1), 0, "deep jump to endpoint");
    LCAFold fold(g, vector<lng>(n, 1), lng(0), [](lng x, lng y) { return x + y; });
    for (int i = 0; i < 2000; ++i) {
        int u = int(rng() % n), v = int(rng() % n);
        require(fold.pathFold(u, v) == std::pair<bool, lng>{true, abs(u - v) + 1}, "deep vertex pathFold");
        require(fold.pathFold(u, v, true) == std::pair<bool, lng>{true, abs(u - v)}, "deep edge pathFold");
        require((pathIntersection(euler, u, v, n - 1 - u, n - 1 - v).first != -1) == (max(u, v) >= min(n - 1 - u, n - 1 - v) && min(u, v) <= max(n - 1 - u, n - 1 - v)), "deep interval intersection");}
    require(offlineLCA(g, {{0, n - 1}, {n - 1, n - 1}, {n / 2, n - 1}}) == vector<int>({0, n - 1, n / 2}), "deep Tarjan stack safety");
    std::cout << "PASS FCB block boundaries, adversarial shapes and iterative chain n=" << n << '\n';}
void invalid(const string &probe) {
    Graph g(3); g.addEdge(0, 1); g.addEdge(1, 2);
    if (probe == "directed") { LCA x(Graph(1, true)); }
    else if (probe == "root-negative") { LCA x(g, -2); }
    else if (probe == "root-end") { EulerLCA x(g, 3); }
    else if (probe == "empty-root") { LCA x(Graph(0), 0); }
    else if (probe == "legacy-size") { LCA x(4, 0, {{1}, {0}}); }
    else if (probe == "legacy-label") { LCA x(2, 0, {{2}, {0}}); }
    else if (probe == "legacy-asymmetric") { LCA x(2, 0, {{1}, {}}); }
    else if (probe == "legacy-duplicate") { LCA x(2, 0, {{1, 1}, {0, 0}}); }
    else if (probe == "cycle" || probe == "euler-cycle" || probe == "offline-cycle") {
        g.addEdge(2, 0);
        if (probe == "cycle") { LCA x(g); }
        if (probe == "euler-cycle") { EulerLCA x(g); }
        if (probe == "offline-cycle") { (void)offlineLCA(g, {}); }}
    else if (probe == "loop") { g.addEdge(0, 0); LCA x(g); }
    else if (probe == "parallel") { g.addEdge(0, 1); LCA x(g); }
    else {
        LCA x(g); EulerLCA y(g);
        if (probe == "query-negative") { (void)x.getLCA(-1, 0); }
        else if (probe == "query-end") { (void)x.distance(0, 3); }
        else if (probe == "ancestor-negative") { (void)x.kthAncestor(0, -1); }
        else if (probe == "jump-negative") { (void)x.jump(0, 1, -1); }
        else if (probe == "reroot-end") { (void)x.rerootedLCA(0, 1, 3); }
        else if (probe == "euler-query-end") { (void)y.getLCA(0, 3); }
        else if (probe == "offline-query-end") { (void)offlineLCA(g, {{0, 3}}); }
        else if (probe == "intersection-end") { (void)pathIntersection(y, 0, 1, 2, 3); }
        else if (probe == "fold-size") { LCAFold z(g, vector<int>(2), 0, [](int p, int q) { return p + q; }); }
        else if (probe == "fold-query-end") { LCAFold z(g, vector<int>(3), 0, [](int p, int q) { return p + q; }); (void)z.pathFold(0, 3); }
        else { throw std::runtime_error("unknown invalid probe " + probe); }}
    throw std::runtime_error("precondition accepted " + probe);}
int main(int argc, char **argv) {
    string mode = "full";
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); return 1; }
            else { throw std::runtime_error("unknown argument " + arg); }}
        exhaustive(mode); legacy(); randomized(mode); shapes(mode);
        std::cout << "PASS 07-lca mode=" << mode << " seed=" << seed << " checks=" << checks << '\n';} catch (const std::exception &e) {
        std::cerr << "FAIL 07-lca seed=" << seed << " " << e.what() << '\n'; return 1;}}
