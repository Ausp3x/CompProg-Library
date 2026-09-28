#include "../../04-Graphs/09-bridges_articulation.hpp"

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
    vector<int> components(const Graph &g, int vertex = -1, int edge = -1, const vector<bool> &removed = {}) {
        vector<int> id(g.n, -1); int count = 0;
        for (int s = 0; s < g.n; ++s) {
            if (s == vertex || id[s] != -1) { continue; }
            vector<int> queue{s}; id[s] = count++;
            for (int i = 0; i < int(queue.size()); ++i) {
                int u = queue[i];
                for (int e = 0; e < int(g.edges.size()); ++e) {
                    if (e == edge || (!removed.empty() && removed[e])) { continue; }
                    auto [a, b, w] = g.edges[e];
                    if (b == u) { swap(a, b); }
                    if (a == u && b != vertex && id[b] == -1) { id[b] = id[s]; queue.push_back(b); }}}}
        return id; }
    int count(const vector<int> &id) { return id.empty() ? 0 : *std::max_element(id.begin(), id.end()) + 1; }
    using Block = pair<vector<int>, vector<int>>;
    vector<Block> blocks(const Graph &g) {
        vector<int> adj(g.n);
        for (auto e : g.edges) { if (e.u != e.v) { adj[e.u] |= 1 << e.v; adj[e.v] |= 1 << e.u; }}
        auto connected = [&](int mask) {
            if (!mask) { return true; }
            int reached = mask & -mask, old;
            do {
                old = reached;
                for (int u = 0; u < g.n; ++u) { if ((reached >> u) & 1) { reached |= adj[u] & mask; }}
            } while (old != reached);
            return reached == mask;
        };
        vector<int> valid;
        for (int mask = (1 << g.n) - 1; mask; --mask) {
            if (std::popcount(uint(mask)) < 2 || !connected(mask)) { continue; }
            bool ok = true;
            for (int u = 0; u < g.n; ++u) {
                if (((mask >> u) & 1) && !connected(mask ^ (1 << u))) { ok = false; }}
            if (ok) { valid.push_back(mask); }}
        vector<Block> out;
        for (int mask : valid) {
            bool maximal = true;
            for (int other : valid) { if (other != mask && (other & mask) == mask) { maximal = false; break; }}
            if (!maximal) { continue; }
            Block b;
            for (int u = 0; u < g.n; ++u) { if ((mask >> u) & 1) { b.second.push_back(u); }}
            for (int e = 0; e < int(g.edges.size()); ++e) {
                auto [u, v, w] = g.edges[e];
                if (u != v && ((mask >> u) & 1) && ((mask >> v) & 1)) { b.first.push_back(e); }}
            out.push_back(std::move(b)); }
        for (int e = 0; e < int(g.edges.size()); ++e) {
            auto [u, v, w] = g.edges[e]; if (u == v) { out.push_back({{e}, {u}}); }}
        for (int u = 0; u < g.n; ++u) { if (g[u].empty()) { out.push_back({{}, {u}}); }}
        sort(out.begin(), out.end()); return out; }
    void partition(const vector<int> &id, const vector<vector<int>> &groups, const vector<int> &expected, const string &name) {
        check(id == expected, name + " ID minimum vertex contract"); vector<int> seen(id.size());
        check(int(groups.size()) == count(expected), name + " count");
        for (int c = 0; c < int(groups.size()); ++c) {
            check(!groups[c].empty(), name + " nonempty");
            for (int u : groups[c]) { check(0 <= u && u < int(id.size()) && id[u] == c && !seen[u]++, name + " membership"); }}
        for (int x : seen) { check(x == 1, name + " vertex coverage"); }}
    template<class G>
    void result(const G &g, const LowlinkResult &out, const vector<int> &connected,
                const vector<bool> &bridge, const vector<bool> &art, const vector<int> &component,
                const vector<Block> &expected) {
        int n = g.n, m = int(g.edges.size());
        check(out.is_bridge == bridge, "edge deletion bridge oracle"); check(out.is_art == art, "vertex deletion articulation oracle");
        check(int(out.tin.size()) == n && int(out.low.size()) == n && int(out.parent_arc.size()) == n, "DFS array sizes");
        vector<int> sorted = out.tin; sort(sorted.begin(), sorted.end());
        for (int u = 0; u < n; ++u) { check(sorted[u] == u, "discovery permutation"); }
        vector<int> roots, minimum(count(connected), n), present(m);
        for (int u = 0; u < n; ++u) { minimum[connected[u]] = min(minimum[connected[u]], u); }
        for (int e : out.bridges) { check(0 <= e && e < m && bridge[e] && !present[e]++, "bridge ID witness"); }
        for (int e = 0; e < m; ++e) { check(present[e] == int(bridge[e]), "bridge list coverage"); }
        vector<vector<bool>> descendant(n, vector<bool>(n));
        for (int u = 0; u < n; ++u) {
            int a = out.parent_arc[u];
            if (a == -1) { roots.push_back(u); }
            else { check(0 <= a && a < int(g.arcs.size()) && g.arcs[a].to == u && out.tin[g.arcs[a].from] < out.tin[u], "parent arc/root"); }
            int v = u;
            for (int length = 0; v != -1; ++length) {
                check(length < n, "acyclic parent forest"); descendant[v][u] = true;
                int p = out.parent_arc[v]; v = p == -1 ? -1 : g.arcs[p].from; }}
        check(roots == minimum && out.roots == minimum, "component roots minimum vertex order");
        for (int u = 0; u < n; ++u) {
            int low = out.tin[u];
            for (int a = 0; a < int(g.arcs.size()); ++a) {
                auto e = g.arcs[a];
                if (!descendant[u][e.from] || out.tin[e.to] >= out.tin[e.from]) { continue; }
                if (out.parent_arc[e.from] != -1 && g.arcs[out.parent_arc[e.from]].id == e.id) { continue; }
                low = min(low, out.tin[e.to]); }
            check(out.low[u] == low, "lowlink subtree/backedge definition"); }
        partition(out.component, out.components, component, "edge components");
        check(out.edge_block.size() == size_t(m) && out.block_edges.size() == out.block_vertices.size(), "block array dimensions");
        vector<Block> actual; fill(present.begin(), present.end(), 0);
        for (int b = 0; b < int(out.block_edges.size()); ++b) {
            auto es = out.block_edges[b], vs = out.block_vertices[b];
            for (int e : es) { check(0 <= e && e < m && !present[e]++ && out.edge_block[e] == b, "block edge partition/index"); }
            sort(es.begin(), es.end()); sort(vs.begin(), vs.end());
            check(std::adjacent_find(vs.begin(), vs.end()) == vs.end(), "block unique vertices");
            actual.push_back({es, vs}); }
        sort(actual.begin(), actual.end()); check(actual == expected, "maximal vertex-subset deletion oracle including loops/dyads/isolates");
        for (int seen : present) { check(seen == 1, "block edge coverage"); }
        auto orientation = strongOrientation(g);
        check(orientation.arcs == out.orientation, "orientation consistency");
        check(orientation.ok == (count(connected) <= 1 && out.bridges.empty()), "Robbins existence");
        check(orientation.count == count(connected) + int(out.bridges.size()), "minimum possible SCC count");
        partition(orientation.component, orientation.groups, component, "orientation SCCs");
        check((orientation.bridge == -1) == out.bridges.empty(), "bridge obstruction absence");
        if (orientation.bridge != -1) { check(bridge[orientation.bridge], "bridge obstruction witness"); }
        auto [u, v] = orientation.disconnected;
        if (count(connected) <= 1) { check(u == -1 && v == -1, "disconnected witness absent"); }
        else { check(0 <= u && u < n && 0 <= v && v < n && connected[u] != connected[v], "disconnected roots witness"); }
        vector<vector<bool>> reach(n, vector<bool>(n));
        for (int i = 0; i < n; ++i) { reach[i][i] = true; }
        check(int(orientation.arcs.size()) == m, "one orientation per edge");
        for (int e = 0; e < m; ++e) {
            int a = orientation.arcs[e];
            check(0 <= a && a < int(g.arcs.size()) && g.arcs[a].id == e, "orientation input arc ID");
            reach[g.arcs[a].from][g.arcs[a].to] = true; }
        for (int k = 0; k < n; ++k) { for (int i = 0; i < n; ++i) { for (int j = 0; j < n; ++j) {
            reach[i][j] = reach[i][j] || (reach[i][k] && reach[k][j]); }}}
        for (int i = 0; i < n; ++i) { for (int j = 0; j < n; ++j) {
            check((reach[i][j] && reach[j][i]) == (component[i] == component[j]), "orientation closure SCC oracle"); }} }
    void runCase(const Graph &g) {
        ++cases; context = show(g);
        auto connected = components(g); int base = count(connected);
        vector<bool> bridge(g.edges.size()), art(g.n);
        for (int e = 0; e < int(g.edges.size()); ++e) { bridge[e] = count(components(g, -1, e)) > base; }
        for (int u = 0; u < g.n; ++u) { art[u] = count(components(g, u)) > base; }
        auto component = components(g, -1, -1, bridge); auto expected = blocks(g);
        result(g, lowlink(g), connected, bridge, art, component, expected);
        CsrGraph csr(g); result(csr, lowlink(csr), connected, bridge, art, component, expected);
        vector<vector<int>> adj(g.n); set<pair<int, int>> pairs;
        for (int e = 0; e < int(g.edges.size()); ++e) {
            auto [u, v, w] = g.edges[e]; adj[u].push_back(v); adj[v].push_back(u);
            if (bridge[e]) { pairs.emplace(min(u, v), max(u, v)); }}
        BridgeAlgo legacy(g.n, adj); Tarjan tarjan(g.n, adj);
        check(legacy.bridges == pairs && set<pair<int, int>>(tarjan.bridges.begin(), tarjan.bridges.end()) == pairs, "both legacy bridge results");
        check(tarjan.is_art == art && legacy.n == g.n && tarjan.n == g.n && legacy.timer == g.n && tarjan.timer == g.n, "legacy articulation/metadata");
        check(legacy.low == tarjan.low && legacy.t_in == tarjan.t_in && legacy.vst == vector<bool>(g.n, true) && tarjan.vst == legacy.vst, "legacy traversal diagnostics"); }
    void exhaustive() {
        for (int n = 0; n <= (mode == "quick" ? 3 : 4); ++n) {
            vector<pair<int, int>> possible;
            for (int u = 0; u < n; ++u) { for (int v = u; v < n; ++v) { possible.emplace_back(u, v); }}
            for (int mask = 0; mask < (1 << int(possible.size())); ++mask) {
                Graph g(n);
                for (int e = 0; e < int(possible.size()); ++e) {
                    if ((mask >> e) & 1) { g.addEdge(possible[e].first, possible[e].second); }}
                runCase(g); }}
        for (int n = 0; n <= (mode == "quick" ? 2 : 3); ++n) {
            vector<pair<int, int>> possible;
            for (int u = 0; u < n; ++u) { for (int v = u; v < n; ++v) { possible.emplace_back(u, v); }}
            int limit = 1;
            for (int i = 0; i < int(possible.size()); ++i) { limit *= 3; }
            for (int code = 0; code < limit; ++code) {
                Graph g(n); int value = code;
                for (auto [u, v] : possible) {
                    int c = value % 3; value /= 3;
                    for (int i = 0; i < c; ++i) { g.addEdge(u, v); }}
                runCase(g); }}
        if (mode == "stress") {
            for (int n : {5, 6}) {
                vector<pair<int, int>> possible;
                for (int u = 0; u < n; ++u) { for (int v = u + 1; v < n; ++v) { possible.emplace_back(u, v); }}
                for (int mask = 0; mask < (1 << int(possible.size())); ++mask) {
                    Graph g(n);
                    for (int e = 0; e < int(possible.size()); ++e) {
                        if ((mask >> e) & 1) { g.addEdge(possible[e].first, possible[e].second); }}
                    runCase(g); }}}
        cout << "PASS exhaustive looped simple/multiplicity-two graphs cases=" << cases << '\n'; }
    void randomCases() {
        std::mt19937_64 rng(seed); int rounds = mode == "quick" ? 100 : mode == "full" ? 1500 : 12000;
        for (int trial = 0; trial < rounds; ++trial) {
            int n = 1 + int(rng() % (mode == "quick" ? 7 : 8)), m = int(rng() % 25); Graph g(n);
            for (int i = 0; i < m; ++i) {
                g.addEdge(int(rng() % n), int(rng() % n), i % 2 ? std::numeric_limits<lng>::min() : std::numeric_limits<lng>::max()); }
            runCase(g);
            if (trial % 5 == 0 && !g.edges.empty()) {
                auto e = g.edges[rng() % g.edges.size()]; g.addEdge(e.u, e.v, 0); runCase(g); }}
        cout << "PASS random multigraph deletion/subset/closure oracles rounds=" << rounds << '\n'; }
    void boundaries() {
        int n = mode == "quick" ? 20000 : mode == "full" ? 200000 : 500000; Graph g(n);
        for (int u = 1; u < n; ++u) { g.addEdge(u - 1, u); }
        context = "chain n=" + std::to_string(n);
        auto out = lowlink(g);
        check(int(out.bridges.size()) == n - 1 && int(out.components.size()) == n && int(out.block_edges.size()) == n - 1, "large chain counts");
        for (int u = 0; u < n; ++u) { check(out.is_art[u] == (0 < u && u + 1 < n) && out.low[u] == u, "large chain flags/low"); }
        auto orientation = strongOrientation(CsrGraph(g));
        check(!orientation.ok && orientation.count == n, "large chain orientation obstruction");
        g.addEdge(n - 1, 0); context = "cycle n=" + std::to_string(n); out = lowlink(CsrGraph(g));
        check(out.bridges.empty() && out.components.size() == 1 && out.block_edges.size() == 1, "large cycle block counts");
        for (int u = 0; u < n; ++u) { check(!out.is_art[u] && out.low[u] == 0, "large cycle flags/low"); }
        orientation = strongOrientation(g); check(orientation.ok && orientation.count == 1, "large cycle Robbins");
        Graph star(n);
        for (int u = 1; u < n; ++u) { star.addEdge(0, u); star.addEdge(0, u); }
        star.addEdge(0, 0); context = "parallel star n=" + std::to_string(n); out = lowlink(star);
        check(out.bridges.empty() && out.is_art[0] && out.components.size() == 1 && int(out.block_edges.size()) == n, "large root and parallel-edge/loop blocks");
        for (int u = 1; u < n; ++u) { check(!out.is_art[u], "large star leaves not articulation"); }
        Graph fixture(6); fixture.addEdge(0, 1); fixture.addEdge(1, 2); fixture.addEdge(2, 0);
        fixture.addEdge(2, 3); fixture.addEdge(3, 4); fixture.addEdge(4, 2); fixture.addEdge(2, 2); runCase(fixture);
        auto before = lowlink(fixture), copied = before; auto moved = std::move(copied);
        fixture.addEdge(4, 5); runCase(fixture);
        check(before.block_edges == moved.block_edges && before.component == moved.component, "owning copy/move graph mutation snapshots");
        vector<vector<int>> adj{{1}, {0}, {}}; BridgeAlgo legacy(3, adj);
        check(legacy.bridges == set<pair<int, int>>{{0, 1}}, "legacy initial bridge");
        adj[0].push_back(1); adj[1].push_back(0); legacy.dfs(0, 0, adj);
        check(legacy.bridges.empty() && legacy.timer == 3, "legacy dfs recomputation clears prior result");
        adj[1].push_back(2); adj[2].push_back(1); legacy.build(adj);
        check(legacy.bridges == set<pair<int, int>>{{1, 2}}, "legacy build reset");
        legacy.dfs(0, -1, adj); check(legacy.bridges == set<pair<int, int>>{{1, 2}}, "legacy root parent sentinel");
        cout << "PASS stack-safe chain/cycle/parallel-star n=" << n << ", root articulation, snapshots and legacy reset\n"; }
    void invalid(const string &name) {
        Graph g(2, true); vector<vector<int>> adj(2);
        if (name == "lowlink-directed") { lowlink(g); }
        else if (name == "lowlink-csr-directed") { lowlink(CsrGraph(g)); }
        else if (name == "orientation-directed") { strongOrientation(g); }
        else if (name == "orientation-csr-directed") { strongOrientation(CsrGraph(g)); }
        else if (name == "legacy-negative") { Tarjan(-1, adj); }
        else if (name == "legacy-short") { BridgeAlgo(3, adj); }
        else if (name == "legacy-endpoint") { adj[0].push_back(2); Tarjan(2, adj); }
        else if (name == "bridge-dfs-cur") { BridgeAlgo(2, adj).dfs(2, 0, adj); }
        else if (name == "bridge-dfs-prv") { BridgeAlgo(2, adj).dfs(0, -2, adj); }
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
        cout << "PASS lowlink seed=" << seed << " mode=" << mode << " cases=" << cases << " checks=" << checks << '\n';
    } catch (const std::exception &e) {
        cerr << "FAIL lowlink seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1; }}
