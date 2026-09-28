// S: O(n), U: O(log(n)), Q: O(log(n)), M: O(n)
template<typename T>
struct FenTreeRangeAdd1D {
    int n;
    vector<T> v1, v2;

    FenTreeRangeAdd1D(int N) : n(N), v1(N, T(0)), v2(N, T(0)) {}
    FenTreeRangeAdd1D(vector<T> a) : n(a.size()), v1(std::move(a)), v2(n, T(0)) {
        for (int i = 0; i < n; i++) {
            int j = i | (i + 1);
            if (j < n) { v1[j] += v1[i]; }
        }
    }

    void _add(int i_cur, T x1, T x2) {
        for (int i = i_cur; i < n; i |= i + 1) { 
            v1[i] += x1; v2[i] += x2;}
    }
    void addUpdate(int l, int r, T x) {
        if (l > r) { return; }
        _add(l,     -x * T(l - 1), x);
        _add(r + 1, x * T(r),      -x);
    }

    T _sum(int i_cur) const {
        if (i_cur < 0) { return T(0); }
        T r1 = T(0), r2 = T(0);
        for (int i = i_cur; i >= 0; i = (i & (i + 1)) - 1) {
            r1 += v1[i]; r2 += v2[i];}
        return r2 * T(i_cur) + r1;
    }
    T sumQuery(int l, int r) const {
        if (l > r) { return T(0); }
        return _sum(r) - _sum(l - 1);
    }
};