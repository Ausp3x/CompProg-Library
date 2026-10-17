#pragma once

#include "../01-Core/01-template.hpp"
#include "01-graph.hpp"
#include "02-traversal.hpp"
#include "07-lca.hpp"
#include "09-bridges_articulation.hpp"
#include "../02-Data Structures/01-dsu.hpp"

// Extended block forest on n vertices: node v < n is a vertex, node n + b block b; tree edge (v, n + b) weighs |b|; roots are vertices.
// S: O(n + k), Q: O(1), O(log(n)) componentSizeAfterRemovingVertex, M: O(n + k); k blocks, removal queries assert x != u, v.
struct BlockCutTree {
    int n, k;
    Graph tree;
    EulerLCA lca;
    vector<int> size, start, child;

    BlockCutTree(int N, Graph t) : n(N), k(t.n - N), tree(std::move(t)), lca(tree), size(tree.n), start(tree.n + 1) {
        assert(0 <= n && n <= tree.n);
        for (int x : lca.postorder) {
            size[x] += x < n;
            if (lca.parent[x] != -1) { size[lca.parent[x]] += size[x]; ++start[lca.parent[x] + 1]; }}
        for (int x = 0; x < tree.n; ++x) { start[x + 1] += start[x]; }
        child.resize(start[tree.n]); vector<int> pos(start.begin(), start.end() - 1);
        for (int x : lca.order) {
            if (lca.parent[x] != -1) { child[pos[lca.parent[x]]++] = x; }}}

    void check(int u) const { assert(0 <= u && u < n); (void)u; }
    int blockOf(int v) const { check(v); return tree[v].size() == 1 ? tree.arcs[tree[v][0]].to - n : -1; }
    bool sameBlock(int u, int v) const {
        check(u); check(v); int a = lca.parent[u], b = lca.parent[v];
        return u == v || (a != -1 && (a == b || lca.parent[a] == v)) || (b != -1 && lca.parent[b] == u);}
    bool connectedAfterRemovingVertex(int u, int v, int x) const {
        check(u); check(v); check(x); assert(x != u && x != v);
        int l = lca.getLCA(u, v);
        return l != -1 && !(lca.covers(l, x) && (lca.covers(x, u) || lca.covers(x, v)));}
    int componentSizeAfterRemovingVertex(int v, int x) const {
        check(v); check(x); assert(v != x);
        int r = lca.component[v];
        if (lca.component[x] != r) { return size[r]; }
        if (!lca.covers(x, v)) { return size[r] - size[x]; }
        auto it = std::upper_bound(child.begin() + start[x], child.begin() + start[x + 1], lca.t_in[v],
            [&](int t, int c) { return t < lca.t_in[c]; });
        return size[*prev(it)];}
    // Interior vertex nodes of the forest path are exactly the vertices on every u-v path.
    int mandatoryVertexCount(int u, int v) const {
        check(u); check(v); int d = lca.distance(u, v);
        return d <= 0 ? d : d / 2 - 1;}
    int simplePathVertexCount(int u, int v) const {
        check(u); check(v); lll w = 0;
        if (!lca.weightedDistance(u, v, w)) { return 0; }
        return int(w / 2) - lca.distance(u, v) / 2 + 1;}
};

// T: O(n + m), M: O(n + m); undirected, loops and multiedges allowed; loop-only and isolated vertices get singleton blocks.
template<class G>
BlockCutTree blockCutTree(const G &g) {
    auto low = lowlink(g);
    vector<vector<int>> blocks;
    vector<uint8_t> has(g.n);
    for (int b = 0; b < int(low.block_vertices.size()); ++b) {
        auto &es = low.block_edges[b];
        if (es.size() == 1 && g.edges[es[0]].u == g.edges[es[0]].v) { continue; }
        for (int v : low.block_vertices[b]) { has[v] = 1; }
        blocks.push_back(std::move(low.block_vertices[b]));}
    for (int v = 0; v < g.n; ++v) {
        if (!has[v]) { blocks.push_back({v}); }}
    Graph t(g.n + int(blocks.size()));
    for (int b = 0; b < int(blocks.size()); ++b) {
        for (int v : blocks[b]) { t.addEdge(v, g.n + b, lng(blocks[b].size())); }}
    return BlockCutTree(g.n, std::move(t));}

// node[v] is v's 2-edge-connected component, tree edge i is bridge edge[i] (input weight), tree_edge maps back (-1 off bridges).
// S: O(n + m), Q: O(1), M: O(n + m); components rooted at their minimum node.
struct BridgeTree {
    Graph tree;
    vector<int> node, edge, tree_edge;
    EulerLCA lca;
    vector<int> size;

    BridgeTree(Graph t, vector<int> nd, vector<int> ed, vector<int> te) : tree(std::move(t)), node(std::move(nd)),
        edge(std::move(ed)), tree_edge(std::move(te)), lca(tree), size(tree.n) {
        for (int x : node) { assert(0 <= x && x < tree.n); ++size[x]; }
        for (int x : lca.postorder) {
            if (lca.parent[x] != -1) { size[lca.parent[x]] += size[x]; }}}

    int lower(int i) const { auto e = tree.edges[i]; return lca.parent[e.u] == e.v ? e.u : e.v; }
    bool connectedAfterRemovingEdge(int u, int v, int e) const {
        assert(0 <= u && u < int(node.size()) && 0 <= v && v < int(node.size()) && 0 <= e && e < int(tree_edge.size()));
        int a = node[u], b = node[v], i = tree_edge[e];
        return lca.component[a] == lca.component[b] && (i == -1 || lca.covers(lower(i), a) == lca.covers(lower(i), b));}
    int componentSizeAfterRemovingEdge(int v, int e) const {
        assert(0 <= v && v < int(node.size()) && 0 <= e && e < int(tree_edge.size()));
        int a = node[v], r = lca.component[a], i = tree_edge[e];
        if (i == -1 || lca.component[tree.edges[i].u] != r) { return size[r]; }
        return lca.covers(lower(i), a) ? size[lower(i)] : size[r] - size[lower(i)];}
};

// T: O(n + m), M: O(n + m); undirected, loops and multiedges allowed; bridges in ascending ID order.
template<class G>
BridgeTree bridgeTree(const G &g) {
    auto low = lowlink(g);
    Graph t(int(low.components.size()));
    vector<int> edge, tree_edge(g.edges.size(), -1);
    for (int e = 0; e < int(g.edges.size()); ++e) {
        if (!low.is_bridge[e]) { continue; }
        tree_edge[e] = t.addEdge(low.component[g.edges[e].u], low.component[g.edges[e].v], g.edges[e].w); edge.push_back(e);}
    return BridgeTree(std::move(t), std::move(low.component), std::move(edge), std::move(tree_edge));}

// T: O(n + m), M: O(n + m); undirected; ceil(p / 2) + q vertex pairs for p leaves and q isolated nodes of the bridge forest, none if one node.
template<class G>
vector<pair<int, int>> twoEdgeConnectivityAugmentation(const G &g) {
    auto bt = bridgeTree(g);
    int k = bt.tree.n;
    if (k <= 1) { return {}; }
    vector<int> rep(k), first, last;
    for (int v = 0; v < g.n; ++v) { rep[bt.node[v]] = v; }
    // Chain the components leaf to leaf (an isolated node is two slots) into one tree, then pair DFS-order leaves i and i + p / 2.
    for (int x : bt.lca.order) {
        if (bt.tree[x].size() > 1) { continue; }
        if (first.empty() || bt.lca.component[x] != bt.lca.component[first.back()]) { first.push_back(x); last.push_back(x); }
        else { last.back() = x; }}
    Graph c = bt.tree;
    vector<pair<int, int>> res;
    for (int i = 0; i + 1 < int(first.size()); ++i) { c.addEdge(last[i], first[i + 1]); res.push_back({rep[last[i]], rep[first[i + 1]]}); }
    vector<int> leaves;
    for (int x : dfs(c, {0}).order) {
        if (c[x].size() == 1) { leaves.push_back(x); }}
    int p = int(leaves.size()), h = p / 2;
    for (int i = 0; i < p - h; ++i) { res.push_back({rep[leaves[i]], rep[leaves[i + h]]}); }
    return res;}

// Vertices [0, n); extended block forest under edge insertions: vertex v, block node n + b; loops ignored.
// S: O(n), U: O(log(n)) amortized, Q: O(alpha(n)) amortized (alpha inverse Ackermann), M: O(n)
struct IncrementalBlockCutTree {
    int n, blocks = 0, iter = 0;
    vector<int> par, stamp, pa, pb;
    DSU merged, comp;

    explicit IncrementalBlockCutTree(int N) : n(N), par(2 * size_t(max(N, 0)), -1), stamp(par.size()), merged(max(N, 0)), comp(max(N, 0)) { assert(n >= 0); }

    int up(int x) { return x >= n ? par[x] : par[x] == -1 ? -1 : n + merged.findSet(par[x] - n); }
    void check(int u) const { assert(0 <= u && u < n); (void)u; }

    // A bridge links the smaller tree, rerooted at its endpoint; otherwise every block on the forest path merges into one.
    void addEdge(int u, int v) {
        check(u); check(v);
        if (u == v) { return; }
        if (!comp.isSameSet(u, v)) {
            if (comp.getSize(u) > comp.getSize(v)) { swap(u, v); }
            for (int x = u, last = -1; x != -1;) { int p = up(x); par[x] = last; last = x; x = p; }
            par[u] = n + blocks; par[n + blocks++] = v; comp.uniteSets(u, v);
            return;}
        ++iter; pa.clear(); pb.clear();
        int a = u, b = v, l = -1;
        while (l == -1) {
            if (a != -1) { if (stamp[a] == iter) { l = a; break; } stamp[a] = iter; pa.push_back(a); a = up(a); }
            if (b != -1) { if (stamp[b] == iter) { l = b; break; } stamp[b] = iter; pb.push_back(b); b = up(b); }}
        int top = l >= n ? par[l] : l, r = l >= n ? l - n : -1;
        for (auto *p : {&pa, &pb}) {
            for (int x : *p) {
                if (x == l) { break; }
                if (x >= n) { r = r == -1 ? x - n : (merged.uniteSets(r, x - n), merged.findSet(r)); }}}
        par[n + merged.findSet(r)] = top;}

    bool sameBlock(int u, int v) {
        check(u); check(v); int a = up(u), b = up(v);
        return u == v || (a != -1 && (a == b || par[a] == v)) || (b != -1 && par[b] == u);}
};
