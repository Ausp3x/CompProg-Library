#pragma once
#include "../01-Core/01-template.hpp"
#include "01-graph.hpp"

// Graph/CsrGraph inputs; graph weights are ignored. Witness arc indices refer to
// the unchanged input graph. A present cycle is simple except its repeated end:
// vertices.size() == arcs.size() + 1, vertices.front() == vertices.back().
// Self-loops have one arc; two distinct parallel undirected edges may form a cycle.
// T: O(1), M: O(n) for a witness on n vertices; empty arcs means no cycle.
struct CycleWitness {
    vector<int> vertices, arcs;
};

// T: O(n), M: O(n). Unreached parent/parent_arc/depth/root entries are -1.
// Roots have depth 0, parent/parent_arc -1 and root equal to their own vertex.
// order is discovery order; DFS also supplies postorder and its first cycle.
// BFS leaves postorder/cycle empty. Returned arrays remain valid after graph
// mutation, but their arc references require unchanged arc numbering/content.
struct TraversalResult {
    vector<int> order, parent, parent_arc, depth, root, postorder;
    CycleWitness cycle;
    explicit TraversalResult(int n) {
        assert(n >= 0);
        parent.assign(n, -1); parent_arc.assign(n, -1);
        depth.assign(n, -1); root.assign(n, -1); }
};

namespace traversal_detail {
    // Tree path from the LCA to a.from, then a, then back to the LCA. In a
    // directed DFS the destination is an ancestor, so no reversed arc is used.
    template<class G>
    CycleWitness treeCycle(const G &g, const vector<int> &parent_arc,
                           const vector<int> &depth, int a) {
        int u = g.arcs[a].from, v = g.arcs[a].to;
        vector<int> left, right;
        while (u != v) {
            if (depth[u] >= depth[v]) {
                int b = parent_arc[u]; left.push_back(b); u = g.arcs[b].from; }
            else {
                int b = parent_arc[v]; right.push_back(g.arcs[b].rev); v = g.arcs[b].from; }}
        reverse(left.begin(), left.end()); left.push_back(a);
        left.insert(left.end(), right.begin(), right.end());
        CycleWitness out; out.arcs = std::move(left);
        out.vertices.push_back(g.arcs[out.arcs[0]].from);
        for (int b : out.arcs) { out.vertices.push_back(g.arcs[b].to); }
        return out; }
} // namespace traversal_detail

// T: O(n + m + k), M: O(n), including output; m is arc count, k source count.
// Multi-source unweighted shortest distances. All sources in [0,n) are seeded
// before traversal; duplicate sources are ignored. Ties follow source/arc order.
template<class G>
TraversalResult bfs(const G &g, const vector<int> &sources) {
    TraversalResult out(g.n);
    for (int s : sources) {
        assert(0 <= s && s < g.n);
        if (out.depth[s] != -1) { continue; }
        out.depth[s] = 0; out.root[s] = s; out.order.push_back(s); }
    for (int i = 0; i < int(out.order.size()); ++i) {
        int u = out.order[i];
        for (int a : g[u]) {
            int v = g.arcs[a].to;
            if (out.depth[v] != -1) { continue; }
            out.parent[v] = u; out.parent_arc[v] = a;
            out.depth[v] = out.depth[u] + 1; out.root[v] = out.root[u];
            out.order.push_back(v); }}
    return out; }

// T: O(n + m), M: O(n), including output; s must be in [0,n).
template<class G>
TraversalResult bfs(const G &g, int s) { return bfs(g, vector<int>{s}); }

// T: O(n + m + k), M: O(n), including output. Iterative recursive-order DFS;
// sources in [0,n) are considered sequentially, skipping already reached ones.
// Thus depth is tree depth, not shortest distance. Directed cycles use gray
// ancestors; undirected DFS skips only the exact reverse of the parent arc.
template<class G>
TraversalResult dfs(const G &g, const vector<int> &sources) {
    TraversalResult out(g.n);
    vector<int> pos(g.n), path; vector<uint8_t> state(g.n);
    for (int s : sources) {
        assert(0 <= s && s < g.n);
        if (state[s]) { continue; }
        out.depth[s] = 0; out.root[s] = s;
        state[s] = 1; path.push_back(s); out.order.push_back(s);
        while (!path.empty()) {
            int u = path.back();
            if (pos[u] == int(g[u].size())) {
                state[u] = 2; out.postorder.push_back(u); path.pop_back(); continue; }
            int a = g[u][pos[u]++], v = g.arcs[a].to;
            if (!g.directed && out.parent_arc[u] != -1 && a == g.arcs[out.parent_arc[u]].rev) { continue; }
            if (!state[v]) {
                out.parent[v] = u; out.parent_arc[v] = a;
                out.depth[v] = out.depth[u] + 1; out.root[v] = out.root[u];
                state[v] = 1; path.push_back(v); out.order.push_back(v); }
            else if (state[v] == 1 && out.cycle.arcs.empty()) {
                out.cycle = traversal_detail::treeCycle(g, out.parent_arc, out.depth, a); }}}
    return out; }

// T: O(n + m), M: O(n), including output; s must be in [0,n).
template<class G>
TraversalResult dfs(const G &g, int s) { return dfs(g, vector<int>{s}); }

// T: O(n + m), M: O(n), including output. Roots considered in vertex order.
template<class G>
TraversalResult dfsForest(const G &g) {
    vector<int> sources(g.n); iota(sources.begin(), sources.end(), 0);
    return dfs(g, sources); }

// T: O(n + m), M: O(n), including returned cycle; directed or undirected.
template<class G>
CycleWitness findCycle(const G &g) { return dfsForest(g).cycle; }

// T: O(1), M: O(n). IDs follow increasing component minimum vertex.
struct ComponentsResult {
    int count = 0;
    vector<int> id;
    vector<vector<int>> groups;
};

// T: O(n + m), M: O(n), including output. Requires an undirected graph.
template<class G>
ComponentsResult connectedComponents(const G &g) {
    assert(!g.directed);
    ComponentsResult out; out.id.assign(g.n, -1);
    for (int s = 0; s < g.n; ++s) {
        if (out.id[s] != -1) { continue; }
        out.groups.push_back({s}); out.id[s] = out.count++;
        auto &group = out.groups.back();
        for (int i = 0; i < int(group.size()); ++i) {
            for (int a : g[group[i]]) {
                int v = g.arcs[a].to;
                if (out.id[v] == -1) { out.id[v] = out.id[s]; group.push_back(v); }}}}
    return out; }

// T: O(1), M: O(n). On success color is a complete 0/1 coloring; on failure
// it is partial (-1 denotes unvisited) and cycle is an odd-cycle certificate.
struct BipartiteResult {
    bool ok = true;
    vector<int> color;
    CycleWitness cycle;
};

// T: O(n + m), M: O(n), including output. Requires an undirected graph.
template<class G>
BipartiteResult bipartiteCheck(const G &g) {
    assert(!g.directed);
    BipartiteResult out; out.color.assign(g.n, -1);
    vector<int> parent_arc(g.n, -1), depth(g.n), queue;
    for (int s = 0; s < g.n; ++s) {
        if (out.color[s] != -1) { continue; }
        out.color[s] = 0; queue.clear(); queue.push_back(s);
        for (int i = 0; i < int(queue.size()); ++i) {
            int u = queue[i];
            for (int a : g[u]) {
                int v = g.arcs[a].to;
                if (out.color[v] == -1) {
                    out.color[v] = out.color[u] ^ 1; parent_arc[v] = a;
                    depth[v] = depth[u] + 1; queue.push_back(v); }
                else if (out.color[v] == out.color[u]) {
                    out.ok = false; out.cycle = traversal_detail::treeCycle(g, parent_arc, depth, a);
                    return out; }}}}
    return out; }
