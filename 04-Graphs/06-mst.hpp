#pragma once

#include "../01-Core/01-template.hpp"
#include "01-graph.hpp"
#include "04-dsu.hpp"

// Minimum spanning forest of an undirected multigraph; loops are excluded.
// Original logical edge IDs, exact signed 128-bit total, including negative weights.
// components includes isolated vertices; an empty graph has zero components.
struct SpanningForest { lll weight = 0; int components = 0; vector<int> edges; };

namespace mst_detail {
    template<class G>
    bool edgeLess(const G &g, int a, int b, bool maximum = false) {
        if (g.edges[a].w != g.edges[b].w) {
            return maximum ? g.edges[a].w > g.edges[b].w : g.edges[a].w < g.edges[b].w; }
        return a < b; }
    template<class G>
    vector<int> edgeOrder(const G &g, bool maximum = false) {
        vector<int> order(g.edges.size()); iota(order.begin(), order.end(), 0);
        sort(order.begin(), order.end(), [&](int a, int b) { return edgeLess(g, a, b, maximum); });
        return order; }
} // namespace mst_detail

// Graph/CSR inputs must be undirected. Equal weights break ties by edge ID.
// T: O(n + m * log(m + 1)), M: O(n + m), including returned forest.
template<class G>
SpanningForest kruskal(const G &g) {
    assert(!g.directed); DSU dsu(g.n); SpanningForest res; res.components = g.n;
    for (int id : mst_detail::edgeOrder(g)) {
        auto e = g.edges[id];
        if (dsu.uniteSets(e.u, e.v)) {
            res.weight += lll(e.w); res.edges.push_back(id); --res.components; }}
    return res; }

// A set holds at most one crossing-edge candidate per unused vertex.
// T: O((n + m) * log(n + 1)), M: O(n), including returned forest.
template<class G>
SpanningForest primSparse(const G &g) {
    assert(!g.directed); SpanningForest res;
    vector<bool> used(g.n); vector<int> best(g.n, -1);
    set<tuple<lng, int, int>> queue;
    for (int s = 0; s < g.n; ++s) {
        if (used[s]) { continue; }
        ++res.components; queue.emplace(0, -1, s);
        while (!queue.empty()) {
            auto [w, id, u] = *queue.begin(); queue.erase(queue.begin()); used[u] = true;
            if (id != -1) { res.weight += lll(w); res.edges.push_back(id); }
            for (int a : g[u]) {
                auto e = g.arcs[a]; int v = e.to;
                if (used[v] || (best[v] != -1 && !mst_detail::edgeLess(g, e.id, best[v]))) { continue; }
                if (best[v] != -1) { queue.erase({g.edges[best[v]].w, best[v], v}); }
                best[v] = e.id; queue.emplace(e.w, e.id, v); }} }
    return res; }

// Uses the existing dense minimum-edge view; preprocessing it is O(n^2 + m).
// T: O(n^2), M: O(n), including returned forest; caller owns the dense matrix.
inline SpanningForest primDense(const DenseGraph &dense) {
    const auto &g = dense.graph; assert(!g.directed); SpanningForest res;
    vector<bool> used(g.n); vector<int> best(g.n, -1);
    for (int step = 0; step < g.n; ++step) {
        int u = -1;
        for (int v = 0; v < g.n; ++v) {
            if (!used[v] && (u == -1 || (best[v] != -1 &&
                (best[u] == -1 || mst_detail::edgeLess(g, best[v], best[u]))))) { u = v; }}
        used[u] = true;
        if (best[u] == -1) { ++res.components; }
        else { res.weight += lll(g.edges[best[u]].w); res.edges.push_back(best[u]); }
        for (int v = 0; v < g.n; ++v) {
            int a = dense.best[u][v];
            if (used[v] || a == -1) { continue; }
            int id = g.arcs[a].id;
            if (best[v] == -1 || mst_detail::edgeLess(g, id, best[v])) { best[v] = id; }} }
    return res; }

// Representatives are cached before each edge scan; active components halve.
// T: O((n + m) * log(n + 1)), M: O(n), including returned forest.
template<class G>
SpanningForest boruvka(const G &g) {
    assert(!g.directed); DSU dsu(g.n); SpanningForest res; res.components = g.n;
    vector<int> component(g.n), best(g.n);
    while (true) {
        fill(best.begin(), best.end(), -1);
        for (int u = 0; u < g.n; ++u) { component[u] = dsu.findSet(u); }
        for (int id = 0; id < int(g.edges.size()); ++id) {
            auto e = g.edges[id]; int u = component[e.u], v = component[e.v];
            if (u == v) { continue; }
            if (best[u] == -1 || mst_detail::edgeLess(g, id, best[u])) { best[u] = id; }
            if (best[v] == -1 || mst_detail::edgeLess(g, id, best[v])) { best[v] = id; }}
        bool changed = false;
        for (int id : best) {
            if (id == -1) { continue; }
            auto e = g.edges[id];
            if (dsu.uniteSets(e.u, e.v)) {
                changed = true; res.weight += lll(e.w); res.edges.push_back(id); --res.components; }}
        if (!changed) { return res; }}}

// value/edge are meaningful only for connected, nonempty paths.
struct BottleneckResult { bool connected = false, empty = false; lng value = 0; int edge = -1; };

// Immutable owning union forest; n <= (INT_MAX + 1) / 2. Original vertices are
// leaves [0,n); each merge node has two children, its joining edge ID and weight.
// parent=-1 at roots; root[u] is its tree root; leaf_count counts original leaves.
// Leaf value=0 is unused. Public state must not be edited directly.
// Ascending construction answers minimax paths; maximum=true answers maximin.
// S: O(m * log(m + 1) + n * log(n + 1)), Q: O(log(n + 1)), M: O(n * log(n + 1))
// Construction workspace O(n + m); lca accepts all nodes, bottleneck original vertices.
struct KruskalReconstruction {
    int n;
    bool maximum;
    vector<array<int, 2>> child;
    vector<int> parent, edge, leaf_count, depth, root;
    vector<lng> value;
    vector<vector<int>> up;

    template<class G>
    explicit KruskalReconstruction(const G &g, bool maximum = false) : n(g.n), maximum(maximum) {
        assert(!g.directed && 0 <= n && 2 * lng(n) <= lng(INT_MAX) + 1);
        child.assign(n, {-1, -1}); parent.assign(n, -1); edge.assign(n, -1);
        value.assign(n, 0); leaf_count.assign(n, 1);
        DSU dsu(n); vector<int> top(n); iota(top.begin(), top.end(), 0);
        for (int id : mst_detail::edgeOrder(g, maximum)) {
            auto e = g.edges[id]; int u = dsu.findSet(e.u), v = dsu.findSet(e.v);
            if (u == v) { continue; }
            int a = top[u], b = top[v], node = int(child.size());
            child.push_back({a, b}); parent.push_back(-1); edge.push_back(id); value.push_back(e.w);
            leaf_count.push_back(leaf_count[a] + leaf_count[b]); parent[a] = parent[b] = node;
            dsu.uniteSets(u, v); top[dsu.findSet(u)] = node; }
        int count = int(child.size()), levels = max(1, int(std::bit_width(uint(count))));
        depth.resize(count); root.resize(count); up.assign(levels, vector<int>(count));
        // Parents always have larger IDs, so descending IDs replace a recursive DFS.
        for (int u = count - 1; u >= 0; --u) {
            int p = parent[u]; root[u] = p == -1 ? u : root[p]; depth[u] = p == -1 ? 0 : depth[p] + 1;
            up[0][u] = p == -1 ? u : p;
            for (int j = 1; j < levels; ++j) { up[j][u] = up[j - 1][up[j - 1][u]]; }}}

    // -1 across trees; otherwise the lowest common ancestor node (possibly a leaf).
    int lca(int u, int v) const {
        assert(0 <= u && u < int(child.size()) && 0 <= v && v < int(child.size()));
        if (root[u] != root[v]) { return -1; }
        if (depth[u] < depth[v]) { swap(u, v); }
        int delta = depth[u] - depth[v];
        for (int j = int(up.size()) - 1; j >= 0; --j) { if (delta >> j & 1) { u = up[j][u]; }}
        if (u == v) { return u; }
        for (int j = int(up.size()) - 1; j >= 0; --j) {
            if (up[j][u] != up[j][v]) { u = up[j][u]; v = up[j][v]; }}
        return up[0][u]; }
    BottleneckResult bottleneck(int u, int v) const {
        assert(0 <= u && u < n && 0 <= v && v < n);
        if (u == v) { return {true, true, 0, -1}; }
        int node = lca(u, v);
        return node == -1 ? BottleneckResult{} : BottleneckResult{true, false, value[node], edge[node]}; }
    // Node whose leaves are u's component using edges <= threshold (>= for maximum).
    int componentAt(int u, lng threshold) const {
        assert(0 <= u && u < n);
        for (int j = int(up.size()) - 1; j >= 0; --j) {
            int a = up[j][u];
            if (a >= n && (maximum ? value[a] >= threshold : value[a] <= threshold)) { u = a; }}
        return u; }
};
