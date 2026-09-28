#pragma once

// Legacy extraction: original algorithm bodies retained byte-for-byte.
// Status: existing-unverified.
// Historical comments are not current correctness or performance evidence.
#include "../01-Core/01-template.hpp"

// Source: OLD/algorithms.cpp:4906-5003
// Original section: AI GENERATED FAST section.
// S: O(n), U: O(log(n)), Q: O(1), M: O(n)
struct OnlineBridges {
    int bridges, lca_itr;
    vector<int> par, d_2ec, d_cc, sz_cc, last_vst;

    OnlineBridges(int n) : bridges(0), lca_itr(0), par(n, -1), d_2ec(n), 
        d_cc(n), sz_cc(n, 1), last_vst(n, 0) {
        iota(d_2ec.begin(), d_2ec.end(), 0);
        iota(d_cc.begin(), d_cc.end(), 0);
    }

    int find2ec(int v) {
        if (v == -1) {
            return -1;}
            
        return d_2ec[v] == v ? v : d_2ec[v] = find2ec(d_2ec[v]);
    }

    int findCC(int v) {
        v = find2ec(v);
        
        return d_cc[v] == v ? v : d_cc[v] = findCC(d_cc[v]);
    }

    void makeRoot(int v) {
        int root = v, child = -1;
        while (v != -1) {
            int p = find2ec(par[v]);
            par[v] = child;
            d_cc[v] = root;
            child = v;
            v = p;
        }
        sz_cc[root] = sz_cc[child];
    }

    void mergePath(int a, int b) {
        lca_itr++;
        vector<int> path_a, path_b;
        int lca = -1;
        while (lca == -1) {
            if (a != -1) {
                a = find2ec(a);
                path_a.push_back(a);
                if (last_vst[a] == lca_itr) {
                    lca = a;
                    break;
                }
                last_vst[a] = lca_itr;
                a = par[a];
            }
            if (b != -1) {
                b = find2ec(b);
                path_b.push_back(b);
                if (last_vst[b] == lca_itr) {
                    lca = b;
                    break;
                }
                last_vst[b] = lca_itr;
                b = par[b];
            }
        }
        for (int v : path_a) {
            d_2ec[v] = lca;
            if (v == lca) {
                break;}
            bridges--;
        }
        for (int v : path_b) {
            d_2ec[v] = lca;
            if (v == lca) {
                break;}
            bridges--;
        }
    }

    void addEdge(int a, int b) {
        a = find2ec(a);
        b = find2ec(b);
        if (a == b) {
            return;}

        int ca = findCC(a);
        int cb = findCC(b);
        if (ca != cb) {
            bridges++;
            if (sz_cc[ca] > sz_cc[cb]) {
                swap(a, b);
                swap(ca, cb);
            }
            makeRoot(a);
            par[a] = d_cc[a] = b;
            sz_cc[cb] += sz_cc[a];
        } else {
            mergePath(a, b);
        }
    }
};

