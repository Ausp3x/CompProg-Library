#pragma once
#include "../01-Core/01-template.hpp"
#include "01-graph.hpp"

// T: O(1), M: O(n); closed walk vertices[0..k] along arcs[0..k), both empty when absent.
struct CycleWitness {
    vector<int> vertices, arcs;
};

// T: O(n), M: O(n); unreached parent, parent_arc, depth and root entries are -1.
struct TraversalResult {
    vector<int> order, parent, parent_arc, depth, root, postorder;
    CycleWitness cycle;

    explicit TraversalResult(int n) {
        assert(n >= 0);
        parent.assign(n, -1); parent_arc.assign(n, -1);
        depth.assign(n, -1); root.assign(n, -1);}
};

namespace traversal_detail {
    // T: O(n), M: O(n); tree path from the LCA of arc a's ends to a.from, then a, then back.
    template<class G = Graph>
    CycleWitness treeCycle(const G &g, const vector<int> &parent_arc,
                           const vector<int> &depth, int a) {
        int u = g.arcs[a].from, v = g.arcs[a].to;
        vector<int> left, right;
        while (u != v) {
            if (depth[u] >= depth[v]) {
                int b = parent_arc[u]; left.push_back(b); u = g.arcs[b].from;}
            else {
                int b = parent_arc[v]; right.push_back(g.arcs[b].rev); v = g.arcs[b].from;}}
        reverse(left.begin(), left.end()); left.push_back(a);
        left.insert(left.end(), right.begin(), right.end());
        CycleWitness out; out.arcs = std::move(left);
        out.vertices.push_back(g.arcs[out.arcs[0]].from);
        for (int b : out.arcs) { out.vertices.push_back(g.arcs[b].to); }
        return out;}
} // namespace traversal_detail

// T: O(n + m + k), M: O(n); k sources seeded first in order, ties follow source then arc order.
template<class G = Graph>
TraversalResult bfs(const G &g, const vector<int> &sources) {
    TraversalResult out(g.n);
    for (int s : sources) {
        assert(0 <= s && s < g.n);
        if (out.depth[s] != -1) { continue; }
        out.depth[s] = 0; out.root[s] = s; out.order.push_back(s);}
    for (int i = 0; i < int(out.order.size()); ++i) {
        int u = out.order[i];
        for (int a : g[u]) {
            int v = g.arcs[a].to;
            if (out.depth[v] != -1) { continue; }
            out.parent[v] = u; out.parent_arc[v] = a;
            out.depth[v] = out.depth[u] + 1; out.root[v] = out.root[u];
            out.order.push_back(v);}}
    return out;}

// T: O(n + m + k), M: O(n); iterative recursive-order DFS from each unreached source in turn.
template<class G = Graph>
TraversalResult dfs(const G &g, const vector<int> &sources) {
    TraversalResult out(g.n);
    vector<int> pos(g.n), path; vector<uint8_t> state(g.n);
    for (int s : sources) {
        assert(0 <= s && s < g.n);
        if (state[s]) { continue; }
        out.depth[s] = 0; out.root[s] = s;
        state[s] = 1; path.push_back(s); out.order.push_back(s);
        while (!path.empty()) {
            int u = path.back(), next = -1;
            int skip = g.directed || out.parent_arc[u] == -1 ? -1 : g.arcs[out.parent_arc[u]].rev;
            const auto &adj = g[u];
            while (next == -1 && pos[u] < int(adj.size())) {
                int a = adj[pos[u]++], v = g.arcs[a].to;
                if (a == skip) { continue; }
                if (!state[v]) {
                    out.parent[v] = u; out.parent_arc[v] = a;
                    out.depth[v] = out.depth[u] + 1; out.root[v] = out.root[u];
                    state[v] = 1; path.push_back(v); out.order.push_back(v); next = v;}
                else if (state[v] == 1 && out.cycle.arcs.empty()) {
                    out.cycle = traversal_detail::treeCycle(g, out.parent_arc, out.depth, a);}}
            if (next == -1) {
                state[u] = 2; out.postorder.push_back(u); path.pop_back();}}}
    return out;}

// T: O(n + m), M: O(n); DFS from every vertex in order, and its first cycle.
template<class G = Graph>
TraversalResult dfsForest(const G &g) {
    vector<int> sources(g.n); iota(sources.begin(), sources.end(), 0);
    return dfs(g, sources);}

// T: O(1), M: O(n); components numbered by increasing minimum vertex.
struct ComponentsResult {
    int count = 0;
    vector<int> id;
    vector<vector<int>> groups;
};

// T: O(n + m), M: O(n); undirected only.
template<class G = Graph>
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
    return out;}

// T: O(1), M: O(n); on failure color is partial (-1 unvisited) and cycle is odd.
struct BipartiteResult {
    bool ok = true;
    vector<int> color;
    CycleWitness cycle;
};

// T: O(n + m), M: O(n); undirected only.
template<class G = Graph>
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
                    depth[v] = depth[u] + 1; queue.push_back(v);}
                else if (out.color[v] == out.color[u]) {
                    out.ok = false; out.cycle = traversal_detail::treeCycle(g, parent_arc, depth, a);
                    return out;}}}}
    return out;}

enum class ArcKind { None = -1, Tree, Back, Forward, Cross };

// T: O(n + m), M: O(n + m); t from dfs or dfsForest of g; both arcs of an undirected tree edge are Tree, a loop is Back.
template<class G = Graph>
vector<ArcKind> classifyArcs(const G &g, const TraversalResult &t) {
    assert(int(t.parent_arc.size()) == g.n && t.order.size() == t.postorder.size());
    vector<int> pre(g.n, -1), post(g.n);
    for (int i = 0; i < int(t.order.size()); ++i) { pre[t.order[i]] = i; post[t.postorder[i]] = i; }
    auto ancestor = [&](int u, int v) { return pre[u] <= pre[v] && post[v] <= post[u]; };
    vector<ArcKind> res(g.arcs.size(), ArcKind::None);
    for (int a = 0; a < int(g.arcs.size()); ++a) {
        auto e = g.arcs[a];
        if (pre[e.from] == -1) { continue; }
        if (t.parent_arc[e.to] == a || (e.rev != -1 && t.parent_arc[e.from] == e.rev)) { res[a] = ArcKind::Tree; }
        else if (ancestor(e.to, e.from)) { res[a] = ArcKind::Back; }
        else if (ancestor(e.from, e.to)) { res[a] = ArcKind::Forward; }
        else { res[a] = ArcKind::Cross; }}
    return res;}

enum class BfsArcKind { None = -1, Tree, Level, Forward, Reverse };

// T: O(n + m), M: O(m); t from bfs of g; both arcs of an undirected tree edge are Tree, a loop is Level.
template<class G = Graph>
vector<BfsArcKind> classifyBfsArcs(const G &g, const TraversalResult &t) {
    assert(int(t.depth.size()) == g.n && t.postorder.empty());
    vector<BfsArcKind> res(g.arcs.size(), BfsArcKind::None);
    for (int a = 0; a < int(g.arcs.size()); ++a) {
        auto e = g.arcs[a]; int du = t.depth[e.from], dv = t.depth[e.to];
        if (du == -1) { continue; }
        if (t.parent_arc[e.to] == a || (e.rev != -1 && t.parent_arc[e.from] == e.rev)) { res[a] = BfsArcKind::Tree; }
        else if (dv == du) { res[a] = BfsArcKind::Level; }
        else if (dv > du) { res[a] = BfsArcKind::Forward; }
        else { res[a] = BfsArcKind::Reverse; }}
    return res;}

// T: O(n + d), M: O(n + d); radius >= 0, induced on the vertices within radius hops of center (out-arcs), in BFS order; d = degree sum of those vertices.
template<class G = Graph>
Subgraph egoGraph(const G &g, int center, int radius) {
    assert(0 <= center && center < g.n && radius >= 0);
    vector<int> depth(g.n, -1), vs{center};
    depth[center] = 0;
    for (int i = 0; i < int(vs.size()); ++i) {
        int u = vs[i];
        if (depth[u] == radius) { continue; }
        for (int a : g[u]) {
            int v = g.arcs[a].to;
            if (depth[v] == -1) { depth[v] = depth[u] + 1; vs.push_back(v); }}}
    return inducedSubgraph(g, vs);}
