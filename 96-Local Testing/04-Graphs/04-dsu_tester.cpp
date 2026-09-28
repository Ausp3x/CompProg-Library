#include "../../02-Data Structures/01-dsu.hpp"
#include "../../04-Graphs/04-dsu.hpp"

static_assert(std::is_same_v<decltype(graphComponents(std::declval<const Graph &>())), DSU>);
static_assert(std::is_same_v<decltype(graphComponents(std::declval<const CsrGraph &>())), DSU>);
static ulng seed = 20260927;
static lng checks = 0;
static string context;

void require(bool ok, const string &operation, const string &want = "true", const string &got = "false") {
    ++checks; if (ok) { return; }
    throw std::runtime_error(context + " operation=" + operation + " expected=" + want + " actual=" + got); }
void verify(const Graph &g) {
    context = "n=" + std::to_string(g.n) + " directed=" + std::to_string(g.directed) + " edges=";
    vector<vector<bool>> reachable(g.n, vector<bool>(g.n));
    for (int i = 0; i < g.n; ++i) { reachable[i][i] = true; }
    for (auto e : g.edges) {
        reachable[e.u][e.v] = reachable[e.v][e.u] = true;
        context += "(" + std::to_string(e.u) + "," + std::to_string(e.v) + ")"; }
    for (int k = 0; k < g.n; ++k) { for (int i = 0; i < g.n; ++i) { for (int j = 0; j < g.n; ++j) {
        reachable[i][j] = reachable[i][j] || (reachable[i][k] && reachable[k][j]); }}}
    vector<vector<int>> expected; vector<bool> assigned(g.n);
    for (int i = 0; i < g.n; ++i) {
        if (assigned[i]) { continue; }
        expected.emplace_back();
        for (int j = 0; j < g.n; ++j) { if (reachable[i][j]) { expected.back().push_back(j); assigned[j] = true; } }}
    for (auto dsu : {graphComponents(g), graphComponents(CsrGraph(g)), graphComponents(g.reverse())}) {
        require(dsu.n == g.n && dsu.count() == int(expected.size()), "canonical size/component count");
        require(dsu.groups() == expected, "canonical groups against transitive closure");
        for (int i = 0; i < g.n; ++i) {
            int count = 0;
            for (int j = 0; j < g.n; ++j) {
                require(dsu.isSameSet(i, j) == reachable[i][j], "weak connectivity pair=" + std::to_string(i) + "," + std::to_string(j));
                count += reachable[i][j]; }
            require(dsu.getSize(i) == count, "canonical component size");
            require(dsu.findSet(i) == dsu.findSet(dsu.findSet(i)), "canonical representative idempotence"); }
        auto copy = dsu;
        if (g.n) {
            bool merged = copy.uniteSets(0, g.n - 1);
            require(merged == !reachable[0][g.n - 1], "canonical union remains usable");
            require(copy.count() == dsu.count() - int(merged), "canonical union count update");
            require(dsu.groups() == expected, "adapter result has independent value semantics"); }}
}
void exhaustive(const string &mode) {
    int bound = mode == "quick" ? 3 : 4, cases = 0;
    for (int n = 0; n <= bound; ++n) { for (bool directed : {false, true}) {
        vector<pair<int, int>> slots;
        for (int u = 0; u < n; ++u) { for (int v = directed ? 0 : u; v < n; ++v) { slots.push_back({u, v}); }}
        for (uint mask = 0; mask < (1U << slots.size()); ++mask) {
            Graph g(n, directed);
            for (int i = 0; i < int(slots.size()); ++i) {
                if (mask >> i & 1U) { g.addEdge(slots[i].first, slots[i].second, i - 8); }}
            verify(g); ++cases; }} }
    std::cout << "PASS exhaustive graph DSU/transitive closure cases=" << cases << '\n';
}
void randomCases(const string &mode) {
    int count = mode == "quick" ? 50 : mode == "full" ? 500 : 4000; std::mt19937_64 rng(seed);
    for (int test = 0; test < count; ++test) {
        int n = int(rng() % 31), m = n ? int(rng() % 90) : 0; Graph g(n, bool(rng() & 1));
        for (int i = 0; i < m; ++i) {
            g.addEdge(int(rng() % n), int(rng() % n), (i & 1) ? std::numeric_limits<lng>::max() : std::numeric_limits<lng>::min()); }
        verify(g); }
    std::cout << "PASS multiedges/loops/empty/disconnected/extreme ignored weights cases=" << count << '\n';
}
void large(const string &mode) {
    int n = mode == "quick" ? 1000 : mode == "full" ? 100000 : 500000;
    context = "two oppositely oriented directed chains n=" + std::to_string(n); Graph g(n, true);
    for (int i = 1; i < n; ++i) { if (i != n / 2) { g.addEdge((i & 1) ? i : i - 1, (i & 1) ? i - 1 : i); } }
    auto dsu = graphComponents(g);
    require(dsu.count() == 2 && dsu.getSize(0) == n / 2 && dsu.getSize(n - 1) == n - n / 2,
        "large weak components from alternating edge directions");
    require(!dsu.isSameSet(0, n - 1), "large disconnected components");
    g.addEdge(0, n - 1); auto joined = graphComponents(CsrGraph(g));
    require(joined.count() == 1 && joined.getSize(0) == n && dsu.count() == 2, "repeated construction and snapshot independence");
    std::cout << "PASS graph DSU sparse chain n=" << n << '\n';
}
int main(int argc, char **argv) {
    string mode = "full";
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") {
                string probe = argv[++i];
                if (probe == "negative-size") { DSU dsu(-1); }
                if (probe == "negative-vertex") { (void)DSU(1).findSet(-1); }
                if (probe == "end-vertex") { (void)DSU(1).uniteSets(0, 1); }
                throw std::runtime_error("unknown or unasserted precondition=" + probe); }}
        exhaustive(mode); randomCases(mode); large(mode);
        std::cout << "PASS graph DSU checks=" << checks << " seed=" << seed << '\n'; return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL graph DSU seed=" << seed << " mode=" << mode << ' ' << error.what() << '\n'; return 1; }
}
