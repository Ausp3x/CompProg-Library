#pragma once

#include "../01-Core/01-template.hpp"
#include "01-graph.hpp"
#include "../02-Data Structures/01-dsu.hpp"
#include "../02-Data Structures/25-heap_deque.hpp"

// T: O(1), M: O(n); parentEdge[v] is the edge ID entering v, -1 at roots; ok false leaves weight 0 and parentEdge empty.
struct DirectedMstResult { bool ok = false; lll weight = 0; vector<int> parentEdge; };

// Arcs from, to in [0, n), |w| < 2^64, IDs copied to parentEdge; loops ignored; any root once built, ok false when a vertex is unreachable.
// S: O(n + m * log(m)), Q: O(n), M: O(n + m)
struct DirectedMst {
    struct Arc { int from, to, id; lll w; };
    static constexpr lll INF = lll(1) << 100;
    int n, top = -1;
    vector<Arc> arcs;
    vector<int> fa, nxt, ed;

    template<class G> requires requires(const G &x) { x.edges; x.directed; }
    explicit DirectedMst(const G &g) : DirectedMst(g.n, [&] {
        assert(g.directed); vector<Arc> res;
        for (int i = 0; i < int(g.edges.size()); ++i) { res.push_back({g.edges[i].u, g.edges[i].v, i, lll(g.edges[i].w)}); }
        return res;}()) {}
    // Tarjan contraction: grow a path along cheapest in-arcs, contract each closed cycle; the INF ring makes everything one node.
    DirectedMst(int N, vector<Arc> es) : n(max(N, 0)), arcs(std::move(es)), fa(2 * size_t(n), -1), nxt(fa), ed(fa) {
        assert(N >= 0 && n <= INT_MAX / 2 && arcs.size() < size_t(INT_MAX) - size_t(n));
        assert(std::ranges::all_of(arcs, [&](const Arc &e) { return 0 <= e.from && e.from < n && 0 <= e.to && e.to < n && -(lll(1) << 64) < e.w && e.w < lll(1) << 64; }));
        for (int v = 0; n > 1 && v < n; ++v) { arcs.push_back({v, (v + 1) % n, -1, INF}); }
        if (!n) { return; }
        LeftistHeap<lll> heap; DSU dsu(2 * n);
        vector<int> q(2 * n), label(2 * n), mark(2 * n);
        vector<lll> cut(2 * n);
        iota(label.begin(), label.end(), 0);
        auto find = [&](int v) { return label[dsu.findSet(v)]; };
        for (auto &e : arcs) { heap.push(q[e.to], e.w); }
        int b = 0, cnt = n; mark[0] = 1;
        while (true) {
            int a = b;
            while (q[b] && a == b) {
                ed[b] = q[b] - 1; cut[b] = heap.top(q[b]); heap.pop(q[b]);
                a = find(arcs[ed[b]].from);}
            if (a == b) { break; }
            if (!mark[a]) { mark[a] = 1; b = a; continue; }
            int c = cnt++;
            for (int x = b, p; x != c; x = p) {
                fa[x] = c; dsu.uniteSets(x, c); label[dsu.findSet(c)] = c;
                heap.applyAdd(q[x], -cut[x]); q[c] = heap.meld(q[c], q[x]);
                p = find(arcs[ed[x]].from); nxt[p == c ? b : p] = x;}
            mark[c] = 1; b = c;}
        top = b;}

    DirectedMstResult arborescence(int root) const {
        assert(0 <= root && root < n);
        DirectedMstResult res{true, 0, vector<int>(n, -1)};
        vector<pair<int, int>> st{{root, top}};
        while (!st.empty()) {
            auto [x, t] = st.back(); st.pop_back();
            for (; x != t; x = fa[x]) {
                for (int u = nxt[x]; u != x; u = nxt[u]) {
                    const Arc &e = arcs[ed[u]];
                    if (e.w == INF) { return {}; }
                    res.weight += e.w; res.parentEdge[e.to] = e.id; st.push_back({e.to, u});}}}
        return res;}
};

// T: O(n + m * log(m)), M: O(n + m); directed, weights of any sign, only positive arcs are chosen; ok is always true.
template<class G>
DirectedMstResult maximumBranching(const G &g) {
    assert(g.directed);
    vector<DirectedMst::Arc> arcs;
    for (int v = 0; v < g.n; ++v) { arcs.push_back({g.n, v, -1, 0}); }
    for (int i = 0; i < int(g.edges.size()); ++i) {
        if (g.edges[i].w > 0) { arcs.push_back({g.edges[i].u, g.edges[i].v, i, -lll(g.edges[i].w)}); }}
    auto res = DirectedMst(g.n + 1, std::move(arcs)).arborescence(g.n);
    res.weight = -res.weight; res.parentEdge.pop_back();
    return res;}
