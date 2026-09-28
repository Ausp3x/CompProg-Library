#pragma once
#include "../01-Core/01-template.hpp"
#include "01-graph.hpp"

// Undirected Graph/CsrGraph; weights ignored, edge/arc IDs refer to input.
// component partitions vertices after deleting bridges (IDs by minimum vertex).
// block_edges partitions edges into vertex-biconnected blocks: bridges are
// dyads, each loop is a separate block, isolated vertices have an empty block.
// block_vertices has unique vertices; articulation vertices may appear in many
// blocks. Loop blocks do NOT imply articulation. Block/bridge order is DFS order.
// The total number of vertex blocks must fit int (isolates and loops count).
// parent_arc=-1 at roots; tin is a permutation of [0,n), low is DFS lowlink.
// orientation[e] selects one input arc: tree edges down, non-tree edges up.
// T: O(1), M: O(n + m) for a completed result; storage owns all returned arrays.
struct LowlinkResult {
    vector<int> tin, low, parent_arc, roots, bridges, component, edge_block, orientation;
    vector<bool> is_bridge, is_art;
    vector<vector<int>> components, block_edges, block_vertices;
};

// T: O(n + m), M: O(n + m), including output. Iterative; no recursion limit.
template<class G>
LowlinkResult lowlink(const G &g) {
    assert(!g.directed);
    LowlinkResult out; int n = g.n, m = int(g.edges.size()), timer = 0;
    out.tin.assign(n, -1); out.low.resize(n); out.parent_arc.assign(n, -1);
    out.is_art.assign(n, false); out.is_bridge.assign(m, false);
    out.edge_block.assign(m, -1); out.orientation.assign(m, -1);
    vector<int> pos(n), children(n), path, edges, marked(n, -1);
    auto block = [&](int last) {
        assert(out.block_edges.size() < size_t(INT_MAX));
        int b = int(out.block_edges.size()); out.block_edges.emplace_back(); out.block_vertices.emplace_back();
        int e;
        do {
            e = edges.back(); edges.pop_back(); out.edge_block[e] = b; out.block_edges.back().push_back(e);
            for (int v : {g.edges[e].u, g.edges[e].v}) {
                if (marked[v] != b) { marked[v] = b; out.block_vertices.back().push_back(v); }}
        } while (e != last);
    };
    for (int s = 0; s < n; ++s) {
        if (out.tin[s] != -1) { continue; }
        out.roots.push_back(s); out.tin[s] = out.low[s] = timer++; path.push_back(s);
        if (g[s].empty()) {
            assert(out.block_edges.size() < size_t(INT_MAX));
            out.block_edges.emplace_back(); out.block_vertices.push_back({s}); }
        while (!path.empty()) {
            int u = path.back(), p = out.parent_arc[u];
            if (pos[u] == int(g[u].size())) {
                path.pop_back();
                if (p == -1) { out.is_art[u] = children[u] > 1; continue; }
                int v = g.arcs[p].from, e = g.arcs[p].id;
                out.low[v] = min(out.low[v], out.low[u]);
                if (out.low[u] > out.tin[v]) { out.is_bridge[e] = true; out.bridges.push_back(e); }
                if (out.low[u] >= out.tin[v]) {
                    if (out.parent_arc[v] != -1) { out.is_art[v] = true; }
                    block(e); }
                continue; }
            int a = g[u][pos[u]++], v = g.arcs[a].to, e = g.arcs[a].id;
            if (p != -1 && a == g.arcs[p].rev) { continue; }
            if (u == v) {
                if (out.orientation[e] == -1) { out.orientation[e] = a; edges.push_back(e); block(e); }
                continue; }
            if (out.tin[v] == -1) {
                ++children[u]; out.orientation[e] = a; edges.push_back(e);
                out.parent_arc[v] = a; out.tin[v] = out.low[v] = timer++; path.push_back(v); }
            else if (out.tin[v] < out.tin[u]) {
                out.orientation[e] = a; edges.push_back(e); out.low[u] = min(out.low[u], out.tin[v]); }}}
    out.component.assign(n, -1);
    for (int s = 0; s < n; ++s) {
        if (out.component[s] != -1) { continue; }
        out.component[s] = int(out.components.size()); out.components.push_back({s});
        auto &group = out.components.back();
        for (int i = 0; i < int(group.size()); ++i) {
            for (int a : g[group[i]]) {
                int v = g.arcs[a].to;
                if (!out.is_bridge[g.arcs[a].id] && out.component[v] == -1) {
                    out.component[v] = out.component[s]; group.push_back(v); }}}}
    return out; }

// arcs[e] orients original edge e. Its SCCs are exactly component/groups,
// minimizing SCC count over every orientation, even when ok=false.
// ok iff connected and bridgeless; the empty graph succeeds by convention.
// bridge is a bridge ID or -1; disconnected contains roots of distinct original
// components, or {-1,-1}. Both obstruction witnesses may be present on failure.
// T: O(1), M: O(n + m).
struct StrongOrientationResult {
    bool ok = true;
    int count = 0, bridge = -1;
    pair<int, int> disconnected = {-1, -1};
    vector<int> arcs, component;
    vector<vector<int>> groups;
};

// T: O(n + m), M: O(n + m), including output and transient lowlink result.
template<class G>
StrongOrientationResult strongOrientation(const G &g) {
    auto low = lowlink(g); StrongOrientationResult out;
    if (!low.bridges.empty()) { out.ok = false; out.bridge = low.bridges[0]; }
    if (low.roots.size() > 1) { out.ok = false; out.disconnected = {low.roots[0], low.roots[1]}; }
    out.count = int(low.components.size()); out.arcs = std::move(low.orientation);
    out.component = std::move(low.component); out.groups = std::move(low.components);
    return out; }

namespace lowlink_detail {
    // Legacy symmetric adjacency: n>=0, at least n lists, endpoints in [0,n).
    // Nonloop multiplicities must match in both directions (unchecked). Loops are ignored:
    // they affect neither bridges/articulation nor DFS lowlink values.
    inline Graph graph(int n, const vector<vector<int>> &adj) {
        assert(n >= 0 && adj.size() >= size_t(n)); Graph g(n);
        for (int u = 0; u < n; ++u) { for (int v : adj[u]) {
            assert(0 <= v && v < n); if (u < v) { g.addEdge(u, v); }}}
        return g; }
} // namespace lowlink_detail

// Legacy constructor/results, zero-based symmetric adjacency; diagnostic DFS
// numbering can differ from the old recursive traversal. See 93-lowlink.md.
// S: O(n + m), Q: O(1), M: O(n); construction workspace O(n + m).
struct Tarjan {
    int n, timer;
    vector<bool> vst, is_art;
    vector<int> t_in, low;
    vector<pair<int, int>> bridges;

    Tarjan(int N, const vector<vector<int>> &adj) : n(N), timer(N) {
        auto g = lowlink_detail::graph(n, adj); auto out = lowlink(g);
        vst.assign(n, true); is_art = std::move(out.is_art);
        t_in = std::move(out.tin); low = std::move(out.low);
        for (int e : out.bridges) { auto [u, v, w] = g.edges[e]; bridges.emplace_back(min(u, v), max(u, v)); } }
};

// Legacy ordered bridge set. dfs(cur,prv,adj) is retained as a full recomputation
// hook: cur is a vertex, prv is -1 or a vertex; manual partial DFS state is unsupported.
// S: O(n + m + b * log(b + 1)), Q: O(1), M: O(n), b = number of bridges;
// construction/dfs workspace O(n + m); dfs has the same time as construction.
struct BridgeAlgo {
    int n, timer = 0;
    vector<bool> vst;
    vector<int> t_in, low;
    set<pair<int, int>> bridges;

    BridgeAlgo(int N, const vector<vector<int>> &adj) : n(N) { build(adj); }
    void build(const vector<vector<int>> &adj) {
        Tarjan out(n, adj); timer = out.timer; vst = std::move(out.vst);
        t_in = std::move(out.t_in); low = std::move(out.low);
        bridges.clear(); bridges.insert(out.bridges.begin(), out.bridges.end()); }
    void dfs(int cur, int prv, const vector<vector<int>> &adj) {
        assert(0 <= cur && cur < n && -1 <= prv && prv < n); build(adj); }
};
