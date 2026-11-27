#pragma once

#include "../01-Core/01-template.hpp"
#include "001-graph.hpp"
#include "../02-Data Structures/001-dsu.hpp"
#include "../02-Data Structures/004-sparsetable.hpp"

namespace lca_detail {
    // S: O(n), Q: O(1), M: O(n); iterative DFS forest with inclusive [t_in, t_out] subtree intervals.
    struct Forest {
        int n, root, timer = 0;
        vector<int> parent, parent_edge, component, dep, t_in, t_out, order, postorder;
        vector<lll> weight;

        template<class G = Graph>
        explicit Forest(const G &g, int r) : n(g.n), root(r), parent(n, -1), parent_edge(n, -1),
            component(n, -1), dep(n), t_in(n), t_out(n), weight(n) {
            assert(!g.directed && -1 <= r && r < n);
            if (r == -1 && n) { root = 0; }
            vector<pair<int, int>> stack;
            auto visit = [&](int u, int p, int id, int c, lng w) {
                parent[u] = p; parent_edge[u] = id; component[u] = c;
                if (p != -1) { dep[u] = dep[p] + 1; weight[u] = weight[p] + lll(w); }
                t_in[u] = timer++; order.push_back(u); stack.push_back({u, 0});};
            auto traverse = [&](int s) {
                visit(s, -1, -1, s, 0);
                while (!stack.empty()) {
                    auto &[u, i] = stack.back();
                    if (i == int(g[u].size())) {
                        t_out[u] = timer - 1; postorder.push_back(u); stack.pop_back(); continue;}
                    auto e = g.arcs[g[u][i++]];
                    if (e.id == parent_edge[u]) { continue; }
                    assert(component[e.to] == -1);
                    if (component[e.to] != -1) { continue; }
                    visit(e.to, u, e.id, s, e.w);}};
            if (root != -1) { traverse(root); }
            for (int u = 0; u < n; ++u) { if (component[u] == -1) { traverse(u); }}}

        void check(int u) const { assert(0 <= u && u < n); (void)u; }
        bool covers(int u, int v) const { return t_in[u] <= t_in[v] && t_out[v] <= t_out[u]; }
        bool isAncestor(int u, int v) const { check(u); check(v); return covers(u, v); }
    };

    // T: O(1), M: O(1); the SparseTable monoid of SparseTableLCA.
    struct MinInt { using S = int; static S op(S a, S b) { return min(a, b); } };

    // T: O(1) lca queries per call, M: O(1); path queries shared by the LCA structs D through D::lca.
    template<class D>
    struct Paths : Forest {
        using Forest::Forest;

        int distance(int u, int v) const {
            int a = static_cast<const D &>(*this).lca(u, v); return a == -1 ? -1 : (dep[u] - dep[a]) + (dep[v] - dep[a]);}
        bool distanceWeighted(int u, int v, lll &out) const {
            int a = static_cast<const D &>(*this).lca(u, v);
            if (a == -1) { return false; }
            out = weight[u] + weight[v] - 2 * weight[a]; return true;}
        int rerootedLCA(int u, int v, int r) const {
            const D &t = static_cast<const D &>(*this);
            check(r); int a = t.lca(u, v);
            if (a == -1 || component[u] != component[r]) { return -1; }
            int b = t.lca(u, r), c = t.lca(v, r);
            return dep[a] >= dep[b] && dep[a] >= dep[c] ? a : dep[b] >= dep[c] ? b : c;}
    };
} // namespace lca_detail

// Undirected simple forest, lng weights; root=-1 roots each component at its minimum; no answer is -1.
// S: O(n * log(n)), Q: O(log(n)), M: O(n * log(n)); isAncestor is O(1).
struct LCA : lca_detail::Paths<LCA> {
    int l;
    vector<vector<int>> up;

    template<class G = Graph>
    explicit LCA(const G &g, int root = -1) : Paths(g, root), l(int(std::bit_width(uint(max(1, n)))) - 1),
        up(n, vector<int>(l + 1)) {
        for (int u : order) {
            up[u][0] = parent[u] == -1 ? u : parent[u];
            for (int j = 1; j <= l; ++j) { up[u][j] = up[up[u][j - 1]][j - 1]; }}}

    int kthAncestor(int u, lng k) const {
        check(u); assert(k >= 0);
        if (k < 0 || k > dep[u]) { return -1; }
        for (int j = 0; k; ++j, k >>= 1) { if (k & 1) { u = up[u][j]; }}
        return u;}
    int lca(int u, int v) const {
        check(u); check(v);
        if (component[u] != component[v]) { return -1; }
        if (covers(u, v)) { return u; }
        if (covers(v, u)) { return v; }
        for (int j = l; j >= 0; --j) { if (!covers(up[u][j], v)) { u = up[u][j]; }}
        return parent[u];}
    int jump(int u, int v, lng k) const {
        assert(k >= 0); int a = lca(u, v);
        if (a == -1) { return -1; }
        int left = dep[u] - dep[a], length = left + dep[v] - dep[a];
        return k > length ? -1 : k <= left ? kthAncestor(u, k) : kthAncestor(v, length - k);}
};

// Labels on vertices, or on child vertices for edges (edges=true skips the LCA), under a monoid M (S, op, e); path order u -> v.
// S: O(n * log(n)), Q: O(log(n)), M: O(n * log(n)); no answer is {false, e()}.
template<class M>
struct LCAFold : LCA {
    using S = typename M::S;
    [[no_unique_address]] M m;
    vector<vector<S>> rise, fall;

    template<class G = Graph>
    LCAFold(const G &g, const vector<S> &values, M mon = {}, int root = -1) : LCA(g, root), m(std::move(mon)), rise(n), fall(n) {
        assert(values.size() == size_t(n));
        for (int u : order) {
            rise[u].assign(l + 1, values[u]); fall[u].assign(l + 1, values[u]);
            for (int j = 1; j <= l; ++j) {
                int v = up[u][j - 1];
                rise[u][j] = m.op(rise[u][j - 1], rise[v][j - 1]); fall[u][j] = m.op(fall[v][j - 1], fall[u][j - 1]);}}}

    pair<bool, S> pathFold(int u, int v, bool edges = false) const {
        int a = lca(u, v);
        if (a == -1) { return {false, m.e()}; }
        S left = m.e(), right = m.e();
        for (int k = dep[u] - dep[a], j = 0; k; ++j, k >>= 1) { if (k & 1) { left = m.op(left, rise[u][j]); u = up[u][j]; }}
        for (int k = dep[v] - dep[a], j = 0; k; ++j, k >>= 1) { if (k & 1) { right = m.op(fall[v][j], right); v = up[v][j]; }}
        if (!edges) { left = m.op(left, rise[a][0]); }
        return {true, m.op(left, right)};}
};

// Same contract as LCA with 2 * n + 1 <= INT_MAX; Farach-Colton-Bender RMQ, no kthAncestor or jump.
// S: O(n), Q: O(1), M: O(n).
struct EulerLCA : lca_detail::Paths<EulerLCA> {
    int block_size;
    vector<int> first, tour, mask;
    vector<vector<int>> table;
    vector<vector<uint8_t>> micro;

    template<class G = Graph>
    explicit EulerLCA(const G &g, int root = -1) : Paths(g, root), first(n) {
        assert(n <= (INT_MAX - 1) / 2); tour.reserve(2 * n + 1); tour.push_back(-1);
        int cur = -1;
        for (int u : order) {
            while (cur != parent[u]) { cur = parent[cur]; tour.push_back(cur); }
            first[u] = int(tour.size()); tour.push_back(u); cur = u;}
        while (cur != -1) { cur = parent[cur]; tour.push_back(cur); }
        int m = int(tour.size()); block_size = max(1, (int(std::bit_width(uint(m))) - 1) / 2);
        int count = (m - 1) / block_size + 1;
        mask.assign(count, 0); table.push_back(vector<int>(count));
        micro.resize(1 << (block_size - 1));
        for (int b = 0; b < count; ++b) {
            int start = b * block_size, length = min(block_size, m - start), best = start;
            for (int j = 1; j < length; ++j) {
                if (height(start + j) > height(start + j - 1)) { mask[b] |= 1 << (j - 1); }
                best = better(best, start + j);}
            table[0][b] = best; auto &local = micro[mask[b]];
            if (!local.empty()) { continue; }
            local.resize(block_size * block_size); vector<int> h(block_size);
            for (int j = 1; j < block_size; ++j) { h[j] = h[j - 1] + ((mask[b] >> (j - 1) & 1) ? 1 : -1); }
            for (int l = 0; l < block_size; ++l) {
                int pos = l;
                for (int r = l; r < block_size; ++r) {
                    if (h[r] < h[pos]) { pos = r; }
                    local[l * block_size + r] = uint8_t(pos);}}}
        for (int j = 1; (1 << j) <= count; ++j) {
            table.push_back(vector<int>(count - (1 << j) + 1));
            for (int i = 0; i < int(table[j].size()); ++i) {
                table[j][i] = better(table[j - 1][i], table[j - 1][i + (1 << (j - 1))]);}}}

    int height(int i) const { return tour[i] == -1 ? -1 : dep[tour[i]]; }
    int better(int i, int j) const { return height(i) <= height(j) ? i : j; }
    int inside(int b, int l, int r) const { return b * block_size + micro[mask[b]][l * block_size + r]; }
    int lca(int u, int v) const {
        check(u); check(v);
        if (component[u] != component[v]) { return -1; }
        int l = first[u], r = first[v]; if (l > r) { swap(l, r); }
        int a = l / block_size, b = r / block_size;
        if (a == b) { return tour[inside(a, l % block_size, r % block_size)]; }
        int best = better(inside(a, l % block_size, block_size - 1), inside(b, 0, r % block_size));
        if (a + 1 < b) {
            int j = int(std::bit_width(uint(b - a - 1))) - 1;
            best = better(best, better(table[j][a + 1], table[j][b - (1 << j)]));}
        return tour[best];}
};

// Same contract as LCA; the LCA of u != v is the parent with the least t_in over preorder (t_in[u], t_in[v]], no kthAncestor or jump.
// S: O(n * log(n)), Q: O(1), M: O(n * log(n)).
struct SparseTableLCA : lca_detail::Paths<SparseTableLCA> {
    SparseTable<lca_detail::MinInt> table;

    template<class G = Graph>
    explicit SparseTableLCA(const G &g, int root = -1) : Paths(g, root) {
        vector<int> a(n);
        for (int i = 0; i < n; ++i) { int p = parent[order[i]]; a[i] = p == -1 ? -1 : t_in[p]; }
        table = SparseTable<lca_detail::MinInt>(std::move(a));}

    int lca(int u, int v) const {
        check(u); check(v);
        if (component[u] != component[v]) { return -1; }
        if (u == v) { return u; }
        int l = t_in[u], r = t_in[v];
        if (l > r) { swap(l, r); }
        return order[table.prod(l + 1, r + 1)];}
};

// Same forest contract as LCA; answers keep query order, -1 across components.
// T: O((n + q) * alpha(n)) amortized, M: O(n + q); alpha is inverse Ackermann.
template<class G = Graph>
vector<int> lcaOffline(const G &g, const vector<pair<int, int>> &queries, int root = -1) {
    assert(queries.size() <= size_t(INT_MAX)); lca_detail::Forest f(g, root);
    vector<vector<pair<int, int>>> adj(g.n); vector<int> res(queries.size(), -1), ancestor(g.n);
    vector<bool> done(g.n); DSU dsu(g.n); iota(ancestor.begin(), ancestor.end(), 0);
    for (int i = 0; i < int(queries.size()); ++i) {
        auto [u, v] = queries[i]; f.check(u); f.check(v);
        adj[u].push_back({v, i}); adj[v].push_back({u, i});}
    for (int u : f.postorder) {
        done[u] = true;
        for (auto [v, i] : adj[u]) {
            if (done[v] && f.component[u] == f.component[v]) { res[i] = ancestor[dsu.findSet(v)]; }}
        int p = f.parent[u];
        if (p != -1) { dsu.uniteSets(u, p); ancestor[dsu.findSet(p)] = p; }}
    return res;}

// Median of a, b, c (the vertex on all three pairwise paths), for any LCA struct here; -1 unless all three are connected.
// T: O(1) LCA queries, M: O(1).
template<class L = LCA>
int meet(const L &t, int a, int b, int c) { return t.rerootedLCA(a, b, c); }

// LCA of a vertex set, for any LCA struct here; -1 when empty or across components.
// T: O(k) plus one LCA query for k vertices, M: O(1).
template<class L = LCA>
int lcaSet(const L &t, const vector<int> &vs) {
    if (vs.empty()) { return -1; }
    int lo = vs[0], hi = vs[0]; bool same = true;
    for (int u : vs) {
        t.check(u); same &= t.component[u] == t.component[lo];
        if (t.t_in[u] < t.t_in[lo]) { lo = u; }
        if (t.t_in[u] > t.t_in[hi]) { hi = u; }}
    return same ? t.lca(lo, hi) : -1;}

// Endpoints of path(a, b) & path(c, d), nearest c first, for any LCA struct here; empty is {-1, -1}.
// T: O(1) LCA queries, M: O(1).
template<class L = LCA>
pair<int, int> pathIntersection(const L &t, int a, int b, int c, int d) {
    t.check(a); t.check(b); t.check(c); t.check(d);
    int k = t.component[a];
    if (t.component[b] != k || t.component[c] != k || t.component[d] != k) { return {-1, -1}; }
    int x = meet(t, a, b, c), y = meet(t, a, b, d);
    if (x != y || meet(t, c, d, x) == x) { return {x, y}; }
    return {-1, -1};}
