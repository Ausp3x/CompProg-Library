#include "../../04-Graphs/03-toposort.hpp"

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
        return out.str(); }
    template<class G>
    void result(const G &g, const TopologicalResult &r, bool acyclic, const vector<int> &least, bool lexicographic) {
        check(r.acyclic == acyclic, "permutation existence oracle");
        if (acyclic) {
            check(r.cycle.vertices.empty() && r.cycle.arcs.empty(), "DAG empty cycle");
            check(int(r.order.size()) == g.n, "DAG order length");
            vector<int> position(g.n, -1);
            for (int i = 0; i < g.n; ++i) {
                int u = r.order[i]; check(0 <= u && u < g.n && position[u] == -1, "order permutation");
                position[u] = i; }
            for (auto e : g.edges) { check(position[e.u] < position[e.v], "topological edge inequality"); }
            if (lexicographic) { check(r.order == least, "lexicographically first valid permutation"); }
            return; }
        check(r.order.empty(), "cyclic order sentinel");
        check(!r.cycle.arcs.empty(), "present cycle certificate");
        const auto &c = r.cycle;
        check(c.vertices.size() == c.arcs.size() + 1 && c.vertices.front() == c.vertices.back(), "closed cycle lengths");
        set<int> vertices, ids;
        for (int i = 0; i < int(c.arcs.size()); ++i) {
            int a = c.arcs[i]; check(0 <= a && a < int(g.arcs.size()), "cycle arc domain");
            check(g.arcs[a].from == c.vertices[i] && g.arcs[a].to == c.vertices[i + 1], "cycle arc orientation");
            check(vertices.insert(c.vertices[i]).second && ids.insert(g.arcs[a].id).second, "simple cycle vertices/edge IDs"); }}
    template<class G>
    void graphCase(const G &g, bool acyclic, const vector<int> &least) {
        result(g, topologicalSort(g), acyclic, least, false);
        result(g, topologicalSort(g, false), acyclic, least, false);
        result(g, topologicalSort(g, true), acyclic, least, true);
        result(g, topologicalSortDfs(g), acyclic, least, false); }
    void runCase(const Graph &g) {
        ++cases; context = show(g);
        // Enumerating vertex permutations does not use indegrees or DFS states.
        vector<int> permutation(g.n), least; iota(permutation.begin(), permutation.end(), 0);
        bool acyclic = false;
        do {
            vector<int> position(g.n);
            for (int i = 0; i < g.n; ++i) { position[permutation[i]] = i; }
            bool valid = true;
            for (auto e : g.edges) { if (position[e.u] >= position[e.v]) { valid = false; break; }}
            if (valid) { acyclic = true; least = permutation; break; }
        } while (std::next_permutation(permutation.begin(), permutation.end()));
        graphCase(g, acyclic, least); graphCase(CsrGraph(g), acyclic, least); }
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
                    runCase(g); }}}
        cout << "PASS exhaustive directed graphs with permutation oracle cases=" << cases << '\n'; }
    void randomCases() {
        std::mt19937_64 rng(seed);
        int rounds = mode == "quick" ? 100 : mode == "full" ? 1200 : 10000;
        for (int trial = 0; trial < rounds; ++trial) {
            int n = 1 + int(rng() % 7), m = int(rng() % 25); Graph g(n, true);
            vector<int> permutation(n); iota(permutation.begin(), permutation.end(), 0);
            std::shuffle(permutation.begin(), permutation.end(), rng);
            for (int i = 0; i < m; ++i) {
                int u = int(rng() % n), v = int(rng() % n);
                if (trial % 2 == 0) { if (u == v) { continue; } if (u > v) { swap(u, v); }}
                g.addEdge(permutation[u], permutation[v], i % 2 ? std::numeric_limits<lng>::min() : std::numeric_limits<lng>::max()); }
            runCase(g);
            if (!g.edges.empty()) {
                auto e = g.edges[rng() % g.edges.size()]; g.addEdge(e.u, e.v, 0);
                runCase(g); }}
        cout << "PASS random relabeled DAGs/general digraphs, duplicate arcs, extreme ignored weights rounds=" << rounds << '\n'; }
    void boundaries() {
        int n = mode == "quick" ? 20000 : mode == "full" ? 200000 : 500000;
        Graph g(n, true); vector<int> least(n); iota(least.begin(), least.end(), 0);
        for (int i = 1; i < n; ++i) { g.addEdge(i - 1, i); }
        context = "chain n=" + std::to_string(n);
        graphCase(g, true, least); graphCase(CsrGraph(g), true, least);
        g.addEdge(n - 1, 0); context = "closed chain n=" + std::to_string(n);
        graphCase(g, false, {}); graphCase(CsrGraph(g), false, {});
        // Checked libstdc++ validates the entire heap at every pop; bound this
        // all-ready fixture separately so that its debug cost stays practical.
        int width = mode == "quick" ? 500 : mode == "full" ? 2000 : 5000;
        Graph empty(width, true); least.resize(width);
        context = "edgeless width=" + std::to_string(width);
        graphCase(empty, true, least);
        Graph fixture(7, true); fixture.addEdge(5, 0); fixture.addEdge(3, 1); fixture.addEdge(3, 2);
        fixture.addEdge(3, 2); runCase(fixture);
        auto copied = topologicalSort(fixture, true); auto moved = std::move(copied);
        check(moved.order == topologicalSort(fixture, true).order, "result copy/move/repeated-call");
        fixture.addEdge(2, 3); runCase(fixture);
        cout << "PASS iterative chain/cycle n=" << n << " edgeless width=" << width
             << ", FIFO-vs-heap and late disconnected cycle\n"; }
    void invalid(const string &name) {
        Graph g(2);
        if (name == "kahn-undirected") { topologicalSort(g); }
        else if (name == "lexicographic-undirected") { topologicalSort(g, true); }
        else if (name == "dfs-undirected") { topologicalSortDfs(g); }
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
        cout << "PASS topological seed=" << seed << " mode=" << mode << " cases=" << cases << " checks=" << checks << '\n';
    } catch (const std::exception &e) {
        cerr << "FAIL topological seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1; }}
