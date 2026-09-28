#pragma once

#include "../01-Core/01-template.hpp"

// Unchanged legacy extraction: OLD/algorithms.cpp:1968-2038.
// Status: existing-unverified; correctness, performance and API audit pending.
// S: O(q * log(q)), U: NA, Q: O(n * sqrt(q)), M: O(q)
struct Mo {
    struct Query {
        int l, r, i;
        lng hil_ord;
    };

    int n, q;
    vector<Query> Q;

    Mo(int N) : n(N), q(0) {}

    static lng getHilOrd(int x, int y, int pow = 20) {
        lng res = 0;
        for (int i = 1 << (pow - 1); i > 0; i >>= 1) {
            int xi = (x & i) > 0, yi = (y & i) > 0;            
            if (yi == 0) {
                x += (xi == 1) * ((1 << pow) - 2 * x - 1);
                y += (xi == 1) * ((1 << pow) - 2 * y - 1);
                std::swap(x, y);
            }
            
            res += lng(i) * i * ((3 * xi) ^ yi);
        }

        return res;
    }

    void addQuery(int l, int r) {
        Q.push_back({l, r, q++, getHilOrd(l, r)});
    }

    template<typename Add, typename Del, typename Ret>
    void solve(Add &&add, Del &&del, Ret &&ret) {
        if (q == 0) {
            return;}

        sort(Q.begin(), Q.end(), [&](const Query &a, const Query &b) {
            return a.hil_ord < b.hil_ord;
        });

        int l = 0, r = -1;
        for (const Query &cur : Q) {
            while (l > cur.l) {
                add(--l);}
            while (r < cur.r) {
                add(++r);}
            
            while (l < cur.l) { 
                del(l++);}
            while (r > cur.r) { 
                del(r--);}
            
            ret(cur.i);
        }
    }

    friend ostream &operator<<(ostream &os, const Mo &a) {
        if (a.q == 0) {
            return os << "[]\n";}
        
        os << "\n[\n";
        for (int i = 0; i < a.Q.size(); i++) {
            os << "  [Q" << i << " | id: " << a.Q[i].i << "]: "
               << "[l: " << a.Q[i].l << ", r: " << a.Q[i].r << ", hil_ord: " << a.Q[i].hil_ord << "]" 
               << (i < a.Q.size() - 1 ? ",\n" : "\n");
        }
        
        return os << "]\n";
    }
};

