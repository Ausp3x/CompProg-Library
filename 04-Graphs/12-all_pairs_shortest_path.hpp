#pragma once

// Legacy extraction: original algorithm bodies retained byte-for-byte.
// Status: existing-unverified.
// Historical comments are not current correctness or performance evidence.
#include "../01-Core/01-template.hpp"

// Source: OLD/algorithms.cpp:4528-4561
// Original section: INFERIOR VERSIONS section.
struct FloydWarshall {
    int n;
    vector<vector<lng>> dis;
    
    FloydWarshall(int n, vector<vector<lng>> dis) : n(n), dis(dis) {
        assert(dis.size() == n + 1);
        for (int i = 0; i <= n; i++) {
            assert(dis[i].size() == n + 1);
        }
    }
    
    void runFloydWarshall() {
        for (int k = 1; k <= n; k++) {
            for (int i = 1; i <= n; i++) {
                for (int j = 1; j <= n; j++) {
                    if (dis[i][k] < INF64 && dis[k][j] < INF64) {
                        dis[i][j] = max(min(dis[i][j], dis[i][k] + dis[k][j]), -INF64);
                    }
                }
            }
        }
    
        for (int i = 1; i <= n; i++) {
            for (int j = 1; j <= n; j++) {
                for (int k = 1; k <= n; k++) {
                    if (dis[i][k] < INF64 && dis[k][k] < 0 && dis[k][j] < INF64) {
                        dis[i][j] = -INF64;
                        break;
                    }
                }
            }
        }
    }
};

