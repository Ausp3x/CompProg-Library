// S: O(n * log(n)), U: N/A, Q: O(log(n)), M: O(n * log(n))
struct KSPerSegTree {
    struct Mon {
        struct S { int cnt; lng sum; };
        struct F {
            int cnt_upd; lng sum_upd;
            bool operator==(const F&) const = default;
        };

        static constexpr inline S idS() { return {0, 0}; }
        static constexpr inline F idF() { return {0, 0}; }
        static S defR(int l, int r) { return {0, 0}; }
        static S init(int i, lng x) { return {0, 0}; }

        static constexpr inline S ope(const S &a, const S &b) { 
            return {a.cnt + b.cnt, a.sum + b.sum}; 
        }
        static constexpr inline S map(const F &f, const S &a) { 
            return {a.cnt + f.cnt_upd, a.sum + f.sum_upd}; 
        }
        static constexpr inline F cmp(const F &f, const F &g) { 
            return {f.cnt_upd + g.cnt_upd, f.sum_upd + g.sum_upd}; 
        }
    };

    int n;
    vector<lng> v;
    std::optional<PerSegTree<Mon>> segt;

    KSPerSegTree(const vector<lng> &a) : n(a.size()), v(a) {
        sort(v.begin(), v.end());
        v.erase(unique(v.begin(), v.end()), v.end());
        segt.emplace(v.size());
        for (int i = 0; i < n; i++) {
            int ord = lower_bound(v.begin(), v.end(), a[i]) - v.begin();
            segt->update(i, ord, ord, {1, a[i]});
        }
    }

    int _cntLeq(int l, int r, lng x) {
        auto it = upper_bound(v.begin(), v.end(), x);
        if (it == v.begin()) { return 0; }
        int ord = (it - v.begin()) - 1; 
        int l_cnt = segt->query(l,     0, ord).cnt;
        int r_cnt = segt->query(r + 1, 0, ord).cnt;
        return r_cnt - l_cnt;
    }
    int cntEql(int l, int r, lng x) {
        assert(0 <= l && l <= r && r < n);
        auto it = lower_bound(v.begin(), v.end(), x);
        if (it == v.end() || *it != x) { return 0; }
        int ord = it - v.begin();
        int l_cnt = segt->query(l,     ord, ord).cnt;
        int r_cnt = segt->query(r + 1, ord, ord).cnt;
        return r_cnt - l_cnt;
    }
    int cntRan(int l, int r, lng x1, lng x2) {
        assert(0 <= l && l <= r && r < n);
        if (x1 > x2) { return 0; }
        return _cntLeq(l, r, x2) - _cntLeq(l, r, x1 - 1);
    }

    int _kthMin(int l_i, int r_i, int l, int r, int k) {
        if (l == r) { return l; }
        int cnt = 0;
        int l_lc = l_i == -1 ? -1 : segt->tree[l_i].lc;
        if (l_lc != -1) { cnt -= segt->tree[l_lc].val.cnt; }
        int r_lc = r_i == -1 ? -1 : segt->tree[r_i].lc;
        if (r_lc != -1) { cnt += segt->tree[r_lc].val.cnt; }
        int md = std::midpoint(l, r);
        if (cnt >= k) { return _kthMin(l_lc, r_lc, l, md, k); }
        int l_rc = l_i == -1 ? -1 : segt->tree[l_i].rc;
        int r_rc = r_i == -1 ? -1 : segt->tree[r_i].rc;
        return _kthMin(l_rc, r_rc, md + 1, r, k - cnt);
    }
    lng kthMin(int l, int r, int k) {
        assert(0 <= l && l <= r && r < n);
        assert(1 <= k && k <= r - l + 1);
        int ord = _kthMin(segt->roots[l], segt->roots[r + 1], 0, v.size() - 1, k);
        return v[ord];
    }

    lng _kthSum(int l_i, int r_i, int l, int r, int k) {
        if (k == 0) { return 0; }
        if (l == r) { return lng(k) * v[l]; }
        int l_cnt = 0; lng l_sum = 0;
        int l_lc = l_i == -1 ? -1 : segt->tree[l_i].lc;
        if (l_lc != -1) {
            l_cnt -= segt->tree[l_lc].val.cnt;
            l_sum -= segt->tree[l_lc].val.sum;
        }
        int r_lc = r_i == -1 ? -1 : segt->tree[r_i].lc;
        if (r_lc != -1) {
            l_cnt += segt->tree[r_lc].val.cnt;
            l_sum += segt->tree[r_lc].val.sum;
        }
        int md = std::midpoint(l, r);
        if (l_cnt >= k) { return _kthSum(l_lc, r_lc, l, md, k); } 
        else {
            int l_rc = l_i == -1 ? -1 : segt->tree[l_i].rc;
            int r_rc = r_i == -1 ? -1 : segt->tree[r_i].rc;
            return l_sum + _kthSum(l_rc, r_rc, md + 1, r, k - l_cnt);
        }
    }
    lng kthSum(int l, int r, int k) {
        assert(0 <= l && l <= r && r < n);
        assert(0 <= k && k <= r - l + 1);
        return _kthSum(segt->roots[l], segt->roots[r + 1], 0, v.size() - 1, k);
    }
    
    lng maxLeq(int l, int r, lng x) {
        assert(0 <= l && l <= r && r < n);
        int cnt = _cntLeq(l, r, x);
        if (cnt == 0) { return -INF64; }
        return kthMin(l, r, cnt);
    }
    lng minGeq(int l, int r, lng x) {
        assert(0 <= l && l <= r && r < n);
        int cnt = _cntLeq(l, r, x - 1);
        if (cnt == r - l + 1) { return INF64; }
        return kthMin(l, r, cnt + 1);
    }
};