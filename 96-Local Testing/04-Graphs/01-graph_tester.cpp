#include "../../04-Graphs/01-graph.hpp"
#include "../../02-Data Structures/07-ordered_set.hpp"

static ulng seed = 20260927;
static lng checks = 0;
static string context;
using Edges = vector<tuple<int, int, lng>>;

void require(bool ok, const string &operation, const string &want = "true", const string &got = "false") {
    ++checks; if (ok) { return; }
    throw std::runtime_error(context + " operation=" + operation + " expected=" + want + " actual=" + got);}
void describe(int n, bool directed, const Edges &edges) {
    context = "n=" + std::to_string(n) + " directed=" + std::to_string(directed) + " edges=";
    for (auto [u, v, w] : edges) {
        context += "(" + std::to_string(u) + "," + std::to_string(v) + "," + std::to_string(w) + ")";}}
template<class G>
void storage(const G &g, int n, bool directed, const Edges &edges) {
    require(g.n == n && g.directed == directed, "vertex count/direction");
    int m = int(edges.size()), multiple = directed ? 1 : 2;
    require(int(g.edges.size()) == m && int(g.arcs.size()) == multiple * m, "logical edge and arc counts");
    vector<int> seen(multiple * m), per_edge(m);
    for (int id = 0; id < m; ++id) {
        auto [u, v, w] = edges[id]; auto e = g.edges[id];
        require(e.u == u && e.v == v && e.w == w, "logical edge insertion identity=" + std::to_string(id));}
    for (int u = 0; u < n; ++u) {
        int previous = -1;
        for (int a : g[u]) {
            require(0 <= a && a < int(g.arcs.size()), "adjacency arc bounds");
            require(a > previous, "adjacency insertion order"); previous = a;
            ++seen[a]; auto e = g.arcs[a];
            require(e.from == u && 0 <= e.id && e.id < m, "arc origin/logical ID");
            auto [x, y, w] = edges[e.id]; ++per_edge[e.id];
            require(e.w == w && ((e.from == x && e.to == y) || (!directed && e.from == y && e.to == x)),
                "arc endpoint/weight fidelity");
            if (directed) { require(e.rev == -1, "directed arc has no reciprocal"); }
            else {
                require(0 <= e.rev && e.rev < int(g.arcs.size()) && e.rev != a, "distinct reverse including loop");
                auto r = g.arcs[e.rev];
                require(r.rev == a && r.id == e.id && r.from == e.to && r.to == e.from && r.w == e.w,
                    "reciprocal arc involution and edge ID");}}}
    for (int count : seen) { require(count == 1, "each arc occurs exactly once in adjacency"); }
    for (int count : per_edge) { require(count == multiple, "each logical edge has required multiplicity"); }}
Edges listed(const Graph &g) {
    Edges res;
    for (auto e : g.edges) { res.emplace_back(e.u, e.v, e.w); }
    return res;}
// Independent oracles: incidence counting, filtered edge lists, relabelled endpoints and a pair map.
template<class G>
void lineOracle(const G &g, bool directed, const Edges &edges) {
    int m = int(edges.size()); multiset<pair<int, int>> want, got;
    for (int i = 0; i < m; ++i) {
        auto [ui, vi, wi] = edges[i];
        for (int j = 0; j < m; ++j) {
            auto [uj, vj, wj] = edges[j];
            if (directed && vi == uj) { want.insert({i, j}); }
            if (directed || i >= j) { continue; }
            set<int> a{ui, vi}, b{uj, vj};
            for (int x : a) { if (b.count(x)) { want.insert({i, j}); }}}}
    auto line = lineGraph(g);
    require(line.n == m && line.directed == directed, "line graph vertex per edge");
    storage(line, m, directed, listed(line));
    for (auto e : line.edges) {
        require(e.w == 1, "line graph unit weights");
        got.insert(directed ? pair(e.u, e.v) : pair(min(e.u, e.v), max(e.u, e.v)));}
    require(got == want, "line graph incidence multiset", std::to_string(want.size()), std::to_string(got.size()));}
template<class G>
void inducedOracle(const G &g, int n, bool directed, const Edges &edges, const vector<int> &vs) {
    vector<int> pos(n, -1), want;
    for (int i = 0; i < int(vs.size()); ++i) { pos[vs[i]] = i; }
    for (int id = 0; id < int(edges.size()); ++id) {
        if (pos[get<0>(edges[id])] != -1 && pos[get<1>(edges[id])] != -1) { want.push_back(id); }}
    auto sub = inducedSubgraph(g, vs); auto ids = sub.edge; sort(ids.begin(), ids.end());
    require(sub.graph.n == int(vs.size()) && sub.graph.directed == directed, "induced vertex count/direction");
    require(ids == want && sub.edge.size() == sub.graph.edges.size(), "induced edge set equals filtered source edges");
    storage(sub.graph, sub.graph.n, directed, listed(sub.graph));
    for (int i = 0; i < int(sub.edge.size()); ++i) {
        auto [u, v, w] = edges[sub.edge[i]]; auto e = sub.graph.edges[i];
        require(e.u == pos[u] && e.v == pos[v] && e.w == w, "induced edge keeps orientation/weight under relabelling");}}
template<class G>
void simplifyOracle(const G &g, int n, bool directed, const Edges &edges) {
    map<pair<int, int>, pair<lng, int>> best;
    for (int id = 0; id < int(edges.size()); ++id) {
        auto [u, v, w] = edges[id];
        if (u == v) { continue; }
        auto key = directed ? pair(u, v) : pair(min(u, v), max(u, v));
        if (!best.count(key) || pair(w, id) < best[key]) { best[key] = {w, id}; }}
    vector<int> want;
    for (auto [key, value] : best) { want.push_back(value.second); }
    sort(want.begin(), want.end());
    auto sub = simplify(g);
    require(sub.graph.n == n && sub.graph.directed == directed && sub.edge == want, "simplify keeps lightest earliest edge per pair");
    Edges kept;
    for (int id : want) { kept.push_back(edges[id]); }
    storage(sub.graph, n, directed, kept);}
template<class G>
void utilities(const G &g, int n, bool directed, const Edges &edges, std::mt19937_64 &rng) {
    lineOracle(g, directed, edges); simplifyOracle(g, n, directed, edges);
    vector<int> all(n); iota(all.begin(), all.end(), 0);
    inducedOracle(g, n, directed, edges, {}); inducedOracle(g, n, directed, edges, all);
    for (int round = 0; round < 3; ++round) {
        vector<int> vs;
        for (int u = 0; u < n; ++u) { if (rng() & 1) { vs.push_back(u); }}
        std::shuffle(vs.begin(), vs.end(), rng); inducedOracle(g, n, directed, edges, vs);}
    // Labels: identity, all merged into one vertex, and random with spare unused labels.
    for (int round = 0; round < 3; ++round) {
        int k = round == 0 ? n : round == 1 ? (n ? 1 : 0) : n + 2;
        vector<int> label(n);
        for (int u = 0; u < n; ++u) { label[u] = round == 0 ? u : round == 1 ? 0 : int(rng() % uint(k)); }
        auto c = contract(g, label, k); Edges want;
        for (auto [u, v, w] : edges) { want.emplace_back(label[u], label[v], w); }
        require(c.n == k, "contract vertex count"); storage(c, k, directed, want);
        simplifyOracle(c, k, directed, want);}}
// Builders: shuffled lists; directed keeps list order, undirected must equal the sorted (min, max, w) edge multiset.
void builders(int n, bool directed, const Edges &edges, std::mt19937_64 &rng) {
    vector<vector<pair<int, lng>>> adj(n);
    for (auto [u, v, w] : edges) {
        adj[u].push_back({v, w});
        if (!directed) { adj[v].push_back({u, w}); }}
    for (auto &list : adj) { std::shuffle(list.begin(), list.end(), rng); }
    vector<vector<int>> unit(n); Edges want, ones;
    for (int u = 0; u < n; ++u) {
        for (auto [v, w] : adj[u]) {
            unit[u].push_back(v);
            if (directed) { want.emplace_back(u, v, w); }}}
    if (!directed) {
        for (auto [u, v, w] : edges) { want.emplace_back(min(u, v), max(u, v), w); }
        sort(want.begin(), want.end());}
    for (auto [u, v, w] : want) { ones.emplace_back(u, v, 1); }
    storage(Graph::fromAdjacency(adj, directed), n, directed, want);
    storage(Graph::fromAdjacency(unit, directed), n, directed, ones);}
template<class G, class H>
void identical(const G &g, const H &h) {
    require(g.n == h.n && g.directed == h.directed && g.arcs.size() == h.arcs.size(), "snapshot shape");
    for (int a = 0; a < int(g.arcs.size()); ++a) {
        auto e = g.arcs[a], f = h.arcs[a];
        require(e.from == f.from && e.to == f.to && e.id == f.id && e.rev == f.rev && e.w == f.w,
            "snapshot exact arc identity");}
    for (int u = 0; u < g.n; ++u) {
        vector<int> a(g[u].begin(), g[u].end()), b(h[u].begin(), h[u].end());
        require(a == b, "snapshot exact adjacency order");}}
void verify(int n, bool directed, const Edges &edges) {
    describe(n, directed, edges); Graph g(n, directed);
    for (int i = 0; i < int(edges.size()); ++i) {
        auto [u, v, w] = edges[i]; require(g.addEdge(u, v, w) == i, "returned insertion edge ID");}
    storage(g, n, directed, edges); CsrGraph csr(g); storage(csr, n, directed, edges); identical(g, csr);
    auto back = edges;
    if (directed) { for (auto &[u, v, w] : back) { swap(u, v); } }
    auto reversed = g.reverse(); auto reversed_csr = csr.reverse();
    storage(reversed, n, directed, back); storage(reversed_csr, n, directed, back); identical(reversed, reversed_csr);
    auto twice = reversed.reverse(); auto twice_csr = reversed_csr.reverse();
    storage(twice, n, directed, edges); identical(g, twice); identical(g, twice_csr);
    DenseGraph dense(g), heavy(g, true); storage(dense.graph, n, directed, edges); identical(g, dense.graph);
    require(int(dense.best.size()) == n && !dense.maximum && heavy.maximum, "dense row count/view flag");
    for (int u = 0; u < n; ++u) {
        require(int(dense.best[u].size()) == n && int(heavy.best[u].size()) == n, "dense column count");
        for (int v = 0; v < n; ++v) {
            int low = -1, high = -1;
            for (int a : g[u]) {
                if (g.arcs[a].to != v) { continue; }
                if (low == -1 || g.arcs[a].w < g.arcs[low].w) { low = a; }
                if (high == -1 || g.arcs[a].w > g.arcs[high].w) { high = a; }}
            require(dense.best[u][v] == low, "dense minimum/earliest tie/absent diagonal", std::to_string(low),
                std::to_string(dense.best[u][v]));
            require(heavy.best[u][v] == high, "dense maximum/earliest tie/absent diagonal", std::to_string(high),
                std::to_string(heavy.best[u][v]));}}
    static std::mt19937_64 pick(seed); utilities(g, n, directed, edges, pick); utilities(csr, n, directed, edges, pick);
    builders(n, directed, edges, pick);
    Graph copy = g, moved = std::move(copy); storage(moved, n, directed, edges);
    CsrGraph csr_copy = csr, csr_moved = std::move(csr_copy); identical(g, csr_moved);
    if (n) {
        g.addEdge(n - 1, 0); storage(csr, n, directed, edges); storage(dense.graph, n, directed, edges);
        storage(moved, n, directed, edges); require(g.edges.back().w == 1, "default edge weight");}}
void exhaustive(const string &mode) {
    int bound = mode == "quick" ? 2 : 3; int cases = 0;
    for (int n = 0; n <= bound; ++n) { for (bool directed : {false, true}) {
        vector<pair<int, int>> slots;
        for (int u = 0; u < n; ++u) { for (int v = directed ? 0 : u; v < n; ++v) { slots.push_back({u, v}); }}
        for (uint mask = 0; mask < (1U << slots.size()); ++mask) {
            Edges edges;
            for (int i = 0; i < int(slots.size()); ++i) {
                if (mask >> i & 1U) { edges.emplace_back(slots[i].first, slots[i].second, i % 3 - 1); }}
            verify(n, directed, edges); ++cases;}}}
    std::cout << "PASS exhaustive storage/reverse/CSR/dense graphs=" << cases << '\n';}
struct OrderedLabel {
    int key;
    friend bool operator<(OrderedLabel a, OrderedLabel b) { return a.key < b.key; }
};
void adapters() {
    context = "input and label adapters";
    Edges weighted{{0, 0, std::numeric_limits<lng>::min()}, {0, 2, std::numeric_limits<lng>::max()}, {0, 2, -7}, {2, 1, 0}};
    for (bool directed : {false, true}) { for (bool weights : {false, true}) { for (int base : {0, 1}) {
        std::stringstream in; Edges want = weighted;
        for (auto &[u, v, w] : want) {
            in << u + base << ' ' << v + base;
            if (weights) { in << ' ' << w; } else { w = 1; }
            in << '\n';}
        in << "12345"; auto g = Graph::read(in, 3, int(want.size()), directed, weights, base);
        storage(g, 3, directed, want); int tail = 0; in >> tail;
        require(tail == 12345, "input adapter consumes exactly m records");}}}
    std::istringstream empty; auto g = Graph::read(empty, 0, 0); storage(g, 0, false, {});
    Graph default_graph; storage(default_graph, 0, false, {});
    // Vertex labels on CoordinateCompression; absent labels give -1.
    CoordinateCompression<lng> labels({7, -3, 7, std::numeric_limits<lng>::min(), std::numeric_limits<lng>::max(), 0});
    vector<lng> expected{std::numeric_limits<lng>::min(), -3, 0, 7, std::numeric_limits<lng>::max()};
    require(labels.v == expected, "sorted deduplicated integer labels");
    for (int i = 0; i < int(expected.size()); ++i) {
        require(labels.index(expected[i]) == i && labels.decode(i) == expected[i], "integer label roundtrip");}
    require(labels.index(-4) == -1 && labels.index(8) == -1, "absent integer label");
    CoordinateCompression<string> names({"z", "", "a", "a"});
    require(names.v == vector<string>({"", "a", "z"}) && names.index("z") == 2 && names.decode(0).empty(),
        "string labels including empty label");
    CoordinateCompression<OrderedLabel> order({{9}, {3}, {9}, {-1}});
    require(order.v.size() == 3 && order.index({3}) == 1 && order.decode(2).key == 9 && order.index({4}) == -1,
        "strict ordering only, no equality operator required");
    CoordinateCompression<int> zero(vector<int>{}); require(zero.v.empty() && zero.index(0) == -1, "empty label set");
    auto copied = names; auto moved = std::move(copied);
    require(moved.v == names.v && moved.index("a") == 1, "label copy/move");
    // Builder fixtures: defaults, a loop listed twice, parallel edges listed in different orders, empty inputs.
    vector<vector<pair<int, lng>>> lists{{{1, 7}, {0, -2}, {1, 5}, {0, -2}, {2, 0}}, {{0, 5}, {0, 7}}, {{0, 0}}};
    storage(Graph::fromAdjacency(lists), 3, false, {{0, 0, -2}, {0, 1, 5}, {0, 1, 7}, {0, 2, 0}});
    storage(Graph::fromAdjacency(lists, true), 3, true, {{0, 1, 7}, {0, 0, -2}, {0, 1, 5}, {0, 0, -2}, {0, 2, 0},
        {1, 0, 5}, {1, 0, 7}, {2, 0, 0}});
    storage(Graph::fromAdjacency(vector<vector<int>>{{1, 1}, {0, 0, 1, 1}}), 2, false, {{0, 1, 1}, {0, 1, 1}, {1, 1, 1}});
    storage(Graph::fromAdjacency(vector<vector<int>>{}), 0, false, {});
    vector<vector<lng>> matrix{{0, 4, 0}, {4, -1, std::numeric_limits<lng>::min()}, {0, std::numeric_limits<lng>::min(), 0}};
    storage(Graph::fromMatrix(matrix), 3, true, {{0, 1, 4}, {1, 0, 4}, {1, 1, -1}, {1, 2, std::numeric_limits<lng>::min()},
        {2, 1, std::numeric_limits<lng>::min()}});
    storage(Graph::fromMatrix(matrix, false), 3, false, {{0, 1, 4}, {1, 1, -1}, {1, 2, std::numeric_limits<lng>::min()}});
    storage(Graph::fromMatrix(vector<vector<lng>>{}), 0, true, {});
    verify(3, false, {{0, 0, -5}, {0, 0, -5}, {0, 1, 3}, {0, 1, 3}, {1, 0, -9},
        {1, 2, std::numeric_limits<lng>::min()}, {2, 2, std::numeric_limits<lng>::max()}});
    verify(3, true, weighted);
    std::cout << "PASS defaults/copy/move/input/labels/builders/extreme weights/multiedges/loops\n";}
void randomCases(const string &mode) {
    std::mt19937_64 rng(seed); int count = mode == "quick" ? 70 : mode == "full" ? 500 : 4000;
    for (int test = 0; test < count; ++test) {
        int n = int(rng() % 25), m = n ? int(rng() % 120) : 0; Edges edges;
        for (int i = 0; i < m; ++i) {
            lng w = lng(rng() % 15) - 7;
            if (i % 19 == 0) { w = std::numeric_limits<lng>::min(); }
            if (i % 23 == 0) { w = std::numeric_limits<lng>::max(); }
            edges.emplace_back(int(rng() % n), int(rng() % n), w);}
        verify(n, bool(rng() & 1), edges);}
    std::cout << "PASS deterministic multigraph storage cases=" << count << '\n';}
void builderCases(const string &mode) {
    std::mt19937_64 rng(seed + 1); int count = mode == "quick" ? 100 : mode == "full" ? 1000 : 8000;
    for (int test = 0; test < count; ++test) {
        // Full-range weights that differ in one random byte exercise every radix pass and its skip.
        lng x = lng(rng()); int b = int(rng() % 8);
        vector<lng> pool{x, lng(ulng(x) ^ (rng() % 255 + 1) << (8 * b)), std::numeric_limits<lng>::min(),
            std::numeric_limits<lng>::max(), 0};
        int n = 1 + int(rng() % 8), m = int(rng() % 40); bool directed = bool(rng() & 1); Edges edges;
        for (int i = 0; i < m; ++i) { edges.emplace_back(int(rng() % n), int(rng() % n), pool[rng() % pool.size()]); }
        describe(n, directed, edges); builders(n, directed, edges, rng);
        vector<vector<lng>> w(n, vector<lng>(n));
        for (int u = 0; u < n; ++u) {
            for (int v = directed ? 0 : u; v < n; ++v) {
                w[u][v] = rng() % 3 ? 0 : pool[rng() % pool.size()];
                if (!directed) { w[v][u] = w[u][v]; }}}
        Edges want; vector<vector<pair<int, lng>>> adj(n);
        for (int u = 0; u < n; ++u) {
            for (int v = directed ? 0 : u; v < n; ++v) {
                if (!w[u][v]) { continue; }
                want.emplace_back(u, v, w[u][v]); adj[u].push_back({v, w[u][v]});
                if (!directed) { adj[v].push_back({u, w[u][v]}); }}}
        auto g = Graph::fromMatrix(w, directed); storage(g, n, directed, want);
        identical(g, Graph::fromAdjacency(adj, directed));}
    std::cout << "PASS builders: shuffled full-range adjacency lists and matrices cases=" << count << '\n';}
void invalid(const string &probe) {
    Graph g(2); CsrGraph csr(g);
    using Lists = vector<vector<int>>; using Weighted = vector<vector<pair<int, lng>>>;
    if (probe == "negative-size") { Graph bad(-1); }
    if (probe == "add-negative") { g.addEdge(-1, 0); }
    if (probe == "add-end") { g.addEdge(0, 2); }
    if (probe == "row-negative") { (void)g[-1]; }
    if (probe == "row-end") { (void)g[2]; }
    if (probe == "csr-row-negative") { (void)csr[-1]; }
    if (probe == "csr-row-end") { (void)csr[2]; }
    if (probe == "adjacency-negative") { (void)Graph::fromAdjacency(Lists{{-1}}, true); }
    if (probe == "adjacency-end") { (void)Graph::fromAdjacency(Lists{{1}}, true); }
    if (probe == "adjacency-asymmetric") { (void)Graph::fromAdjacency(Lists{{1}, {}}); }
    if (probe == "adjacency-multiplicity") { (void)Graph::fromAdjacency(Lists{{1, 1}, {0}}); }
    if (probe == "adjacency-loop-once") { (void)Graph::fromAdjacency(Lists{{0}}); }
    if (probe == "adjacency-weight") { (void)Graph::fromAdjacency(Weighted{{{1, 2}}, {{0, 3}}}); }
    if (probe == "adjacency-loop-weight") { (void)Graph::fromAdjacency(Weighted{{{0, 5}, {0, 7}}}); }
    if (probe == "matrix-ragged") { (void)Graph::fromMatrix({{0, 1}, {0}}); }
    if (probe == "matrix-asymmetric") { (void)Graph::fromMatrix({{0, 1}, {0, 0}}, false); }
    if (probe == "induced-negative") { (void)inducedSubgraph(g, {-1}); }
    if (probe == "induced-end") { (void)inducedSubgraph(csr, {2}); }
    if (probe == "induced-duplicate") { (void)inducedSubgraph(g, {1, 0, 1}); }
    if (probe == "contract-size") { (void)contract(g, {0}, 1); }
    if (probe == "contract-count") { (void)contract(Graph(0), {}, -1); }
    if (probe == "contract-negative") { (void)contract(g, {0, -1}, 2); }
    if (probe == "contract-end") { (void)contract(csr, {0, 2}, 2); }
    std::istringstream empty;
    if (probe == "input-negative-size") { (void)Graph::read(empty, -1, 0); }
    if (probe == "input-negative-count") { (void)Graph::read(empty, 2, -1); }
    if (probe == "input-base") { (void)Graph::read(empty, 2, 0, false, false, 2); }
    map<string, string> input{{"input-truncated", "1"}, {"input-weight-truncated", "1 2"},
        {"input-low", "0 2"}, {"input-high", "1 3"}, {"input-extreme-low", "-9223372036854775808 1"},
        {"input-extreme-high", "1 9223372036854775807"}};
    if (input.count(probe)) {
        std::istringstream in(input[probe]); (void)Graph::read(in, 2, 1, false, probe == "input-weight-truncated");}
    throw std::runtime_error("unknown or unasserted precondition=" + probe);}
int main(int argc, char **argv) {
    string mode = "full";
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); }}
        adapters(); exhaustive(mode); randomCases(mode); builderCases(mode);
        std::cout << "PASS graph checks=" << checks << " seed=" << seed << '\n'; return 0;} catch (const std::exception &error) {
        std::cerr << "FAIL graph seed=" << seed << " mode=" << mode << ' ' << error.what() << '\n'; return 1;}}
