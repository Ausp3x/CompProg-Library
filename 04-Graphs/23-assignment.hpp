#pragma once

// Legacy extraction: original algorithm bodies retained byte-for-byte.
// Status: existing-unverified.
// Historical comments are not current correctness or performance evidence.
#include "../01-Core/01-template.hpp"

// Source: OLD/algorithms.cpp:4797-4862
// Original section: AI GENERATED FAST section.
// S: O(n^3), U: NA, Q: O(1), M: O(n^2)
template<typename T>
struct Hungarian {
    int n, m;
    vector<vector<T>> a;
    vector<T> u, v;
    vector<int> p, way;

    Hungarian(int n, int m) : n(n), m(m), a(n + 1, vector<T>(m + 1, 0)), 
        u(n + 1, 0), v(m + 1, 0), p(m + 1, 0), way(m + 1, 0) {}

    void addEdge(int i, int j, T w) {
        a[i + 1][j + 1] = w;}

    T solve() {
        for (int i = 1; i <= n; i++) {
            p[0] = i;
            int j0 = 0;
            vector<T> minv(m + 1, INF64);
            vector<bool> used(m + 1, false);
            do {
                used[j0] = true;
                int i0 = p[j0], j1 = 0;
                T delta = INF64;
                for (int j = 1; j <= m; j++) {
                    if (!used[j]) {
                        T cur = a[i0][j] - u[i0] - v[j];
                        if (cur < minv[j]) {
                            minv[j] = cur;
                            way[j] = j0;
                        }
                        if (minv[j] < delta) {
                            delta = minv[j];
                            j1 = j;
                        }
                    }
                }
                for (int j = 0; j <= m; j++) {
                    if (used[j]) {
                        u[p[j]] += delta;
                        v[j] -= delta;
                    } else {
                        minv[j] -= delta;
                    }
                }
                j0 = j1;
            } while (p[j0] != 0);
            do {
                int j1 = way[j0];
                p[j0] = p[j1];
                j0 = j1;
            } while (j0 != 0);
        }

        return -v[0];
    }
    
    vector<int> getAssignment() {
        vector<int> ans(n, -1);
        for (int j = 1; j <= m; j++) {
            if (p[j] != 0) {
                ans[p[j] - 1] = j - 1;}}
        
        return ans;
    }
};

