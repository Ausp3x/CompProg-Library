#pragma once

// Legacy extraction: original algorithm bodies retained byte-for-byte.
// Status: existing-unverified.
// Historical comments are not current correctness or performance evidence.
#include "../01-Core/01-template.hpp"

// Source: OLD/algorithms.cpp:4724-4766
// Original section: AI GENERATED FAST section.
// SPFA
// S: O(n * m), U: NA, Q: O(1), M: O(n + m)
struct SPFA {
    int n;
    vector<lng> d;
    vector<int> p;

    SPFA(int N) : n(N) {}

    bool solve(int s, const vector<vector<pair<int, lng>>> &adj) {
        d.assign(n, INF64);
        p.assign(n, -1);
        vector<int> cnt(n, 0);
        vector<bool> in_q(n, false);
        queue<int> q;

        d[s] = 0;
        q.push(s);
        in_q[s] = true;

        while (!q.empty()) {
            int u = q.front();
            q.pop();
            in_q[u] = false;

            for (auto [v, w] : adj[u]) {
                if (d[u] + w < d[v]) {
                    d[v] = d[u] + w;
                    p[v] = u;
                    if (!in_q[v]) {
                        q.push(v);
                        in_q[v] = true;
                        cnt[v]++;
                        if (cnt[v] > n) {
                            return false;}
                    }
                }
            }
        }

        return true;
    }
};

