#pragma once
#include "../01-Core/01-template.hpp"
#include "01-graph.hpp"
#include "../02-Data Structures/00-monoids.hpp"
#include "../02-Data Structures/03-segmenttree.hpp"
#include "../02-Data Structures/12-lazysegmenttree.hpp"

// Undirected simple forest, root=-1 roots each component at its minimum; heavy-first preorder, order[tin[v]] = v; no answer is -1.
// S: O(n), Q: O(log(n)), M: O(n); processPath makes O(log(n)) calls.
struct HLD {
    int n;
    vector<int> parent, parent_edge, component, dep, size, head, tin, tout, order, edge_vertex;

    template<class G = Graph>
    explicit HLD(const G &g, int root = -1) : n(g.n), parent(n, -1), parent_edge(n, -1), component(n, -1), dep(n),
        size(n, 1), head(n), tin(n), tout(n), edge_vertex(g.edges.size(), -1) {
        assert(!g.directed && -1 <= root && root < n);
        vector<int> bfs, heavy(n, -1), roots, stack;
        auto grow = [&](int s) {
            roots.push_back(s); component[s] = s; bfs.push_back(s);
            for (int j = int(bfs.size()) - 1; j < int(bfs.size()); ++j) {
                int u = bfs[j];
                for (int a : g[u]) {
                    auto e = g.arcs[a];
                    if (component[e.to] != -1) { continue; }
                    parent[e.to] = u; parent_edge[e.to] = e.id; component[e.to] = s; dep[e.to] = dep[u] + 1;
                    edge_vertex[e.id] = e.to; bfs.push_back(e.to);}}};
        if (root != -1) { grow(root); }
        for (int s = 0; s < n; ++s) { if (component[s] == -1) { grow(s); }}
        assert(g.edges.size() + roots.size() == size_t(n));
        for (int i = n - 1; i >= 0; --i) {
            int v = bfs[i], p = parent[v];
            if (p == -1) { continue; }
            size[p] += size[v];
            if (heavy[p] == -1 || size[v] >= size[heavy[p]]) { heavy[p] = v; }}
        for (int s : roots) {
            stack.push_back(s);
            while (!stack.empty()) {
                int u = stack.back(); stack.pop_back();
                tin[u] = int(order.size()); tout[u] = tin[u] + size[u]; order.push_back(u);
                head[u] = parent[u] != -1 && heavy[parent[u]] == u ? head[parent[u]] : u;
                for (int a : g[u]) {
                    int v = g.arcs[a].to;
                    if (parent[v] == u && v != heavy[u] && parent_edge[v] == g.arcs[a].id) { stack.push_back(v); }}
                if (heavy[u] != -1) { stack.push_back(heavy[u]); }}}}

    void check(int u) const { assert(0 <= u && u < n); (void)u; }
    int edgeVertex(int id) const { assert(0 <= id && id < int(edge_vertex.size())); return edge_vertex[id]; }
    int getLCA(int u, int v) const {
        check(u); check(v);
        if (component[u] != component[v]) { return -1; }
        while (head[u] != head[v]) {
            if (dep[head[u]] < dep[head[v]]) { swap(u, v); }
            u = parent[head[u]];}
        return dep[u] < dep[v] ? u : v;}
    int distance(int u, int v) const {
        int a = getLCA(u, v); return a == -1 ? -1 : dep[u] + dep[v] - 2 * dep[a];}
    int kthAncestor(int u, lng x) const {
        check(u); assert(x >= 0);
        if (x < 0 || x > dep[u]) { return -1; }
        int k = int(x);
        while (dep[u] - dep[head[u]] < k) {
            k -= dep[u] - dep[head[u]] + 1; u = parent[head[u]];}
        return order[tin[u] - k];}
    // Calls f(l, r, up) for order[l, r) along u -> v, up meaning r - 1 down to l; edges skips the LCA; false across components.
    template<class F>
    bool processPath(int u, int v, bool edges, F f) const {
        check(u); check(v);
        if (component[u] != component[v]) { return false; }
        array<pair<int, int>, 64> down; int k = 0;
        while (head[u] != head[v]) {
            if (dep[head[u]] >= dep[head[v]]) { f(tin[head[u]], tin[u] + 1, true); u = parent[head[u]]; }
            else { down[k++] = {tin[head[v]], tin[v] + 1}; v = parent[head[v]]; }}
        if (dep[u] < dep[v]) { f(tin[u] + edges, tin[v] + 1, false); }
        else if (tin[v] + edges <= tin[u]) { f(tin[v] + edges, tin[u] + 1, true); }
        while (k) { --k; f(down[k].first, down[k].second, false); }
        return true;}
    pair<int, int> subtreeRange(int v, bool edges = false) const { check(v); return {tin[v] + edges, tout[v]}; }
    // Nonempty ranges of v's subtree when v's component is rerooted at r; edges skips the component root's slot.
    vector<pair<int, int>> rerootedSubtreeIntervals(int v, int r, bool edges = false) const {
        check(v); check(r); assert(component[v] == component[r]);
        int c = component[v], l = tin[c] + edges;
        vector<pair<int, int>> res;
        auto add = [&](int x, int y) { if (x < y) { res.push_back({x, y}); }};
        if (v == r) { add(l, tout[c]); }
        else if (tin[v] < tin[r] && tin[r] < tout[v]) {
            int x = kthAncestor(r, dep[r] - dep[v] - 1);
            add(l, tin[x]); add(tout[x], tout[c]);}
        else { add(tin[v] + edges, tout[v]); }
        return res;}
    // T: O(n), M: O(n).
    template<class T>
    vector<T> reorderArray(const vector<T> &a) const {
        assert(int(a.size()) == n);
        vector<T> res(n);
        for (int v = 0; v < n; ++v) { res[tin[v]] = a[v]; }
        return res;}
};

namespace hld_detail {
    template<class A>
    concept Acted = requires { typename A::F; };
    template<class A, bool C>
    using Base = std::conditional_t<C, A, Reversible<A>>;
    template<class A, bool C>
    using Tree = std::conditional_t<Acted<A>, LazySegmentTree<Base<A, C>>, SegmentTree<Base<A, C>>>;
} // namespace hld_detail

// HLD with values on vertices, or on edges by id (edges=true: child slots, roots hold e()); A a monoid or acted monoid, COMMUTATIVE drops reversed products.
// S: O(n), U: O(log(n)) set and subtreeApply, O(log(n)^2) pathApply, Q: O(log(n)) get and subtreeFold, O(log(n)^2) pathFold and maxPath, M: O(n).
template<class A, bool COMMUTATIVE = false>
struct TreeMonoid : HLD {
    using S = typename A::S;
    using B = hld_detail::Base<A, COMMUTATIVE>;
    using T = typename B::S;
    bool edges;
    hld_detail::Tree<A, COMMUTATIVE> seg;

    template<class G = Graph>
    TreeMonoid(const G &g, const vector<S> &values, bool edges = false, int root = -1, A mon = {}) : HLD(g, root), edges(edges),
        seg(layout(values, mon), B{mon}) {}

    static T leaf(const S &x) { if constexpr (COMMUTATIVE) { return x; } else { return {x, x}; }}
    static S forward(const T &x) { if constexpr (COMMUTATIVE) { return x; } else { return x.x; }}
    static S backward(const T &x) { if constexpr (COMMUTATIVE) { return x; } else { return x.y; }}
    const A &monoid() const { return seg.m; }
    int slot(int i) const {
        if (!edges) { check(i); return tin[i]; }
        return tin[max(0, edgeVertex(i))];}
    vector<T> layout(const vector<S> &values, const A &mon) const {
        assert(values.size() == (edges ? edge_vertex.size() : size_t(n)));
        vector<T> res(n, leaf(mon.e()));
        for (int i = 0; i < int(values.size()); ++i) { res[slot(i)] = leaf(values[i]); }
        return res;}

    S get(int i) { return forward(seg.get(slot(i))); }
    void set(int i, const S &x) { seg.set(slot(i), leaf(x)); }
    template<class F>
    bool pathApply(int u, int v, const F &f) { return processPath(u, v, edges, [&](int l, int r, bool) { seg.apply(l, r, f); }); }
    template<class F>
    void subtreeApply(int v, const F &f) { auto [l, r] = subtreeRange(v, edges); seg.apply(l, r, f); }

    pair<bool, S> pathFold(int u, int v) {
        S res = monoid().e();
        bool ok = processPath(u, v, edges, [&](int l, int r, bool up) {
            T x = seg.prod(l, r); res = monoid().op(res, up ? backward(x) : forward(x));});
        return {ok, ok ? res : monoid().e()};}
    S subtreeFold(int v) { auto [l, r] = subtreeRange(v, edges); return forward(seg.prod(l, r)); }
    // Last x on path u -> v with pred(pathFold(u, x)) for monotone pred with pred(e()); -1 across components or when u fails.
    template<class P>
    int maxPath(int u, int v, P pred) {
        S acc = monoid().e(); int res = edges ? u : -1; bool stop = false;
        auto part = [&](int l, int r, bool up) { T x = seg.prod(l, r); return monoid().op(acc, up ? backward(x) : forward(x)); };
        auto reach = [&](int l, int r, bool up, int c) { return up ? (edges ? parent[order[r - c]] : order[r - c]) : order[l + c - 1]; };
        bool ok = processPath(u, v, edges, [&](int l, int r, bool up) {
            if (stop) { return; }
            if (S y = part(l, r, up); pred(y)) { acc = y; res = reach(l, r, up, r - l); return; }
            // pred is monotone only along the path, so the search stays inside [l, r).
            int lo = 0, hi = r - l; stop = true;
            while (hi - lo > 1) {
                int c = std::midpoint(lo, hi);
                if (pred(up ? part(r - c, r, true) : part(l, l + c, false))) { lo = c; } else { hi = c; }}
            if (lo) { res = reach(l, r, up, lo); }});
        return ok ? res : -1;}
};
