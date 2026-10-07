#include "../../04-Graphs/06-mst.hpp"

static ulng seed = 20260927;
static lng checks = 0;
static string context;
using Reach = vector<vector<bool>>;

string show(lll x) {
    if (!x) { return "0"; }
    bool negative = x < 0; ulll u = negative ? ulll(0) - ulll(x) : ulll(x); string s;
    while (u) { s += char('0' + u % 10); u /= 10; }
    if (negative) { s += '-'; } reverse(s.begin(), s.end()); return s;}
void require(bool ok, const string &operation, const string &want = "true", const string &got = "false") {
    ++checks; if (ok) { return; }
    throw std::runtime_error(context + " operation=" + operation + " expected=" + want + " actual=" + got);}
Reach reachability(const Graph &g, const vector<int> &ids, bool filtered = false, lng threshold = 0, bool maximum = false) {
    Reach r(g.n, vector<bool>(g.n));
    for (int i = 0; i < g.n; ++i) { r[i][i] = true; }
    for (int id : ids) {
        auto e = g.edges[id];
        if (!filtered || (maximum ? e.w >= threshold : e.w <= threshold)) { r[e.u][e.v] = r[e.v][e.u] = true; }}
    for (int k = 0; k < g.n; ++k) { for (int i = 0; i < g.n; ++i) { for (int j = 0; j < g.n; ++j) {
        r[i][j] = r[i][j] || (r[i][k] && r[k][j]);}}}
    return r;}
int components(const Reach &r) {
    int count = 0;
    for (int i = 0; i < int(r.size()); ++i) {
        bool first = true;
        for (int j = 0; j < i; ++j) { first = first && !r[i][j]; }
        count += first;}
    return count;}
struct Oracle { Reach reachable; int components; lll minimum, maximum; };
Oracle brute(const Graph &g) {
    int m = int(g.edges.size()); vector<int> ids(m); iota(ids.begin(), ids.end(), 0);
    Reach reachable = reachability(g, ids); int count = components(reachable), need = g.n - count;
    Oracle res{reachable, count, 0, 0}; bool found = false;
    require(m < 25, "bounded brute-force fixture");
    for (uint mask = 0; mask < (1U << m); ++mask) {
        if (std::popcount(mask) != need) { continue; }
        vector<int> subset; lll sum = 0;
        for (int i = 0; i < m; ++i) { if (mask >> i & 1U) { subset.push_back(i); sum += lll(g.edges[i].w); } }
        if (reachability(g, subset) != reachable) { continue; }
        if (!found) { res.minimum = res.maximum = sum; found = true; }
        else { res.minimum = min(res.minimum, sum); res.maximum = max(res.maximum, sum); }}
    require(found, "finite graph has spanning forest subset"); return res;}
void forest(const Graph &g, const SpanningForest &result, const Oracle &oracle, bool maximum = false) {
    require(result.components == oracle.components, "forest component count", std::to_string(oracle.components), std::to_string(result.components));
    require(int(result.edges.size()) == g.n - oracle.components, "forest edge count n-components");
    vector<bool> seen(g.edges.size()); lll sum = 0;
    for (int id : result.edges) {
        require(0 <= id && id < int(g.edges.size()), "forest logical ID in range");
        require(!seen[id], "forest no duplicate logical edge"); seen[id] = true;
        auto e = g.edges[id]; require(e.u != e.v, "forest excludes loops"); sum += lll(e.w);}
    require(reachability(g, result.edges) == oracle.reachable, "forest spans every original component");
    require(result.weight == sum, "reported exact total equals selected weights", show(sum), show(result.weight));
    lll want = maximum ? oracle.maximum : oracle.minimum;
    require(result.weight == want, "optimal total against exhaustive subsets", show(want), show(result.weight));}
vector<vector<lng>> bottlenecks(const Graph &g, bool maximum, Reach &has) {
    has.assign(g.n, vector<bool>(g.n)); vector<vector<lng>> d(g.n, vector<lng>(g.n));
    for (int u = 0; u < g.n; ++u) {
        has[u][u] = true; d[u][u] = maximum ? std::numeric_limits<lng>::max() : std::numeric_limits<lng>::min();}
    for (auto e : g.edges) {
        if (e.u == e.v) { continue; }
        if (!has[e.u][e.v] || (maximum ? e.w > d[e.u][e.v] : e.w < d[e.u][e.v])) {
            has[e.u][e.v] = has[e.v][e.u] = true; d[e.u][e.v] = d[e.v][e.u] = e.w;}}
    for (int k = 0; k < g.n; ++k) { for (int u = 0; u < g.n; ++u) { for (int v = 0; v < g.n; ++v) {
        if (!has[u][k] || !has[k][v]) { continue; }
        lng candidate = maximum ? min(d[u][k], d[k][v]) : max(d[u][k], d[k][v]);
        if (!has[u][v] || (maximum ? candidate > d[u][v] : candidate < d[u][v])) {
            d[u][v] = candidate; has[u][v] = true;}}}}
    return d;}
void reconstruction(const Graph &g, const Oracle &oracle, bool maximum) {
    KruskalReconstruction tree(g, maximum), csr_tree(CsrGraph(g), maximum);
    require(tree.n == g.n && tree.maximum == maximum, "reconstruction input metadata");
    int count = int(tree.child.size());
    require(count == 2 * g.n - oracle.components, "one merge node per selected forest edge");
    require(tree.child == csr_tree.child && tree.parent == csr_tree.parent && tree.edge == csr_tree.edge && tree.value == csr_tree.value,
        "CSR identical reconstruction");
    vector<vector<int>> leaves(count); int roots = 0; SpanningForest chosen; chosen.components = oracle.components;
    for (int u = 0; u < count; ++u) {
        if (u < g.n) {
            require(tree.child[u] == array<int, 2>{-1, -1} && tree.edge[u] == -1 && tree.leaf_count[u] == 1,
                "original vertices are leaves"); leaves[u] = {u};}
        else {
            auto [a, b] = tree.child[u];
            require(0 <= a && a < u && 0 <= b && b < u && a != b, "merge children precede parent");
            require(tree.parent[a] == u && tree.parent[b] == u, "child-parent reciprocity");
            leaves[u] = leaves[a]; leaves[u].insert(leaves[u].end(), leaves[b].begin(), leaves[b].end());
            sort(leaves[u].begin(), leaves[u].end());
            require(std::adjacent_find(leaves[u].begin(), leaves[u].end()) == leaves[u].end(), "disjoint merge subtrees");
            require(tree.leaf_count[u] == int(leaves[u].size()), "stored subtree leaf count");
            require(0 <= tree.edge[u] && tree.edge[u] < int(g.edges.size()), "merge edge in graph");
            require(tree.value[u] == g.edges[tree.edge[u]].w, "merge weight matches logical edge");
            auto e = g.edges[tree.edge[u]];
            require((binary_search(leaves[a].begin(), leaves[a].end(), e.u) && binary_search(leaves[b].begin(), leaves[b].end(), e.v)) ||
                (binary_search(leaves[a].begin(), leaves[a].end(), e.v) && binary_search(leaves[b].begin(), leaves[b].end(), e.u)),
                "merge edge joins the two child components");
            chosen.edges.push_back(tree.edge[u]); chosen.weight += lll(tree.value[u]);}
        int p = tree.parent[u];
        require(p == -1 || (u < p && p < count), "parent ID increases or root");
        if (p == -1) { ++roots; require(tree.root[u] == u && tree.depth[u] == 0, "root identity/depth"); }
        else { require(tree.root[u] == tree.root[p] && tree.depth[u] == tree.depth[p] + 1, "root propagation/depth"); }
        for (int j = 0; j < int(tree.up.size()); ++j) {
            int a = u;
            for (int step = 0; step < (1 << j) && tree.parent[a] != -1; ++step) { a = tree.parent[a]; }
            require(tree.up[j][u] == a, "binary ancestor table against parent chain");}}
    require(roots == oracle.components, "one reconstruction root per component"); forest(g, chosen, oracle, maximum);
    vector<int> order = tree.leaves; sort(order.begin(), order.end());
    vector<int> identity(g.n); iota(identity.begin(), identity.end(), 0);
    require(order == identity, "leaf order is a permutation of the vertices");
    for (int u = 0; u < count; ++u) {
        auto [l, r] = tree.leafRange(u);
        require(0 <= l && l <= r && r <= g.n, "leaf range bounds");
        vector<int> got(tree.leaves.begin() + l, tree.leaves.begin() + r); sort(got.begin(), got.end());
        require(got == leaves[u], "leaf range holds exactly the subtree leaves");}
    for (int u = 0; u < count; ++u) { for (int v = 0; v < count; ++v) {
        set<int> ancestors;
        for (int a = u; a != -1; a = tree.parent[a]) { ancestors.insert(a); }
        int want = v;
        while (want != -1 && !ancestors.count(want)) { want = tree.parent[want]; }
        require(tree.lca(u, v) == want, "all-node LCA against ancestor-chain intersection");}}
    Reach has; auto value = bottlenecks(g, maximum, has);
    for (int u = 0; u < g.n; ++u) { for (int v = 0; v < g.n; ++v) {
        auto result = tree.bottleneck(u, v);
        require(result.connected == has[u][v] && result.empty == (u == v), "bottleneck disconnected/empty status");
        if (!result.connected || result.empty) { require(result.edge == -1, "no edge witness for disconnected/empty"); continue; }
        require(result.value == value[u][v], "bottleneck vs independent Floyd", std::to_string(value[u][v]), std::to_string(result.value));
        require(0 <= result.edge && result.edge < int(g.edges.size()) && g.edges[result.edge].w == result.value,
            "bottleneck original-edge weight witness");
        int node = tree.lca(u, v); auto [a, b] = tree.child[node];
        require(binary_search(leaves[a].begin(), leaves[a].end(), u) != binary_search(leaves[a].begin(), leaves[a].end(), v),
            "bottleneck merge separates queried vertices");}}
    vector<lng> thresholds{std::numeric_limits<lng>::min(), -1, 0, 1, std::numeric_limits<lng>::max()};
    vector<int> ids(g.edges.size()); iota(ids.begin(), ids.end(), 0);
    for (auto e : g.edges) {
        thresholds.push_back(e.w);
        if (e.w != std::numeric_limits<lng>::min()) { thresholds.push_back(e.w - 1); }
        if (e.w != std::numeric_limits<lng>::max()) { thresholds.push_back(e.w + 1); }}
    sort(thresholds.begin(), thresholds.end()); thresholds.erase(unique(thresholds.begin(), thresholds.end()), thresholds.end());
    for (lng threshold : thresholds) {
        Reach r = reachability(g, ids, true, threshold, maximum);
        for (int u = 0; u < g.n; ++u) {
            vector<int> want;
            for (int v = 0; v < g.n; ++v) { if (r[u][v]) { want.push_back(v); } }
            int node = tree.componentAt(u, threshold);
            require(0 <= node && node < count && leaves[node] == want, "inclusive threshold component against transitive closure");}}}
void verify(const Graph &g) {
    context = "n=" + std::to_string(g.n) + " edges=";
    for (auto e : g.edges) {
        context += "(" + std::to_string(e.u) + "," + std::to_string(e.v) + "," + std::to_string(e.w) + ")";}
    Oracle oracle = brute(g); CsrGraph csr(g); DenseGraph dense(g), heavy(g, true);
    for (auto result : {kruskal(g), primSparse(g), primDense(dense), boruvka(g), kruskal(csr), primSparse(csr), boruvka(csr)}) {
        forest(g, result, oracle);}
    for (auto result : {kruskal(g, true), primSparse(g, true), primDense(heavy), boruvka(g, true), kruskal(csr, true),
                        primSparse(csr, true), boruvka(csr, true)}) {
        forest(g, result, oracle, true);}
    reconstruction(g, oracle, false); reconstruction(g, oracle, true);}
void exhaustive(const string &mode) {
    int bound = mode == "quick" ? 3 : 4, cases = 0;
    for (int n = 0; n <= bound; ++n) {
        vector<pair<int, int>> slots;
        for (int u = 0; u < n; ++u) { for (int v = u; v < n; ++v) { slots.push_back({u, v}); }}
        for (uint mask = 0; mask < (1U << slots.size()); ++mask) {
            Graph g(n);
            for (int i = 0; i < int(slots.size()); ++i) {
                if (mask >> i & 1U) { g.addEdge(slots[i].first, slots[i].second, (5 * i + int(mask % 11)) % 7 - 3); }}
            verify(g); ++cases;}}
    std::cout << "PASS exhaustive weighted looped graphs/forest subsets/union-tree oracles cases=" << cases << '\n';}
void fixtures() {
    lng low = std::numeric_limits<lng>::min(), high = std::numeric_limits<lng>::max();
    for (lng weight : {low, lng(-1), lng(0), lng(1), high}) {
        Graph g(5); g.addEdge(0, 1, weight); g.addEdge(1, 2, weight); g.addEdge(0, 2, weight);
        g.addEdge(0, 1, weight); g.addEdge(3, 3, low); g.addEdge(4, 4, high); verify(g);}
    Graph mixed(4); mixed.addEdge(0, 1, low); mixed.addEdge(1, 2, high); mixed.addEdge(2, 3, high);
    mixed.addEdge(0, 3, low); mixed.addEdge(1, 1, low); mixed.addEdge(0, 1, high); verify(mixed);
    KruskalReconstruction tree(mixed); auto copied = tree; auto moved = std::move(copied);
    context = "union-tree copy/move/owning snapshot";
    require(moved.bottleneck(0, 2).value == tree.bottleneck(0, 2).value && moved.child == tree.child,
        "copy/move preserves queries and nodes");
    auto before = tree.bottleneck(1, 2); mixed.addEdge(1, 2, low);
    require(tree.bottleneck(1, 2).value == before.value, "reconstruction owns snapshot after graph insertion");
    Graph tied(4);
    for (auto [u, v] : vector<pair<int, int>>{{2, 3}, {0, 1}, {1, 2}, {0, 3}, {0, 1}, {2, 2}}) {
        tied.addEdge(u, v, 0);}
    context = "all-zero cycle/parallel/loop tie fixture edges=(2,3),(0,1),(1,2),(0,3),(0,1),(2,2)";
    for (auto result : {kruskal(tied), primSparse(tied), primDense(DenseGraph(tied)), boruvka(tied), kruskal(tied, true),
                        primSparse(tied, true), primDense(DenseGraph(tied, true)), boruvka(tied, true)}) {
        sort(result.edges.begin(), result.edges.end());
        require(result.edges == vector<int>({0, 1, 2}), "canonical edge-ID tie breaking across all minimum/maximum engines");}
    for (bool maximum : {false, true}) {
        KruskalReconstruction tied_tree(tied, maximum);
        vector<int> selected(tied_tree.edge.begin() + tied.n, tied_tree.edge.end());
        require(selected == vector<int>({0, 1, 2}), "ascending/descending reconstruction canonical tie sequence");}
    std::cout << "PASS signed extrema/exact wide sums/equal-weight parallel edges/isolates/loops/copy/move\n";}
void randomCases(const string &mode) {
    int count = mode == "quick" ? 40 : mode == "full" ? 400 : 2500; std::mt19937_64 rng(seed);
    for (int test = 0; test < count; ++test) {
        int n = int(rng() % 8), m = n ? int(rng() % 12) : 0; Graph g(n);
        for (int i = 0; i < m; ++i) {
            lng w = lng(rng() % 13) - 6;
            if (test % 5 == 0 && i % 3 == 0) { w = (i & 1) ? std::numeric_limits<lng>::min() : std::numeric_limits<lng>::max(); }
            g.addEdge(int(rng() % n), int(rng() % n), w);}
        verify(g);}
    std::cout << "PASS random multigraph brute forests/minimax/maximin/thresholds cases=" << count << '\n';}
void large(const string &mode) {
    int n = mode == "quick" ? 1000 : mode == "full" ? 100000 : 300000; Graph g(n);
    for (int i = 1; i < n; ++i) { g.addEdge(i - 1, i, i - n / 2); }
    context = "long increasing-weight chain n=" + std::to_string(n);
    lll expected = lll(n - 1) * n / 2 - lll(n - 1) * (n / 2);
    for (auto result : {kruskal(g), primSparse(g), boruvka(g), kruskal(g, true), primSparse(g, true), boruvka(g, true)}) {
        require(result.components == 1 && int(result.edges.size()) == n - 1 && result.weight == expected,
            "large sparse forest exact total");}
    for (bool maximum : {false, true}) {
        KruskalReconstruction tree(g, maximum); auto result = tree.bottleneck(0, n - 1);
        require(result.connected && !result.empty && result.value == (maximum ? 1 - n / 2 : n - 1 - n / 2),
            "long union-tree bottleneck without recursion");
        int node = tree.componentAt(maximum ? n - 1 : 0, 0);
        require(tree.leaf_count[node] == (maximum ? n - n / 2 + 1 : n / 2 + 1), "long chain inclusive threshold");}
    for (int i = 2; i < n; i += 2) { g.addEdge(i - 2, i, 0); }
    lll lighter = expected, heavier = expected;
    for (int i = 2; i < n; i += 2) {
        lighter += min(lll(0), -lll(i - n / 2)); heavier += max(lll(0), -lll(i - 1 - n / 2));}
    for (auto [result, want] : vector<pair<SpanningForest, lll>>{{kruskal(g), lighter}, {primSparse(g), lighter}, {boruvka(g), lighter},
            {kruskal(g, true), heavier}, {primSparse(g, true), heavier}, {boruvka(g, true), heavier}}) {
        require(result.components == 1 && result.weight == want, "large triangle-chain minimum/maximum totals", show(want), show(result.weight));}
    std::cout << "PASS sparse minimum/maximum algorithms, triangle chain and deep reconstruction forest n=" << n << '\n';}
void invalid(const string &probe) {
    Graph directed(2, true); directed.addEdge(0, 1);
    if (probe == "directed-kruskal") { (void)kruskal(directed); }
    if (probe == "directed-prim-sparse") { (void)primSparse(directed); }
    if (probe == "directed-prim-dense") { (void)primDense(DenseGraph(directed)); }
    if (probe == "directed-boruvka") { (void)boruvka(directed); }
    if (probe == "directed-reconstruction") { KruskalReconstruction bad(directed); }
    if (probe == "too-many-nodes") {
        struct OversizedGraph { int n = INT_MAX; bool directed = false; vector<GraphEdge> edges; } g;
        KruskalReconstruction bad(g);}
    KruskalReconstruction tree(Graph(2));
    if (probe == "bottleneck-negative") { (void)tree.bottleneck(-1, 0); }
    if (probe == "bottleneck-end") { (void)tree.bottleneck(0, 2); }
    if (probe == "lca-negative") { (void)tree.lca(-1, 0); }
    if (probe == "lca-end") { (void)tree.lca(0, 2); }
    if (probe == "threshold-negative") { (void)tree.componentAt(-1, 0); }
    if (probe == "threshold-end") { (void)tree.componentAt(2, 0); }
    if (probe == "leaf-range-negative") { (void)tree.leafRange(-1); }
    if (probe == "leaf-range-end") { (void)tree.leafRange(2); }
    throw std::runtime_error("unknown or unasserted precondition=" + probe);}
int main(int argc, char **argv) {
    string mode = "full";
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); }}
        fixtures(); exhaustive(mode); randomCases(mode); large(mode);
        std::cout << "PASS MST checks=" << checks << " seed=" << seed << '\n'; return 0;} catch (const std::exception &error) {
        std::cerr << "FAIL MST seed=" << seed << " mode=" << mode << ' ' << error.what() << '\n'; return 1;}}
