#pragma once

// Legacy extraction: original algorithm bodies retained byte-for-byte.
// Status: existing-unverified.
// Historical comments are not current correctness or performance evidence.
#include "../01-Core/01-template.hpp"

// Source: OLD/algorithms.cpp:4641-4700
// Original section: AI GENERATED FAST section.
// 2-SAT
// S: O(n + m), U: NA, Q: O(1), M: O(n + m)
struct TwoSat {
    int n;
    vector<vector<int>> adj, adj_t;
    vector<bool> used;
    vector<int> ord, comp;
    vector<bool> ans;

    TwoSat(int N) : n(N), adj(2 * N), adj_t(2 * N), ans(N) {}

    void addClause(int a, bool na, int b, bool nb) {
        int u = 2 * a ^ na;
        int v = 2 * b ^ nb;
        int neg_u = u ^ 1;
        int neg_v = v ^ 1;
        adj[neg_u].push_back(v);
        adj[neg_v].push_back(u);
        adj_t[v].push_back(neg_u);
        adj_t[u].push_back(neg_v);
    }

    bool solve() {
        used.assign(2 * n, false);
        ord.clear();
        
        auto dfs1 = [&](auto &&dfs1, int u) -> void {
            used[u] = true;
            for (int v : adj[u]) {
                if (!used[v]) {
                    dfs1(dfs1, v);}}
            ord.push_back(u);
        };

        for (int i = 0; i < 2 * n; i++) {
            if (!used[i]) {
                dfs1(dfs1, i);}}

        comp.assign(2 * n, -1);
        auto dfs2 = [&](auto &&dfs2, int u, int c) -> void {
            comp[u] = c;
            for (int v : adj_t[u]) {
                if (comp[v] == -1) {
                    dfs2(dfs2, v, c);}}
        };

        for (int i = 0, j = 0; i < 2 * n; i++) {
            int u = ord[2 * n - i - 1];
            if (comp[u] == -1) {
                dfs2(dfs2, u, j++);}}

        for (int i = 0; i < 2 * n; i += 2) {
            if (comp[i] == comp[i + 1]) {
                return false;}
            ans[i / 2] = comp[i] > comp[i + 1];
        }

        return true;
    }
};

