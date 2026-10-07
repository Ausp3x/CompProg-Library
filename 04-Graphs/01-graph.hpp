#pragma once

#include "../01-Core/01-template.hpp"

// T: O(1), M: O(1); logical edge and oriented arc, rev = -1 for directed arcs.
struct GraphEdge { int u, v; lng w; };
struct GraphArc { int from, to, id, rev; lng w; };

// S: O(n), U: O(1) amortized addEdge, Q: O(1), M: O(n + m); reverse and read O(n + m).
struct Graph {
    int n;
    bool directed;
    vector<GraphEdge> edges;
    vector<GraphArc> arcs;
    vector<vector<int>> adj;

    explicit Graph(int N = 0, bool directed = false) : n(N), directed(directed) {
        assert(n >= 0); adj.resize(n);}

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

// S: O(k * log(k)), Q: O(log(k)) index, O(1) value, M: O(k)
template<class T>
struct GraphLabels {
    vector<T> labels;

    explicit GraphLabels(vector<T> values) : labels(std::move(values)) {
        assert(labels.size() <= size_t(INT_MAX)); sort(labels.begin(), labels.end());
        labels.erase(unique(labels.begin(), labels.end(), [](const T &a, const T &b) {
            return !(a < b) && !(b < a);}), labels.end());}

    int index(const T &value) const {
        auto it = lower_bound(labels.begin(), labels.end(), value);
        assert(it != labels.end() && !(value < *it)); return int(it - labels.begin());}
    const T &value(int u) const {
        assert(0 <= u && u < int(labels.size())); return labels[u];}
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
