#pragma once

#include "../01-Core/01-template.hpp"

// T: O(1), M: O(1); logical edge.
struct GraphEdge { int u, v; lng w; };
// T: O(1), M: O(1); oriented arc, rev = -1 for directed arcs.
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

    // T: O(n), M: O(n); 0, 1, ..., n - 1.
    inline vector<int> identity(int n) {
        vector<int> res(n); iota(res.begin(), res.end(), 0);
        return res;}
} // namespace graph_detail

// S: O(n), U: O(1) amortized addEdge, O(k) amortized addPath/addCycle/addStar, Q: O(1), M: O(n + m); reverse, read, fromAdjacency O(n + m), fromMatrix O(n^2).
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
        for (int i = 0, j = 0; i < int(es.size()); i = j) {
            auto [u, v, w] = key(es[i]); int forward = 0;
            for (j = i; j < int(es.size()) && key(es[j]) == key(es[i]); ++j) { forward += es[j].u <= es[j].v; }
            assert(u == v ? forward % 2 == 0 : 2 * forward == j - i);
            for (int k = u == v ? forward / 2 : forward; k > 0; --k) { res.addEdge(u, v, w); }}
        return res;}
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
    int addPath(const vector<int> &vs, lng w = 1) {
        int res = int(edges.size());
        for (int i = 0; i + 1 < int(vs.size()); ++i) { addEdge(vs[i], vs[i + 1], w); }
        return res;}
    int addCycle(const vector<int> &vs, lng w = 1) {
        assert(!vs.empty());
        int res = addPath(vs, w);
        if (!vs.empty()) { addEdge(vs.back(), vs[0], w); }
        return res;}
    int addStar(int center, const vector<int> &vs, lng w = 1) {
        int res = int(edges.size());
        for (int v : vs) { addEdge(center, v, w); }
        return res;}

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

// T: O(1), M: O(n + m); vertex[i] and edge[i] are the source vertex and edge IDs of graph vertex i and edge i.
struct Subgraph { Graph graph; vector<int> vertex, edge; };

// T: O(n + m + out), M: O(m + out); vertex i is edge i, out = sum of indeg * outdeg or C(deg, 2) per vertex.
template<class G = Graph>
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
template<class G = Graph>
Subgraph inducedSubgraph(const G &g, const vector<int> &vs) {
    vector<int> id(g.n, -1);
    for (int i = 0; i < int(vs.size()); ++i) {
        assert(0 <= vs[i] && vs[i] < g.n && id[vs[i]] == -1); id[vs[i]] = i;}
    Subgraph res{Graph(int(vs.size()), g.directed), vs, {}};
    for (int u : vs) {
        for (int a : g[u]) {
            auto e = g.arcs[a];
            if (id[e.to] != -1 && (g.directed || a < e.rev)) {
                res.graph.addEdge(id[u], id[e.to], e.w); res.edge.push_back(e.id);}}}
    return res;}

// T: O(n + k + m), M: O(k + m); edge i joins label[u], label[v] of source edge i, loops and multiedges kept.
template<class G = Graph>
Graph contract(const G &g, const vector<int> &label, int k) {
    assert(int(label.size()) == g.n && k >= 0);
    assert(std::ranges::all_of(label, [&](int x) { return 0 <= x && x < k; }));
    Graph res(k, g.directed);
    for (auto e : g.edges) { res.addEdge(label[e.u], label[e.v], e.w); }
    return res;}

// T: O(n + m), M: O(n + m); drops loops, keeps the lightest then earliest edge per pair, in ID order.
template<class G = Graph>
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
    Subgraph res{Graph(g.n, g.directed), graph_detail::identity(g.n), {}};
    for (int id = 0; id < int(g.edges.size()); ++id) {
        if (keep[id]) { res.graph.addEdge(g.edges[id].u, g.edges[id].v, g.edges[id].w); res.edge.push_back(id); }}
    return res;}

// T: O(n + m + k), M: O(n + m + k); all n vertices, edge i is source edge ids[i] (distinct), orientation and weight kept.
template<class G = Graph>
Subgraph edgeSubgraph(const G &g, const vector<int> &ids) {
    vector<char> seen(g.edges.size());
    Subgraph res{Graph(g.n, g.directed), graph_detail::identity(g.n), ids};
    for (int id : ids) {
        assert(0 <= id && id < int(g.edges.size()) && !seen[id]); seen[id] = true;
        res.graph.addEdge(g.edges[id].u, g.edges[id].v, g.edges[id].w);}
    return res;}

// T: O(n^2 + m), M: O(n + out); simple complement, weight 1, no loops; edges by (u, v), u < v when undirected.
template<class G = Graph>
Graph complementGraph(const G &g) {
    Graph res(g.n, g.directed);
    vector<int> seen(g.n, -1);
    for (int u = 0; u < g.n; ++u) {
        for (int a : g[u]) { seen[g.arcs[a].to] = u; }
        for (int v = g.directed ? 0 : u + 1; v < g.n; ++v) {
            if (v != u && seen[v] != u) { res.addEdge(u, v); }}}
    return res;}

// T: O(n + m), M: O(n + m); h's vertices and edge IDs follow g's, shifted by g.n and g's edge count.
template<class G = Graph, class H = Graph>
Graph disjointUnion(const G &g, const H &h) {
    assert(g.directed == h.directed && lng(g.n) + h.n <= INT_MAX);
    Graph res(g.n + h.n, g.directed);
    for (auto e : g.edges) { res.addEdge(e.u, e.v, e.w); }
    for (auto e : h.edges) { res.addEdge(g.n + e.u, g.n + e.v, e.w); }
    return res;}

namespace graph_detail {
    // T: O(n_g * n_h), M: O(n_g * n_h); empty product graph, vertex (u, x) is u * h.n + x; n_g, m_g and n_h, m_h size g and h.
    template<class G, class H>
    Graph productBase(const G &g, const H &h) {
        assert(g.directed == h.directed && lng(g.n) * h.n <= INT_MAX);
        return Graph(g.n * h.n, g.directed);}

    // T: O(n_g * m_h), M: O(1); the h edges in each layer u; n_g, m_g and n_h, m_h size g and h.
    template<class G, class H>
    void addLayers(Graph &res, const G &g, const H &h) {
        for (int u = 0; u < g.n; ++u) {
            for (auto f : h.edges) { res.addEdge(u * h.n + f.u, u * h.n + f.v, f.w); }}}

    // T: O(n_g * m_h + m_g * n_h), M: O(1); g edges copied in each layer x, then addLayers; n_g, m_g and n_h, m_h size g and h.
    template<class G, class H>
    void addCartesian(Graph &res, const G &g, const H &h) {
        for (auto e : g.edges) {
            for (int x = 0; x < h.n; ++x) { res.addEdge(e.u * h.n + x, e.v * h.n + x, e.w); }}
        addLayers(res, g, h);}

    // T: O(m_g * m_h), M: O(1); per edge pair (u, x)-(v, y), and (u, y)-(v, x) when undirected and neither is a loop; n_g, m_g and n_h, m_h size g and h.
    template<class G, class H>
    void addTensor(Graph &res, const G &g, const H &h) {
        for (auto e : g.edges) {
            for (auto f : h.edges) {
                res.addEdge(e.u * h.n + f.u, e.v * h.n + f.v, e.w);
                if (!g.directed && e.u != e.v && f.u != f.v) { res.addEdge(e.u * h.n + f.v, e.v * h.n + f.u, e.w); }}}}
} // namespace graph_detail

// T: O(n_g * n_h + n_g * m_h + m_g * n_h), M: O(n_g * n_h + out); vertex (u, x) is u * h.n + x, edges keep their factor's weight; n_g, m_g and n_h, m_h size g and h.
template<class G = Graph, class H = Graph>
Graph cartesianProduct(const G &g, const H &h) {
    Graph res = graph_detail::productBase(g, h);
    graph_detail::addCartesian(res, g, h);
    return res;}

// T: O(n_g * n_h + m_g * m_h), M: O(n_g * n_h + out); vertex (u, x) is u * h.n + x, each edge takes its g edge's weight; n_g, m_g and n_h, m_h size g and h.
template<class G = Graph, class H = Graph>
Graph tensorProduct(const G &g, const H &h) {
    Graph res = graph_detail::productBase(g, h);
    graph_detail::addTensor(res, g, h);
    return res;}

// T: O(n_g * n_h + n_g * m_h + m_g * n_h + m_g * m_h), M: O(n_g * n_h + out); Cartesian edges, then tensor edges; n_g, m_g and n_h, m_h size g and h.
template<class G = Graph, class H = Graph>
Graph strongProduct(const G &g, const H &h) {
    Graph res = graph_detail::productBase(g, h);
    graph_detail::addCartesian(res, g, h);
    graph_detail::addTensor(res, g, h);
    return res;}

// T: O(n_g * n_h + m_g * n_h^2 + n_g * m_h), M: O(n_g * n_h + out); g[h]: a g edge joins its ends' layers completely, then h edges per layer; n_g, m_g and n_h, m_h size g and h.
template<class G = Graph, class H = Graph>
Graph lexicographicProduct(const G &g, const H &h) {
    Graph res = graph_detail::productBase(g, h);
    for (auto e : g.edges) {
        for (int x = 0; x < h.n; ++x) {
            for (int y = g.directed || e.u != e.v ? 0 : x; y < h.n; ++y) { res.addEdge(e.u * h.n + x, e.v * h.n + y, e.w); }}}
    graph_detail::addLayers(res, g, h);
    return res;}

// T: O(n * (n + m)), M: O(n + out); k >= 0, an edge u-v (arc u -> v) of weight 1 iff 0 < hops(u, v) <= k.
template<class G = Graph>
Graph graphPower(const G &g, int k) {
    assert(k >= 0);
    Graph res(g.n, g.directed);
    vector<int> seen(g.n, -1), depth(g.n), queue;
    for (int s = 0; s < g.n; ++s) {
        seen[s] = s; depth[s] = 0; queue.assign(1, s);
        for (int i = 0; i < int(queue.size()) && depth[queue[i]] < k; ++i) {
            int u = queue[i];
            for (int a : g[u]) {
                int v = g.arcs[a].to;
                if (seen[v] != s) { seen[v] = s; depth[v] = depth[u] + 1; queue.push_back(v); }}}
        for (int v = g.directed ? 0 : s + 1; v < g.n; ++v) {
            if (v != s && seen[v] == s) { res.addEdge(s, v); }}}
    return res;}

// T: O(n + m), M: O(n); arcs entering each vertex: the degree when undirected, a loop counted twice.
template<class G = Graph>
vector<int> inDegrees(const G &g) {
    vector<int> res(g.n);
    for (const auto &a : g.arcs) { ++res[a.to]; }
    return res;}

// T: O(deg(u)), M: O(1); smallest ID of an edge u -> v (either orientation when undirected), -1 if none.
template<class G = Graph>
int edgeId(const G &g, int u, int v) {
    assert(0 <= u && u < g.n && 0 <= v && v < g.n);
    for (int a : g[u]) {
        if (g.arcs[a].to == v) { return g.arcs[a].id; }}
    return -1;}

// T: O(n + m + k + l), M: O(n + m); k = vs.size(), l = ids.size(); numbering kept, drops the edges ids and every edge touching vs.
template<class G = Graph>
Subgraph restrictedView(const G &g, const vector<int> &vs, const vector<int> &ids) {
    vector<char> hidden(g.n), dropped(g.edges.size());
    for (int v : vs) { assert(0 <= v && v < g.n); hidden[v] = true; }
    for (int id : ids) { assert(0 <= id && id < int(g.edges.size())); dropped[id] = true; }
    vector<int> keep;
    for (int id = 0; id < int(g.edges.size()); ++id) {
        if (!dropped[id] && !hidden[g.edges[id].u] && !hidden[g.edges[id].v]) { keep.push_back(id); }}
    return edgeSubgraph(g, keep);}

// T: O(n + m), M: O(n + m); the same edges and IDs as undirected edges.
template<class G = Graph>
Graph toUndirected(const G &g) {
    Graph res(g.n, false);
    for (auto e : g.edges) { res.addEdge(e.u, e.v, e.w); }
    return res;}

// T: O(n + m), M: O(n + m); a directed g is copied, undirected edge i becomes arcs 2i (u -> v) and 2i + 1 (v -> u).
template<class G = Graph>
Graph toDirected(const G &g) {
    Graph res(g.n, true);
    for (auto e : g.edges) {
        res.addEdge(e.u, e.v, e.w);
        if (!g.directed) { res.addEdge(e.v, e.u, e.w); }}
    return res;}

// T: O(n_g * n_h + m_g + m_h), M: O(n_g * n_h + m_g + m_h); disjointUnion plus a weight-1 edge u-x (arcs both ways when directed) per u in g, x in h; n_g, m_g and n_h, m_h size g and h.
template<class G = Graph, class H = Graph>
Graph graphJoin(const G &g, const H &h) {
    Graph res = disjointUnion(g, h);
    for (int u = 0; u < g.n; ++u) {
        for (int x = g.n; x < res.n; ++x) {
            res.addEdge(u, x);
            if (g.directed) { res.addEdge(x, u); }}}
    return res;}

namespace graph_detail {
    // T: O(n + m_g + m_h), M: O(n + m_g + m_h); the i-th edge of an endpoint pair in g (ID order) matches the i-th in h; n_g, m_g and n_h, m_h size g and h.
    template<class G, class H>
    array<vector<char>, 2> matchEdges(const G &g, const H &h) {
        assert(g.n == h.n && g.directed == h.directed);
        vector<GraphEdge> es;
        auto add = [&](const auto &x, int side) {
            for (int id = 0; id < int(x.edges.size()); ++id) {
                auto [u, v, w] = x.edges[id];
                es.push_back({x.directed ? u : min(u, v), x.directed ? v : max(u, v), 2 * lng(id) + side});}};
        add(g, 0); add(h, 1);
        countingSort(es, g.n, [](const GraphEdge &e) { return e.v; });
        countingSort(es, g.n, [](const GraphEdge &e) { return e.u; });
        array<vector<char>, 2> res{vector<char>(g.edges.size()), vector<char>(h.edges.size())};
        for (int i = 0, j = 0; i < int(es.size()); i = j) {
            array<int, 2> count{}, rank{};
            for (j = i; j < int(es.size()) && es[j].u == es[i].u && es[j].v == es[i].v; ++j) { ++count[es[j].w & 1]; }
            for (int k = i; k < j; ++k) {
                int side = int(es[k].w & 1);
                res[side][es[k].w >> 1] = rank[side]++ < count[side ^ 1];}}
        return res;}

    // T: O(m), M: O(1); appends the edges of g whose ID satisfies keep, in ID order.
    template<class G, class P>
    void addEdgesIf(Graph &res, const G &g, P keep) {
        for (int id = 0; id < int(g.edges.size()); ++id) {
            if (keep(id)) { res.addEdge(g.edges[id].u, g.edges[id].v, g.edges[id].w); }}}
} // namespace graph_detail

// T: O(n + m_g + m_h), M: O(n + m_g + m_h); same n and directedness, per endpoint pair the larger multiplicity: g's edges, then h's unmatched; n_g, m_g and n_h, m_h size g and h.
template<class G = Graph, class H = Graph>
Graph graphUnion(const G &g, const H &h) {
    auto [mg, mh] = graph_detail::matchEdges(g, h);
    Graph res(g.n, g.directed);
    graph_detail::addEdgesIf(res, g, [](int) { return true; });
    graph_detail::addEdgesIf(res, h, [&](int id) { return !mh[id]; });
    return res;}

// T: O(n + m_g + m_h), M: O(n + m_g + m_h); per endpoint pair the smaller multiplicity, g's first edges; n_g, m_g and n_h, m_h size g and h.
template<class G = Graph, class H = Graph>
Graph graphIntersection(const G &g, const H &h) {
    auto [mg, mh] = graph_detail::matchEdges(g, h);
    Graph res(g.n, g.directed);
    graph_detail::addEdgesIf(res, g, [&](int id) { return bool(mg[id]); });
    return res;}

// T: O(n + m_g + m_h), M: O(n + m_g + m_h); per endpoint pair the multiplicity excess of g, g's last edges; n_g, m_g and n_h, m_h size g and h.
template<class G = Graph, class H = Graph>
Graph graphDifference(const G &g, const H &h) {
    auto [mg, mh] = graph_detail::matchEdges(g, h);
    Graph res(g.n, g.directed);
    graph_detail::addEdgesIf(res, g, [&](int id) { return !mg[id]; });
    return res;}

// T: O(n + m_g + m_h), M: O(n + m_g + m_h); per endpoint pair the multiplicity difference: g's unmatched, then h's; n_g, m_g and n_h, m_h size g and h.
template<class G = Graph, class H = Graph>
Graph graphSymmetricDifference(const G &g, const H &h) {
    auto [mg, mh] = graph_detail::matchEdges(g, h);
    Graph res(g.n, g.directed);
    graph_detail::addEdgesIf(res, g, [&](int id) { return !mg[id]; });
    graph_detail::addEdgesIf(res, h, [&](int id) { return !mh[id]; });
    return res;}

// T: O(n_g * n_h + m_g + n_g * m_h), M: O(n_g * n_h + out); 0 <= root < h.n, copy u of h hangs at (u, root), vertex (u, x) is u * h.n + x; n_g, m_g and n_h, m_h size g and h.
template<class G = Graph, class H = Graph>
Graph rootedProduct(const G &g, const H &h, int root) {
    assert(0 <= root && root < h.n);
    Graph res = graph_detail::productBase(g, h);
    for (auto e : g.edges) { res.addEdge(e.u * h.n + root, e.v * h.n + root, e.w); }
    graph_detail::addLayers(res, g, h);
    return res;}

// T: O(n_g * n_h + m_g + n_g * m_h), M: O(n_g * n_h + out); g, then per u its copy of h (vertex g.n + u * h.n + x) joined to u by weight 1; n_g, m_g and n_h, m_h size g and h.
template<class G = Graph, class H = Graph>
Graph coronaProduct(const G &g, const H &h) {
    assert(g.directed == h.directed && lng(g.n) * (h.n + 1) <= INT_MAX);
    Graph res(g.n * (h.n + 1), g.directed);
    for (auto e : g.edges) { res.addEdge(e.u, e.v, e.w); }
    for (int u = 0; u < g.n; ++u) {
        int base = g.n + u * h.n;
        for (auto f : h.edges) { res.addEdge(base + f.u, base + f.v, f.w); }
        for (int x = base; x < base + h.n; ++x) {
            res.addEdge(u, x);
            if (g.directed) { res.addEdge(x, u); }}}
    return res;}

// T: O(n_g^2 * n_h^2 + m_g + m_h), M: O(n_g^2 + n_h^2 + out); undirected, weight 1, (u, x)-(v, y) iff u != v, x != y and uv, xy both or neither edges; n_g, m_g and n_h, m_h size g and h.
template<class G = Graph, class H = Graph>
Graph modularProduct(const G &g, const H &h) {
    assert(!g.directed);
    Graph res = graph_detail::productBase(g, h);
    auto matrix = [](const auto &x) {
        vector<vector<char>> res(x.n, vector<char>(x.n));
        for (auto e : x.edges) { res[e.u][e.v] = res[e.v][e.u] = true; }
        return res;};
    auto a = matrix(g), b = matrix(h);
    for (int p = 0; p < res.n; ++p) {
        for (int q = p + 1; q < res.n; ++q) {
            int u = p / h.n, x = p % h.n, v = q / h.n, y = q % h.n;
            if (u != v && x != y && a[u][v] == b[x][y]) { res.addEdge(p, q); }}}
    return res;}

// T: O(n + m), M: O(n + m); undirected, vertices v, then copies n + v, then 2n; edge i kept, then per edge (n + u, v), (u, n + v), then (n + v, 2n).
template<class G = Graph>
Graph mycielskian(const G &g) {
    assert(!g.directed && 2 * lng(g.n) + 1 <= INT_MAX);
    Graph res(2 * g.n + 1, false);
    for (auto e : g.edges) { res.addEdge(e.u, e.v, e.w); }
    for (auto e : g.edges) { res.addEdge(g.n + e.u, e.v, e.w); res.addEdge(e.u, g.n + e.v, e.w); }
    for (int v = 0; v < g.n; ++v) { res.addEdge(g.n + v, 2 * g.n); }
    return res;}

// T: O(n + m), M: O(n + m); edge i becomes edges 2i (u, n + i) of weight w and 2i + 1 (n + i, v) of weight 0.
template<class G = Graph>
Graph subdivideEdges(const G &g) {
    assert(lng(g.n) + lng(g.edges.size()) <= INT_MAX);
    Graph res(g.n + int(g.edges.size()), g.directed);
    for (int id = 0; id < int(g.edges.size()); ++id) {
        res.addEdge(g.edges[id].u, g.n + id, g.edges[id].w); res.addEdge(g.n + id, g.edges[id].v, 0);}
    return res;}
