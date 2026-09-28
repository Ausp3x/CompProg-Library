// S: O(n), U: O(sqrt(n)), Q: O(sqrt(n)), M: O(n)
template<typename T, typename F = std::plus<T>>
struct SqrtDecomp {
    int n, m;
    vector<T> v, blks;
    T id; F f;

    SqrtDecomp(int N, T ID = T(0), F f_ = F()): 
        n(N), m(std::sqrt(max(1, N))), v(N, ID), blks((N + m - 1) / m, ID), id(ID), f(f_) {}

    SqrtDecomp(const vector<T> &a, T ID = T(0), F f_ = F()): 
        SqrtDecomp(a.size(), ID, f_) {
        for (int i = 0; i < n; i++) {
            v[i] = a[i];
            blks[i / m] = f(blks[i / m], v[i]);
        }
    }

    inline void _rebuild(int b_i) {
        blks[b_i] = id;
        int l = b_i * m; int r = min(n, l + m);
        for (int i = l; i < r; i++) { blks[b_i] = f(blks[b_i], v[i]); }
    }
    void opeUpdate(int i, T x) { v[i] = f(v[i], x); _rebuild(i / m); }
    void setUpdate(int i, T x) { v[i] = x; _rebuild(i / m); }

    T query(int l, int r) const {
        if (l > r) { return id; }
        T res = id;
        int l_blk = l / m, r_blk = r / m;
        if (l_blk == r_blk) {
            for (int i = l; i <= r; i++) { res = f(res, v[i]); }
        } else {
            for (int i = l; i < (l_blk + 1) * m; i++) { res = f(res, v[i]); }
            for (int i = l_blk + 1; i < r_blk; i++) { res = f(res, blks[i]); }
            for (int i = r_blk * m; i <= r; i++) { res = f(res, v[i]); }
        }
        return res;
    }
};