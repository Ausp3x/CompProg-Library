#pragma once

#include "../01-Core/01-template.hpp"

struct GraphEdge { int u, v; lng w; };
struct GraphArc { int from, to, id, rev; lng w; };

// Vertices [0,n), logical edges [0,m), all counts fit int. Weights span lng.
// Undirected edges (including loops) have two distinct reciprocal arcs;
// directed arcs have rev=-1. IDs are stable until assignment/destruction.
// Public storage must not be edited. addEdge invalidates references/iterators.
// S: O(n), U: O(1) amortized per edge, Q: O(1), M: O(n + m)
struct Graph {
    int n;
    bool directed;
    vector<GraphEdge> edges;
    vector<GraphArc> arcs;
    vector<vector<int>> adj;

    explicit Graph(int N = 0, bool directed = false) : n(N), directed(directed) {
        assert(n >= 0); adj.resize(n); }

    const vector<int> &operator[](int u) const {
        assert(0 <= u && u < n); return adj[u]; }

    int addEdge(int u, int v, lng w = 1) {
        assert(0 <= u && u < n && 0 <= v && v < n);
        assert(arcs.size() <= size_t(INT_MAX - (directed ? 1 : 2)));
        int id = int(edges.size()), a = int(arcs.size());
        edges.push_back({u, v, w});
        arcs.push_back({u, v, id, directed ? -1 : a + 1, w}); adj[u].push_back(a);
        if (!directed) { arcs.push_back({v, u, id, a, w}); adj[v].push_back(a + 1); }
        return id; }

    // O(n + m); logical edge IDs preserved, undirected orientation is unchanged.
    Graph reverse() const {
        Graph res(n, directed);
        for (auto e : edges) { res.addEdge(directed ? e.v : e.u, directed ? e.u : e.v, e.w); }
        return res; }
    // O(n + m). Exactly m records; base is 0 or 1, omitted weights become 1.
    // Well-formed input is a precondition (including successful extraction).
    static Graph read(istream &in, int n, int m, bool directed = false,
                      bool weighted = false, int base = 1) {
        assert(m >= 0 && (base == 0 || base == 1));
        Graph res(n, directed);
        for (int i = 0; i < m; ++i) {
            lng u = 0, v = 0, w = 1;
            bool ok = bool(in >> u >> v);
            if (weighted) { ok = bool(in >> w) && ok; }
            assert(ok && base <= u && u < lng(n) + base && base <= v && v < lng(n) + base);
            (void)ok; res.addEdge(int(u - base), int(v - base), w);}
        return res; }
};

// Immutable owning CSR snapshot: original edge/arc IDs and adjacency order retained.
// Spans borrow this object and expire on assignment/destruction.
// S: O(n + m), Q: O(1), M: O(n + m); construction workspace O(1) excluding result.
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
        for (int u = 0; u < n; ++u) { adj.insert(adj.end(), g[u].begin(), g[u].end()); } }

    std::span<const int> operator[](int u) const {
        assert(0 <= u && u < n);
        return std::span<const int>(adj).subspan(offset[u], offset[u + 1] - offset[u]); }
    // O(n + m), including transient adjacency-list storage.
    CsrGraph reverse() const {
        Graph res(n, directed);
        for (auto e : edges) { res.addEdge(directed ? e.v : e.u, directed ? e.u : e.v, e.w); }
        return CsrGraph(res); }
};

// Owning dense minimum-edge view; full multigraph remains in graph.
// best[u][v] is an arc index, -1 for absence; equal weights choose earliest arc.
// No implicit diagonal edges: loops are preserved normally.
// S: O(n^2 + m), Q: O(1), M: O(n^2 + m)
struct DenseGraph {
    Graph graph;
    vector<vector<int>> best;

    explicit DenseGraph(const Graph &g) : graph(g), best(g.n, vector<int>(g.n, -1)) {
        for (int a = 0; a < int(g.arcs.size()); ++a) {
            auto e = g.arcs[a]; int &b = best[e.from][e.to];
            if (b == -1 || e.w < g.arcs[b].w) { b = a; }} }
};

// Sorted label compression using strict weak ordering; equivalence is !(a<b||b<a).
// Labels must include every endpoint. value() borrows this object's storage.
// S: O(k * log(k)), Q: O(log(k)) index / O(1) value, M: O(k)
template<class T>
struct GraphLabels {
    vector<T> labels;

    explicit GraphLabels(vector<T> values) : labels(std::move(values)) {
        assert(labels.size() <= size_t(INT_MAX)); sort(labels.begin(), labels.end());
        labels.erase(unique(labels.begin(), labels.end(), [](const T &a, const T &b) {
            return !(a < b) && !(b < a); }), labels.end()); }

    int index(const T &value) const {
        auto it = lower_bound(labels.begin(), labels.end(), value);
        assert(it != labels.end() && !(value < *it)); return int(it - labels.begin()); }
    const T &value(int u) const {
        assert(0 <= u && u < int(labels.size())); return labels[u]; }
};
