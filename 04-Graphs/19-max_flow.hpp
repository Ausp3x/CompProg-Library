#pragma once

// Legacy extraction: original algorithm bodies retained byte-for-byte.
// Status: existing-unverified.
// Historical comments are not current correctness or performance evidence.
#include "../01-Core/01-template.hpp"

// Source: OLD/[1] algorithms.cpp:348-469
// TESTED
struct EdmondsKarp {
    int n;
    vector<vector<bool>> adjm;
    vector<vector<int>> adjl;
    vector<vector<lng>> ocap, ncap;

    lng max_flow = 0, add_flow = 0;
    vector<int> par;
    vector<bool> in_S;
    vector<pair<int, int>> cut_set;
    
    EdmondsKarp(int n, const vector<vector<int>> &adj, const vector<vector<lng>> &cap): n(n) {
        assert(adj.size() == n + 1 && cap.size() == n + 1);
        for (int i = 0; i <= n; i++) {
            assert(cap[i].size() == n + 1);
        }

        adjm.resize(n + 1, vector<bool>(n + 1));
        ocap.resize(n + 1, vector<lng>(n + 1));
        vector<set<int>> distinct_adjl(n + 1);
        for (int u = 0; u <= n; u++) {
            for (int v : adj[u]) {
                adjm[u][v] = true;
                ocap[u][v] = cap[u][v];
                distinct_adjl[u].insert(v);
                distinct_adjl[v].insert(u);
            }
        }
        
        adjl.resize(n + 1);
        for (int u = 0; u <= n; u++) {
            for (int v : distinct_adjl[u]) {
                adjl[u].pb(v);
            }
        }

        par.resize(n + 1, -1);
        in_S.resize(n + 1);
    }

    lng augmentFlow(int s, int t) {
        fill(par.begin(), par.end(), -1);
        par[s] = s;

        queue<pair<int, lng>> q;
        q.push({s, INF64});
        while (!q.empty()) {
            auto [cur, cur_flow] = q.front();
            q.pop();

            for (int nxt : adjl[cur]) {
                if (par[nxt] == -1 && ncap[cur][nxt] > 0) {
                    par[nxt] = cur;
                    
                    lng nxt_flow = min(cur_flow, ncap[cur][nxt]);
                    if (nxt == t) {
                        return nxt_flow;
                    }
                    
                    q.push({nxt, nxt_flow});
                }
            }
        }

        return 0;
    }

    lng getMaxFlow(int s, int t) {
        assert(0 <= s && s <= n && 0 <= t && t <= n);
        max_flow = 0;
        add_flow = 0;
        ncap = ocap;

        while ((add_flow = augmentFlow(s, t)) > 0) {
            max_flow += add_flow;

            int cur = t;
            while (cur != s) {
                int prv = par[cur];
                ncap[prv][cur] -= add_flow;
                ncap[cur][prv] += add_flow;
                cur = prv;
            }
        }

        return max_flow;
    }

    void getMinCut(int s, int t) {
        getMaxFlow(s, t);
        fill(in_S.begin(), in_S.end(), false);
        in_S[s] = true;
        cut_set.clear();
        
        queue<int> q;
        q.push(s);
        while (!q.empty()) {
            int cur = q.front();
            q.pop();

            for (int nxt : adjl[cur]) {
                if (ncap[cur][nxt] > 0 && !in_S[nxt]) {
                    q.push(nxt);
                    in_S[nxt] = true;
                }
            }
        }

        for (int u = 0; u <= n; u++) {
            if (!in_S[u]) {
                continue;
            }

            for (int v : adjl[u]) {
                if (adjm[u][v] && !in_S[v]) {
                    cut_set.push_back({u, v});
                }
            }
        }
    }
};

