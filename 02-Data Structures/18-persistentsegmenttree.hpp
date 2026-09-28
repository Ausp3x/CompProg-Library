#pragma once

#include "../01-Core/01-template.hpp"

// Unchanged legacy extraction: OLD/algorithms.cpp:2445-2725.
// Status: existing-unverified; correctness, performance and API audit pending.
// S: O(q * log(n)), U: O(log(n)), Q: O(log(n)), M: O(q * log(n))
template<typename Mon>
struct PerSegTree {
    using S = typename Mon::S;
    using F = typename Mon::F;

    struct Node {
        S val;
        F lzy;
        int lc = -1, rc = -1;

        Node(lng l, lng r) : val(Mon::defR(l, r)), lzy(Mon::idF()) {}
    };

    lng n;
    vector<int> roots;
    vector<Node> tree;

    PerSegTree(lng N) : n(N) {
        tree.reserve(10'000'000); 
        roots.push_back(cloneNode(-1, 0, n - 1));
    }

    template<typename T>
    PerSegTree(std::span<const T> &a) : n(a.size()) {
        tree.reserve(10'000'000); 
        roots.push_back(build(0, n - 1, a));
    }

    // S: O(n)
    template<typename T>
    int build(lng l, lng r, std::span<const T> &a) {
        int cur = cloneNode(-1, l, r);
        
        if (l == r) {
            tree[cur].val = Mon::init(l, a[l]);
            return cur;
        }

        lng md = std::midpoint(l, r);
        int nlc = build(l,      md, a);
        tree[cur].lc = nlc;
        int nrc = build(md + 1, r,  a);
        tree[cur].rc = nrc;
        tree[cur].val = Mon::ope(tree[nlc].val, tree[nrc].val);

        return cur;
    }

    int cloneNode(int i, lng l, lng r) {
        if (i == -1) {
            tree.emplace_back(l, r);
        } else {
            Node tmp = tree[i];
            tree.push_back(tmp);
        }

        return tree.size() - 1;
    }

    inline void apply(int i, F f) {
        tree[i].val = Mon::map(f, tree[i].val);
        tree[i].lzy = Mon::cmp(f, tree[i].lzy);
    }

    inline void push(int i, lng l, lng r) {
        if (tree[i].lzy == Mon::idF()) {
            return;}
        
        lng md = std::midpoint(l, r);
        int nlc = cloneNode(tree[i].lc, l,      md);
        tree[i].lc = nlc;
        int nrc = cloneNode(tree[i].rc, md + 1, r);
        tree[i].rc = nrc;

        apply(tree[i].lc, tree[i].lzy);
        apply(tree[i].rc, tree[i].lzy);
        tree[i].lzy = Mon::idF();
    }

    int _modify(int i1, int i2, lng l, lng r, lng ql, lng qr, F acc) {
        if (ql <= l && r <= qr) {
            int j = cloneNode(i2, l, r);
            apply(j, acc);
            return j;
        }

        int j = cloneNode(i1, l, r);
       
        push(j, l, r);

        int lc2 = -1, rc2 = -1;
        F nac = acc;
        if (i2 != -1) {
            lc2 = tree[i2].lc;
            rc2 = tree[i2].rc;
            nac = Mon::cmp(acc, tree[i2].lzy);
        }

        lng md = std::midpoint(l, r);
        if (ql <= md) {
            int nlc = _modify(tree[j].lc, lc2, l,      md, ql, qr, nac);
            tree[j].lc = nlc;
        }
        if (qr > md) {
            int nrc = _modify(tree[j].rc, rc2, md + 1, r,  ql, qr, nac);
            tree[j].rc = nrc;
        }

        S l_val = tree[j].lc != -1 ? tree[tree[j].lc].val : Mon::defR(l, md);
        S r_val = tree[j].rc != -1 ? tree[tree[j].rc].val : Mon::defR(md + 1, r);
        tree[j].val = Mon::ope(l_val, r_val);

        return j;
    }

    int modify(int ver1, int ver2, lng l, lng r) {
        if (l < 0 || r > n - 1 || l > r) {
            int cur = roots[ver1];
            roots.push_back(cur);
            return roots.size() - 1;
        }
        
        int cur = _modify(roots[ver1], roots[ver2], 0, n - 1, l, r, Mon::idF());
        roots.push_back(cur);
        
        return roots.size() - 1;
    }

    int _update(int i, lng l, lng r, lng ql, lng qr, F f) {
        int j = cloneNode(i, l, r);
        
        if (ql <= l && r <= qr) {
            apply(j, f);
            return j;
        }

        push(j, l, r);
        
        lng md = std::midpoint(l, r);
        if (ql <= md) {
            int nlc = _update(tree[j].lc, l,      md, ql, qr, f);
            tree[j].lc = nlc;
        }
        if (qr > md)  {
            int nrc = _update(tree[j].rc, md + 1, r,  ql, qr, f);
            tree[j].rc = nrc;
        }

        S l_val = tree[j].lc != -1 ? tree[tree[j].lc].val : Mon::defR(l, md);
        S r_val = tree[j].rc != -1 ? tree[tree[j].rc].val : Mon::defR(md + 1, r);
        tree[j].val = Mon::ope(l_val, r_val);
        
        return j;
    }

    int update(int ver, lng l, lng r, F f) { 
        if (l < 0 || r > n - 1 || l > r) {
            int cur = roots[ver];
            roots.push_back(cur);
            return roots.size() - 1;
        }
        
        int cur = _update(roots[ver], 0, n - 1, l, r, f);
        roots.push_back(cur);

        return roots.size() - 1;
    }

    template<class G>
    lng _maxR(int i, lng l, lng r, lng ql, G &g, S &acc, F tac) {
        if (r < ql) {
            return -1;}
        
        if (ql <= l) {
            S val = i == -1 ? Mon::defR(l, r) : tree[i].val;
            val = Mon::map(tac, val);
            S nac = Mon::ope(acc, val);
            if (g(nac)) {
                acc = nac;
                return -1;
            }
        }

        if (l == r) {
            return l - 1;}

        tac = i == -1 ? tac : Mon::cmp(tac, tree[i].lzy);
 
        lng md = std::midpoint(l, r);
        lng res = _maxR(i == -1 ? -1 : tree[i].lc, l, md, ql, g, acc, tac);
        if (res != -1) {
            return res;}
        
        return _maxR(i == -1 ? -1 : tree[i].rc, md + 1, r, ql, g, acc, tac);
    }

    template<class G>
    lng maxR(int ver, lng l, G g) { // Returns max r in [l, n - 1] where g(query(l, r)) is true. If g(a[l]) is false, returns l - 1.
        if (l < 0 || l > n - 1) {
            return l - 1;}

        S acc = Mon::idS();
        lng res = _maxR(roots[ver], 0, n - 1, l, g, acc, Mon::idF());
        
        return res == -1 ? n - 1 : res;
    }

    template<class G>
    lng _minL(int i, lng l, lng r, lng qr, G &g, S &acc, F tac) {
        if (l > qr) {
            return -1;}
        
        if (r <= qr) {
            S val = i == -1 ? Mon::defR(l, r) : tree[i].val;
            val = Mon::map(tac, val);
            S nac = Mon::ope(val, acc);
            if (g(nac)) {
                acc = nac;
                return -1;
            }
        }

        if (l == r) {
            return l + 1;}
        
        tac = i == -1 ? tac : Mon::cmp(tac, tree[i].lzy);

        lng md = std::midpoint(l, r);
        lng res = _minL(i == -1 ? -1 : tree[i].rc, md + 1, r, qr, g, acc, tac);
        if (res != -1) {
            return res;}
        
        return _minL(i == -1 ? -1 : tree[i].lc, l, md, qr, g, acc, tac);
    }

    template<class G>
    lng minL(int ver, lng r, G g) { // Returns min l in [0, r] where g(query(l, r)) is true. If g(a[r]) is false, returns r + 1.
        if (r < 0 || r > n - 1) {
            return r + 1;}
        
        S acc = Mon::idS();
        lng res = _minL(roots[ver], 0, n - 1, r, g, acc, Mon::idF());
        
        return res == -1 ? 0 : res;
    }

    S _query(int i, lng l, lng r, lng ql, lng qr, F acc) {
        if (i == -1) {
            if (max(l, ql) > min(r, qr)) {
                return Mon::idS();}

            return Mon::map(acc, Mon::defR(max(l, ql), min(r, qr))); 
        }

        if (ql <= l && r <= qr) {
            return Mon::map(acc, tree[i].val);}

        F nac = Mon::cmp(acc, tree[i].lzy); 

        lng md = std::midpoint(l, r);
        if (qr <= md) {
            return _query(tree[i].lc, l,      md, ql, qr, nac);}
        if (ql > md) {
            return _query(tree[i].rc, md + 1, r,  ql, qr, nac);}
        
        return Mon::ope(_query(tree[i].lc, l, md, ql, qr, nac), _query(tree[i].rc, md + 1, r, ql, qr, nac));
    }

    S query(int ver, lng l, lng r) { 
        if (l < 0 || r > n - 1 || l > r) {
            return Mon::idS();}

        return _query(roots[ver], 0, n - 1, l, r, Mon::idF()); 
    }

    // Q: O(1)
    S queryAll(int ver) {
        return roots[ver] == -1 ? Mon::defR(0, n - 1) : tree[roots[ver]].val;
    }
};

// Unchanged legacy extraction: OLD/algorithms.cpp:3461-3649.
// Status: existing-unverified; correctness, performance and API audit pending.
// S: O(n * log(n)), U: N/A, Q: O(log(n)), M: O(n * log(n))
struct KSPerSegTree {
    struct Mon {
        struct S { 
            int cnt;
            lng sum;
        };
        
        struct F {
            int cnt_upd;
            lng sum_upd;

            bool operator==(const F o) const {
                return cnt_upd == o.cnt_upd && sum_upd == o.sum_upd;
            }

            bool operator!=(const F o) const {
                return !(*this == o);
            }
        };

        static constexpr inline S idS() { 
            return {0, 0}; 
        }
        
        static constexpr inline F idF() { 
            return {0, 0}; 
        }
        
        static S defR(int l, int r) { 
            return {0, 0}; 
        }
        
        static S init(int i, lng x) { 
            return {0, 0}; 
        }
        
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
        if (it == v.begin()) {
            return 0;}

        int ord = (it - v.begin()) - 1; 
        int l_cnt = segt->query(l,     0, ord).cnt;
        int r_cnt = segt->query(r + 1, 0, ord).cnt;
        
        return r_cnt - l_cnt;
    }

    int cntEql(int l, int r, lng x) {
        assert(0 <= l && l <= r && r < n);

        auto it = lower_bound(v.begin(), v.end(), x);
        if (it == v.end() || *it != x) {
            return 0;}
        
        int ord = it - v.begin();
        int l_cnt = segt->query(l,     ord, ord).cnt;
        int r_cnt = segt->query(r + 1, ord, ord).cnt;

        return r_cnt - l_cnt;
    }

    int cntRan(int l, int r, lng x1, lng x2) {
        assert(0 <= l && l <= r && r < n);

        if (x1 > x2) {
            return 0;}

        return _cntLeq(l, r, x2) - _cntLeq(l, r, x1 - 1);
    }

    int _kthMin(int l_i, int r_i, int l, int r, int k) {
        if (l == r) {
            return l;}

        int cnt = 0;
        int l_lc = l_i == -1 ? -1 : segt->tree[l_i].lc;
        if (l_lc != -1) {
            cnt -= segt->tree[l_lc].val.cnt;}
        int r_lc = r_i == -1 ? -1 : segt->tree[r_i].lc;
        if (r_lc != -1) {
            cnt += segt->tree[r_lc].val.cnt;}
        
        int md = std::midpoint(l, r);
        if (cnt >= k) {
            return _kthMin(l_lc, r_lc, l, md, k);}

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
        if (k == 0) {
            return 0;}
            
        if (l == r) {
            return lng(k) * v[l];}

        int l_cnt = 0;
        lng l_sum = 0;
        
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
        if (l_cnt >= k) {
            return _kthSum(l_lc, r_lc, l, md, k);
        } else {
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
        if (cnt == 0) {
            return -INF64;}

        return kthMin(l, r, cnt);
    }

    lng minGeq(int l, int r, lng x) {
        assert(0 <= l && l <= r && r < n);

        int cnt = _cntLeq(l, r, x - 1);
        if (cnt == r - l + 1) {
            return INF64;}

        return kthMin(l, r, cnt + 1);
    }
};

