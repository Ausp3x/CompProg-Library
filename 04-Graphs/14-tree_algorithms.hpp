#pragma once
#include "../01-Core/01-template.hpp"
#include "01-graph.hpp"
#include "02-traversal.hpp"
#include "07-lca.hpp"

namespace tree_detail {
    // T: O(n), M: O(n); BFS of an undirected tree from root, parent_arc[v] runs parent -> v.
    template<class G = Graph>
    TraversalResult rooted(const G &g, int root) {
        assert(!g.directed && 0 <= root && root < g.n);
        TraversalResult t = bfs(g, {root});
        assert(int(t.order.size()) == g.n && g.edges.size() + 1 == size_t(g.n));
        return t;}

    // T: O(deg(u)), M: O(1); calls f(child, arc parent -> child) in adjacency order.
    template<class G = Graph, class F>
    void children(const G &g, const TraversalResult &t, int u, F f) {
        for (int a : g[u]) {
            int v = g.arcs[a].to;
            if (t.parent_arc[v] == a) { f(v, a); }}}
} // namespace tree_detail

// T: O(1), M: O(n); path vertices[0] .. vertices.back() along arcs, total weight length.
struct TreeDiameter {
    lll length = 0;
    vector<int> vertices, arcs;
};

// T: O(n), M: O(n); undirected tree, n >= 1, full-range lng weights; the heaviest simple path, a single vertex when no edge helps.
template<class G = Graph>
TreeDiameter treeDiameter(const G &g) {
    assert(g.n >= 1);
    if (g.n == 0) { return {}; }
    auto t = tree_detail::rooted(g, 0);
    vector<lll> down(g.n); vector<int> next(g.n, -1);
    TreeDiameter res{0, {0}, {}};
    int top = 0, left = -1, right = -1;
    for (int i = int(t.order.size()) - 1; i >= 0; --i) {
        int u = t.order[i];
        tree_detail::children(g, t, u, [&](int v, int a) {
            lll d = down[v] + g.arcs[a].w;
            if (down[u] + d > res.length) { res.length = down[u] + d; top = u; left = next[u]; right = a; }
            if (d > down[u]) { down[u] = d; next[u] = a; }});}
    vector<int> up;
    for (int a = left; a != -1; a = next[g.arcs[a].to]) { up.push_back(g.arcs[a].rev); }
    reverse(up.begin(), up.end());
    for (int a = right; a != -1; a = next[g.arcs[a].to]) { up.push_back(a); }
    res.arcs = std::move(up); res.vertices = {res.arcs.empty() ? top : g.arcs[res.arcs[0]].from};
    for (int a : res.arcs) { res.vertices.push_back(g.arcs[a].to); }
    return res;}

// T: O(1), M: O(n); ecc[v] = max over u of dist(v, u) >= 0, radius its minimum, center every minimizer ascending.
struct TreeEccentricities {
    lll radius = 0;
    vector<lll> ecc;
    vector<int> center;
};

// T: O(n), M: O(n); undirected tree, n >= 1, full-range lng weights.
template<class G = Graph>
TreeEccentricities treeEccentricities(const G &g) {
    assert(g.n >= 1);
    if (g.n == 0) { return {}; }
    auto t = tree_detail::rooted(g, 0);
    vector<lll> first(g.n), second(g.n), up(g.n);
    vector<int> best(g.n, -1);
    for (int i = int(t.order.size()) - 1; i >= 0; --i) {
        int u = t.order[i];
        tree_detail::children(g, t, u, [&](int v, int a) {
            lll d = first[v] + g.arcs[a].w;
            if (d > first[u]) { second[u] = first[u]; first[u] = d; best[u] = v; }
            else if (d > second[u]) { second[u] = d; }});}
    TreeEccentricities res; res.ecc.resize(g.n);
    for (int u : t.order) {
        tree_detail::children(g, t, u, [&](int v, int a) {
            up[v] = max(up[u], best[u] == v ? second[u] : first[u]) + g.arcs[a].w;});
        res.ecc[u] = max(up[u], first[u]);}
    res.radius = *std::ranges::min_element(res.ecc);
    for (int v = 0; v < g.n; ++v) { if (res.ecc[v] == res.radius) { res.center.push_back(v); }}
    return res;}

// T: O(n), M: O(n); undirected tree, n >= 1; the one or two vertices whose removal leaves components of at most n / 2 vertices, ascending.
template<class G = Graph>
vector<int> treeCentroid(const G &g) {
    assert(g.n >= 1);
    if (g.n == 0) { return {}; }
    auto t = tree_detail::rooted(g, 0);
    vector<int> size(g.n, 1), heavy(g.n), res;
    for (int i = int(t.order.size()) - 1; i > 0; --i) {
        int v = t.order[i], p = t.parent[v];
        size[p] += size[v]; heavy[p] = max(heavy[p], size[v]);}
    for (int v = 0; v < g.n; ++v) {
        if (2 * max(heavy[v], g.n - size[v]) <= g.n) { res.push_back(v); }}
    return res;}

// T: O(n), M: O(n); undirected tree, full-range lng weights; res[v] = sum over u of dist(v, u), exact in lll.
template<class G = Graph>
vector<lll> sumOfDistances(const G &g) {
    if (g.n == 0) { return {}; }
    auto t = tree_detail::rooted(g, 0);
    vector<int> size(g.n, 1); vector<lll> res(g.n);
    for (int i = int(t.order.size()) - 1; i > 0; --i) {
        int v = t.order[i], p = t.parent[v];
        size[p] += size[v]; res[0] += lll(size[v]) * g.arcs[t.parent_arc[v]].w;}
    for (int i = 1; i < int(t.order.size()); ++i) {
        int v = t.order[i];
        res[v] = res[t.parent[v]] + lll(g.n - 2 * size[v]) * g.arcs[t.parent_arc[v]].w;}
    return res;}

// T: O(n), M: O(n); undirected simple forest, root=-1 roots each component at its minimum; preorder in adjacency order, subtree of v is order[tin[v], tout[v]).
struct EulerTour {
    vector<int> tin, tout, order, parent, dep;

    template<class G = Graph>
    explicit EulerTour(const G &g, int root = -1) {
        lca_detail::Forest f(g, root);
        tin = std::move(f.t_in); tout = std::move(f.t_out); order = std::move(f.order);
        parent = std::move(f.parent); dep = std::move(f.dep);
        for (int &x : tout) { ++x; }}

    pair<int, int> subtreeRange(int v) const {
        assert(0 <= v && v < int(tin.size())); return {tin[v], tout[v]};}
};

// Undirected simple forest, roots as EulerTour; order is BFS by depth with children in adjacency order, id[order[i]] = i; ranges are half-open over order.
// S: O(n), Q: O(log(n)) levelRange, O((min(r, h) + 1) * log(n)) diskRanges for height h, M: O(n).
struct BfsNumbering {
    vector<int> order, id, parent, dep, start, tin, tout, key;

    template<class G = Graph>
    explicit BfsNumbering(const G &g, int root = -1) : id(g.n) {
        lca_detail::Forest f(g, root);
        for (int v : f.order) { if (f.parent[v] == -1) { order.push_back(v); }}
        for (int i = 0; i < int(order.size()); ++i) {
            int u = order[i];
            for (int a : g[u]) { if (f.parent_edge[g.arcs[a].to] == g.arcs[a].id && f.parent[g.arcs[a].to] == u) { order.push_back(g.arcs[a].to); }}}
        for (int i = 0; i < int(order.size()); ++i) {
            id[order[i]] = i; key.push_back(f.t_in[order[i]]);
            while (int(start.size()) <= f.dep[order[i]]) { start.push_back(i); }}
        start.push_back(int(order.size()));
        parent = std::move(f.parent); dep = std::move(f.dep); tin = std::move(f.t_in); tout = std::move(f.t_out);
        for (int &x : tout) { ++x; }}

    pair<int, int> levelRange(int v, int d) const {
        assert(0 <= v && v < int(id.size()));
        if (d < dep[v] || d >= int(start.size()) - 1) { return {0, 0}; }
        auto l = key.begin() + start[d], r = key.begin() + start[d + 1];
        int x = int(lower_bound(l, r, tin[v]) - key.begin()), y = int(lower_bound(l, r, tout[v]) - key.begin());
        return x < y ? pair(x, y) : pair(0, 0);}
    vector<pair<int, int>> diskRanges(int v, int r) const {
        assert(0 <= v && v < int(id.size()) && r >= 0);
        vector<int> up{v};
        while (int(up.size()) <= min(r, dep[v])) { up.push_back(parent[up.back()]); }
        vector<pair<int, int>> res;
        for (lng d = max(0, dep[v] - r); d <= lng(dep[v]) + r && d + 1 < lng(start.size()); ++d) {
            lng k = min(lng(dep[v]), (r - d + dep[v]) / 2);
            auto range = levelRange(up[k], int(d));
            if (range.first < range.second) { res.push_back(range); }}
        return res;}
};

// Undirected tree, n >= 1; R has S, op, e (commutative over sibling results), addEdge(x, e) lifting the result x of the part rooted at e.to across arc e, addVertex(x, v) closing v.
// T: O(n) calls of R, M: O(n); get(v) roots the tree at v, get(v, p) is the part containing v after deleting edge (v, p), rooted at v.
template<class R>
struct Rerooting {
    using S = typename R::S;
    [[no_unique_address]] R m;
    vector<int> parent;
    vector<S> down, up, all;

    template<class G = Graph>
    explicit Rerooting(const G &g, R mon = {}) : m(std::move(mon)) {
        auto t = tree_detail::rooted(g, 0);
        vector<S> lift(g.n, m.e()), pre, suf;
        down.assign(g.n, m.e()); up.assign(g.n, m.e()); all.assign(g.n, m.e()); parent = t.parent;
        for (int i = int(t.order.size()) - 1; i >= 0; --i) {
            int u = t.order[i]; S x = m.e();
            tree_detail::children(g, t, u, [&](int v, int) { x = m.op(x, lift[v]); });
            down[u] = m.addVertex(x, u);
            if (i) { lift[u] = m.addEdge(down[u], g.arcs[t.parent_arc[u]]); }}
        vector<int> kids;
        for (int u : t.order) {
            kids.clear();
            tree_detail::children(g, t, u, [&](int v, int) { kids.push_back(v); });
            int k = int(kids.size());
            pre.assign(1, t.parent[u] != -1 ? m.addEdge(up[u], g.arcs[g.arcs[t.parent_arc[u]].rev]) : m.e());
            suf.assign(k + 1, m.e());
            for (int j = 0; j < k; ++j) { pre.push_back(m.op(pre[j], lift[kids[j]])); }
            for (int j = k - 1; j >= 0; --j) { suf[j] = m.op(lift[kids[j]], suf[j + 1]); }
            for (int j = 0; j < k; ++j) { up[kids[j]] = m.addVertex(m.op(pre[j], suf[j + 1]), u); }
            all[u] = m.addVertex(pre[k], u);}}

    S get(int v) const { assert(0 <= v && v < int(all.size())); return all[v]; }
    S get(int v, int p) const {
        assert(0 <= v && v < int(all.size()) && 0 <= p && p < int(all.size()));
        if (parent[v] == p) { return down[v]; }
        assert(parent[p] == v); return up[p];}
};

// T: O(n) calls of R, M: O(n); undirected tree, R as Rerooting (op may be noncommutative: children in adjacency order); res[v] is v's subtree under root.
template<class G = Graph, class R>
vector<typename R::S> subtreeDp(const G &g, int root, R mon = {}) {
    auto t = tree_detail::rooted(g, root);
    vector<typename R::S> res(g.n, mon.e());
    for (int i = int(t.order.size()) - 1; i >= 0; --i) {
        int u = t.order[i]; auto x = mon.e();
        tree_detail::children(g, t, u, [&](int v, int a) { x = mon.op(x, mon.addEdge(res[v], g.arcs[a])); });
        res[u] = mon.addVertex(x, u);}
    return res;}

// Undirected tree, k >= 0; R has S, add, zero, mul, one and closeVertex(v, dp), dp[j] = add over child choices of total j of the mul of their entries.
// T: O(n * (min(n, k) + 1)) calls of R when closeVertex leaves at most min(size(v), k) + 1 entries, M: O(n); returns the root's vector, every vector cut to k + 1 entries.
template<class G = Graph, class R>
vector<typename R::S> subtreeMergeDp(const G &g, int root, int k, R mon = {}) {
    assert(k >= 0);
    using S = typename R::S;
    auto t = tree_detail::rooted(g, root);
    vector<vector<S>> dp(g.n);
    for (int i = int(t.order.size()) - 1; i >= 0; --i) {
        int u = t.order[i]; vector<S> cur{mon.one()};
        tree_detail::children(g, t, u, [&](int v, int) {
            vector<S> &b = dp[v];
            if (cur.empty() || b.empty()) { cur.clear(); }
            else {
                vector<S> res(min(cur.size() + b.size() - 1, size_t(k) + 1), mon.zero());
                for (int x = 0; x < int(cur.size()); ++x) {
                    for (int y = 0; y < int(b.size()) && x + y <= k; ++y) { res[x + y] = mon.add(res[x + y], mon.mul(cur[x], b[y])); }}
                cur = std::move(res);}
            vector<S>().swap(b);});
        mon.closeVertex(u, cur);
        if (cur.size() > size_t(k) + 1) { cur.resize(size_t(k) + 1); }
        dp[u] = std::move(cur);}
    return std::move(dp[root]);}

// T: O(1), M: O(k); vertices in preorder, parent[i] indexes vertices, -1 at a root of the result.
struct VirtualTreeResult {
    vector<int> vertices, parent;
};

// S: O(n), Q: O(k * log(k)) for k = |vs|, M: O(n); undirected simple forest, roots as LCA; build closes vs (duplicates allowed) under LCA, one tree per touched component.
struct VirtualTree {
    EulerLCA lca;

    template<class G = Graph>
    explicit VirtualTree(const G &g, int root = -1) : lca(g, root) {}

    VirtualTreeResult build(vector<int> vs) const {
        for (int v : vs) { lca.check(v); }
        auto byTin = [&](int u, int v) { return lca.t_in[u] < lca.t_in[v]; };
        sort(vs.begin(), vs.end(), byTin); vs.erase(unique(vs.begin(), vs.end()), vs.end());
        for (int i = 0, k = int(vs.size()); i + 1 < k; ++i) {
            if (lca.component[vs[i]] == lca.component[vs[i + 1]]) { vs.push_back(lca.getLCA(vs[i], vs[i + 1])); }}
        sort(vs.begin(), vs.end(), byTin); vs.erase(unique(vs.begin(), vs.end()), vs.end());
        VirtualTreeResult res{vs, vector<int>(vs.size(), -1)};
        for (int i = 1; i < int(vs.size()); ++i) {
            if (lca.component[vs[i - 1]] != lca.component[vs[i]]) { continue; }
            int a = lca.getLCA(vs[i - 1], vs[i]);
            res.parent[i] = int(lower_bound(vs.begin(), vs.end(), a, byTin) - vs.begin());}
        return res;}
};

// T: O(n), M: O(n); undirected tree, n >= 2; repeatedly removes the smallest leaf and records its neighbour.
template<class G = Graph>
vector<int> pruferEncode(const G &g) {
    assert(g.n >= 2);
    auto t = tree_detail::rooted(g, g.n - 1);
    vector<int> degree(g.n), res;
    for (int v = 0; v < g.n; ++v) { degree[v] = int(g[v].size()); }
    int ptr = 0;
    while (ptr < g.n - 1 && degree[ptr] != 1) { ++ptr; }
    for (int leaf = ptr, i = 0; i < g.n - 2; ++i) {
        int v = max(0, t.parent[leaf]); res.push_back(v);
        if (--degree[v] == 1 && v < ptr) { leaf = v; continue; }
        do { ++ptr; } while (ptr < g.n - 1 && degree[ptr] != 1);
        leaf = min(ptr, g.n - 1);}
    return res;}

// T: O(n), M: O(n); values in [0, n) for n = |code| + 2; the unique labelled tree with that code, edges in removal order, weight 1.
inline Graph pruferDecode(const vector<int> &code) {
    assert(code.size() <= size_t(INT_MAX - 2));
    int n = int(code.size()) + 2; vector<int> degree(n, 1);
    for (int v : code) { assert(0 <= v && v < n); ++degree[std::clamp(v, 0, n - 1)]; }
    Graph res(n);
    int ptr = 0;
    while (degree[ptr] != 1) { ++ptr; }
    int leaf = ptr;
    for (int x : code) {
        int v = std::clamp(x, 0, n - 1); res.addEdge(leaf, v);
        if (--degree[v] == 1 && v < ptr) { leaf = v; continue; }
        do { ++ptr; } while (ptr < n - 1 && degree[ptr] != 1);
        leaf = ptr;}
    res.addEdge(leaf, n - 1);
    return res;}
