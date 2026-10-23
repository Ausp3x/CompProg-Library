#pragma once

#include "../01-Core/01-template.hpp"
#include "01-dsu.hpp"
#include "09-rollbackdsu.hpp"

// Vertices [0, n); G is a group (S with ==, op, e, inv), not necessarily commutative; uniteSets(u, v, w) records x_u^-1 x_v = w.
// S: O(n), U: O(alpha(n)) amortized, Q: O(alpha(n)) amortized, M: O(n)
template<typename G>
struct WeightedDSU {
    using S = typename G::S;
    int n, ncon;
    [[no_unique_address]] G m;
    vector<int> par, siz, bad;
    vector<S> pot;

    explicit WeightedDSU(int N = 0, G mon = {}) : n(N), ncon(N), m(std::move(mon)) {
        assert(n >= 0);
        par.resize(n); iota(par.begin(), par.end(), 0);
        siz.assign(n, 1); bad.assign(n, 0); pot.assign(n, m.e());}

    bool uniteSets(int u, int v, const S &w) {
        int ru = findSet(u), rv = findSet(v);
        // x = x_ru^-1 x_rv: rv's potential if it hangs below ru.
        S x = m.op(m.op(pot[u], w), m.inv(pot[v]));
        if (ru == rv) { bad[ru] += !(x == m.e()); return false; }
        if (siz[ru] < siz[rv]) { swap(ru, rv); x = m.inv(x); }
        par[rv] = ru; pot[rv] = std::move(x); siz[ru] += siz[rv]; bad[ru] += bad[rv]; --ncon;
        return true;}

    int findSet(int u) { assert(0 <= u && u < n); return root(u); }
    int root(int u) {
        int p = par[u];
        if (p == u) { return u; }
        int r = root(p);
        if (p != r) { pot[u] = m.op(pot[p], pot[u]); par[u] = r; }
        return r;}
    typename vector<S>::const_reference potential(int u) { findSet(u); return pot[u]; }
    S diff(int u, int v) {
        [[maybe_unused]] bool same = findSet(u) == findSet(v);
        assert(same);
        return m.op(m.inv(pot[u]), pot[v]);}
    bool isSameSet(int u, int v) { return findSet(u) == findSet(v); }
    int getSize(int u) { return siz[findSet(u)]; }
    int count() const { return ncon; }
    int contradictions(int u) { return bad[findSet(u)]; }
    // T: O(n), M: O(n); groups and members ordered by smallest vertex.
    vector<vector<int>> groups() {
        vector<int> id(n, -1);
        vector<vector<int>> res;
        for (int u = 0; u < n; ++u) {
            int r = findSet(u);
            if (id[r] < 0) { id[r] = int(res.size()); res.emplace_back(); }
            res[id[r]].push_back(u);}
        return res;}
};

// Vertices [0, n), n <= INT_MAX / 2; node u + n stands for u's opposite parity; parity is -1 when u and v are unrelated.
// S: O(n), U: O(alpha(n)) amortized, Q: O(alpha(n)) amortized, M: O(n)
struct ParityDSU {
    int n;
    DSU d;

    explicit ParityDSU(int N = 0) : n(N) {
        assert(0 <= n && n <= INT_MAX / 2);
        d = DSU(2 * n);}

    bool uniteSets(int u, int v, bool w) {
        bool res = parity(u, v) < 0;
        d.uniteSets(u, w ? v + n : v); d.uniteSets(u + n, w ? v : v + n);
        return res;}

    int parity(int u, int v) {
        assert(0 <= u && u < n && 0 <= v && v < n);
        return d.isSameSet(u, v) ? 0 : d.isSameSet(u, v + n) ? 1 : -1;}
    bool isBipartite(int u) { assert(0 <= u && u < n); return !d.isSameSet(u, u + n); }
};

// RollbackDSU with WeightedDSU's relations; without path compression, undo and rollback(t) restore potentials and counts.
// S: O(n), U: O(log(n)) (undo O(1)), Q: O(log(n)), M: O(n + q)
template<typename G>
struct RollbackWeightedDSU : RollbackDSU {
    using S = typename G::S;
    [[no_unique_address]] G m;
    vector<S> pot;
    vector<int> bad, old;

    explicit RollbackWeightedDSU(int N = 0, G mon = {}) : RollbackDSU(N), m(std::move(mon)), pot(n, m.e()), bad(n, 0) {}

    bool uniteSets(int u, int v, const S &w) {
        auto [ru, pu] = climb(u);
        auto [rv, pv] = climb(v);
        S x = m.op(m.op(pu, w), m.inv(pv));
        bool res = RollbackDSU::uniteSets(ru, rv);
        int y = his.back(), r = res ? par[y] : ru;
        old.push_back(bad[r]);
        if (res) { pot[y] = y == rv ? std::move(x) : m.inv(x); bad[r] += bad[y]; }
        else { bad[r] += !(x == m.e()); }
        return res;}
    void undo() {
        assert(!his.empty());
        int y = his.back();
        bad[y >= 0 ? par[y] : ~y] = old.back();
        old.pop_back();
        RollbackDSU::undo();}
    void rollback(int t) {
        assert(0 <= t && t <= snapshot());
        while (snapshot() > max(t, 0)) { undo(); }}

    pair<int, S> climb(int u) const {
        assert(0 <= u && u < n);
        S p = m.e();
        for (; par[u] != u; u = par[u]) { p = m.op(pot[u], p); }
        return {u, p};}
    S potential(int u) const { return climb(u).second; }
    S diff(int u, int v) const {
        auto [ru, pu] = climb(u);
        auto [rv, pv] = climb(v);
        assert(ru == rv);
        return m.op(m.inv(pu), pv);}
    int contradictions(int u) const { return bad[findSet(u)]; }
};
