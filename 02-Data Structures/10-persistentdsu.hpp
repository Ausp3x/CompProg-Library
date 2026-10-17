#pragma once

#include "../01-Core/01-template.hpp"
#include "35-persistentarray.hpp"

// Vertices [0, n); time t is the state after the first t uniteSets calls; uniteTime returns -1 when never connected.
// S: O(n), U: O(log(n)), Q: O(log(n)), M: O(n + q)
struct PartiallyPersistentDSU {
    int n, now = 0;
    vector<int> par, tim, ncon;
    vector<vector<pair<int, int>>> siz;

    explicit PartiallyPersistentDSU(int N = 0) : n(N), ncon{N} {
        assert(n >= 0);
        par.resize(n); iota(par.begin(), par.end(), 0);
        tim.assign(n, INT_MAX); siz.resize(n);}

    bool uniteSets(int u, int v) {
        u = findSet(now, u); v = findSet(now, v);
        ncon.push_back(ncon.back()); ++now;
        if (u == v) { return false; }
        int su = siz[u].empty() ? 1 : siz[u].back().second, sv = siz[v].empty() ? 1 : siz[v].back().second;
        if (su < sv) { swap(u, v); }
        par[v] = u; tim[v] = now; --ncon.back();
        siz[u].emplace_back(now, su + sv);
        return true;}

    int findSet(int t, int u) const {
        assert(0 <= t && t <= now && 0 <= u && u < n);
        while (par[u] != u && tim[u] <= t) { u = par[u]; }
        return u;}
    bool isSameSet(int t, int u, int v) const { return findSet(t, u) == findSet(t, v); }
    int getSize(int t, int u) const {
        u = findSet(t, u);
        auto it = upper_bound(siz[u].begin(), siz[u].end(), pair{t, INT_MAX});
        return it == siz[u].begin() ? 1 : prev(it)->second;}
    int count(int t) const { assert(0 <= t && t <= now); return ncon[t]; }
    int uniteTime(int u, int v) const {
        assert(0 <= u && u < n && 0 <= v && v < n);
        int res = 0;
        while (u != v) {
            if (tim[u] > tim[v]) { swap(u, v); }
            if (tim[u] == INT_MAX) { return -1; }
            res = tim[u]; u = par[u];}
        return res;}
};

// Vertices [0, n); versions are dense ids from 0 (version 0 has no edges); each uniteSets call creates one.
// k = 8, q calls: S: O(k + log_k(n)), U: O(log(n) * log_k(n) + k * log_k(n)), Q: O(log(n) * log_k(n)), M: O(k * log_k(n) * (q + 1))
struct PersistentDSU {
    PersistentArray<int> a;
    vector<int> ver, ncon;

    explicit PersistentDSU(int N = 0) : a(N, -1), ver{0}, ncon{N} {}

    int uniteSets(int t, int u, int v) {
        u = findSet(t, u); v = findSet(t, v);
        int w = ver[t], c = ncon[t];
        if (u != v) {
            int su = a.get(w, u), sv = a.get(w, v);
            if (su > sv) { swap(u, v); swap(su, sv); }
            w = a.set(a.set(w, u, su + sv), v, u); --c;}
        ver.push_back(w); ncon.push_back(c);
        return int(ver.size()) - 1;}

    // Depth <= 30 under union by size; the step cap only bounds a violated precondition.
    int findSet(int t, int u) const {
        assert(0 <= t && t < int(ver.size()) && 0 <= u && u < a.n);
        for (int d = 0, p = a.get(ver[t], u); p >= 0 && d < 31; p = a.get(ver[t], u), ++d) { u = p; }
        return u;}
    bool isSameSet(int t, int u, int v) const { return findSet(t, u) == findSet(t, v); }
    int getSize(int t, int u) const { u = findSet(t, u); return -a.get(ver[t], u); }
    int count(int t) const { assert(0 <= t && t < int(ver.size())); return ncon[t]; }
};
