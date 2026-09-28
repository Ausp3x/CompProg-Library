#pragma once

// Legacy extraction: original algorithm bodies retained byte-for-byte.
// Status: existing-unverified.
// Historical comments are not current correctness or performance evidence.
#include "../01-Core/01-template.hpp"

// Source: OLD/algorithms.cpp:5005-5044
// Original section: AI GENERATED FAST section.
// S: O(n * log(n)), U: NA, Q: O(1), M: O(n)
struct CentroidDecomposition {
    int n;
    vector<int> sz, par;
    vector<bool> rmv;

    CentroidDecomposition(int N, const vector<vector<int>> &adj) : 
        n(N), sz(N, 0), par(N, -1), rmv(N, false) {
        auto getSz = [&](auto &&getSz, int u, int p) -> int {
            sz[u] = 1;
            for (int v : adj[u]) {
                if (v != p && !rmv[v]) {
                    sz[u] += getSz(getSz, v, u);}}
                    
            return sz[u];
        };

        auto getCent = [&](auto &&getCent, int u, int p, int t_sz) -> int {
            for (int v : adj[u]) {
                if (v != p && !rmv[v] && sz[v] * 2 > t_sz) {
                    return getCent(getCent, v, u, t_sz);}}
                    
            return u;
        };

        auto build = [&](auto &&build, int u, int p) -> void {
            int t_sz = getSz(getSz, u, -1);
            int cent = getCent(getCent, u, -1, t_sz);
            
            par[cent] = p;
            rmv[cent] = true;
            
            for (int v : adj[cent]) {
                if (!rmv[v]) {
                    build(build, v, cent);}}
        };

        build(build, 0, -1);
    }
};

