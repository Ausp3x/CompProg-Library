#pragma once

#include "../01-Core/01-template.hpp"
#include "01-graph.hpp"
#include "../02-Data Structures/00-monoids.hpp"
#include "../02-Data Structures/02-fenwick.hpp"
#include "../02-Data Structures/09-rollbackdsu.hpp"
#include "../02-Data Structures/63-offline_deletion.hpp"

// Vertices [0, n), times [0, T), T <= 2^29; M a commutative monoid of vertex values, all e() initially; answers come in increasing t.
// k intervals: S: O(T), U: O(log(T)) (insertEdge, eraseEdge O(log(k)) more), Q: run, bipartiteness O(n + T + k * log(T) * log(n)), M: O(n + T + k * log(T))
template<typename M = Sum<int>>
struct OfflineDynamicConnectivity {
    using S = typename M::S;
    int n;
    [[no_unique_address]] M m;
    OfflineDeletion<pair<int, int>> od;
    vector<S> val;

    OfflineDynamicConnectivity(int N, int T, M mon = {}) : n(max(N, 0)), m(std::move(mon)), od(T) { assert(N >= 0); }

    void check(int u) const { assert(0 <= u && u < n); (void)u; }
    void addEdge(int l, int r, int u, int v) { check(u); check(v); od.addInterval(l, r, {min(u, v), max(u, v)}); }
    void insertEdge(int t, int u, int v) { check(u); check(v); od.insert(t, {min(u, v), max(u, v)}); }
    void eraseEdge(int t, int u, int v) { check(u); check(v); od.erase(t, {min(u, v), max(u, v)}); }
    void addValue(int l, int r, int u, const S &x) {
        check(u); assert(val.size() < size_t(INT_MAX));
        od.addInterval(l, r, {u, ~int(val.size())}); val.push_back(x);}
    void query(int t) { od.addQuery(t); }

    template<typename F>
    void run(F answer) {
        RollbackDSUAggregate<M> dsu(vector<S>(n, m.e()), m);
        od.solve([&](const pair<int, int> &x) {
                if (x.second >= 0) { dsu.uniteSets(x.first, x.second); }
                else { dsu.addValue(x.first, val[~x.second]); }},
            [&] { dsu.undo(); }, [&](int t) { answer(t, std::as_const(dsu)); });}
    // Double cover: u, v and u + n, v + n share a set iff an even walk joins them, so an edge inside one set closes an odd cycle.
    template<typename F>
    void bipartiteness(F answer) {
        RollbackDSU dsu(2 * n); vector<int8_t> st; int odd = 0;
        od.solve([&](const pair<int, int> &x) {
                auto [u, v] = x; int8_t s = -1;
                if (v >= 0) { s = int8_t(dsu.isSameSet(u, v)); odd += s; dsu.uniteSets(u, v + n); dsu.uniteSets(u + n, v); }
                st.push_back(s);},
            [&] {
                if (st.back() >= 0) { odd -= st.back(); dsu.undo(); dsu.undo(); }
                st.pop_back();},
            [&](int t) { answer(t, odd == 0); });}
};

// Vertices [0, n); bridges under edge insertions, loops and multiedges allowed; find returns a 2-edge-connected component representative.
// S: O(n), U: O(log(n)) amortized, Q: O(log(n)) amortized, M: O(n)
struct IncrementalBridgeConnectivity {
    int n, bridges = 0, iter = 0;
    vector<int> par, two, cc, sz, stamp, pa, pb;

    explicit IncrementalBridgeConnectivity(int N) : n(N), par(max(N, 0), -1), two(max(N, 0)), cc(two), sz(max(N, 0), 1), stamp(two) {
        assert(n >= 0); iota(two.begin(), two.end(), 0); iota(cc.begin(), cc.end(), 0);}

    void check(int u) const { assert(0 <= u && u < n); (void)u; }
    // Bridges form a forest over component representatives: par links them, cc is a DSU of trees rooted at its representative.
    void addEdge(int u, int v) {
        u = find(u); v = find(v);
        if (u == v) { return; }
        int cu = root(u), cv = root(v);
        if (cu != cv) {
            ++bridges;
            if (sz[cu] > sz[cv]) { swap(u, v); swap(cu, cv); }
            for (int x = u, last = -1; x != -1;) { int p = par[x] == -1 ? -1 : find(par[x]); par[x] = last; cc[x] = u; last = x; x = p; }
            sz[u] = sz[cu]; par[u] = cc[u] = v; sz[cv] += sz[u];
            return;}
        ++iter; pa.clear(); pb.clear();
        int l = -1;
        while (l == -1) {
            if (u != -1) { u = find(u); pa.push_back(u); if (stamp[u] == iter) { l = u; break; } stamp[u] = iter; u = par[u]; }
            if (v != -1) { v = find(v); pb.push_back(v); if (stamp[v] == iter) { l = v; break; } stamp[v] = iter; v = par[v]; }}
        for (auto *p : {&pa, &pb}) {
            for (int x : *p) {
                two[x] = l;
                if (x == l) { break; }
                --bridges;}}}

    int find(int v) {
        check(v); int r = v;
        while (two[r] != r) { r = two[r]; }
        while (two[v] != r) { int p = two[v]; two[v] = r; v = p; }
        return r;}
    int root(int v) {
        int r = find(v);
        while (cc[r] != r) { r = find(cc[r]); }
        for (v = find(v); v != r;) { int p = find(cc[v]); cc[v] = r; v = p; }
        return r;}
    bool twoEdgeConnected(int u, int v) { return find(u) == find(v); }
    bool connected(int u, int v) { return root(u) == root(v); }
    int bridgeCount() const { return bridges; }
};

// T: O(n + m * log(m) * alpha(n) + q * log(m)), M: O(n + m + q), alpha inverse Ackermann; undirected; components of (V, edges l..r-1) for each [l, r).
template<class G>
vector<int> rangeEdgeComponentCount(const G &g, const vector<pair<int, int>> &queries) {
    assert(!g.directed);
    int n = g.n, m = int(g.edges.size());
    // b[i]: least j > i whose edge joins edge i's ends within edges i+1..j (m if none, i for a loop); i is in the max-ID forest of [0, r) iff i < r <= b[i].
    vector<int> b(m), par(n), rep(n), seen(n, -1), mark(n, -1);
    int cur = 0;
    // Union-find on vertex IDs restarted per call by the seen stamp: a root holds -size, rep a terminal of its component or -1.
    auto find = [&](int x) {
        if (seen[x] != cur) { seen[x] = cur; par[x] = -1; rep[x] = mark[x] == cur ? x : -1; }
        for (; par[x] >= 0; x = par[x]) {
            if (par[par[x]] >= 0) { par[x] = par[par[x]]; }}
        return x;};
    // Kruskal over edges [el, er) (weight = ID) then t, keeping the ends of edges [l, r) as terminals.
    auto compress = [&](int el, int er, const vector<array<int, 3>> &t, int l, int r) {
        ++cur; vector<array<int, 3>> res;
        for (int i = l; i < r; ++i) { mark[g.edges[i].u] = mark[g.edges[i].v] = cur; }
        auto add = [&](int x, int y, int w) {
            int a = find(x), c = find(y);
            if (a == c) { return; }
            if (rep[a] != -1 && rep[c] != -1) { res.push_back({rep[a], rep[c], w}); }
            if (par[a] > par[c]) { swap(a, c); }
            par[a] += par[c]; par[c] = a;
            if (rep[a] == -1) { rep[a] = rep[c]; }};
        for (int i = el; i < er; ++i) { add(g.edges[i].u, g.edges[i].v, i); }
        for (auto &[x, y, w] : t) { add(x, y, w); }
        return res;};
    // t is the bottleneck forest of edges [r, m) on the ends of edges [l, r), weights ascending: compressing keeps bottlenecks.
    auto solve = [&](auto &&self, int l, int r, vector<array<int, 3>> t) -> void {
        if (r - l == 1) {
            b[l] = g.edges[l].u == g.edges[l].v ? l : t.empty() ? m : t[0][2];
            return;}
        int mid = std::midpoint(l, r);
        auto tl = compress(mid, r, t, l, mid), tr = compress(0, 0, t, mid, r);
        vector<array<int, 3>>().swap(t);
        self(self, l, mid, std::move(tl)); self(self, mid, r, std::move(tr));};
    if (m) { solve(solve, 0, m, {}); }
    vector<vector<int>> by_b(m + 1), by_r(m + 1);
    for (int i = 0; i < m; ++i) {
        if (b[i] > i) { by_b[b[i]].push_back(i); }}
    for (int j = 0; j < int(queries.size()); ++j) {
        auto [l, r] = queries[j];
        assert(0 <= l && l <= r && r <= m); by_r[r].push_back(j);}
    vector<int> res(queries.size());
    Fenwick<int> fw(m);
    for (int r = m; r >= 0; --r) {
        for (int i : by_b[r]) { fw.add(i, 1); }
        for (int j : by_r[r]) { res[j] = n - fw.sum(queries[j].first, r); }}
    return res;}
