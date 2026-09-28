// Legacy extraction: original algorithm bodies retained byte-for-byte.
// Status: legacy-reference; excluded from aggregates.
// Historical comments are not current correctness or performance evidence.
#include "../../01-Core/01-template.hpp"

// Source: OLD/algorithms.cpp:4768-4795
// Original section: AI GENERATED FAST section.
// Check Bipartite
// T: O(n + m), M: O(n)
bool isBipartite(int n, const vector<vector<int>> &adj, vector<int> &side) {
    side.assign(n, -1);
    bool is_bip = true;
    queue<int> q;

    for (int st = 0; st < n; st++) {
        if (side[st] == -1) {
            q.push(st);
            side[st] = 0;
            while (!q.empty()) {
                int u = q.front();
                q.pop();
                for (int v : adj[u]) {
                    if (side[v] == -1) {
                        side[v] = side[u] ^ 1;
                        q.push(v);
                    } else {
                        is_bip &= side[v] != side[u];
                    }
                }
            }
        }
    }

    return is_bip;
}

