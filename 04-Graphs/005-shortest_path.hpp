#pragma once

#include "../01-Core/01-template.hpp"
#include "001-graph.hpp"
#include "002-traversal.hpp"
#include "003-toposort.hpp"

// T: O(1), M: O(n); exists is false for no path, a source's own path has one vertex.
struct PathWitness {
    bool exists = false;
    vector<int> vertices, arcs;
};

// S: O(n + k), U: O(1) relax, Q: O(1) finite, O(p) path with p the path's vertices, M: O(n); dist exact where finite.
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
    template<class G = Graph>
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
template<class G = Graph>
ShortestPathResult bfsShortestPaths(const G &g, const vector<int> &sources) {
    auto walk = bfs(g, sources); ShortestPathResult out(g.n);
    out.parent = std::move(walk.parent); out.parent_arc = std::move(walk.parent_arc);
    for (int v : walk.order) { out.reachable[v] = true; out.dist[v] = walk.depth[v]; }
    return out;}

// T: O(n + m + k), M: O(n + m); every weight is 0 or one common c > 0.
template<class G = Graph>
ShortestPathResult zeroOneBfs(const G &g, const vector<int> &sources) {
    ShortestPathResult out(g.n, sources); deque<pair<lll, int>> ready; lng c = 0;
    bool ok = std::all_of(g.arcs.begin(), g.arcs.end(), [&](const GraphArc &e) {
        if (!c) { c = e.w; }
        return e.w == 0 || (e.w == c && c > 0);});
    assert(ok);
    if (!ok) { return out; }
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

namespace shortest_path_detail {
    // T: O(n + d * log(d + 1)), M: O(n + d), d = arcs scanned; lazy heap from out's sources. If bounded, settles
    // the vertices within limit into order by nondecreasing distance, false at the first negative scanned arc.
    template<bool bounded, class G = Graph>
    bool dijkstra(const G &g, ShortestPathResult &out, lll limit, vector<int> &order) {
        priority_queue<pair<lll, int>, vector<pair<lll, int>>, std::greater<pair<lll, int>>> ready;
        for (int u = 0; u < g.n; ++u) { if (out.reachable[u]) { ready.push({0, u}); }}
        while (!ready.empty() && (!bounded || ready.top().first <= limit)) {
            auto [d, u] = ready.top(); ready.pop();
            if (d != out.dist[u]) { continue; }
            if constexpr (bounded) { order.push_back(u); }
            for (int a : g[u]) {
                const auto &e = g.arcs[a];
                if constexpr (bounded) { if (e.w < 0) { return false; }}
                if (relax(out, e, a)) { ready.push({out.dist[e.to], e.to}); }}}
        return true;}
} // namespace shortest_path_detail

// T: O(k + (n + m) * log(n + m + 1)), M: O(n + m); every weight must be nonnegative.
template<class G = Graph>
ShortestPathResult dijkstra(const G &g, const vector<int> &sources) {
    ShortestPathResult out(g.n, sources); vector<int> order;
    bool ok = std::all_of(g.arcs.begin(), g.arcs.end(), [](const GraphArc &e) { return e.w >= 0; });
    assert(ok);
    if (ok) { shortest_path_detail::dijkstra<false>(g, out, 0, order); }
    return out;}

// T: O(1), M: O(n); center[v] is v's cell, -1 when unreached.
struct VoronoiResult {
    ShortestPathResult paths;
    vector<int> center;
};

// T: O(k + (n + m) * log(n + m + 1)), M: O(n + m); weights nonnegative, ties to the smallest nearest center.
template<class G = Graph>
VoronoiResult voronoiCells(const G &g, const vector<int> &centers) {
    VoronoiResult res{ShortestPathResult(g.n, centers), vector<int>(g.n, -1)};
    auto &out = res.paths; auto &cell = res.center;
    for (int c : centers) { cell[c] = c; }
    bool ok = std::all_of(g.arcs.begin(), g.arcs.end(), [](const GraphArc &e) { return e.w >= 0; });
    assert(ok);
    if (!ok) { return res; }
    using Key = tuple<lll, int, int>;
    priority_queue<Key, vector<Key>, std::greater<Key>> ready;
    for (int u = 0; u < g.n; ++u) { if (cell[u] == u) { ready.push({0, u, u}); }}
    while (!ready.empty()) {
        auto [d, c, u] = ready.top(); ready.pop();
        if (d != out.dist[u] || c != cell[u]) { continue; }
        for (int a : g[u]) {
            const auto &e = g.arcs[a]; int v = e.to; lll x = d + e.w;
            if (out.reachable[v] && pair(x, c) >= pair(out.dist[v], cell[v])) { continue; }
            out.reachable[v] = true; out.dist[v] = x; cell[v] = c;
            out.parent[v] = u; out.parent_arc[v] = a; ready.push({x, c, v});}}
    return res;}

// T: O(n + d * log(d + 1)), M: O(n + d), d = arcs scanned; radius >= 0, weights nonnegative (scanned arcs checked),
// induced on the vertices within weighted distance radius of center (out-arcs), by nondecreasing distance.
template<class G = Graph>
Subgraph egoGraphWeighted(const G &g, int center, lll radius) {
    assert(0 <= center && center < g.n && radius >= 0);
    ShortestPathResult out(g.n, {center}); vector<int> order;
    bool ok = shortest_path_detail::dijkstra<true>(g, out, radius, order);
    assert(ok);
    return inducedSubgraph(g, ok ? order : vector<int>{center});}

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
    bool acyclic = false;
    ShortestPathResult paths;
    CycleWitness cycle;
};

namespace shortest_path_detail {
    // T: O(n + m + k), M: O(n); relaxation in topological order, maximum weight paths when longest.
    template<class G = Graph>
    DagPathResult dagPaths(const G &g, const vector<int> &sources, bool longest) {
        ShortestPathResult out(g.n, sources); auto topo = topologicalSort(g);
        if (!topo.acyclic) { return {false, ShortestPathResult(g.n), std::move(topo.cycle)}; }
        for (int u : topo.order) {
            if (out.reachable[u]) { for (int a : g[u]) { relax(out, g.arcs[a], a, longest); }}}
        return {true, std::move(out), {}};}
} // namespace shortest_path_detail

// T: O(n + m + k), M: O(n); directed, signed weights, minimum weight paths.
template<class G = Graph>
DagPathResult dagShortestPaths(const G &g, const vector<int> &sources) {
    return shortest_path_detail::dagPaths(g, sources, false);}

// T: O(n + m + k), M: O(n); directed, signed weights, maximum weight paths.
template<class G = Graph>
DagPathResult dagLongestPathsFrom(const G &g, const vector<int> &sources) {
    return shortest_path_detail::dagPaths(g, sources, true);}

// T: O(1), M: O(n); if acyclic is false, cycle is a witness; path is absent only then or when n == 0.
struct DagLongestPathResult {
    bool acyclic = false;
    lll length = 0;
    PathWitness path;
    CycleWitness cycle;
};

// T: O(n + m), M: O(n); directed, signed weights, a maximum weight path from any start, ties to the smallest end.
template<class G = Graph>
DagLongestPathResult dagLongestPath(const G &g) {
    vector<int> sources(g.n); iota(sources.begin(), sources.end(), 0);
    auto res = dagLongestPathsFrom(g, sources);
    if (!res.acyclic) { return {false, 0, {}, std::move(res.cycle)}; }
    if (!g.n) { return {true, 0, {}, {}}; }
    const auto &dist = res.paths.dist;
    int v = int(std::max_element(dist.begin(), dist.end()) - dist.begin());
    return {true, dist[v], res.paths.path(g, v), {}};}

// T: O(1), M: O(n + m); if acyclic is false, only cycle is set; critical lists arc IDs in increasing order.
struct CriticalPathResult {
    bool acyclic = false;
    lll length = 0;
    vector<lll> earliest, latest;
    vector<int> critical;
    CycleWitness cycle;
};

// T: O(n + m), M: O(n + m); directed AOE network, arcs are activities with signed durations.
template<class G = Graph>
CriticalPathResult criticalPath(const G &g) {
    auto topo = topologicalSort(g);
    if (!topo.acyclic) { return {false, 0, {}, {}, {}, std::move(topo.cycle)}; }
    CriticalPathResult res{true, 0, vector<lll>(g.n), vector<lll>(g.n), {}, {}};
    vector<char> seen(g.n);
    for (int u : topo.order) {
        for (int a : g[u]) {
            const auto &e = g.arcs[a]; lll x = res.earliest[u] + e.w;
            if (!seen[e.to] || x > res.earliest[e.to]) { res.earliest[e.to] = x; seen[e.to] = true; }}}
    bool sink = false;
    for (int v = 0; v < g.n; ++v) {
        if (g[v].empty() && (!sink || res.earliest[v] > res.length)) { res.length = res.earliest[v]; sink = true; }}
    for (int i = g.n - 1; i >= 0; --i) {
        int u = topo.order[i]; bool first = true; res.latest[u] = res.length;
        for (int a : g[u]) {
            const auto &e = g.arcs[a]; lll x = res.latest[e.to] - e.w;
            if (first || x < res.latest[u]) { res.latest[u] = x; first = false; }}}
    for (int a = 0; a < int(g.arcs.size()); ++a) {
        const auto &e = g.arcs[a];
        if (res.earliest[e.from] + e.w == res.latest[e.to]) { res.critical.push_back(a); }}
    return res;}

// T: O(n * m + n + k), M: O(n); signed weights, negative marks vertices reachable from a negative cycle.
template<class G = Graph>
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
template<class G = Graph>
CycleWitness findNegativeCycle(const G &g) {
    vector<int> sources(g.n); iota(sources.begin(), sources.end(), 0);
    return bellmanFord(g, sources).negative_cycle;}

// T: O(n * m + n + k) with a negative weight, O(n + m + k) with weights in {0, c}, else as dijkstra; M: O(n + m).
template<class G = Graph>
ShortestPathResult shortestPaths(const G &g, const vector<int> &sources) {
    lng c = 0; bool negative = false, uniform = true;
    for (const auto &e : g.arcs) {
        negative |= e.w < 0;
        if (e.w) { c = c ? c : e.w; uniform &= e.w == c; }}
    if (negative) { return bellmanFord(g, sources); }
    return uniform ? zeroOneBfs(g, sources) : dijkstra(g, sources);}
