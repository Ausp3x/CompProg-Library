#pragma once

#include "../01-Core/01-template.hpp"

// Vertices [0, n), n <= INT_MAX; representatives are arbitrary and may change after a union.
// S: O(n), U: O(alpha(n)) amortized (alpha inverse Ackermann; O(log(n)) worst case), Q: O(alpha(n)) amortized, M: O(n)
struct DSU {
    int n, ncon;
    vector<int> par, siz;

    explicit DSU(int N = 0) : n(N), ncon(N) {
        assert(n >= 0);
        par.resize(n); iota(par.begin(), par.end(), 0);
        siz.assign(n, 1);}
    int makeSet() {
        assert(n < INT_MAX);
        par.push_back(n); siz.push_back(1); ++ncon;
        return n++;}

    bool uniteSets(int u, int v) {
        u = findSet(u); v = findSet(v);
        if (u == v) { return false; }
        if (siz[u] < siz[v]) { swap(u, v); }
        par[v] = u; siz[u] += siz[v]; --ncon;
        return true;}

    int findSet(int u) {
        assert(0 <= u && u < n);
        int r = u;
        while (r != par[r]) { r = par[r]; }
        while (u != r) { int p = par[u]; par[u] = r; u = p; }
        return r;}
    int getSize(int u) { return siz[findSet(u)]; }
    bool isSameSet(int u, int v) { return findSet(u) == findSet(v); }
    int count() const { return ncon; }
    // T: O(n), M: O(n); groups and members ordered by smallest vertex.
    vector<vector<int>> groups() {
        vector<int> id(n, -1);
        vector<vector<int>> res;
        res.reserve(ncon);
        for (int u = 0; u < n; ++u) {
            int r = findSet(u);
            if (id[r] == -1) {
                id[r] = int(res.size()); res.emplace_back(); res.back().reserve(siz[r]);}
            res[id[r]].push_back(u);}
        return res;}
};
