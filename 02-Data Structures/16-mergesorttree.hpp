#pragma once

#include "../01-Core/01-template.hpp"

// Unchanged legacy extraction: OLD/algorithms.cpp:3185-3459.
// Status: existing-unverified; correctness, performance and API audit pending.
// S: O(n * log(n)), U: O(log(n)^2), Q: O(log(n)^2), M: O(n * log(n))
struct DynMergeSortTree {
    struct Node {
        indexed_set<pair<lng, int>> j_set;
        int lc = -1, rc = -1;
    };

    lng mn_i, mx_i;
    int root = -1;
    vector<Node> tree;

    bool is_1d = false;
    int cnt = 0; 
    vector<lng> v;

    DynMergeSortTree(const vector<lng> &a) : mn_i(0), mx_i(int(a.size()) - 1), is_1d(true), v(a) {
        tree.reserve(2'500'000); 
        root = createNode();
        for (int i = 0; i < int(a.size()); i++) {
            _insert(root, mn_i, mx_i, i, v[i], i);}
    }

    DynMergeSortTree(lng Mn_i, lng Mx_i) : mn_i(Mn_i), mx_i(Mx_i) {
        tree.reserve(2'500'000); 
        root = createNode();
    }

    int createNode() {
        tree.emplace_back();
        
        return tree.size() - 1;
    }

    int _cntRan(int u, lng l, lng r, lng i1, lng i2, lng j1, lng j2) {
        if (u == -1 || i1 > r || i2 < l) {
            return 0;}

        if (i1 <= l && r <= i2) {
            return tree[u].j_set.order_of_key({j2 + 1, -1}) - tree[u].j_set.order_of_key({j1, -1});}

        lng md = std::midpoint(l, r);
        return _cntRan(tree[u].lc, l,      md, i1, i2, j1, j2) + 
               _cntRan(tree[u].rc, md + 1, r,  i1, i2, j1, j2);
    }

    void _collect(int u, lng l, lng r, lng i1, lng i2, vector<int> &act) {
        if (u == -1 || i1 > r || i2 < l) {
            return;}

        if (i1 <= l && r <= i2) {
            act.push_back(u);
            return;
        }

        lng md = std::midpoint(l, r);
        _collect(tree[u].lc, l,      md, i1, i2, act);
        _collect(tree[u].rc, md + 1, r,  i1, i2, act);
    }

    void _erase(int u, lng l, lng r, lng i, lng j, int id) {
        if (u == -1) {
            return;}
        
        tree[u].j_set.erase({j, id});

        if (l == r) {
            return;}

        lng md = std::midpoint(l, r);
        if (i <= md) {
            _erase(tree[u].lc, l,      md, i, j, id);
        } else {
            _erase(tree[u].rc, md + 1, r,  i, j, id);
        }
    }

    void _insert(int u, lng l, lng r, lng i, lng j, int id) {
        tree[u].j_set.insert({j, id});

        if (l == r) {
            return;}

        lng md = std::midpoint(l, r);
        if (i <= md) {
            if (tree[u].lc == -1) {
                int nlc = createNode();
                tree[u].lc = nlc;
            }

            _insert(tree[u].lc, l,      md, i, j, id);
        } else {
            if (tree[u].rc == -1) {
                int nrc = createNode();
                tree[u].rc = nrc;
            }            
            
            _insert(tree[u].rc, md + 1, r,  i, j, id);
        }
    }

    lng _maxLeq(int u, lng l, lng r, lng i1, lng i2, lng x) {
        if (u == -1 || i1 > r || i2 < l) {
            return -INF64;}

        if (i1 <= l && r <= i2) {
            auto it = tree[u].j_set.upper_bound({x, INF32}); 
            if (it == tree[u].j_set.begin()) {
                return -INF64;}
            it--;

            return it->first;
        }

        lng md = std::midpoint(l, r);
        return max(_maxLeq(tree[u].lc, l,      md, i1, i2, x), 
                   _maxLeq(tree[u].rc, md + 1, r,  i1, i2, x));
    }

    lng _minGeq(int u, lng l, lng r, lng i1, lng i2, lng x) {
        if (u == -1 || i1 > r || i2 < l) {
            return INF64;}

        if (i1 <= l && r <= i2) {
            auto it = tree[u].j_set.lower_bound({x, -1});
            if (it == tree[u].j_set.end()) {
                return INF64;}
            
            return it->first;
        }

        lng md = std::midpoint(l, r);
        return min(_minGeq(tree[u].lc, l,      md, i1, i2, x), 
                   _minGeq(tree[u].rc, md + 1, r,  i1, i2, x));
    }

    // 1D
    void update1D(int i, lng x) {
        assert(is_1d);
        assert(0 <= i && i <= mx_i);
        
        _erase(root, mn_i, mx_i, i, v[i], i); 
        v[i] = x;                         
        _insert(root, mn_i, mx_i, i, v[i], i); 
    }

    int cntEql1D(int l, int r, lng x) {
        assert(is_1d && mn_i <= l && l <= r && r <= mx_i);

        return _cntRan(root, mn_i, mx_i, l, r, x, x);
    }

    int cntRan1D(int l, int r, lng j1, lng j2) {
        assert(is_1d && mn_i <= l && l <= r && r <= mx_i);

        if (j1 > j2) {
            return 0;}
        
        return _cntRan(root, mn_i, mx_i, l, r, j1, j2);
    }

    // Q: O(log(n)^3)
    lng kthMin1D(int l, int r, int k) {
        assert(is_1d);

        vector<int> act;
        _collect(root, mn_i, mx_i, l, r, act);

        lng lo = -INF64 - 1, hi = INF64 + 1;
        while (hi - lo > 1) {
            lng m = lo + (hi - lo) / 2;
            int cnt = 0;
            for (int u : act) {
                cnt += tree[u].j_set.order_of_key({m + 1, -1});}
            
            if (cnt >= k) {
                hi = m;
            } else {
                lo = m;
            }
        }
        
        return hi;
    }

    lng maxLeq1D(int l, int r, lng x) {
        assert(is_1d);

        if (l > r) {
            return -INF64;}
        
        return _maxLeq(root, mn_i, mx_i, l, r, x);
    }

    lng minGeq1D(int l, int r, lng x) {
        assert(is_1d);
        
        if (l > r) {
            return INF64;}
        
        return _minGeq(root, mn_i, mx_i, l, r, x);
    }

    // 2D
    int insert2D(lng i, lng j) {
        assert(!is_1d);

        int id = cnt++;
        _insert(root, mn_i, mx_i, i, j, id);
        
        return id; 
    }

    void erase2D(lng i, lng j, int id) {
        assert(!is_1d);

        _erase(root, mn_i, mx_i, i, j, id);
    }

    int cntRan2D(lng i1, lng i2, lng j1, lng j2) {
        assert(!is_1d);

        if (i1 > i2 || j1 > j2) {
            return 0;}
        
        return _cntRan(root, mn_i, mx_i, i1, i2, j1, j2);
    }

    int cntEql2D(lng i1, lng i2, lng x) {
        assert(!is_1d);

        return _cntRan(root, mn_i, mx_i, i1, i2, x, x);
    }

    // Q: O(log(n)^3)
    lng kthMin2D(lng i1, lng i2, int k) {
        assert(!is_1d);

        vector<int> act;
        _collect(root, mn_i, mx_i, i1, i2, act);

        lng lo = -INF64 - 1, hi = INF64 + 1;
        while (hi - lo > 1) {
            lng m = lo + (hi - lo) / 2;
            int cnt = 0;
            for (int u : act) {
                cnt += tree[u].j_set.order_of_key({m + 1, -1});}
            
            if (cnt >= k) {
                hi = m;
            } else {
                lo = m;
            }
        }
        
        return hi;
    }

    lng maxLeq2D(lng i1, lng i2, lng x) {
        assert(!is_1d);

        if (i1 > i2) {
            return -INF64;}
        
        return _maxLeq(root, mn_i, mx_i, i1, i2, x);
    }

    lng minGeq2D(lng i1, lng i2, lng x) {
        assert(!is_1d);
        
        if (i1 > i2) {
            return INF64;}
        
        return _minGeq(root, mn_i, mx_i, i1, i2, x);
    }
};

