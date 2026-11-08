// Legacy extraction: original algorithm bodies retained byte-for-byte.
// Status: legacy-reference; excluded from aggregates.
// Historical comments are not current correctness or performance evidence.
#include "../../01-Core/01-template.hpp"

// Source: OLD/algorithms.cpp:4702-4722
// Original section: AI GENERATED FAST section.
// Floyd-Warshall
// T: O(n^3), M: O(1)
void floydWarshall(vector<vector<lng>> &d) {
    int n = sze(d);
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (d[i][k] < INF64 && d[k][j] < INF64) {
                    d[i][j] = min(d[i][j], d[i][k] + d[k][j]);}}}}
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            for (int k = 0; k < n; k++) {
                if (d[i][k] < INF64 && d[k][k] < 0 && d[k][j] < INF64) {
                    d[i][j] = -INF64;
                    break;
                }
            }
        }
    }
}

