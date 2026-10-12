#pragma once

#include "../01-Core/01-template.hpp"
#include "01-graph.hpp"
#include "02-traversal.hpp"
#include "03-toposort.hpp"

// T: O(1), M: O(n); exists is false for no path, a source's own path has one vertex.
struct PathWitness {
    bool exists = false;
    vector<int> vertices, arcs;
};

// S: O(n + k), U: O(1) relax, Q: O(1) finite, O(path) path, M: O(n); dist is exact only where finite.
struct ShortestPathResult {
    vector<lll> dist;
    vector<char> reachable, negative;
    vector<int> parent, parent_arc;
    CycleWitness negative_cycle;

    explicit ShortestPathResult(int n = 0, const vector<int> &sources = {}) {
        assert(n >= 0); dist.resize(n); reachable.resize(n); negative.resize(n);
        parent.assign(n, -1); parent_arc.assign(n, -1);
        for (int s : sources) { assert(0 <= s && s < n); reachable[s] = true; }}

    bool finite(int v) const {
        assert(0 <= v && v < int(dist.size())); return reachable[v] && !negative[v];}
    bool relax(const GraphArc &e, int a, bool longest = false);
    template<class G>
    PathWitness path([[maybe_unused]] const G &g, int v) const {
        assert(g.n == int(dist.size()));
        if (!finite(v)) { return {}; }
        PathWitness out; out.exists = true;
        while (v != -1) {
            out.vertices.push_back(v);
            if (parent_arc[v] != -1) { out.arcs.push_back(parent_arc[v]); }
            v = parent[v];}
        reverse(out.vertices.begin(), out.vertices.end()); reverse(out.arcs.begin(), out.arcs.end());
        return out;}
};

namespace shortest_path_detail {
    // T: O(1), M: O(1); unchecked relax for algorithms that validated the graph at entry.
    inline bool relax(ShortestPathResult &r, const GraphArc &e, int a, bool longest = false) {
        lll d = r.dist[e.from] + e.w;
        if (r.reachable[e.to] && (longest ? d <= r.dist[e.to] : d >= r.dist[e.to])) { return false; }
        r.reachable[e.to] = true; r.dist[e.to] = d;
        r.parent[e.to] = e.from; r.parent_arc[e.to] = a; return true;}
} // namespace shortest_path_detail

inline bool ShortestPathResult::relax(const GraphArc &e, int a, bool longest) {
    assert(0 <= e.from && e.from < int(dist.size()) && 0 <= e.to && e.to < int(dist.size()));
    assert(reachable[e.from] && a >= 0);
    return shortest_path_detail::relax(*this, e, a, longest);}

// T: O(n + m + k), M: O(n); weights ignored, distances count arcs, ties as in bfs.
template<class G>
ShortestPathResult bfsShortestPaths(const G &g, const vector<int> &sources) {
    auto walk = bfs(g, sources); ShortestPathResult out(g.n);
    out.parent = std::move(walk.parent); out.parent_arc = std::move(walk.parent_arc);
    for (int v : walk.order) { out.reachable[v] = true; out.dist[v] = walk.depth[v]; }
    return out;}

// T: O(n + m + k), M: O(n + m); every weight must be 0 or 1.
template<class G>
ShortestPathResult zeroOneBfs(const G &g, const vector<int> &sources) {
    for (const auto &e : g.arcs) { assert(e.w == 0 || e.w == 1); (void)e; }
    ShortestPathResult out(g.n, sources); deque<pair<lll, int>> ready;
    for (int u = 0; u < g.n; ++u) { if (out.reachable[u]) { ready.push_back({0, u}); }}
    while (!ready.empty()) {
        auto [d, u] = ready.front(); ready.pop_front();
        if (d != out.dist[u]) { continue; }
        for (int a : g[u]) {
            const auto &e = g.arcs[a];
            if (shortest_path_detail::relax(out, e, a)) {
                if (e.w == 0) { ready.push_front({out.dist[e.to], e.to}); }
                else { ready.push_back({out.dist[e.to], e.to}); }}}}
    return out;}

// T: O(k + (n + m) * log(n + m + 1)), M: O(n + m); every weight must be nonnegative.
template<class G>
ShortestPathResult dijkstra(const G &g, const vector<int> &sources) {
    for (const auto &e : g.arcs) { assert(e.w >= 0); (void)e; }
    ShortestPathResult out(g.n, sources);
    priority_queue<pair<lll, int>, vector<pair<lll, int>>, std::greater<pair<lll, int>>> ready;
    for (int u = 0; u < g.n; ++u) { if (out.reachable[u]) { ready.push({0, u}); }}
    while (!ready.empty()) {
        auto [d, u] = ready.top(); ready.pop();
        if (d != out.dist[u]) { continue; }
        for (int a : g[u]) {
            const auto &e = g.arcs[a];
            if (shortest_path_detail::relax(out, e, a)) { ready.push({out.dist[e.to], e.to}); }}}
    return out;}

// T: O(n^2 + m + k), M: O(n); minimum view, every weight of the source graph nonnegative.
inline ShortestPathResult dijkstraDense(const DenseGraph &dense, const vector<int> &sources) {
    const auto &g = dense.graph; assert(!dense.maximum);
    for (const auto &e : g.arcs) { assert(e.w >= 0); (void)e; }
    ShortestPathResult out(g.n, sources); vector<char> used(g.n);
    for (int i = 0; i < g.n; ++i) {
        int u = -1;
        for (int v = 0; v < g.n; ++v) {
            if (!used[v] && out.reachable[v] && (u == -1 || out.dist[v] < out.dist[u])) { u = v; }}
        if (u == -1) { break; }
        used[u] = true;
        for (int v = 0; v < g.n; ++v) {
            int a = dense.best[u][v];
            if (a != -1) { shortest_path_detail::relax(out, g.arcs[a], a); }}}
    return out;}

// T: O(1), M: O(n); if acyclic is false, paths reaches nothing and cycle is a witness.
struct DagPathResult {
    bool acyclic;
    ShortestPathResult paths;
    CycleWitness cycle;
};

// T: O(n + m + k), M: O(n); directed, signed weights, longest picks maximum weight paths.
template<class G>
DagPathResult dagShortestPaths(const G &g, const vector<int> &sources, bool longest = false) {
    ShortestPathResult out(g.n, sources); auto topo = topologicalSort(g);
    if (!topo.acyclic) { return {false, ShortestPathResult(g.n), std::move(topo.cycle)}; }
    for (int u : topo.order) {
        if (out.reachable[u]) { for (int a : g[u]) { shortest_path_detail::relax(out, g.arcs[a], a, longest); }}}
    return {true, std::move(out), {}};}

// T: O(n * m + n + k), M: O(n); signed weights, negative marks vertices reachable from a negative cycle.
template<class G>
ShortestPathResult bellmanFord(const G &g, const vector<int> &sources) {
    ShortestPathResult out(g.n, sources); vector<int> ready; int last = -1;
    for (int i = 0; i < g.n; ++i) {
        last = -1;
        for (int a = 0; a < int(g.arcs.size()); ++a) {
            const auto &e = g.arcs[a];
            if (out.reachable[e.from] && shortest_path_detail::relax(out, e, a)) {
                last = e.to;
                if (i == g.n - 1 && !out.negative[e.to]) {
                    out.negative[e.to] = true; ready.push_back(e.to);}}}
        if (last == -1) { return out; }}
    if (last != -1) {
        int u = last;
        for (int i = 0; i < g.n; ++i) { u = out.parent[u]; }
        int v = u;
        do {
            out.negative_cycle.vertices.push_back(v); out.negative_cycle.arcs.push_back(out.parent_arc[v]);
            v = out.parent[v];} while (v != u);
        out.negative_cycle.vertices.push_back(u);
        reverse(out.negative_cycle.vertices.begin(), out.negative_cycle.vertices.end());
        reverse(out.negative_cycle.arcs.begin(), out.negative_cycle.arcs.end());}
    for (int i = 0; i < int(ready.size()); ++i) {
        for (int a : g[ready[i]]) {
            int v = g.arcs[a].to;
            if (!out.negative[v]) { out.negative[v] = true; ready.push_back(v); }}}
    return out;}

// T: O(n * m + n), M: O(n); a negative directed arc-cycle anywhere in g, empty if none.
template<class G>
CycleWitness findNegativeCycle(const G &g) {
    vector<int> sources(g.n); iota(sources.begin(), sources.end(), 0);
    return bellmanFord(g, sources).negative_cycle;}
