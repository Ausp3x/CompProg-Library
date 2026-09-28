#include "../../04-Graphs/02-traversal.hpp"

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
    template<class G>
    void witness(const G &g, const CycleWitness &c, bool exists, bool odd = false) {
        check(!c.arcs.empty() == exists, "cycle existence");
        if (!exists) { check(c.vertices.empty(), "empty witness vertices"); return; }
        check(c.vertices.size() == c.arcs.size() + 1, "cycle lengths");
        check(c.vertices.front() == c.vertices.back(), "closed cycle");
        set<int> vertices, ids;
        for (int i = 0; i < int(c.arcs.size()); ++i) {
            int a = c.arcs[i]; check(0 <= a && a < int(g.arcs.size()), "valid witness arc");
            check(g.arcs[a].from == c.vertices[i] && g.arcs[a].to == c.vertices[i + 1], "oriented witness arc");
            check(vertices.insert(c.vertices[i]).second, "simple cycle vertices");
            check(ids.insert(g.arcs[a].id).second, "distinct logical cycle edges"); }
        if (odd) { check(c.arcs.size() % 2 == 1, "odd witness length"); }}
    template<class G>
    void traversal(const G &g, const TraversalResult &r, const vector<int> &sources,
                   const vector<vector<int>> &distance, bool breadth) {
        vector<int> visited(g.n), source(g.n);
        for (int s : sources) { source[s] = 1; }
        for (int u : r.order) { check(0 <= u && u < g.n && !visited[u]++, "discovery uniqueness"); }
        for (int u = 0; u < g.n; ++u) {
            int best = 1000000;
            for (int s : sources) { best = min(best, distance[s][u]); }
            check(visited[u] == (best < 1000000), "reachability closure");
            if (!visited[u]) {
                check(r.parent[u] == -1 && r.parent_arc[u] == -1 && r.depth[u] == -1 && r.root[u] == -1,
                      "unreachable sentinels"); continue; }
            check(0 <= r.root[u] && r.root[u] < g.n && source[r.root[u]], "source root");
            if (breadth) { check(r.depth[u] == best, "Floyd unweighted minimum distance"); }
            if (r.parent[u] == -1) {
                check(source[u] && r.parent_arc[u] == -1 && r.depth[u] == 0 && r.root[u] == u, "root contract"); }
            else {
                int a = r.parent_arc[u], p = r.parent[u];
                check(0 <= p && p < g.n && 0 <= a && a < int(g.arcs.size()), "parent domain");
                check(g.arcs[a].from == p && g.arcs[a].to == u, "parent arc orientation");
                check(r.depth[p] + 1 == r.depth[u] && r.root[p] == r.root[u], "tree depth/root"); }}
        if (breadth) {
            check(r.postorder.empty() && r.cycle.arcs.empty(), "BFS unused DFS fields");
            for (int i = 1; i < int(r.order.size()); ++i) {
                check(r.depth[r.order[i - 1]] <= r.depth[r.order[i]], "BFS layer order"); }
            return; }
        // Recursive traversal is independent of the library's explicit frame state.
        vector<int> seen(g.n), parent(g.n, -1), parent_arc(g.n, -1), pre, post;
        auto visit = [&](auto &&visit, int u) -> void {
            seen[u] = 1; pre.push_back(u);
            for (int a : g[u]) {
                int v = g.arcs[a].to;
                if (!seen[v]) { parent[v] = u; parent_arc[v] = a; visit(visit, v); }}
            post.push_back(u); };
        for (int s : sources) { if (!seen[s]) { visit(visit, s); }}
        check(r.order == pre && r.postorder == post && r.parent == parent && r.parent_arc == parent_arc,
              "recursive DFS discovery/finish/parent oracle"); }
    template<class G>
    void graphCase(const G &g, const vector<vector<int>> &distance, bool cyclic, bool bipartite) {
        vector<int> all(g.n); iota(all.begin(), all.end(), 0);
        vector<vector<int>> seeds{{}, all};
        if (g.n) { seeds.push_back({0}); seeds.push_back({g.n - 1}); seeds.push_back({g.n - 1, 0, g.n - 1}); }
        for (const auto &sources : seeds) {
            auto b = bfs(g, sources), d = dfs(g, sources);
            traversal(g, b, sources, distance, true); traversal(g, d, sources, distance, false);
            if (!d.cycle.arcs.empty()) { witness(g, d.cycle, true); }}
        if (g.n) {
            check(bfs(g, 0).order == bfs(g, vector<int>{0}).order, "BFS single-source overload");
            check(dfs(g, 0).order == dfs(g, vector<int>{0}).order, "DFS single-source overload"); }
        auto forest = dfsForest(g); traversal(g, forest, all, distance, false);
        witness(g, forest.cycle, cyclic); witness(g, findCycle(g), cyclic);
        if (g.directed) { return; }
        auto components = connectedComponents(g); vector<int> seen(g.n); int previous_min = -1;
        check(components.count == int(components.groups.size()) && int(components.id.size()) == g.n, "component dimensions");
        for (int i = 0; i < components.count; ++i) {
            check(!components.groups[i].empty(), "nonempty component");
            int least = *std::min_element(components.groups[i].begin(), components.groups[i].end());
            check(least > previous_min, "component ordering"); previous_min = least;
            for (int u : components.groups[i]) {
                check(0 <= u && u < g.n && !seen[u]++ && components.id[u] == i, "component partition"); }}
        for (int u = 0; u < g.n; ++u) {
            check(seen[u] == 1, "component coverage");
            for (int v = 0; v < g.n; ++v) {
                check((components.id[u] == components.id[v]) == (distance[u][v] < 1000000), "component reachability equivalence"); }}
        auto b = bipartiteCheck(g); check(b.ok == bipartite, "enumerated coloring oracle");
        witness(g, b.cycle, !bipartite, !bipartite);
        if (b.ok) {
            for (int c : b.color) { check(c == 0 || c == 1, "color domain"); }
            for (auto e : g.edges) { check(b.color[e.u] != b.color[e.v], "proper coloring"); }} }
    void runCase(const Graph &g) {
        ++cases; context = show(g);
        vector<vector<int>> distance(g.n, vector<int>(g.n, 1000000));
        for (const auto &a : g.arcs) { distance[a.from][a.to] = 1; }
        // Preserve positive path diagonal for cycle detection before reflexive closure.
        for (int k = 0; k < g.n; ++k) { for (int u = 0; u < g.n; ++u) { for (int v = 0; v < g.n; ++v) {
            distance[u][v] = min(distance[u][v], distance[u][k] + distance[k][v]); }}}
        bool cyclic = false, bipartite = false;
        for (int u = 0; u < g.n; ++u) { cyclic |= distance[u][u] < 1000000; distance[u][u] = 0; }
        if (!g.directed) {
            int components = 0;
            for (int u = 0; u < g.n; ++u) {
                bool first = true;
                for (int v = 0; v < u; ++v) { first &= distance[u][v] == 1000000; }
                components += first; }
            cyclic = int(g.edges.size()) > g.n - components;
            for (int coloring = 0; coloring < (1 << g.n); ++coloring) {
                bool valid = true;
                for (auto e : g.edges) { valid &= ((coloring >> e.u) & 1) != ((coloring >> e.v) & 1); }
                bipartite |= valid; }}
        graphCase(g, distance, cyclic, bipartite); graphCase(CsrGraph(g), distance, cyclic, bipartite); }
    void exhaustive() {
        for (bool directed : {false, true}) {
            int bound = directed ? (mode == "stress" ? 4 : 3) : (mode == "quick" ? 3 : 4);
            for (int n = 0; n <= bound; ++n) {
                vector<pair<int, int>> possible;
                for (int u = 0; u < n; ++u) { for (int v = directed ? 0 : u; v < n; ++v) { possible.emplace_back(u, v); }}
                for (int mask = 0; mask < (1 << int(possible.size())); ++mask) {
                    Graph g(n, directed);
                    for (int i = 0; i < int(possible.size()); ++i) {
                        if ((mask >> i) & 1) { g.addEdge(possible[i].first, possible[i].second); }}
                    runCase(g); }}}
        if (mode != "quick") {
            for (int mask = 0; mask < 729; ++mask) {
                Graph g(3); int digits = mask;
                for (int u = 0; u < 3; ++u) { for (int v = u; v < 3; ++v) {
                    int copies = digits % 3; digits /= 3;
                    for (int i = 0; i < copies; ++i) { g.addEdge(u, v); }}}
                runCase(g); }}
        cout << "PASS exhaustive simple looped digraphs/undirected graphs and ternary multigraphs cases=" << cases << '\n'; }
    void randomCases() {
        std::mt19937_64 rng(seed);
        int rounds = mode == "quick" ? 100 : mode == "full" ? 1800 : 12000;
        for (int trial = 0; trial < rounds; ++trial) {
            int n = 1 + int(rng() % 9), m = int(rng() % 30); Graph g(n, rng() & 1);
            for (int i = 0; i < m; ++i) {
                int u = int(rng() % n), v = int(rng() % n);
                lng w = i % 3 == 0 ? std::numeric_limits<lng>::min() : i % 3 == 1 ? std::numeric_limits<lng>::max() : 0;
                g.addEdge(u, v, w); }
            runCase(g); }
        cout << "PASS random multigraphs/ignored extreme weights rounds=" << rounds << '\n'; }
    template<class G>
    void longGraph(const G &g) {
        vector<int> all(g.n); iota(all.begin(), all.end(), 0);
        auto b = bfs(g, 0), d = dfsForest(g);
        check(d.order == all && b.order == all, "long chain preorder/BFS");
        reverse(all.begin(), all.end()); check(d.postorder == all, "long chain postorder");
        for (int i = 0; i < g.n; ++i) { check(b.depth[i] == i && d.depth[i] == i, "long chain depth"); }
        witness(g, d.cycle, false);
        if (!g.directed) {
            check(connectedComponents(g).count == 1 && bipartiteCheck(g).ok, "long chain components/coloring"); }}
    void boundaries() {
        int n = mode == "quick" ? 20000 : mode == "full" ? 200000 : 500000;
        for (bool directed : {false, true}) {
            Graph g(n, directed);
            for (int i = 1; i < n; ++i) { g.addEdge(i - 1, i); }
            context = "long chain n=" + std::to_string(n) + " directed=" + std::to_string(directed);
            longGraph(g); longGraph(CsrGraph(g));
            g.addEdge(n - 1, 0); witness(g, findCycle(g), true); witness(CsrGraph(g), findCycle(CsrGraph(g)), true); }
        Graph g(7); g.addEdge(5, 2); g.addEdge(2, 3); g.addEdge(3, 5);
        runCase(g);
        auto copied = dfsForest(g); auto moved = std::move(copied);
        check(moved.order == dfsForest(g).order, "result copy/move");
        cout << "PASS iterative chain/closed-chain n=" << n << " and disconnected late cycle\n"; }
    void invalid(const string &name) {
        Graph g(2), directed(2, true);
        if (name == "result-negative") { TraversalResult r(-1); }
        else if (name == "bfs-negative") { bfs(g, -1); }
        else if (name == "bfs-large") { bfs(g, vector<int>{0, 2}); }
        else if (name == "bfs-empty-source") { bfs(Graph(0), 0); }
        else if (name == "dfs-negative") { dfs(g, vector<int>{0, -1}); }
        else if (name == "dfs-large") { dfs(g, 2); }
        else if (name == "components-directed") { connectedComponents(directed); }
        else if (name == "bipartite-directed") { bipartiteCheck(directed); }
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
        cout << "PASS traversal seed=" << seed << " mode=" << mode << " cases=" << cases << " checks=" << checks << '\n';
    } catch (const std::exception &e) {
        cerr << "FAIL traversal seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1; }}
