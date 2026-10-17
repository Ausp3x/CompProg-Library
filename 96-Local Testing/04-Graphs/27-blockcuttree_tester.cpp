#include "../../04-Graphs/27-blockcuttree.hpp"

namespace {
    ulng seed = 20260927;
    string mode = "full", context;
    lng checks = 0, cases = 0;
    void check(bool ok, const string &operation) {
        ++checks;
        if (!ok) { throw std::runtime_error(context + " operation=" + operation + " expected=true actual=false"); }}
    string show(const Graph &g) {
        std::ostringstream out; out << "n=" << g.n << " edges=";
        for (auto e : g.edges) { out << e.u << '-' << e.v << ' '; }
        return out.str();}
    // Component labels of g without vertex sx and edge se (-1 for the removed vertex).
    vector<int> labels(const Graph &g, int sx = -1, int se = -1) {
        vector<int> lab(g.n, -1);
        for (int s = 0; s < g.n; ++s) {
            if (lab[s] != -1 || s == sx) { continue; }
            lab[s] = s; vector<int> q{s};
            for (int i = 0; i < int(q.size()); ++i) {
                for (int a : g[q[i]]) {
                    int y = g.arcs[a].to;
                    if (y != sx && g.arcs[a].id != se && lab[y] == -1) { lab[y] = s; q.push_back(y); }}}}
        return lab;}
    int sizeOf(const vector<int> &lab, int v) { return int(std::count(lab.begin(), lab.end(), lab[v])); }
    // Blocks by brute force: union the edges of every simple cycle (2-cycles of parallel edges included); singletons for vertices without non-loop edges.
    vector<vector<int>> bruteBlocks(const Graph &g) {
        int n = g.n, m = int(g.edges.size());
        vector<int> p(m); iota(p.begin(), p.end(), 0);
        auto f = [&](int x) { while (p[x] != x) { x = p[x]; } return x; };
        vector<int> path; vector<char> on(n), used(m);
        auto dfs = [&](auto &&self, int s, int x) -> void {
            for (int a : g[x]) {
                int y = g.arcs[a].to, e = g.arcs[a].id;
                if (used[e] || y == x) { continue; }
                if (y == s && path.size() >= 1) {
                    for (int q : path) { p[f(q)] = f(e); }
                    continue;}
                if (y < s || on[y]) { continue; }
                used[e] = 1; on[y] = 1; path.push_back(e);
                self(self, s, y);
                used[e] = 0; on[y] = 0; path.pop_back();}};
        for (int s = 0; s < n; ++s) { on[s] = 1; dfs(dfs, s, s); on[s] = 0; }
        map<int, set<int>> cls; vector<char> has(n);
        for (int e = 0; e < m; ++e) {
            if (g.edges[e].u == g.edges[e].v) { continue; }
            cls[f(e)].insert(g.edges[e].u); cls[f(e)].insert(g.edges[e].v); has[g.edges[e].u] = has[g.edges[e].v] = 1;}
        vector<vector<int>> res;
        for (auto &[k, s] : cls) { res.push_back(vector<int>(s.begin(), s.end())); }
        for (int v = 0; v < n; ++v) { if (!has[v]) { res.push_back({v}); }}
        sort(res.begin(), res.end());
        return res;}
    int simplePathBrute(const Graph &g, int u, int v) {
        vector<char> on(g.n), any(g.n); vector<int> path{u}; on[u] = 1;
        auto dfs = [&](auto &&self, int x) -> void {
            if (x == v) { for (int y : path) { any[y] = 1; } return; }
            for (int a : g[x]) {
                int y = g.arcs[a].to;
                if (on[y]) { continue; }
                on[y] = 1; path.push_back(y); self(self, y); path.pop_back(); on[y] = 0;}};
        dfs(dfs, u);
        return int(std::count(any.begin(), any.end(), 1));}
    bool bridgeless(const Graph &g) {
        auto base = labels(g);
        for (int v = 0; v < g.n; ++v) { if (base[v] != base[0]) { return false; }}
        for (int e = 0; e < int(g.edges.size()); ++e) {
            auto lab = labels(g, -1, e);
            if (lab[g.edges[e].u] != lab[g.edges[e].v]) { return false; }}
        return true;}
    bool sameBlockBrute(const Graph &g, const vector<int> &base, int u, int v) {
        if (u == v) { return true; }
        if (base[u] != base[v]) { return false; }
        for (int x = 0; x < g.n; ++x) {
            if (x == u || x == v) { continue; }
            auto lab = labels(g, x);
            if (lab[u] != lab[v]) { return false; }}
        return true;}
    void verifyBlockCut(const Graph &g) {
        int n = g.n; auto bc = blockCutTree(g); auto base = labels(g);
        check(bc.n == n && bc.tree.n == n + bc.k, "BlockCutTree shape");
        vector<vector<int>> blocks;
        for (int b = 0; b < bc.k; ++b) {
            vector<int> vs;
            for (int a : bc.tree[n + b]) { vs.push_back(bc.tree.arcs[a].to); check(bc.tree.arcs[a].w == lng(bc.tree[n + b].size()), "tree edge weight is block size"); }
            sort(vs.begin(), vs.end()); blocks.push_back(vs);}
        sort(blocks.begin(), blocks.end());
        check(blocks == bruteBlocks(g), "block vertex sets equal cycle-union blocks");
        auto cc = blockCutTree(CsrGraph(g));
        check(cc.k == bc.k && cc.tree.edges.size() == bc.tree.edges.size(), "CSR block-cut tree shape");
        for (int i = 0; i < int(bc.tree.edges.size()); ++i) {
            auto a = bc.tree.edges[i], b = cc.tree.edges[i];
            check(a.u == b.u && a.v == b.v && a.w == b.w, "CSR block-cut tree edges");}
        vector<vector<int>> cut(n);
        for (int x = 0; x < n; ++x) { cut[x] = labels(g, x); }
        for (int v = 0; v < n; ++v) {
            check(bc.lca.parent[v] == -1 || bc.lca.parent[v] >= n, "vertex parents are blocks");
            bool art = false;
            for (int a = 0; a < n; ++a) { for (int b = 0; b < n; ++b) { art |= a != v && b != v && base[a] == base[b] && cut[v][a] != cut[v][b]; }}
            int bo = bc.blockOf(v);
            check(art ? bo == -1 : 0 <= bo && bo < bc.k, "blockOf articulation sentinel");
            if (bo != -1) {
                bool in = false;
                for (int a : bc.tree[n + bo]) { in |= bc.tree.arcs[a].to == v; }
                check(in, "blockOf block contains the vertex");}}
        for (int u = 0; u < n; ++u) {
            for (int v = 0; v < n; ++v) {
                check(bc.sameBlock(u, v) == sameBlockBrute(g, base, u, v), "sameBlock(" + std::to_string(u) + "," + std::to_string(v) + ")");
                int mand = 0;
                for (int x = 0; x < n; ++x) {
                    if (x == u || x == v) { continue; }
                    bool conn = base[u] == base[v] && cut[x][u] == cut[x][v];
                    check(bc.connectedAfterRemovingVertex(u, v, x) == conn, "connectedAfterRemovingVertex(" + std::to_string(u) + "," + std::to_string(v) + "," + std::to_string(x) + ")");
                    mand += base[u] == base[v] && !conn;}
                check(bc.mandatoryVertexCount(u, v) == (base[u] != base[v] ? -1 : mand), "mandatoryVertexCount");
                check(bc.simplePathVertexCount(u, v) == simplePathBrute(g, u, v), "simplePathVertexCount(" + std::to_string(u) + "," + std::to_string(v) + ")");}
            for (int x = 0; x < n; ++x) {
                if (x != u) { check(bc.componentSizeAfterRemovingVertex(u, x) == sizeOf(cut[x], u), "componentSizeAfterRemovingVertex"); }}}}
    void verifyBridgeTree(const Graph &g) {
        int n = g.n, m = int(g.edges.size()); auto bt = bridgeTree(g); auto base = labels(g);
        vector<vector<int>> cut(m);
        vector<int> bridges;
        for (int e = 0; e < m; ++e) {
            cut[e] = labels(g, -1, e);
            if (cut[e][g.edges[e].u] != cut[e][g.edges[e].v]) { bridges.push_back(e); }}
        check(bt.edge == bridges && int(bt.tree_edge.size()) == m, "tree edges are the bridges in ID order");
        for (int e = 0; e < m; ++e) {
            int i = bt.tree_edge[e];
            if (i == -1) { check(!std::binary_search(bridges.begin(), bridges.end(), e), "tree_edge -1 off bridges"); continue; }
            auto t = bt.tree.edges[i];
            check(bt.edge[i] == e && t.w == g.edges[e].w && minmax(t.u, t.v) == minmax(bt.node[g.edges[e].u], bt.node[g.edges[e].v]), "tree edge joins the end components");}
        set<int> nodes(bt.node.begin(), bt.node.end());
        check(int(nodes.size()) == bt.tree.n, "every tree node is a component");
        for (int u = 0; u < n; ++u) {
            for (int v = 0; v < n; ++v) {
                bool two = base[u] == base[v];
                for (int e = 0; e < m && two; ++e) { two = cut[e][u] == cut[e][v]; }
                check((bt.node[u] == bt.node[v]) == two, "node is the 2-edge-connected component");
                for (int e = 0; e < m; ++e) { check(bt.connectedAfterRemovingEdge(u, v, e) == (cut[e][u] == cut[e][v]), "connectedAfterRemovingEdge"); }}
            for (int e = 0; e < m; ++e) { check(bt.componentSizeAfterRemovingEdge(u, e) == sizeOf(cut[e], u), "componentSizeAfterRemovingEdge"); }}
        auto add = twoEdgeConnectivityAugmentation(g); auto bc = bridgeTree(CsrGraph(g));
        check(bc.node == bt.node && bc.edge == bt.edge && bc.tree_edge == bt.tree_edge && twoEdgeConnectivityAugmentation(CsrGraph(g)) == add, "CSR bridge tree and augmentation agree");
        Graph h = g;
        for (auto [a, b] : add) { check(0 <= a && a < n && 0 <= b && b < n && a != b, "augmentation pair"); h.addEdge(a, b); }
        check(n == 0 || bridgeless(h), "augmentation makes the graph 2-edge-connected");
        int k = bt.tree.n, leaves = 0, isolated = 0;
        for (int x = 0; x < k; ++x) { leaves += bt.tree[x].size() == 1; isolated += bt.tree[x].empty(); }
        int expect = k <= 1 ? 0 : (leaves + 1) / 2 + isolated;
        check(int(add.size()) == expect, "augmentation count ceil(p / 2) + q");
        if (n <= 5 && expect > 0) {
            vector<pair<int, int>> pairs;
            for (int a = 0; a < n; ++a) { for (int b = a + 1; b < n; ++b) { pairs.push_back({a, b}); }}
            bool smaller = false; vector<int> pick;
            auto rec = [&](auto &&self, int from, int left) -> void {
                if (smaller) { return; }
                if (left == 0) {
                    Graph t = g;
                    for (int i : pick) { t.addEdge(pairs[i].first, pairs[i].second); }
                    smaller = bridgeless(t); return;}
                for (int i = from; i < int(pairs.size()); ++i) { pick.push_back(i); self(self, i, left - 1); pick.pop_back(); }};
            rec(rec, 0, expect - 1);
            check(!smaller, "no smaller augmentation exists (exhaustive multisets)");}}
    void verifyIncremental(std::mt19937_64 &rng, int n, int m) {
        IncrementalBlockCutTree ib(n); Graph g(n);
        for (int i = 0; i < m; ++i) {
            int u = int(rng() % n), v = rng() % 7 ? int(rng() % n) : u;
            if (i && rng() % 4 == 0) { u = g.edges[rng() % i].v; v = g.edges[rng() % i].u; }
            g.addEdge(u, v); ib.addEdge(u, v); context = show(g);
            auto base = labels(g);
            for (int x = 0; x < n; ++x) {
                for (int y = 0; y < n; ++y) { check(ib.sameBlock(x, y) == sameBlockBrute(g, base, x, y), "IncrementalBlockCutTree::sameBlock"); }}}}
    Graph randomGraph(std::mt19937_64 &rng, int n, int m) {
        Graph g(n);
        for (int i = 0; i < m; ++i) {
            int u = int(rng() % n), v = rng() % 7 ? int(rng() % n) : u;
            if (i && rng() % 5 == 0) { u = g.edges[rng() % i].v; v = g.edges[rng() % i].u; }
            g.addEdge(u, v, lng(rng() % 21) - 10);}
        return g;}
    void exhaustive() {
        int bound = mode == "quick" ? 4 : 5;
        for (int n = 0; n <= bound; ++n) {
            int pairs = n * (n - 1) / 2;
            for (int mask = 0; mask < (1 << pairs); ++mask) {
                Graph g(n); int i = 0;
                for (int u = 0; u < n; ++u) { for (int v = u + 1; v < n; ++v) { if (mask >> i++ & 1) { g.addEdge(u, v, i); }}}
                context = show(g); verifyBlockCut(g); verifyBridgeTree(g); ++cases;}}
        cout << "PASS every simple graph n<=" << bound << " cases=" << cases << '\n';}
    void randomCases() {
        std::mt19937_64 rng(seed);
        int rounds = mode == "quick" ? 300 : mode == "full" ? 3000 : 15000;
        for (int r = 0; r < rounds; ++r) {
            int n = 1 + int(rng() % 7), m = int(rng() % 11);
            Graph g = randomGraph(rng, n, m); context = show(g);
            verifyBlockCut(g); verifyBridgeTree(g);
            verifyIncremental(rng, 1 + int(rng() % 7), int(rng() % 12)); ++cases;}
        cout << "PASS random multigraphs with loops n<=7 rounds=" << rounds << " (static, bridge tree, augmentation, incremental)\n";}
    void large() {
        std::mt19937_64 rng(seed ^ 0x6d);
        int n = mode == "quick" ? 3000 : mode == "full" ? 100000 : 300000;
        for (int shape = 0; shape < 3; ++shape) {
            Graph g(n);
            if (shape == 0) {
                for (int v = 1; v < n; ++v) { g.addEdge(int(rng() % v), v); }
                for (int i = 0; i < n / 4; ++i) { int u = int(rng() % n); g.addEdge(u, min(n - 1, u + 1 + int(rng() % 6))); }}
            else if (shape == 1) { for (int v = 1; v < n; ++v) { g.addEdge(v - 1, v); }}
            else { for (int v = 1; v < n; ++v) { g.addEdge(v - 1, v); if (v % 3 == 0) { g.addEdge(v - 3, v); } } g.addEdge(0, 0); }
            context = "large shape=" + std::to_string(shape) + " n=" + std::to_string(n);
            auto bc = blockCutTree(g); auto bt = bridgeTree(g);
            IncrementalBlockCutTree ib(n);
            for (auto e : g.edges) { ib.addEdge(e.u, e.v); }
            for (int t = 0; t < 20000; ++t) {
                int u = int(rng() % n), v = t % 2 ? int(rng() % n) : min(n - 1, u + int(rng() % 4));
                check(bc.sameBlock(u, v) == ib.sameBlock(u, v), "large sameBlock static versus incremental");}
            for (int s = 0; s < 6; ++s) {
                int x = int(rng() % n); auto lab = labels(g, x);
                for (int t = 0; t < 3000; ++t) {
                    int u = int(rng() % n), v = int(rng() % n);
                    if (u != x && v != x) { check(bc.connectedAfterRemovingVertex(u, v, x) == (lab[u] == lab[v]), "large connectedAfterRemovingVertex"); }}
                map<int, int> cnt;
                for (int v = 0; v < n; ++v) { if (v != x) { ++cnt[lab[v]]; }}
                for (int v = 0; v < n; ++v) { if (v != x) { check(bc.componentSizeAfterRemovingVertex(v, x) == cnt[lab[v]], "large componentSizeAfterRemovingVertex"); }}
                int e = int(rng() % g.edges.size()); auto le = labels(g, -1, e);
                map<int, int> ce;
                for (int v = 0; v < n; ++v) { ++ce[le[v]]; }
                for (int t = 0; t < 3000; ++t) {
                    int u = int(rng() % n), v = int(rng() % n);
                    check(bt.connectedAfterRemovingEdge(u, v, e) == (le[u] == le[v]), "large connectedAfterRemovingEdge");
                    check(bt.componentSizeAfterRemovingEdge(u, e) == ce[le[u]], "large componentSizeAfterRemovingEdge");}}
            if (shape == 1) {
                check(bc.mandatoryVertexCount(0, n - 1) == n - 2 && bc.simplePathVertexCount(0, n - 1) == n, "path counts");
                check(int(twoEdgeConnectivityAugmentation(g).size()) == 1, "path needs one edge");}
            auto add = twoEdgeConnectivityAugmentation(g); Graph h = g;
            for (auto [a, b] : add) { h.addEdge(a, b); }
            auto low = lowlink(h);
            check(low.bridges.empty() && low.roots.size() == 1, "large augmentation leaves no bridge");}
        Graph cyc(n);
        for (int v = 0; v < n; ++v) { cyc.addEdge(v, (v + 1) % n); }
        auto bc = blockCutTree(cyc); context = "cycle n=" + std::to_string(n);
        check(bc.k == 1 && bc.simplePathVertexCount(0, n / 2) == n && bc.mandatoryVertexCount(0, n / 2) == 0, "cycle is one block");
        check(twoEdgeConnectivityAugmentation(cyc).empty(), "cycle needs nothing");
        Graph empty(n); context = "edgeless n=" + std::to_string(n);
        check(int(twoEdgeConnectivityAugmentation(empty).size()) == n && blockCutTree(empty).k == n, "edgeless: n singleton blocks, n added edges");
        cout << "PASS large n=" << n << " shapes versus deletion BFS, incremental, lowlink; cycle and edgeless\n";}
    void invalid(const string &name) {
        Graph g(3); g.addEdge(0, 1); g.addEdge(1, 2);
        if (name == "directed") { blockCutTree(Graph(2, true)); }
        else if (name == "bridge-directed") { bridgeTree(Graph(2, true)); }
        else if (name == "vertex-range") { blockCutTree(g).sameBlock(0, 3); }
        else if (name == "removed-endpoint") { blockCutTree(g).connectedAfterRemovingVertex(0, 1, 1); }
        else if (name == "size-removed") { blockCutTree(g).componentSizeAfterRemovingVertex(2, 2); }
        else if (name == "edge-range") { bridgeTree(g).connectedAfterRemovingEdge(0, 1, 2); }
        else if (name == "incremental-range") { IncrementalBlockCutTree(2).addEdge(0, 2); }
        else { throw std::runtime_error("unknown invalid probe " + name); }}
} // namespace

int main(int argc, char **argv) {
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); return 2; }}
        IncrementalBlockCutTree none(0); (void)none;
        exhaustive(); randomCases(); large();
        cout << "PASS blockcuttree seed=" << seed << " mode=" << mode << " cases=" << cases << " checks=" << checks << '\n';}
    catch (const std::exception &e) {
        cerr << "FAIL blockcuttree seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1;}}
