// S: O(n^d), U: O(log(n)^d), Q: O(log(n)^d), M: O(n^d)
template<typename T, typename F = std::plus<T>, typename F_inv = std::minus<T>>
struct FenTree {
    int d, n, m, l;
    vector<T> v;
    T id; F f; F_inv f_inv;

    FenTree(int N,               T ID = T(0), F f_ = F(), F_inv f_inv_ = F_inv()) : 
        d(1), n(N), m(0), l(0), v(N, ID),         id(ID), f(f_), f_inv(f_inv_) {}
    FenTree(int N, int M,        T ID = T(0), F f_ = F(), F_inv f_inv_ = F_inv()) : 
        d(2), n(N), m(M), l(0), v(N * M, ID),     id(ID), f(f_), f_inv(f_inv_) {}
    FenTree(int N, int M, int L, T ID = T(0), F f_ = F(), F_inv f_inv_ = F_inv()) : 
        d(3), n(N), m(M), l(L), v(N * M * L, ID), id(ID), f(f_), f_inv(f_inv_) {}
    // update manually

    // 1D
    void update(int i_cur, T x) {
        assert(d == 1);
        for (int i = i_cur; i < n; i |= i + 1) { v[i] = f(v[i], x); }
    }
    
    T _query(int i_cur) const {
        assert(d == 1);
        T res = id;
        for (int i = i_cur; i >= 0; i = (i & (i + 1)) - 1) { res = f(res, v[i]); }    
        return res;
    }
    T query(int i1, int i2) const {
        assert(d == 1);
        if (i1 > i2) { return id; }
        return f_inv(_query(i2), (i1 > 0 ? _query(i1 - 1) : id));
    }

    int lowerBound(T x) const {
        assert(d == 1);
        if (x <= id || n == 0) { return 0; }
        int cur = 0;
        for (int i = __lg(max(1, n)); i >= 0; i--) {
            int nxt = cur + (1 << i);
            if (nxt <= n && v[nxt - 1] < x) {
                x = f_inv(x, v[nxt - 1]); cur = nxt;}
        }
        return cur;
    }

    // 2D
    inline int getIdx(int i, int j) const { return i * m + j; }

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
        if (i_cur < 0 || j_cur < 0) { return id; }
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
        if (i1 > i2 || j1 > j2) { return id; }
        T res = _query(i2, j2);
        res   = f_inv(res, _query(i1 - 1, j2));
        res   = f_inv(res, _query(i2, j1 - 1));
        res   = f(res, _query(i1 - 1, j1 - 1));
        return res;
    }

    // 3D
    inline int getIdx(int i, int j, int k) const { return (i * m + j) * l + k; }

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
        if (i_cur < 0 || j_cur < 0 || k_cur < 0) { return id; }
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
        if (i1 > i2 || j1 > j2 || k1 > k2) { return id; }
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
};