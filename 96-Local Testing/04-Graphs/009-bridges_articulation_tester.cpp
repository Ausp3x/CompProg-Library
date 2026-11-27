#include "../../04-Graphs/009-bridges_articulation.hpp"

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
        return out.str();}
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
        return id;}
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
                for (int u = 0; u < g.n; ++u) { if ((reached >> u) & 1) { reached |= adj[u] & mask; }}} while (old != reached);
            return reached == mask;};
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
            out.push_back(std::move(b));}
        for (int e = 0; e < int(g.edges.size()); ++e) {
            auto [u, v, w] = g.edges[e]; if (u == v) { out.push_back({{e}, {u}}); }}
        for (int u = 0; u < g.n; ++u) { if (g[u].empty()) { out.push_back({{}, {u}}); }}
        sort(out.begin(), out.end()); return out;}
    void partition(const vector<int> &id, const vector<vector<int>> &groups, const vector<int> &expected, const string &name) {
        check(id == expected, name + " ID minimum vertex contract"); vector<int> seen(id.size());
        check(int(groups.size()) == count(expected), name + " count");
        for (int c = 0; c < int(groups.size()); ++c) {
            check(!groups[c].empty(), name + " nonempty");
            for (int u : groups[c]) { check(0 <= u && u < int(id.size()) && id[u] == c && !seen[u]++, name + " membership"); }}
        for (int x : seen) { check(x == 1, name + " vertex coverage"); }}
    template<class G>
    void result(const G &g, const LowlinkResult &out, const vector<int> &connected,
                const vector<bool> &bridge, const vector<bool> &art, const vector<int> &cut, const vector<int> &component,
                const vector<Block> &expected) {
        int n = g.n, m = int(g.edges.size());
        check(out.is_bridge == bridge, "edge deletion bridge oracle"); check(out.is_art == art, "vertex deletion articulation oracle");
        check(out.cut_components == cut, "vertex deletion component count oracle");
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
                int p = out.parent_arc[v]; v = p == -1 ? -1 : g.arcs[p].from;}}
        check(roots == minimum && out.roots == minimum, "component roots minimum vertex order");
        for (int u = 0; u < n; ++u) {
            int low = out.tin[u];
            for (int a = 0; a < int(g.arcs.size()); ++a) {
                auto e = g.arcs[a];
                if (!descendant[u][e.from] || out.tin[e.to] >= out.tin[e.from]) { continue; }
                if (out.parent_arc[e.from] != -1 && g.arcs[out.parent_arc[e.from]].id == e.id) { continue; }
                low = min(low, out.tin[e.to]);}
            check(out.low[u] == low, "lowlink subtree/backedge definition");}
        partition(out.component, out.components, component, "edge components");
        check(out.edge_block.size() == size_t(m) && out.block_edges.size() == out.block_vertices.size(), "block array dimensions");
        vector<Block> actual; fill(present.begin(), present.end(), 0);
        for (int b = 0; b < int(out.block_edges.size()); ++b) {
            auto es = out.block_edges[b], vs = out.block_vertices[b];
            for (int e : es) { check(0 <= e && e < m && !present[e]++ && out.edge_block[e] == b, "block edge partition/index"); }
            sort(es.begin(), es.end()); sort(vs.begin(), vs.end());
            check(std::adjacent_find(vs.begin(), vs.end()) == vs.end(), "block unique vertices");
            actual.push_back({es, vs});}
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
            reach[g.arcs[a].from][g.arcs[a].to] = true;}
        for (int k = 0; k < n; ++k) { for (int i = 0; i < n; ++i) { for (int j = 0; j < n; ++j) {
            reach[i][j] = reach[i][j] || (reach[i][k] && reach[k][j]);}}}
        for (int i = 0; i < n; ++i) { for (int j = 0; j < n; ++j) {
            check((reach[i][j] && reach[j][i]) == (component[i] == component[j]), "orientation closure SCC oracle");}}}
    // Bridge edge IDs as sorted endpoint pairs.
    set<pair<int, int>> pairsOf(const Graph &g, const vector<int> &bridges) {
        set<pair<int, int>> res;
        for (int e : bridges) { res.insert(std::minmax(g.edges[e].u, g.edges[e].v)); }
        return res;}
    // A DFS order alone fixes the forest: a vertex's parent is its latest earlier-discovered neighbor.
    void reconstructedDfs(const Graph &g, const vector<int> &tin, const vector<int> &low, const string &name) {
        int n = g.n, m = int(g.edges.size());
        check(int(tin.size()) == n && int(low.size()) == n, name + " diagnostic sizes");
        vector<int> sorted = tin; sort(sorted.begin(), sorted.end());
        for (int u = 0; u < n; ++u) { check(sorted[u] == u, name + " tin permutation"); }
        vector<int> parent(n, -1), tree(n, -1);
        for (int e = 0; e < m; ++e) {
            auto [u, v, w] = g.edges[e];
            if (u == v) { continue; }
            if (tin[u] > tin[v]) { swap(u, v); }
            if (parent[v] == -1 || tin[u] > tin[parent[v]]) { parent[v] = u; tree[v] = e; }}
        auto ancestor = [&](int a, int b) {
            for (; b != -1; b = parent[b]) { if (b == a) { return true; }}
            return false;};
        for (int u = 0; u < n; ++u) {
            int want = tin[u];
            for (int e = 0; e < m; ++e) {
                auto [x, y, w] = g.edges[e];
                if (x == y) { continue; }
                if (tin[x] < tin[y]) { swap(x, y); }
                check(ancestor(y, x), name + " nontree edges join ancestor and descendant");
                if (e != tree[x] && ancestor(u, x)) { want = min(want, tin[y]); }}
            check(low[u] == want, name + " low against reconstructed DFS forest");}}
    // Deletion oracle on an edge list: f is a bridge iff its endpoints separate once f alone is removed.
    vector<bool> deletionBridges(int n, const vector<pair<int, int>> &es) {
        int m = int(es.size()); vector<bool> res(m); vector<int> root(n);
        auto find = [&](int u) {
            while (root[u] != u) { root[u] = root[root[u]]; u = root[u]; }
            return u;};
        for (int f = 0; f < m; ++f) {
            std::iota(root.begin(), root.end(), 0);
            for (int e = 0; e < m; ++e) { if (e != f) { root[find(es[e].first)] = find(es[e].second); }}
            res[f] = find(es[f].first) != find(es[f].second);}
        return res;}
    // Tester-side iterative Tarjan skipping only the entering edge ID; validated against deletionBridges in every small case.
    vector<bool> tarjanBridges(int n, const vector<pair<int, int>> &es) {
        int m = int(es.size()), timer = 0; vector<int> start(n + 2), adj(2 * size_t(m));
        for (auto [u, v] : es) { ++start[u + 2]; ++start[v + 2]; }
        for (int u = 0; u < n; ++u) { start[u + 2] += start[u + 1]; }
        for (int e = 0; e < m; ++e) { adj[start[es[e].first + 1]++] = e; adj[start[es[e].second + 1]++] = e; }
        vector<int> tin(n, -1), low(n), via(n, -1), stack, pos(start.begin(), start.begin() + n); vector<bool> bridge(m);
        for (int s = 0; s < n; ++s) {
            if (tin[s] != -1) { continue; }
            tin[s] = low[s] = timer++; stack.push_back(s);
            while (!stack.empty()) {
                int u = stack.back();
                if (pos[u] < start[u + 1]) {
                    int e = adj[pos[u]++];
                    if (e == via[u]) { continue; }
                    int v = es[e].first ^ es[e].second ^ u;
                    if (tin[v] == -1) { via[v] = e; tin[v] = low[v] = timer++; stack.push_back(v); }
                    else { low[u] = min(low[u], tin[v]); }
                    continue;}
                stack.pop_back();
                if (via[u] == -1) { continue; }
                int p = stack.back(); low[p] = min(low[p], low[u]);
                if (low[u] > tin[p]) { bridge[via[u]] = true; }}}
        return bridge;}
    vector<int> indices(const vector<bool> &flags) {
        vector<int> res;
        for (int i = 0; i < int(flags.size()); ++i) { if (flags[i]) { res.push_back(i); }}
        return res;}
    // Every prefix of the stream against the deletion oracle; the final answer against the graph's own deletion oracle.
    void streaming(const Graph &g, const vector<bool> &bridge) {
        int n = g.n; BridgesStreaming s(n); vector<pair<int, int>> es;
        for (int e = 0; e < int(g.edges.size()); ++e) {
            check(s.add(g.edges[e].u, g.edges[e].v) == e, "stream index");
            es.emplace_back(g.edges[e].u, g.edges[e].v);
            check(int(s.kept.size()) <= max(0, 2 * (n - 1)), "stream O(n) kept bound");
            check(s.bridges() == indices(deletionBridges(n, es)), "stream prefix deletion oracle");}
        auto want = indices(bridge);
        check(s.bridges() == want && s.bridges() == want, "stream repeated query against graph deletion oracle");
        check(tarjanBridges(n, es) == bridge, "tester Tarjan against deletion oracle");}
    // comp: any labels in [0, n) equal exactly within connected components.
    template<class G>
    void evenCheck(const G &g, const vector<int> &comp, const string &name) {
        int n = g.n, m = int(g.edges.size()); vector<int> arcs{7, 7, 7}, out(n), parity(n), smallest(n, n);
        bool ok = evenOutdegreeOrientation(g, arcs);
        check(int(arcs.size()) == m, name + " one arc per edge");
        for (int e = 0; e < m; ++e) {
            int a = arcs[e];
            check(0 <= a && a < int(g.arcs.size()) && g.arcs[a].id == e, name + " arc ID orients its edge");
            ++out[g.arcs[a].from]; parity[comp[g.edges[e].u]] ^= 1;}
        for (int u = 0; u < n; ++u) { smallest[comp[u]] = min(smallest[comp[u]], u); }
        check(ok == (std::find(parity.begin(), parity.end(), 1) == parity.end()), name + " even iff every component has even edge count");
        for (int u = 0; u < n; ++u) {
            check(out[u] % 2 == int(smallest[comp[u]] == u && parity[comp[u]]), name + " only odd components' smallest vertex odd");}}
    // Explicit neighbour sets for triangles, Bellman-Ford on the graph minus e for the span.
    vector<pair<int, lll>> localOracle(const Graph &g, bool weighted) {
        int n = g.n, m = int(g.edges.size()); vector<set<int>> nb(n); vector<pair<int, lll>> res;
        for (auto [u, v, w] : g.edges) { if (u != v) { nb[u].insert(v); nb[v].insert(u); }}
        for (int e = 0; e < m; ++e) {
            auto [s, t, w] = g.edges[e]; bool triangle = false;
            if (s == t) { continue; }
            for (int z : nb[s]) { triangle = triangle || (z != t && nb[t].count(z)); }
            if (triangle) { continue; }
            vector<lll> dist(n, -1); dist[s] = 0;
            for (int round = 0; round < n; ++round) {
                for (int f = 0; f < m; ++f) {
                    if (f == e) { continue; }
                    auto [a, b, c] = g.edges[f]; lll cost = weighted ? lll(c) : 1;
                    for (int k = 0; k < 2; ++k, swap(a, b)) {
                        if (dist[a] != -1 && (dist[b] == -1 || dist[a] + cost < dist[b])) { dist[b] = dist[a] + cost; }}}}
            res.push_back({e, dist[t]});}
        return res;}
    template<class G>
    void localCheck(const G &g, bool weighted, const vector<pair<int, lll>> &expected, const string &name) {
        check(localBridges(g, weighted) == expected, name);}
    Graph reweight(const Graph &g, lng base) {
        static std::mt19937_64 rng(seed ^ 0x9e3779b97f4a7c15ULL); Graph h(g.n);
        for (auto e : g.edges) { h.addEdge(e.u, e.v, base + lng(rng() % 4)); }
        return h;}
    void newOperations(const Graph &g, const vector<int> &connected, const vector<bool> &bridge) {
        streaming(g, bridge);
        CsrGraph csr(g); evenCheck(g, connected, "even orientation"); evenCheck(csr, connected, "CSR even orientation");
        auto hops = localOracle(g, false);
        check(localBridges(g) == hops, "local bridges default unweighted oracle");
        localCheck(csr, false, hops, "CSR local bridges unweighted oracle");
        for (lng base : {lng(0), std::numeric_limits<lng>::max() - 3}) {
            auto h = reweight(g, base); auto expected = localOracle(h, true);
            localCheck(h, true, expected, "local bridges weighted Bellman-Ford oracle");
            localCheck(CsrGraph(h), true, expected, "CSR local bridges weighted Bellman-Ford oracle");
            localCheck(h, false, hops, "local bridges ignore weights unweighted");}}
    void runCase(const Graph &g) {
        ++cases; context = show(g);
        auto connected = components(g); int base = count(connected);
        vector<bool> bridge(g.edges.size()), art(g.n); vector<int> cut(g.n);
        for (int e = 0; e < int(g.edges.size()); ++e) { bridge[e] = count(components(g, -1, e)) > base; }
        for (int u = 0; u < g.n; ++u) { cut[u] = count(components(g, u)); art[u] = cut[u] > base; }
        auto component = components(g, -1, -1, bridge); auto expected = blocks(g);
        result(g, lowlink(g), connected, bridge, art, cut, component, expected); newOperations(g, connected, bridge);
        CsrGraph csr(g); result(csr, lowlink(csr), connected, bridge, art, cut, component, expected);
        vector<vector<int>> adj(g.n); set<pair<int, int>> pairs;
        for (int e = 0; e < int(g.edges.size()); ++e) {
            auto [u, v, w] = g.edges[e]; adj[u].push_back(v); adj[v].push_back(u);
            if (bridge[e]) { pairs.emplace(min(u, v), max(u, v)); }}
        auto h = Graph::fromAdjacency(adj); auto low = lowlink(h);
        check(pairsOf(h, low.bridges) == pairs && low.is_art == art, "adjacency-list bridge pairs and articulation");
        reconstructedDfs(g, low.tin, low.low, "adjacency-list lowlink");}
    void exhaustive() {
        for (int n = 0; n <= (mode == "quick" ? 3 : 4); ++n) {
            vector<pair<int, int>> possible;
            for (int u = 0; u < n; ++u) { for (int v = u; v < n; ++v) { possible.emplace_back(u, v); }}
            for (int mask = 0; mask < (1 << int(possible.size())); ++mask) {
                Graph g(n);
                for (int e = 0; e < int(possible.size()); ++e) {
                    if ((mask >> e) & 1) { g.addEdge(possible[e].first, possible[e].second); }}
                runCase(g);}}
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
                runCase(g);}}
        if (mode == "stress") {
            for (int n : {5, 6}) {
                vector<pair<int, int>> possible;
                for (int u = 0; u < n; ++u) { for (int v = u + 1; v < n; ++v) { possible.emplace_back(u, v); }}
                for (int mask = 0; mask < (1 << int(possible.size())); ++mask) {
                    Graph g(n);
                    for (int e = 0; e < int(possible.size()); ++e) {
                        if ((mask >> e) & 1) { g.addEdge(possible[e].first, possible[e].second); }}
                    runCase(g);}}}
        cout << "PASS exhaustive looped simple/multiplicity-two graphs cases=" << cases << '\n';}
    void randomCases() {
        std::mt19937_64 rng(seed); int rounds = mode == "quick" ? 100 : mode == "full" ? 1500 : 12000;
        for (int trial = 0; trial < rounds; ++trial) {
            int n = 1 + int(rng() % (mode == "quick" ? 7 : 8)), m = int(rng() % 25); Graph g(n);
            for (int i = 0; i < m; ++i) {
                g.addEdge(int(rng() % n), int(rng() % n), i % 2 ? std::numeric_limits<lng>::min() : std::numeric_limits<lng>::max());}
            runCase(g);
            if (trial % 5 == 0 && !g.edges.empty()) {
                auto e = g.edges[rng() % g.edges.size()]; g.addEdge(e.u, e.v, 0); runCase(g);}}
        cout << "PASS random multigraph deletion/subset/closure oracles rounds=" << rounds << '\n';}
    void boundaries() {
        int n = mode == "quick" ? 20000 : mode == "full" ? 200000 : 500000; Graph g(n);
        for (int u = 1; u < n; ++u) { g.addEdge(u - 1, u); }
        context = "chain n=" + std::to_string(n);
        auto out = lowlink(g);
        check(int(out.bridges.size()) == n - 1 && int(out.components.size()) == n && int(out.block_edges.size()) == n - 1, "large chain counts");
        for (int u = 0; u < n; ++u) { check(out.is_art[u] == (0 < u && u + 1 < n) && out.low[u] == u && out.cut_components[u] == (0 < u && u + 1 < n ? 2 : 1), "large chain flags/low/cuts"); }
        auto orientation = strongOrientation(CsrGraph(g));
        check(!orientation.ok && orientation.count == n, "large chain orientation obstruction");
        g.addEdge(n - 1, 0); context = "cycle n=" + std::to_string(n); out = lowlink(CsrGraph(g));
        check(out.bridges.empty() && out.components.size() == 1 && out.block_edges.size() == 1, "large cycle block counts");
        for (int u = 0; u < n; ++u) { check(!out.is_art[u] && out.low[u] == 0 && out.cut_components[u] == 1, "large cycle flags/low/cuts"); }
        orientation = strongOrientation(g); check(orientation.ok && orientation.count == 1, "large cycle Robbins");
        Graph star(n);
        for (int u = 1; u < n; ++u) { star.addEdge(0, u); star.addEdge(0, u); }
        star.addEdge(0, 0); context = "parallel star n=" + std::to_string(n); out = lowlink(star);
        check(out.bridges.empty() && out.is_art[0] && out.components.size() == 1 && int(out.block_edges.size()) == n, "large root and parallel-edge/loop blocks");
        check(out.cut_components[0] == n - 1, "large star center cut count");
        for (int u = 1; u < n; ++u) { check(!out.is_art[u] && out.cut_components[u] == 1, "large star leaves not articulation"); }
        Graph fixture(6); fixture.addEdge(0, 1); fixture.addEdge(1, 2); fixture.addEdge(2, 0);
        fixture.addEdge(2, 3); fixture.addEdge(3, 4); fixture.addEdge(4, 2); fixture.addEdge(2, 2); runCase(fixture);
        auto before = lowlink(fixture), copied = before; auto moved = std::move(copied);
        fixture.addEdge(4, 5); runCase(fixture);
        check(before.block_edges == moved.block_edges && before.component == moved.component, "owning copy/move graph mutation snapshots");
        vector<vector<int>> adj{{1}, {0}, {}};
        auto fresh = [&] { auto h = Graph::fromAdjacency(adj); return pairsOf(h, lowlink(h).bridges); };
        check(fresh() == set<pair<int, int>>{{0, 1}}, "adjacency-list bridge");
        adj[0].push_back(1); adj[1].push_back(0);
        check(fresh().empty(), "repeated call after a parallel edge");
        adj[1].push_back(2); adj[2].push_back(1);
        check(fresh() == set<pair<int, int>>{{1, 2}}, "repeated call after a new edge");
        cout << "PASS stack-safe chain/cycle/parallel-star n=" << n << ", root articulation, snapshots and repeated adjacency-list calls\n";}
    void fixtures() {
        context = "local bridges/even orientation fixtures";
        Graph square(4); square.addPath({0, 1, 2, 3}, 0); square.addEdge(3, 0, 5);
        vector<pair<int, lll>> hops{{0, 3}, {1, 3}, {2, 3}, {3, 3}}, weights{{0, 5}, {1, 5}, {2, 5}, {3, 0}};
        check(localBridges(square) == hops && localOracle(square, false) == hops, "square spans 3");
        check(localBridges(square, true) == weights && localOracle(square, true) == weights, "zero-weight square spans");
        Graph triangle(3); triangle.addCycle({0, 1, 2});
        check(localBridges(triangle).empty() && localOracle(triangle, false).empty(), "triangle edges excluded");
        Graph pair2(3); pair2.addEdge(0, 1); pair2.addEdge(0, 1); pair2.addEdge(0, 0); pair2.addEdge(1, 2);
        vector<pair<int, lll>> want{{0, 1}, {1, 1}, {3, -1}};
        check(localBridges(pair2) == want && localOracle(pair2, false) == want, "parallel copies span 1, loop skipped, bridge -1");
        vector<int> arcs;
        check(evenOutdegreeOrientation(triangle, arcs) == false, "odd triangle");
        triangle.addEdge(0, 0); check(evenOutdegreeOrientation(triangle, arcs), "triangle plus loop even");
        evenCheck(triangle, {0, 0, 0}, "fixture even orientation");}
    // Large stream, CSR/Graph orientation and local bridges: stack safety, O(n) memory and the stated bounds.
    void largeOperations() {
        int n = mode == "quick" ? 20000 : mode == "full" ? 200000 : 500000;
        std::mt19937_64 rng(seed + 1); vector<pair<int, int>> es;
        for (int v = 1; v < n; ++v) {
            if (v < n / 2) { es.emplace_back(v - 1, v); }
            else if (rng() % 10) { es.emplace_back(int(rng() % uint(v)), v); }}
        for (int i = 0; i < n / 4; ++i) { int u = int(rng() % uint(n)); es.emplace_back(u, min(n - 1, u + 1 + int(rng() % 3))); }
        for (int i = 0; i < n / 20; ++i) { es.push_back(es[rng() % es.size()]); int u = int(rng() % uint(n)); es.emplace_back(u, u); }
        std::shuffle(es.begin(), es.end(), rng);
        int m = int(es.size()); context = "large stream n=" + std::to_string(n) + " m=" + std::to_string(m);
        BridgesStreaming s(n);
        for (int i = 0; i < m; ++i) {
            check(s.add(es[i].first, es[i].second) == i, "large stream index");
            if (i + 1 != m / 3 && i + 1 != m) { continue; }
            auto want = indices(tarjanBridges(n, vector<pair<int, int>>(es.begin(), es.begin() + i + 1)));
            check(!want.empty() && int(want.size()) <= i, "large stream nontrivial bridge set");
            check(s.bridges() == want && int(s.kept.size()) <= 2 * (n - 1), "large stream prefix Tarjan oracle and kept bound");}
        int small = 1000, dense = mode == "quick" ? 50000 : 300000; BridgesStreaming d(small); vector<pair<int, int>> ds;
        for (int i = 0; i < dense; ++i) { ds.emplace_back(int(rng() % uint(small - 1)), int(rng() % uint(small - 1))); d.add(ds[i].first, ds[i].second); }
        ds.emplace_back(0, small - 1); d.add(0, small - 1);
        check(d.bridges() == indices(tarjanBridges(small, ds)) && d.bridges() == vector<int>{dense}, "dense stream single pendant bridge");
        check(int(d.kept.size()) <= 2 * (small - 1), "dense stream O(n) kept bound");
        Graph g(n); vector<int> root(n), comp(n);
        for (auto [u, v] : es) { g.addEdge(u, v); }
        std::iota(root.begin(), root.end(), 0);
        auto find = [&](int u) {
            while (root[u] != u) { root[u] = root[root[u]]; u = root[u]; }
            return u;};
        for (auto [u, v] : es) { root[find(u)] = find(v); }
        for (int u = 0; u < n; ++u) { comp[u] = find(u); }
        evenCheck(g, comp, "large random even orientation"); evenCheck(CsrGraph(g), comp, "large random CSR even orientation");
        Graph chain(n); chain.addPath([&] { vector<int> vs(n); std::iota(vs.begin(), vs.end(), 0); return vs; }());
        evenCheck(chain, vector<int>(n), "large chain even orientation");
        chain.addEdge(n - 1, 0); evenCheck(CsrGraph(chain), vector<int>(n), "large cycle CSR even orientation");
        int cols = 100, rows = mode == "quick" ? 10 : n / (2 * cols), grid = rows * cols; const lng W = 4'000'000'000'000'000'000;
        Graph local(n); vector<pair<int, lll>> hops, weights;
        auto add = [&](int u, int v, lll span) {
            int e = local.addEdge(u, v, W); hops.push_back({e, span}); weights.push_back({e, span == -1 ? -1 : span * W});};
        for (int r = 0; r < rows; ++r) { for (int c = 0; c < cols; ++c) {
            int u = r * cols + c;
            if (c + 1 < cols) { add(u, u + 1, u == 0 ? 1 : 3); }
            if (r + 1 < rows) { add(u, u + cols, 3); }
            if (u == 0) { add(0, 1, 1); local.addEdge(5, 5, W); }}}
        add(0, grid, -1);
        for (int v = grid + 1; v < n; ++v) { add(v - 1, v, -1); }
        context = "large grid " + std::to_string(rows) + "x" + std::to_string(cols) + " with chain tail n=" + std::to_string(n);
        check(localBridges(local) == hops && localBridges(CsrGraph(local)) == hops, "large grid unweighted spans");
        check(localBridges(local, true) == weights && localBridges(CsrGraph(local), true) == weights, "large grid weighted spans beyond 64 bits");
        Graph wheel(n);
        for (int v = 1; v + 1 < n; ++v) { wheel.addEdge(0, v); }
        for (int v = 1; v + 1 < n; ++v) { wheel.addEdge(v, v + 2 < n ? v + 1 : 1); }
        int pendant = wheel.addEdge(0, n - 1); vector<pair<int, lll>> only{{pendant, -1}};
        context = "large wheel n=" + std::to_string(n);
        check(localBridges(wheel) == only && localBridges(CsrGraph(wheel), true) == only, "large wheel triangles by degree ranking");
        // n parallel copies of 0-1 closing triangle 0-1-2: every edge lies in a triangle; one scan per distinct pair keeps it near-linear.
        Graph bundle(3);
        for (int i = 0; i < n; ++i) { bundle.addEdge(0, 1); }
        bundle.addEdge(1, 2); bundle.addEdge(0, 2);
        context = "parallel bundle m=" + std::to_string(n + 2);
        check(localBridges(bundle).empty() && localBridges(CsrGraph(bundle), true).empty(), "parallel bundle all in triangles");
        // n parallel copies of 0-1 beside the path 0-2-3-1 and no triangle: the copies share one search (weighted: least other copy or the path).
        Graph bare(4); vector<pair<int, lll>> unit, light;
        for (int i = 0; i < n; ++i) { int e = bare.addEdge(0, 1, 10 + i); unit.push_back({e, 1}); light.push_back({e, i ? 10 : 11}); }
        for (auto [u, v] : {pair(0, 2), pair(2, 3), pair(3, 1)}) { int e = bare.addEdge(u, v, 5); unit.push_back({e, 3}); light.push_back({e, 20}); }
        context = "bare parallel bundle m=" + std::to_string(n + 3);
        check(localBridges(bare) == unit && localBridges(CsrGraph(bare)) == unit, "bare bundle unweighted spans");
        check(localBridges(bare, true) == light && localBridges(CsrGraph(bare), true) == light, "bare bundle weighted spans");
        cout << "PASS large stream/orientation/local-bridge cases n=" << n << " stream m=" << m << " dense m=" << dense + 1 << '\n';}
    void invalid(const string &name) {
        Graph g(2, true);
        if (name == "lowlink-directed") { lowlink(g); }
        else if (name == "lowlink-csr-directed") { lowlink(CsrGraph(g)); }
        else if (name == "orientation-directed") { strongOrientation(g); }
        else if (name == "orientation-csr-directed") { strongOrientation(CsrGraph(g)); }
        else if (name == "stream-end") { BridgesStreaming(2).add(0, 2); }
        else if (name == "even-directed") { vector<int> arcs; evenOutdegreeOrientation(g, arcs); }
        else if (name == "local-directed") { localBridges(g); }
        else if (name == "local-negative") { Graph h(2); h.addEdge(0, 1, 0); h.addEdge(0, 1, -1); localBridges(h, true); }
        else if (name == "adjacency-endpoint") { lowlink(Graph::fromAdjacency(vector<vector<int>>{{2}, {}})); }
        else { throw std::runtime_error("unknown invalid probe " + name); }}
} // namespace

int main(int argc, char **argv) {
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); return 2; }}
        exhaustive(); randomCases(); boundaries(); fixtures(); largeOperations();
        cout << "PASS lowlink seed=" << seed << " mode=" << mode << " cases=" << cases << " checks=" << checks << '\n';} catch (const std::exception &e) {
        cerr << "FAIL lowlink seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1;}}
