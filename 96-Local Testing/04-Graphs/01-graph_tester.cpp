#include "../../04-Graphs/01-graph.hpp"

static ulng seed = 20260927;
static lng checks = 0;
static string context;
using Edges = vector<tuple<int, int, lng>>;

void require(bool ok, const string &operation, const string &want = "true", const string &got = "false") {
    ++checks; if (ok) { return; }
    throw std::runtime_error(context + " operation=" + operation + " expected=" + want + " actual=" + got); }
void describe(int n, bool directed, const Edges &edges) {
    context = "n=" + std::to_string(n) + " directed=" + std::to_string(directed) + " edges=";
    for (auto [u, v, w] : edges) {
        context += "(" + std::to_string(u) + "," + std::to_string(v) + "," + std::to_string(w) + ")"; }
}
template<class G>
void storage(const G &g, int n, bool directed, const Edges &edges) {
    require(g.n == n && g.directed == directed, "vertex count/direction");
    int m = int(edges.size()), multiple = directed ? 1 : 2;
    require(int(g.edges.size()) == m && int(g.arcs.size()) == multiple * m, "logical edge and arc counts");
    vector<int> seen(multiple * m), per_edge(m);
    for (int id = 0; id < m; ++id) {
        auto [u, v, w] = edges[id]; auto e = g.edges[id];
        require(e.u == u && e.v == v && e.w == w, "logical edge insertion identity=" + std::to_string(id)); }
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
                    "reciprocal arc involution and edge ID"); }} }
    for (int count : seen) { require(count == 1, "each arc occurs exactly once in adjacency"); }
    for (int count : per_edge) { require(count == multiple, "each logical edge has required multiplicity"); }
}
template<class G, class H>
void identical(const G &g, const H &h) {
    require(g.n == h.n && g.directed == h.directed && g.arcs.size() == h.arcs.size(), "snapshot shape");
    for (int a = 0; a < int(g.arcs.size()); ++a) {
        auto e = g.arcs[a], f = h.arcs[a];
        require(e.from == f.from && e.to == f.to && e.id == f.id && e.rev == f.rev && e.w == f.w,
            "snapshot exact arc identity"); }
    for (int u = 0; u < g.n; ++u) {
        vector<int> a(g[u].begin(), g[u].end()), b(h[u].begin(), h[u].end());
        require(a == b, "snapshot exact adjacency order"); }
}
void verify(int n, bool directed, const Edges &edges) {
    describe(n, directed, edges); Graph g(n, directed);
    for (int i = 0; i < int(edges.size()); ++i) {
        auto [u, v, w] = edges[i]; require(g.addEdge(u, v, w) == i, "returned insertion edge ID"); }
    storage(g, n, directed, edges); CsrGraph csr(g); storage(csr, n, directed, edges); identical(g, csr);
    auto back = edges;
    if (directed) { for (auto &[u, v, w] : back) { swap(u, v); } }
    auto reversed = g.reverse(); auto reversed_csr = csr.reverse();
    storage(reversed, n, directed, back); storage(reversed_csr, n, directed, back); identical(reversed, reversed_csr);
    auto twice = reversed.reverse(); auto twice_csr = reversed_csr.reverse();
    storage(twice, n, directed, edges); identical(g, twice); identical(g, twice_csr);
    DenseGraph dense(g); storage(dense.graph, n, directed, edges); identical(g, dense.graph);
    require(int(dense.best.size()) == n, "dense row count");
    for (int u = 0; u < n; ++u) {
        require(int(dense.best[u].size()) == n, "dense column count");
        for (int v = 0; v < n; ++v) {
            vector<pair<lng, int>> candidates;
            for (int a : g[u]) { if (g.arcs[a].to == v) { candidates.push_back({g.arcs[a].w, a}); } }
            sort(candidates.begin(), candidates.end());
            int want = candidates.empty() ? -1 : candidates.front().second;
            require(dense.best[u][v] == want, "dense minimum/earliest tie/absent diagonal", std::to_string(want),
                std::to_string(dense.best[u][v])); }}
    Graph copy = g, moved = std::move(copy); storage(moved, n, directed, edges);
    CsrGraph csr_copy = csr, csr_moved = std::move(csr_copy); identical(g, csr_moved);
    if (n) {
        g.addEdge(n - 1, 0); storage(csr, n, directed, edges); storage(dense.graph, n, directed, edges);
        storage(moved, n, directed, edges); require(g.edges.back().w == 1, "default edge weight"); }
}
void exhaustive(const string &mode) {
    int bound = mode == "quick" ? 2 : 3; int cases = 0;
    for (int n = 0; n <= bound; ++n) { for (bool directed : {false, true}) {
        vector<pair<int, int>> slots;
        for (int u = 0; u < n; ++u) { for (int v = directed ? 0 : u; v < n; ++v) { slots.push_back({u, v}); }}
        for (uint mask = 0; mask < (1U << slots.size()); ++mask) {
            Edges edges;
            for (int i = 0; i < int(slots.size()); ++i) {
                if (mask >> i & 1U) { edges.emplace_back(slots[i].first, slots[i].second, i % 3 - 1); } }
            verify(n, directed, edges); ++cases; }} }
    std::cout << "PASS exhaustive storage/reverse/CSR/dense graphs=" << cases << '\n';
}
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
            in << '\n'; }
        in << "12345"; auto g = Graph::read(in, 3, int(want.size()), directed, weights, base);
        storage(g, 3, directed, want); int tail = 0; in >> tail;
        require(tail == 12345, "input adapter consumes exactly m records"); }} }
    std::istringstream empty; auto g = Graph::read(empty, 0, 0); storage(g, 0, false, {});
    Graph default_graph; storage(default_graph, 0, false, {});
    GraphLabels<lng> labels({7, -3, 7, std::numeric_limits<lng>::min(), std::numeric_limits<lng>::max(), 0});
    vector<lng> expected{std::numeric_limits<lng>::min(), -3, 0, 7, std::numeric_limits<lng>::max()};
    require(labels.labels == expected, "sorted deduplicated integer labels");
    for (int i = 0; i < int(expected.size()); ++i) {
        require(labels.index(expected[i]) == i && labels.value(i) == expected[i], "integer label roundtrip"); }
    GraphLabels<string> names({"z", "", "a", "a"});
    require(names.labels == vector<string>({"", "a", "z"}) && names.index("z") == 2 && names.value(0).empty(),
        "string labels including empty label");
    GraphLabels<OrderedLabel> order({{9}, {3}, {9}, {-1}});
    require(order.labels.size() == 3 && order.index({3}) == 1 && order.value(2).key == 9,
        "strict ordering only, no equality operator required");
    GraphLabels<int> zero({}); require(zero.labels.empty(), "empty label set");
    auto copied = names; auto moved = std::move(copied);
    require(moved.labels == names.labels && moved.index("a") == 1, "label copy/move");
    verify(3, false, {{0, 0, -5}, {0, 0, -5}, {0, 1, 3}, {0, 1, 3}, {1, 0, -9},
        {1, 2, std::numeric_limits<lng>::min()}, {2, 2, std::numeric_limits<lng>::max()}});
    verify(3, true, weighted);
    std::cout << "PASS defaults/copy/move/input/labels/extreme weights/multiedges/loops\n";
}
void randomCases(const string &mode) {
    std::mt19937_64 rng(seed); int count = mode == "quick" ? 70 : mode == "full" ? 500 : 4000;
    for (int test = 0; test < count; ++test) {
        int n = int(rng() % 25), m = n ? int(rng() % 120) : 0; Edges edges;
        for (int i = 0; i < m; ++i) {
            lng w = lng(rng() % 15) - 7;
            if (i % 19 == 0) { w = std::numeric_limits<lng>::min(); }
            if (i % 23 == 0) { w = std::numeric_limits<lng>::max(); }
            edges.emplace_back(int(rng() % n), int(rng() % n), w); }
        verify(n, bool(rng() & 1), edges); }
    std::cout << "PASS deterministic multigraph storage cases=" << count << '\n';
}
void invalid(const string &probe) {
    Graph g(2); CsrGraph csr(g); GraphLabels<int> labels({1, 3});
    if (probe == "negative-size") { Graph bad(-1); }
    if (probe == "add-negative") { g.addEdge(-1, 0); }
    if (probe == "add-end") { g.addEdge(0, 2); }
    if (probe == "row-negative") { (void)g[-1]; }
    if (probe == "row-end") { (void)g[2]; }
    if (probe == "csr-row-negative") { (void)csr[-1]; }
    if (probe == "csr-row-end") { (void)csr[2]; }
    if (probe == "label-missing-low") { (void)labels.index(0); }
    if (probe == "label-missing-high") { (void)labels.index(4); }
    if (probe == "label-negative") { (void)labels.value(-1); }
    if (probe == "label-end") { (void)labels.value(2); }
    std::istringstream empty;
    if (probe == "input-negative-size") { (void)Graph::read(empty, -1, 0); }
    if (probe == "input-negative-count") { (void)Graph::read(empty, 2, -1); }
    if (probe == "input-base") { (void)Graph::read(empty, 2, 0, false, false, 2); }
    map<string, string> input{{"input-truncated", "1"}, {"input-weight-truncated", "1 2"},
        {"input-low", "0 2"}, {"input-high", "1 3"}, {"input-extreme-low", "-9223372036854775808 1"},
        {"input-extreme-high", "1 9223372036854775807"}};
    if (input.count(probe)) {
        std::istringstream in(input[probe]); (void)Graph::read(in, 2, 1, false, probe == "input-weight-truncated"); }
    throw std::runtime_error("unknown or unasserted precondition=" + probe);
}
int main(int argc, char **argv) {
    string mode = "full";
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); } }
        adapters(); exhaustive(mode); randomCases(mode);
        std::cout << "PASS graph checks=" << checks << " seed=" << seed << '\n'; return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL graph seed=" << seed << " mode=" << mode << ' ' << error.what() << '\n'; return 1; }
}
