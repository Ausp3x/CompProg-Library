#pragma once

#include "../01-Core/01-template.hpp"

// Unchanged legacy extraction: OLD/algorithms.cpp:1481-1555.
// Status: existing-unverified; correctness, performance and API audit pending.
// S: O(n), U: O(log(n)), Q: O(log(n)), M: O(n)
template<typename T>
struct FenTreeRangeAdd1D {
    int n;
    vector<T> v1, v2;

    FenTreeRangeAdd1D(int N) : n(N), v1(N, T(0)), v2(N, T(0)) {}

    FenTreeRangeAdd1D(vector<T> a) : n(a.size()), v1(std::move(a)), v2(n, T(0)) {
        for (int i = 0; i < n; i++) {
            int j = i | (i + 1);
            if (j < n) {
                v1[j] += v1[i];}
        }
    }

    void _add(int i_cur, T x1, T x2) {
        for (int i = i_cur; i < n; i |= i + 1) {
            v1[i] += x1;
            v2[i] += x2;
        }
    }

    void addUpdate(int l, int r, T x) {
        if (l > r) {
            return;}
        
        _add(l,     -x * T(l - 1), x);
        _add(r + 1, x * T(r),      -x);
    }

    T _sum(int i_cur) const {
        if (i_cur < 0) {
            return T(0);}

        T r1 = T(0), r2 = T(0);
        for (int i = i_cur; i >= 0; i = (i & (i + 1)) - 1) {
            r1 += v1[i];
            r2 += v2[i];
        }

        return r2 * T(i_cur) + r1;
    }

    T sumQuery(int l, int r) const {
        if (l > r) {
            return T(0);}
        
        return _sum(r) - _sum(l - 1);
    }

    friend ostream &operator<<(ostream &os, const FenTreeRangeAdd1D &a) {
        if (a.n == 0) {
            return os << "[]";}
            
        int mx_h = 0;
        for (int i = 0; i < a.n; i++) {
            mx_h = max(mx_h, __builtin_ctz(~i));}
        
        os << "\n[\n";
        for (int h = mx_h; h >= 0; h--) {
            os << "  [";
            bool is_fi = true;
            for (int i = 0; i < a.n; i++) {
                if (__builtin_ctz(~i) == h) {
                    os << (is_fi ? "[" : ", [") << (i & (i + 1)) << ", " << i << "]: (" << a.v1[i] << ", " << a.v2[i] << ")";
                    is_fi = false;
                }
            }
            os << "]" << (h > 0 ? ",\n" : "\n");
        }
        
        return os << "]\n";
    }
};

// Unchanged legacy extraction: OLD/algorithms.cpp:1557-1860.
// Status: existing-unverified; correctness, performance and API audit pending.
// S: O(n^d), U: O(log(n)^d), Q: O(log(n)^d), M: O(n^d)
template<typename T, typename F = std::plus<T>, typename F_inv = std::minus<T>>
struct FenTree {
    int d, n, m, l;
    vector<T> v;
    T id;
    F f;
    F_inv f_inv;

    FenTree(int N,               T ID = T(0), F f_ = F(), F_inv f_inv_ = F_inv()) : 
        d(1), n(N), m(0), l(0), v(N, ID),         id(ID), f(f_), f_inv(f_inv_) {}

    FenTree(int N, int M,        T ID = T(0), F f_ = F(), F_inv f_inv_ = F_inv()) : 
        d(2), n(N), m(M), l(0), v(N * M, ID),     id(ID), f(f_), f_inv(f_inv_) {}

    FenTree(int N, int M, int L, T ID = T(0), F f_ = F(), F_inv f_inv_ = F_inv()) : 
        d(3), n(N), m(M), l(L), v(N * M * L, ID), id(ID), f(f_), f_inv(f_inv_) {}

    FenTree(vector<T> a, T ID = T(0), F f_ = F(), F_inv f_inv_ = F_inv()) : 
        d(1), n(a.size()), m(0), l(0), v(std::move(a)), id(ID), f(f_), f_inv(f_inv_) {
        for (int i = 0; i < n; i++) {
            int p = i | (i + 1);
            if (p < n) {
                v[p] = f(v[p], v[i]);}
        }
    }

    FenTree(const vector<vector<T>> &a, T ID = T(0), F f_ = F(), F_inv f_inv_ = F_inv()) : 
        FenTree(a.size(), a.empty() ? 0 : a[0].size(), ID, f_, f_inv_) {
        for (int i = 0; i < n; i++) {
            int i_flat = i * m;
            for (int j = 0; j < m; j++) {
                v[i_flat + j] = a[i][j];}
        }

        for (int i = 0; i < n; i++) {
            int p = i | (i + 1);
            if (p < n) {
                int i_flat = i * m;
                int p_flat = p * m;
                for (int j = 0; j < m; j++) {
                    v[p_flat + j] = f(v[p_flat + j], v[i_flat + j]);}
            }
        }
        
        for (int i = 0; i < n; i++) {
            int i_flat = i * m;
            for (int j = 0; j < m; j++) {
                int p = j | (j + 1);
                if (p < m) {
                    v[i_flat + p] = f(v[i_flat + p], v[i_flat + j]);}
            }
        }
    }

    FenTree(const vector<vector<vector<T>>> &a, T ID = T(0), F f_ = F(), F_inv f_inv_ = F_inv()) : 
        FenTree(a.size(), a.empty() ? 0 : a[0].size(), (a.empty() || a[0].empty()) ? 0 : a[0][0].size(), ID, f_, f_inv_) {
        for (int i = 0; i < n; i++) {
            int i_flat = i * m * l;
            for (int j = 0; j < m; j++) {
                int ij_flat = i_flat + j * l;
                for (int k = 0; k < l; k++) {
                    v[ij_flat + k] = a[i][j][k];}
            }
        }
        
        for (int i = 0; i < n; i++) {
            int p = i | (i + 1);
            if (p < n) {
                int i_flat = i * m * l;
                int p_flat = p * m * l;
                for (int j = 0; j < m; j++) {
                    int ij_flat = i_flat + j * l;
                    int pj_flat = p_flat + j * l;
                    for (int k = 0; k < l; k++) {
                        v[pj_flat + k] = f(v[pj_flat + k], v[ij_flat + k]);}
                }
            }
        }
        
        for (int i = 0; i < n; i++) {
            int i_flat = i * m * l;
            for (int j = 0; j < m; j++) {
                int p = j | (j + 1);
                if (p < m) {
                    int ij_flat = i_flat + j * l;
                    int ip_flat = i_flat + p * l;
                    for (int k = 0; k < l; k++) {
                        v[ip_flat + k] = f(v[ip_flat + k], v[ij_flat + k]);}
                }
            }
        }
        
        for (int i = 0; i < n; i++) {
            int i_flat = i * m * l;
            for (int j = 0; j < m; j++) {
                int ij_flat = i_flat + j * l;
                for (int k = 0; k < l; k++) {
                    int p = k | (k + 1);
                    if (p < l) {
                        v[ij_flat + p] = f(v[ij_flat + p], v[ij_flat + k]);}
                }
            }
        }
    }

    // 1D
    void update(int i_cur, T x) {
        assert(d == 1);

        for (int i = i_cur; i < n; i |= i + 1) {
            v[i] = f(v[i], x);}
    }

    T _query(int i_cur) const {
        assert(d == 1);
        
        T res = id;
        for (int i = i_cur; i >= 0; i = (i & (i + 1)) - 1) {
            res = f(res, v[i]);}
        
        return res;
    }

    T query(int i1, int i2) const {
        assert(d == 1);

        if (i1 > i2) {
            return id;}
        
        return f_inv(_query(i2), (i1 > 0 ? _query(i1 - 1) : id));
    }

    int lowerBound(T x) const {
        assert(d == 1);

        if (x <= id || n == 0) {
            return 0;}

        int cur = 0;
        for (int i = __lg(max(1, n)); i >= 0; i--) {
            int nxt = cur + (1 << i);
            if (nxt <= n && v[nxt - 1] < x) {
                x = f_inv(x, v[nxt - 1]);
                cur = nxt;
            }
        }

        return cur;
    }

    // 2D
    inline int getIdx(int i, int j) const {
        return i * m + j;
    }

    void update(int i_cur, int j_cur, T x) {
        assert(d == 2);

        for (int i = i_cur; i < n; i |= i + 1) {
            int i_flat = i * m;
            for (int j = j_cur; j < m; j |= j + 1) {
                v[i_flat + j] = f(v[i_flat + j], x);}
        }
    }

    T _query(int i_cur, int j_cur) const {
        assert(d == 2);
        
        if (i_cur < 0 || j_cur < 0) {
            return id;}
        
        T res = id;
        for (int i = i_cur; i >= 0; i = (i & (i + 1)) - 1) {
            int i_flat = i * m;
            for (int j = j_cur; j >= 0; j = (j & (j + 1)) - 1) {
                res = f(res, v[i_flat + j]);}
        }
        
        return res;
    }

    T query(int i1, int j1, int i2, int j2) const {
        assert(d == 2);

        if (i1 > i2 || j1 > j2) {
            return id;}
        
        T res = _query(i2, j2);
        res   = f_inv(res, _query(i1 - 1, j2));
        res   = f_inv(res, _query(i2, j1 - 1));
        res   = f(res, _query(i1 - 1, j1 - 1));
        
        return res;
    }

    // 3D
    inline int getIdx(int i, int j, int k) const {
        return (i * m + j) * l + k;
    }

    void update(int i_cur, int j_cur, int k_cur, T x) {
        assert(d == 3);

        for (int i = i_cur; i < n; i |= i + 1) {
            int i_flat = i * m * l;
            for (int j = j_cur; j < m; j |= j + 1) {
                int ij_flat = i_flat + j * l;
                for (int k = k_cur; k < l; k |= k + 1) {
                    v[ij_flat + k] = f(v[ij_flat + k], x);}
            }
        }
    }

    T _query(int i_cur, int j_cur, int k_cur) const {
        assert(d == 3);
        
        if (i_cur < 0 || j_cur < 0 || k_cur < 0) {
            return id;}
        
        T res = id;
        for (int i = i_cur; i >= 0; i = (i & (i + 1)) - 1) {
            int i_flat = i * m * l;
            for (int j = j_cur; j >= 0; j = (j & (j + 1)) - 1) {
                int ij_flat = i_flat + j * l;
                for (int k = k_cur; k >= 0; k = (k & (k + 1)) - 1) {
                    res = f(res, v[ij_flat + k]);}
            }
        }
        
        return res;
    }

    T query(int i1, int j1, int k1, int i2, int j2, int k2) const {
        assert(d == 3);

        if (i1 > i2 || j1 > j2 || k1 > k2) {
            return id;}

        T res = _query(i2, j2, k2);
        res   = f_inv(res, _query(i1 - 1, j2, k2));
        res   = f_inv(res, _query(i2, j1 - 1, k2));
        res   = f_inv(res, _query(i2, j2, k1 - 1));
        res   = f(res, _query(i1 - 1, j1 - 1, k2));
        res   = f(res, _query(i1 - 1, j2, k1 - 1));
        res   = f(res, _query(i2, j1 - 1, k1 - 1));
        res   = f_inv(res, _query(i1 - 1, j1 - 1, k1 - 1));

        return res;
    }

    friend ostream &operator<<(ostream &os, const FenTree &a) {
        if (a.n == 0) {
            return os << "[]";}
        
        os << "\n[\n";
        if (a.d == 1) {
            int mx_h = 0;
            for (int i = 0; i < a.n; i++) {
                mx_h = max(mx_h, __builtin_ctz(~i));}
            
            for (int h = mx_h; h >= 0; h--) {
                os << "  [";
                bool is_fi = true;
                for (int i = 0; i < a.n; i++) {
                    if (__builtin_ctz(~i) == h) {
                        os << (is_fi ? "[" : ", [") << (i & (i + 1)) << ", " << i << "]: " << a.v[i];
                        is_fi = false;
                    }
                }
                os << "]" << (h > 0 ? ",\n" : "\n");
            }
        } else if (a.d == 2) {
            for (int i = 0; i < a.n; i++) {
                os << "  [";
                for (int j = 0; j < a.m; j++) {
                    os << "[" 
                       << (i & (i + 1)) << ", " << i << "; " 
                       << (j & (j + 1)) << ", " << j << "]: " 
                       << a.v[i * a.m + j] << (j < a.m - 1 ? ", " : "");
                }
                os << "]" << (i < a.n - 1 ? ",\n" : "\n");
            }
        } else if (a.d == 3) {
            for (int i = 0; i < a.n; i++) {
                os << "  [\n";
                for (int j = 0; j < a.m; j++) {
                    os << "    [";
                    for (int k = 0; k < a.l; k++) {
                        os << "[" 
                           << (i & (i + 1)) << ", " << i << "; " 
                           << (j & (j + 1)) << ", " << j << "; " 
                           << (k & (k + 1)) << ", " << k << "]: " 
                           << a.v[(i * a.m + j) * a.l + k] << (k < a.l - 1 ? ", " : "");
                    }
                    os << "]" << (j < a.m - 1 ? ",\n" : "\n");
                }
                os << "  ]" << (i < a.n - 1 ? ",\n" : "\n");
            }
        }
        
        return os << "]\n";
    }
};

