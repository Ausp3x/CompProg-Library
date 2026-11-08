// Legacy extraction: original algorithm bodies retained byte-for-byte.
// Status: legacy-reference; excluded from aggregates.
// Historical comments are not current correctness or performance evidence.
#include "../../01-Core/01-template.hpp"

// Source: OLD/[1] algorithms.cpp:473-541
// TESTED
/*
struct EulerTourTree {
    int n, root;
    vector<lng> arr;
    vector<vector<int>> adjl;

    int timer = 0;
    vector<int> t_in, t_out, time_to_orig;
    vector<lng> euler;
    unique_ptr<SegTree> segt;

    EulerTourTree(int n, int root, const vector<lng> &arr, const vector<vector<int>> &adjl) : n(n), root(root), arr(arr), adjl(adjl) {
        assert(0 <= root && root < n);
        assert(arr.size() == n && adjl.size() == n);
        t_in.resize(n, -1);
        t_out.resize(n, -1);
        time_to_orig.resize(n);
        euler.resize(n);
        
        dfs(root, -1);

        segt = make_unique<SegTree>(0, n - 1, euler);
    }

    void dfs(int cur, int prv) {
        t_in[cur] = timer;
        time_to_orig[timer] = cur;
        euler[timer] = arr[cur];
        timer++;

        for (int nxt : adjl[cur]) {
            if (nxt == prv) {
                continue;
            }

            dfs(nxt, cur);
        }

        t_out[cur] = timer - 1;
    }

    void subtreeAddUpdate(int u, lng x, bool u_only = false) {
        segt->rangeAddUpdate(t_in[u], (u_only ? t_in[u] : t_out[u]), x);
    }

    void subtreeSetUpdate(int u, lng x, bool u_only = false) {
        segt->rangeSetUpdate(t_in[u], (u_only ? t_in[u] : t_out[u]), x);
    }

    pair<lng, int> subtreeMaxQuery(int u, bool u_only = false) {
        auto res = segt->rangeMaxQuery(t_in[u], (u_only ? t_in[u] : t_out[u]));
        res.se = time_to_orig[res.se];

        return res;
    }
    
    pair<lng, int> subtreeMinQuery(int u, bool u_only = false) {
        auto res = segt->rangeMinQuery(t_in[u], (u_only ? t_in[u] : t_out[u]));
        res.se = time_to_orig[res.se];

        return res;
    }

    lng subtreeSumQuery(int u, bool u_only = false) {
        return segt->rangeSumQuery(t_in[u], (u_only ? t_in[u] : t_out[u]));
    }
};
*/

