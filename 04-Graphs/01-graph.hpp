#pragma once

#include "../01-Core/01-template.hpp"

// T: O(1), M: O(1); logical edge and oriented arc, rev = -1 for directed arcs.
struct GraphEdge { int u, v; lng w; };
struct GraphArc { int from, to, id, rev; lng w; };

namespace graph_detail {
    // T: O(m + k), M: O(m + k); stable counting sort of m edges by key in [0, k), skipped when all keys are equal.
    template<class F>
    void countingSort(vector<GraphEdge> &es, int k, F key) {
        vector<int> start(size_t(k) + 1);
        for (const auto &e : es) { ++start[key(e) + 1]; }
        if (es.empty() || start[key(es[0]) + 1] == int(es.size())) { return; }
        for (int i = 0; i < k; ++i) { start[i + 1] += start[i]; }
        vector<GraphEdge> res(es.size());
        for (const auto &e : es) { res[start[key(e)]++] = e; }
        es = std::move(res);}
} // namespace graph_detail

// S: O(n), U: O(1) amortized addEdge, Q: O(1), M: O(n + m); reverse and read O(n + m).
struct Graph {
    int n;
    bool directed;
    vector<GraphEdge> edges;
    vector<GraphArc> arcs;
    vector<vector<int>> adj;

    explicit Graph(int N = 0, bool directed = false) : n(N), directed(directed) {
        assert(n >= 0); adj.resize(n);}
    static Graph read(istream &in, int n, int m, bool directed = false, bool weighted = false, int base = 1) {
        assert(m >= 0 && (base == 0 || base == 1));
        Graph res(n, directed);
        for (int i = 0; i < m; ++i) {
            lng u = 0, v = 0, w = 1;
            bool ok = bool(in >> u >> v);
            if (weighted) { ok = bool(in >> w) && ok; }
            assert(ok && base <= u && u < lng(n) + base && base <= v && v < lng(n) + base);
            (void)ok; res.addEdge(int(u - base), int(v - base), w);}
        return res;}
    // T: O(n + m), M: O(n + m); n = adj.size(), E is int (weight 1) or pair<int, lng>, undirected lists mirror as multisets.
    template<class E>
    static Graph fromAdjacency(const vector<vector<E>> &adj, bool directed = false) {
        assert(adj.size() <= size_t(INT_MAX));
        Graph res(int(adj.size()), directed); vector<GraphEdge> es;
        for (int u = 0; u < res.n; ++u) {
            for (const E &e : adj[u]) {
                int v; lng w = 1;
                if constexpr (std::is_same_v<E, int>) { v = e; } else { std::tie(v, w) = e; }
                assert(0 <= v && v < res.n);
                if (directed) { res.addEdge(u, v, w); } else { es.push_back({u, v, w}); }}}
        if (directed) { return res; }
        if constexpr (!std::is_same_v<E, int>) {
            for (int s = 0; s < 64; s += 8) {
                graph_detail::countingSort(es, 256, [s](const GraphEdge &e) { return int((ulng(e.w) ^ ulng(1) << 63) >> s & 255); });}}
        graph_detail::countingSort(es, res.n, [](const GraphEdge &e) { return max(e.u, e.v); });
        graph_detail::countingSort(es, res.n, [](const GraphEdge &e) { return min(e.u, e.v); });
        auto key = [](const GraphEdge &e) { return tuple(min(e.u, e.v), max(e.u, e.v), e.w); };
        // Entries listed at their smaller endpoint (both entries of a loop) count as forward.
        for (int i = 0, j = 0; i < int(es.size()); i = j) {
            auto [u, v, w] = key(es[i]); int forward = 0;
            for (j = i; j < int(es.size()) && key(es[j]) == key(es[i]); ++j) { forward += es[j].u <= es[j].v; }
            assert(u == v ? forward % 2 == 0 : 2 * forward == j - i);
            for (int k = u == v ? forward / 2 : forward; k > 0; --k) { res.addEdge(u, v, w); }}
        return res;}
    // T: O(n^2), M: O(n + m); w square, one arc per nonzero entry in row-major order, undirected needs w symmetric.
    static Graph fromMatrix(const vector<vector<lng>> &w, bool directed = true) {
        assert(w.size() <= size_t(INT_MAX));
        Graph res(int(w.size()), directed);
        for (int u = 0; u < res.n; ++u) {
            assert(w[u].size() == w.size());
            for (int v = 0; v < u && !directed; ++v) { assert(w[u][v] == w[v][u]); }}
        for (int u = 0; u < res.n; ++u) {
            for (int v = directed ? 0 : u; v < res.n; ++v) {
                if (w[u][v]) { res.addEdge(u, v, w[u][v]); }}}
        return res;}

    const vector<int> &operator[](int u) const {
        assert(0 <= u && u < n); return adj[u];}

    int addEdge(int u, int v, lng w = 1) {
        assert(0 <= u && u < n && 0 <= v && v < n);
        assert(arcs.size() <= size_t(INT_MAX - (directed ? 1 : 2)));
        int id = int(edges.size()), a = int(arcs.size());
        edges.push_back({u, v, w});
        arcs.push_back({u, v, id, directed ? -1 : a + 1, w}); adj[u].push_back(a);
        if (!directed) { arcs.push_back({v, u, id, a, w}); adj[v].push_back(a + 1); }
        return id;}

    Graph reverse() const {
        Graph res(n, directed);
        for (auto e : edges) { res.addEdge(directed ? e.v : e.u, directed ? e.u : e.v, e.w); }
        return res;}
};

// S: O(n + m), Q: O(1), M: O(n + m); reverse O(n + m).
struct CsrGraph {
    int n;
    bool directed;
    vector<GraphEdge> edges;
    vector<GraphArc> arcs;
    vector<int> offset, adj;

    explicit CsrGraph(const Graph &g) : n(g.n), directed(g.directed), edges(g.edges), arcs(g.arcs) {
        offset.resize(size_t(n) + 1);
        for (int u = 0; u < n; ++u) { offset[u + 1] = offset[u] + int(g[u].size()); }
        adj.reserve(arcs.size());
        for (int u = 0; u < n; ++u) { adj.insert(adj.end(), g[u].begin(), g[u].end()); }}

    std::span<const int> operator[](int u) const {
        assert(0 <= u && u < n);
        return std::span<const int>(adj).subspan(offset[u], offset[u + 1] - offset[u]);}

    CsrGraph reverse() const {
        Graph res(n, directed);
        for (auto e : edges) { res.addEdge(directed ? e.v : e.u, directed ? e.u : e.v, e.w); }
        return CsrGraph(res);}
};

// S: O(n^2 + m), Q: O(1), M: O(n^2 + m); best[u][v] is the lightest (heaviest if maximum) arc, -1 if none.
struct DenseGraph {
    Graph graph;
    bool maximum;
    vector<vector<int>> best;

    explicit DenseGraph(const Graph &g, bool maximum = false) : graph(g), maximum(maximum), best(g.n, vector<int>(g.n, -1)) {
        for (int a = 0; a < int(g.arcs.size()); ++a) {
            auto e = g.arcs[a]; int &b = best[e.from][e.to];
            if (b == -1 || (maximum ? e.w > g.arcs[b].w : e.w < g.arcs[b].w)) { b = a; }}}
};

// T: O(1), M: O(n + m); edge[i] is the source edge ID of graph edge i.
struct Subgraph { Graph graph; vector<int> edge; };

// T: O(n + m + out), M: O(m + out); vertex i is edge i, out = sum of indeg * outdeg or C(deg, 2) per vertex.
template<class G>
Graph lineGraph(const G &g) {
    Graph res(int(g.edges.size()), g.directed);
    if (g.directed) {
        for (int id = 0; id < res.n; ++id) {
            for (int a : g[g.edges[id].v]) { res.addEdge(id, g.arcs[a].id); }}
        return res;}
    vector<int> ids;
    for (int u = 0; u < g.n; ++u) {
        ids.clear();
        for (int a : g[u]) {
            if (g.arcs[a].to != u || a < g.arcs[a].rev) { ids.push_back(g.arcs[a].id); }}
        for (int i = 0; i < int(ids.size()); ++i) {
            for (int j = 0; j < i; ++j) { res.addEdge(ids[j], ids[i]); }}}
    return res;}

// T: O(n + k + d), M: O(n + k + d); new vertex i is vs[i] (distinct), d = degree sum of vs.
template<class G>
Subgraph inducedSubgraph(const G &g, const vector<int> &vs) {
    vector<int> id(g.n, -1);
    for (int i = 0; i < int(vs.size()); ++i) {
        assert(0 <= vs[i] && vs[i] < g.n && id[vs[i]] == -1); id[vs[i]] = i;}
    Subgraph res{Graph(int(vs.size()), g.directed), {}};
    for (int u : vs) {
        for (int a : g[u]) {
            auto e = g.arcs[a];
            if (id[e.to] != -1 && (g.directed || a < e.rev)) {
                res.graph.addEdge(id[u], id[e.to], e.w); res.edge.push_back(e.id);}}}
    return res;}

// T: O(n + k + m), M: O(k + m); edge i joins label[u], label[v] of source edge i, loops and multiedges kept.
template<class G>
Graph contract(const G &g, const vector<int> &label, int k) {
    assert(int(label.size()) == g.n && k >= 0);
    assert(std::ranges::all_of(label, [&](int x) { return 0 <= x && x < k; }));
    Graph res(k, g.directed);
    for (auto e : g.edges) { res.addEdge(label[e.u], label[e.v], e.w); }
    return res;}

// T: O(n + m), M: O(n + m); drops loops, keeps the lightest then earliest edge per pair, in ID order.
template<class G>
Subgraph simplify(const G &g) {
    vector<int> seen(g.n, -1), best(g.n);
    vector<char> keep(g.edges.size());
    for (int u = 0; u < g.n; ++u) {
        for (int a : g[u]) {
            auto e = g.arcs[a]; int v = e.to;
            if (v == u || (!g.directed && v < u)) { continue; }
            if (seen[v] != u) { seen[v] = u; best[v] = e.id; }
            else if (pair(e.w, e.id) < pair(g.edges[best[v]].w, best[v])) { best[v] = e.id; }}
        for (int a : g[u]) {
            if (seen[g.arcs[a].to] == u) { keep[best[g.arcs[a].to]] = true; }}}
    Subgraph res{Graph(g.n, g.directed), {}};
    for (int id = 0; id < int(g.edges.size()); ++id) {
        if (keep[id]) { res.graph.addEdge(g.edges[id].u, g.edges[id].v, g.edges[id].w); res.edge.push_back(id); }}
    return res;}
