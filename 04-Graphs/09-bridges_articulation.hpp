#pragma once
#include "../01-Core/01-template.hpp"
#include "01-graph.hpp"

// T: O(1), M: O(n + m); blocks are vertex-biconnected: bridges dyads, loops alone, isolates empty.
struct LowlinkResult {
    vector<int> tin, low, parent_arc, roots, bridges, component, edge_block, orientation, cut_components;
    vector<bool> is_bridge, is_art;
    vector<vector<int>> components, block_edges, block_vertices;
};

// T: O(n + m), M: O(n + m) including output; undirected Graph/CsrGraph, loops and multiedges allowed.
template<class G>
LowlinkResult lowlink(const G &g) {
    assert(!g.directed);
    LowlinkResult out; int n = g.n, m = int(g.edges.size()), timer = 0;
    out.tin.assign(n, -1); out.low.resize(n); out.parent_arc.assign(n, -1);
    out.is_art.assign(n, false); out.is_bridge.assign(m, false);
    out.edge_block.assign(m, -1); out.orientation.assign(m, -1);
    vector<int> pos(n), cuts(n), path, edges, marked(n, -1);
    auto block = [&](int last) {
        assert(out.block_edges.size() < size_t(INT_MAX));
        int b = int(out.block_edges.size()); out.block_edges.emplace_back(); out.block_vertices.emplace_back();
        for (int e = -1; e != last;) {
            e = edges.back(); edges.pop_back(); out.edge_block[e] = b; out.block_edges.back().push_back(e);
            for (int v : {g.edges[e].u, g.edges[e].v}) {
                if (marked[v] != b) { marked[v] = b; out.block_vertices.back().push_back(v); }}}};
    for (int s = 0; s < n; ++s) {
        if (out.tin[s] != -1) { continue; }
        out.roots.push_back(s); out.tin[s] = out.low[s] = timer++; path.push_back(s);
        if (g[s].empty()) {
            assert(out.block_edges.size() < size_t(INT_MAX));
            out.block_edges.emplace_back(); out.block_vertices.push_back({s});}
        while (!path.empty()) {
            int u = path.back(), p = out.parent_arc[u];
            if (pos[u] == int(g[u].size())) {
                path.pop_back();
                if (p == -1) { continue; }
                int v = g.arcs[p].from, e = g.arcs[p].id;
                out.low[v] = min(out.low[v], out.low[u]);
                if (out.low[u] > out.tin[v]) { out.is_bridge[e] = true; out.bridges.push_back(e); }
                if (out.low[u] >= out.tin[v]) { ++cuts[v]; block(e); }
                continue;}
            int a = g[u][pos[u]++], v = g.arcs[a].to, e = g.arcs[a].id;
            if (p != -1 && a == g.arcs[p].rev) { continue; }
            if (u == v) {
                if (out.orientation[e] == -1) { out.orientation[e] = a; edges.push_back(e); block(e); }
                continue;}
            if (out.tin[v] == -1) {
                out.orientation[e] = a; edges.push_back(e);
                out.parent_arc[v] = a; out.tin[v] = out.low[v] = timer++; path.push_back(v);}
            else if (out.tin[v] < out.tin[u]) {
                out.orientation[e] = a; edges.push_back(e); out.low[u] = min(out.low[u], out.tin[v]);}}}
    int c = int(out.roots.size()); out.cut_components.resize(n);
    for (int u = 0; u < n; ++u) {
        bool root = out.parent_arc[u] == -1;
        out.is_art[u] = cuts[u] > root; out.cut_components[u] = c - root + cuts[u];}
    out.component.assign(n, -1);
    for (int s = 0; s < n; ++s) {
        if (out.component[s] != -1) { continue; }
        out.component[s] = int(out.components.size()); out.components.push_back({s});
        auto &group = out.components.back();
        for (int i = 0; i < int(group.size()); ++i) {
            for (int a : g[group[i]]) {
                int v = g.arcs[a].to;
                if (!out.is_bridge[g.arcs[a].id] && out.component[v] == -1) {
                    out.component[v] = out.component[s]; group.push_back(v);}}}}
    return out;}

// T: O(1), M: O(n + m); arcs[e] orients edge e with minimum SCC count; ok iff connected and bridgeless.
struct StrongOrientationResult {
    bool ok = true;
    int count = 0, bridge = -1;
    pair<int, int> disconnected = {-1, -1};
    vector<int> arcs, component;
    vector<vector<int>> groups;
};

// T: O(n + m), M: O(n + m) including output and the transient lowlink result.
template<class G>
StrongOrientationResult strongOrientation(const G &g) {
    auto low = lowlink(g); StrongOrientationResult out;
    if (!low.bridges.empty()) { out.ok = false; out.bridge = low.bridges[0]; }
    if (low.roots.size() > 1) { out.ok = false; out.disconnected = {low.roots[0], low.roots[1]}; }
    out.count = int(low.components.size()); out.arcs = std::move(low.orientation);
    out.component = std::move(low.component); out.groups = std::move(low.components);
    return out;}

namespace lowlink_detail {
    // T: O(n + m), M: O(n + m); legacy symmetric adjacency with trusted multiplicities, loops dropped.
    inline Graph graph(int n, const vector<vector<int>> &adj) {
        assert(n >= 0 && adj.size() >= size_t(n)); Graph g(n);
        for (int u = 0; u < n; ++u) { for (int v : adj[u]) {
            assert(0 <= v && v < n); if (u < v) { g.addEdge(u, v); }}}
        return g;}
} // namespace lowlink_detail

// Legacy adapter on zero-based symmetric adjacency; DFS numbering may differ from the old recursion.
// S: O(n + m), Q: O(1), M: O(n) (workspace O(n + m)).
struct Tarjan {
    int n, timer;
    vector<bool> vst, is_art;
    vector<int> t_in, low;
    vector<pair<int, int>> bridges;

    Tarjan(int N, const vector<vector<int>> &adj) : n(N), timer(N) {
        auto g = lowlink_detail::graph(n, adj); auto out = lowlink(g);
        vst.assign(n, true); is_art = std::move(out.is_art);
        t_in = std::move(out.tin); low = std::move(out.low);
        for (int e : out.bridges) { auto [u, v, w] = g.edges[e]; bridges.emplace_back(min(u, v), max(u, v)); }}
};

// Legacy ordered bridge set; dfs(cur, prv, adj) is a full recomputation like build(adj).
// S: O(n + m + b * log(b + 1)), Q: O(1), M: O(n) (workspace O(n + m)); b is the bridge count.
struct BridgeAlgo {
    int n, timer = 0;
    vector<bool> vst;
    vector<int> t_in, low;
    set<pair<int, int>> bridges;

    BridgeAlgo(int N, const vector<vector<int>> &adj) : n(N) { build(adj); }
    void build(const vector<vector<int>> &adj) {
        Tarjan out(n, adj); timer = out.timer; vst = std::move(out.vst);
        t_in = std::move(out.t_in); low = std::move(out.low);
        bridges.clear(); bridges.insert(out.bridges.begin(), out.bridges.end());}
    void dfs(int cur, int prv, const vector<vector<int>> &adj) {
        assert(0 <= cur && cur < n && -1 <= prv && prv < n); build(adj);}
};
