#pragma once

#include "../01-Core/01-template.hpp"

// Unchanged legacy extraction: OLD/algorithms.cpp:3651-3817.
// Status: existing-unverified; correctness, performance and API audit pending.
// S: O(n * log(n)), U: N/A, Q: O(log(n)), M: O(n * log(n)) 
struct WaveletMatrix {
    using uint = unsigned int;
    using ulng = unsigned long long;
        
    struct BitVec {
        int n;
        vector<ulng> blks;
        vector<int> prf;

        BitVec() {}
        
        BitVec(int N) : n(N) {
            blks.assign(((n + 63) >> 6) + 1, 0);
            prf.assign(blks.size(), 0);
        }

        void build() {
            for (int i = 0; i < int(blks.size()) - 1; i++) {
                prf[i + 1] = prf[i] + __builtin_popcountll(blks[i]);}
        }

        void set(int i) {
            blks[i >> 6] |= (1ULL << (i & 63));
        }

        int rank0(int i) {
            return i - rank1(i);
        }

        int rank1(int i) {
            return prf[i >> 6] + __builtin_popcountll(blks[i >> 6] & ((1ULL << (i & 63)) - 1));
        }
    };

    int n, h;
    vector<BitVec> bv;
    vector<int> md;
    vector<lng> v; 

    WaveletMatrix(const vector<lng> &a) : n(a.size()), v(a) {
        sort(v.begin(), v.end());
        v.erase(unique(v.begin(), v.end()), v.end());

        h = v.size() <= 1 ? 1 : 32 - __builtin_clz(uint(v.size() - 1));
        bv.assign(h, BitVec(n));
        md.assign(h, 0);

        vector<int> crnk(n), nxt0(n), nxt1(n);
        for (int i = 0; i < n; i++) {
            crnk[i] = lower_bound(v.begin(), v.end(), a[i]) - v.begin();}
        for (int d = h - 1; d >= 0; d--) {
            int p0 = 0, p1 = 0;
            for (int i = 0; i < n; i++) {
                if ((crnk[i] >> d) & 1) {
                    bv[d].set(i);
                    nxt1[p1] = crnk[i];
                    p1++;
                } else {
                    nxt0[p0] = crnk[i];
                    p0++;
                }
            }
            bv[d].build();
            md[d] = p0;

            for (int i = 0; i < p0; i++) {
                crnk[i] = nxt0[i];}
            for (int i = 0; i < p1; i++) {
                crnk[p0 + i] = nxt1[i];}
        }
    }

    int _cntLeqOrd(int l, int r, int ord) {
        if (ord < 0) {
            return 0;}

        if (ord >= int(v.size())) {
            return r - l + 1;}

        int res = 0;
        for (int d = h - 1; d >= 0; d--) {
            if (l > r) {
                break;}
            
            int l_rnk0 = bv[d].rank0(l);
            int r_rnk0 = bv[d].rank0(r + 1);
            int cnt0 = r_rnk0 - l_rnk0;

            if ((ord >> d) & 1) {
                res += cnt0;
                l = md[d] + (l - l_rnk0);
                r = md[d] + (r + 1 - r_rnk0) - 1;
            } else {
                l = l_rnk0;
                r = r_rnk0 - 1;
            }
        }
        
        return res + (r - l + 1);
    }

    int _cntLeq(int l, int r, lng x) {
        auto it = upper_bound(v.begin(), v.end(), x);
        if (it == v.begin()) {
            return 0;}
        
        int ord = (it - v.begin()) - 1;
        
        return _cntLeqOrd(l, r, ord);
    }
    
    int cntEql(int l, int r, lng x) {
        assert(0 <= l && l <= r && r < n);

        return _cntLeq(l, r, x) - _cntLeq(l, r, x - 1);
    }

    int cntRan(int l, int r, lng x1, lng x2) {
        assert(0 <= l && l <= r && r < n);

        if (x1 > x2) {
            return 0;}

        return _cntLeq(l, r, x2) - _cntLeq(l, r, x1 - 1);
    }

    lng kthMin(int l, int r, int k) {
        assert(0 <= l && l <= r && r < n);
        assert(1 <= k && k <= r - l + 1);

        int ord = 0;
        for (int d = h - 1; d >= 0; d--) {
            int l_rnk0 = bv[d].rank0(l);
            int r_rnk0 = bv[d].rank0(r + 1);
            int cnt0 = r_rnk0 - l_rnk0;

            if (k <= cnt0) {
                l = l_rnk0;
                r = r_rnk0 - 1;
            } else {
                ord |= (1 << d);
                l = md[d] + (l - l_rnk0);
                r = md[d] + (r + 1 - r_rnk0) - 1;
                k -= cnt0;
            }
        }

        return v[ord];
    }

    lng maxLeq(int l, int r, lng x) {
        int cnt = _cntLeq(l, r, x);
        if (cnt == 0) {
            return -INF64;}

        return kthMin(l, r, cnt);
    }

    lng minGeq(int l, int r, lng x) {
        int cnt = _cntLeq(l, r, x - 1);
        if (cnt == r - l + 1) {
            return INF64;}
        
        return kthMin(l, r, cnt + 1);
    }
};

