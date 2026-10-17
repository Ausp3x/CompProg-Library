#pragma once

#include "../01-Core/01-template.hpp"

// Vertices [0, n); every uniteSets call is recorded, redundant ones too; undo reverts the latest, snapshot is the record count.
// S: O(n), U: O(log(n)) (undo O(1), rollback(t) O(snapshot() - t)), Q: O(log(n)), M: O(n + q)
struct RollbackDSU {
    int n, ncon;
    vector<int> par, siz, his;

    explicit RollbackDSU(int N = 0) : n(N), ncon(N) {
        assert(n >= 0);
        par.resize(n); iota(par.begin(), par.end(), 0);
        siz.assign(n, 1);}

    bool uniteSets(int u, int v) {
        u = findSet(u); v = findSet(v);
        if (u == v) { his.push_back(~u); return false; }
        if (siz[u] < siz[v]) { swap(u, v); }
        par[v] = u; siz[u] += siz[v]; --ncon;
        his.push_back(v);
        return true;}
    void undo() {
        assert(!his.empty());
        int v = his.back();
        his.pop_back();
        if (v >= 0) { siz[par[v]] -= siz[v]; par[v] = v; ++ncon; }}
    void rollback(int t) {
        assert(0 <= t && t <= snapshot());
        while (snapshot() > max(t, 0)) { undo(); }}

    int findSet(int u) const {
        assert(0 <= u && u < n);
        while (par[u] != u) { u = par[u]; }
        return u;}
    bool isSameSet(int u, int v) const { return findSet(u) == findSet(v); }
    int getSize(int u) const { return siz[findSet(u)]; }
    int count() const { return ncon; }
    int snapshot() const { return int(his.size()); }
    // T: O(n), M: O(n); groups and members ordered by smallest vertex.
    vector<vector<int>> groups() const {
        vector<int> id(n, -1);
        vector<vector<int>> res;
        res.reserve(ncon);
        for (int u = 0; u < n; ++u) {
            int r = u;
            while (id[r] < 0 && par[r] != r) { r = par[r]; }
            if (id[r] < 0) { id[r] = int(res.size()); res.emplace_back(); res.back().reserve(siz[r]); }
            for (int x = u; id[x] < 0; x = par[x]) { id[x] = id[r]; }
            res[id[u]].push_back(u);}
        return res;}
};

// M is a commutative monoid (S, op); addValue folds x into u's component; edgeCount counts uniteSets calls inside it.
// S: O(n), U: O(log(n)) (undo O(1)), Q: O(log(n)), M: O(n + q)
template<typename M>
struct RollbackDSUAggregate : RollbackDSU {
    using S = typename M::S;
    vector<S> val;
    vector<int> edg;
    vector<pair<S, int>> old;

    explicit RollbackDSUAggregate(const vector<S> &a) : RollbackDSU(int(a.size())), val(a), edg(n, 0) { assert(a.size() <= size_t(INT_MAX)); }

    bool uniteSets(int u, int v) {
        bool res = RollbackDSU::uniteSets(u, v);
        int x = his.back(), r = res ? par[x] : ~x;
        old.emplace_back(val[r], edg[r]);
        if (res) { val[r] = M::op(val[r], val[x]); edg[r] += edg[x] + 1; }
        else { ++edg[r]; }
        return res;}
    void addValue(int u, const S &x) {
        int r = findSet(u);
        old.emplace_back(val[r], edg[r]); his.push_back(~r);
        val[r] = M::op(val[r], x);}
    void undo() {
        assert(!his.empty());
        int x = his.back(), r = x >= 0 ? par[x] : ~x;
        val[r] = std::move(old.back().first); edg[r] = old.back().second;
        old.pop_back();
        RollbackDSU::undo();}
    void rollback(int t) {
        assert(0 <= t && t <= snapshot());
        while (snapshot() > max(t, 0)) { undo(); }}

    typename vector<S>::const_reference componentValue(int u) const { return val[findSet(u)]; }
    int edgeCount(int u) const { return edg[findSet(u)]; }
};
