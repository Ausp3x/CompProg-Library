#pragma once

#include "../01-Core/01-template.hpp"

// T is a commutative ring; 2 * n * max|a_i| over the history fits T.
// S: O(n), U: O(log(n)), Q: O(log(n)), M: O(n)
template<typename T>
struct FenwickRangeAdd {
    int n;
    vector<T> b, c;

    explicit FenwickRangeAdd(int N = 0) : n(N) { assert(n >= 0); b.assign(n, T(0)); c.assign(n, T(0)); }
    explicit FenwickRangeAdd(const vector<T> &a) : FenwickRangeAdd(int(a.size())) {
        for (int i = 0; i < n; ++i) { b[i] = i ? a[i] - a[i - 1] : a[i]; c[i] = b[i] * T(i); }
        for (int i = 0; i < n; ++i) {
            if (int p = i | (i + 1); p < n) { b[p] += b[i]; c[p] += c[i]; }}}

    T get(int i) const {
        assert(0 <= i && i < n);
        T res = T(0);
        for (++i; i > 0; i &= i - 1) { res += b[i - 1]; }
        return res;}

    void update(int i, T x) {
        T y = x * T(i);
        for (; i < n; i |= i + 1) { b[i] += x; c[i] += y; }}
    void add(int l, int r, T x) { assert(0 <= l && l <= r && r <= n); update(l, x); update(r, -x); }

    T prefixSum(int r) const {
        assert(0 <= r && r <= n);
        T sb = T(0), sc = T(0);
        for (int i = r; i > 0; i &= i - 1) { sb += b[i - 1]; sc += c[i - 1]; }
        return sb * T(r) - sc;}
    T sum(int l, int r) const { assert(0 <= l && l <= r && r <= n); return prefixSum(r) - prefixSum(l); }
};

// T is a commutative group; get returns the current point value.
// S: O(n), U: O(log(n)), Q: O(log(n)), M: O(n)
template<typename T>
struct DualFenwick {
    int n;
    vector<T> v;

    explicit DualFenwick(int N = 0) : n(N) { assert(n >= 0); v.assign(n, T(0)); }
    explicit DualFenwick(const vector<T> &a) : DualFenwick(int(a.size())) {
        for (int i = 0; i < n; ++i) { v[i] = i ? a[i] - a[i - 1] : a[i]; }
        for (int i = 0; i < n; ++i) {
            if (int p = i | (i + 1); p < n) { v[p] += v[i]; }}}

    T get(int i) const {
        assert(0 <= i && i < n);
        T res = T(0);
        for (++i; i > 0; i &= i - 1) { res += v[i - 1]; }
        return res;}

    void update(int i, T x) { for (; i < n; i |= i + 1) { v[i] += x; }}
    void add(int l, int r, T x) { assert(0 <= l && l <= r && r <= n); update(l, x); update(r, -x); }
};

// T is a commutative ring; 2 * n^2 * max(|a_i|, |a|, |d|) over the history fits T.
// S: O(n), U: O(log(n)), Q: O(log(n)), M: O(n)
template<typename T>
struct FenwickRangeArithmeticAdd {
    struct Node { T a, b, c; };
    int n;
    vector<Node> v;

    static T tri(int p) { return T(lng(p) * (p - 1) / 2); }
    explicit FenwickRangeArithmeticAdd(int N = 0) : n(N) { assert(n >= 0); v.assign(n, {T(0), T(0), T(0)}); }
    explicit FenwickRangeArithmeticAdd(const vector<T> &a) : FenwickRangeArithmeticAdd(int(a.size())) {
        for (int i = 0; i < n; ++i) { v[i].a = a[i]; }
        for (int i = 0; i < n; ++i) {
            if (int p = i | (i + 1); p < n) { v[p].a += v[i].a; }}}

    T get(int i) const { assert(0 <= i && i < n); return sum(i, i + 1); }

    void update(int i, T a, T b, T c) {
        for (; i < n; i |= i + 1) { v[i].a += a; v[i].b += b; v[i].c += c; }}
    void addProgression(int l, int r, T a, T d) {
        assert(0 <= l && l <= r && r <= n);
        T c0 = a - d * T(l);
        update(l, -(c0 * T(l) + d * tri(l)), c0, d);
        update(r, c0 * T(r) + d * tri(r), -c0, -d);}
    void add(int l, int r, T x) { addProgression(l, r, x, T(0)); }

    T prefixSum(int r) const {
        assert(0 <= r && r <= n);
        Node s{T(0), T(0), T(0)};
        for (int i = r; i > 0; i &= i - 1) { s.a += v[i - 1].a; s.b += v[i - 1].b; s.c += v[i - 1].c; }
        return s.a + s.b * T(r) + s.c * tri(r);}
    T sum(int l, int r) const { assert(0 <= l && l <= r && r <= n); return prefixSum(r) - prefixSum(l); }
};

// T is a commutative group; n * m <= INT_MAX; rectangles are [i1, i2) x [j1, j2).
// S: O(n * m), U: O(log(n) * log(m)), Q: O(log(n) * log(m)), M: O(n * m)
template<typename T>
struct Fenwick2D {
    int n, m;
    vector<T> v;

    Fenwick2D(int N, int M) : n(N), m(M) { assert(n >= 0 && m >= 0 && lng(n) * m <= INT_MAX); v.assign(n * m, T(0)); }
    explicit Fenwick2D(const vector<vector<T>> &a) : Fenwick2D(int(a.size()), a.empty() ? 0 : int(a[0].size())) {
        for (int i = 0; i < n; ++i) { assert(int(a[i].size()) == m); std::copy(a[i].begin(), a[i].end(), v.begin() + i * m); }
        for (int i = 0; i < n; ++i) {
            if (int p = i | (i + 1); p < n) {
                for (int j = 0; j < m; ++j) { v[p * m + j] += v[i * m + j]; }}}
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < m; ++j) {
                if (int q = j | (j + 1); q < m) { v[i * m + q] += v[i * m + j]; }}}}

    T get(int i, int j) const { return sum(i, j, i + 1, j + 1); }

    void add(int i, int j, T x) {
        assert(0 <= i && i < n && 0 <= j && j < m);
        for (; i < n; i |= i + 1) {
            for (int k = j; k < m; k |= k + 1) { v[i * m + k] += x; }}}

    T prefixSum(int i, int j) const {
        assert(0 <= i && i <= n && 0 <= j && j <= m);
        T res = T(0);
        for (; i > 0; i &= i - 1) {
            for (int k = j; k > 0; k &= k - 1) { res += v[(i - 1) * m + k - 1]; }}
        return res;}
    T sum(int i1, int j1, int i2, int j2) const {
        assert(i1 <= i2 && j1 <= j2);
        return prefixSum(i2, j2) - prefixSum(i1, j2) - prefixSum(i2, j1) + prefixSum(i1, j1);}
};

// T is a commutative ring; n * m <= INT_MAX and 4 * n * m * max|a| over the history fits T; rectangles are [i1, i2) x [j1, j2).
// S: O(n * m), U: O(log(n) * log(m)), Q: O(log(n) * log(m)), M: O(n * m)
template<typename T>
struct Fenwick2DRangeAdd {
    struct Node { T d, di, dj, dij; };
    int n, m;
    vector<Node> v;

    Fenwick2DRangeAdd(int N, int M) : n(N), m(M) { assert(n >= 0 && m >= 0 && lng(n) * m <= INT_MAX); v.assign(n * m, {T(0), T(0), T(0), T(0)}); }

    void update(int i, int j, T x) {
        Node u{x, x * T(i), x * T(j), x * T(i) * T(j)};
        for (; i < n; i |= i + 1) {
            for (int k = j; k < m; k |= k + 1) {
                Node &w = v[i * m + k];
                w.d += u.d; w.di += u.di; w.dj += u.dj; w.dij += u.dij;}}}
    void add(int i1, int j1, int i2, int j2, T x) {
        assert(0 <= i1 && i1 <= i2 && i2 <= n && 0 <= j1 && j1 <= j2 && j2 <= m);
        update(i1, j1, x); update(i1, j2, -x); update(i2, j1, -x); update(i2, j2, x);}

    T prefixSum(int i, int j) const {
        assert(0 <= i && i <= n && 0 <= j && j <= m);
        Node s{T(0), T(0), T(0), T(0)};
        for (int p = i; p > 0; p &= p - 1) {
            for (int q = j; q > 0; q &= q - 1) {
                const Node &w = v[(p - 1) * m + q - 1];
                s.d += w.d; s.di += w.di; s.dj += w.dj; s.dij += w.dij;}}
        return s.d * T(i) * T(j) - s.di * T(j) - s.dj * T(i) + s.dij;}
    T sum(int i1, int j1, int i2, int j2) const {
        assert(i1 <= i2 && j1 <= j2);
        return prefixSum(i2, j2) - prefixSum(i1, j2) - prefixSum(i2, j1) + prefixSum(i1, j1);}
};

// T is a commutative group; n * m * h <= INT_MAX; boxes are half-open per axis.
// S: O(n * m * h), U: O(log(n) * log(m) * log(h)), Q: O(log(n) * log(m) * log(h)), M: O(n * m * h)
template<typename T>
struct Fenwick3D {
    int n, m, h;
    vector<T> v;

    Fenwick3D(int N, int M, int H) : n(N), m(M), h(H) { assert(n >= 0 && m >= 0 && h >= 0 && lng(n) * m <= INT_MAX && lng(n) * m * h <= INT_MAX); v.assign(n * m * h, T(0)); }
    explicit Fenwick3D(const vector<vector<vector<T>>> &a)
        : Fenwick3D(int(a.size()), a.empty() ? 0 : int(a[0].size()), a.empty() || a[0].empty() ? 0 : int(a[0][0].size())) {
        for (int i = 0; i < n; ++i) {
            assert(int(a[i].size()) == m);
            for (int j = 0; j < m; ++j) { assert(int(a[i][j].size()) == h); std::copy(a[i][j].begin(), a[i][j].end(), v.begin() + (i * m + j) * h); }}
        for (int i = 0; i < n; ++i) {
            if (int p = i | (i + 1); p < n) {
                for (int t = 0; t < m * h; ++t) { v[p * m * h + t] += v[i * m * h + t]; }}}
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < m; ++j) {
                if (int q = j | (j + 1); q < m) {
                    for (int k = 0; k < h; ++k) { v[(i * m + q) * h + k] += v[(i * m + j) * h + k]; }}}}
        for (int t = 0; t < n * m; ++t) {
            for (int k = 0; k < h; ++k) {
                if (int q = k | (k + 1); q < h) { v[t * h + q] += v[t * h + k]; }}}}

    T get(int i, int j, int k) const { return sum(i, j, k, i + 1, j + 1, k + 1); }

    void add(int i, int j, int k, T x) {
        assert(0 <= i && i < n && 0 <= j && j < m && 0 <= k && k < h);
        for (; i < n; i |= i + 1) {
            for (int p = j; p < m; p |= p + 1) {
                for (int q = k; q < h; q |= q + 1) { v[(i * m + p) * h + q] += x; }}}}

    T prefixSum(int i, int j, int k) const {
        assert(0 <= i && i <= n && 0 <= j && j <= m && 0 <= k && k <= h);
        T res = T(0);
        for (; i > 0; i &= i - 1) {
            for (int p = j; p > 0; p &= p - 1) {
                for (int q = k; q > 0; q &= q - 1) { res += v[((i - 1) * m + p - 1) * h + q - 1]; }}}
        return res;}
    T sum(int i1, int j1, int k1, int i2, int j2, int k2) const {
        assert(i1 <= i2 && j1 <= j2 && k1 <= k2);
        return prefixSum(i2, j2, k2) - prefixSum(i1, j2, k2) - prefixSum(i2, j1, k2) - prefixSum(i2, j2, k1)
             + prefixSum(i1, j1, k2) + prefixSum(i1, j2, k1) + prefixSum(i2, j1, k1) - prefixSum(i1, j1, k1);}
};

// T is a commutative group; add needs a registered point; sum covers [x1, x2) x [y1, y2) for any lng bounds.
// S: O(n * log(n)), U: O(log(n)^2), Q: O(log(n)^2), M: O(n * log(n))
template<typename T>
struct CompressedFenwick2D {
    vector<lng> xs;
    vector<int> start;
    vector<lng> ys;
    vector<T> v;

    explicit CompressedFenwick2D(vector<pair<lng, lng>> pts) {
        sort(pts.begin(), pts.end(), [](const auto &a, const auto &b) { return a.second < b.second; });
        for (auto &[x, y] : pts) { xs.push_back(x); }
        sort(xs.begin(), xs.end());
        xs.erase(unique(xs.begin(), xs.end()), xs.end());
        int n = int(xs.size());
        vector<vector<lng>> col(n);
        for (auto &[x, y] : pts) {
            for (int i = int(lower_bound(xs.begin(), xs.end(), x) - xs.begin()); i < n; i |= i + 1) {
                if (col[i].empty() || col[i].back() != y) { col[i].push_back(y); }}}
        start.assign(n + 1, 0);
        for (int i = 0; i < n; ++i) { start[i + 1] = start[i] + int(col[i].size()); ys.insert(ys.end(), col[i].begin(), col[i].end()); }
        v.assign(ys.size(), T(0));}

    void add(lng x, lng y, T w) {
        int n = int(xs.size()), i = int(lower_bound(xs.begin(), xs.end(), x) - xs.begin());
        assert(i < n && xs[i] == x && binary_search(ys.begin() + start[i], ys.begin() + start[i + 1], y));
        for (; i < n; i |= i + 1) {
            auto b = ys.begin() + start[i], e = ys.begin() + start[i + 1];
            for (int j = int(lower_bound(b, e, y) - b), len = int(e - b); j < len; j |= j + 1) { v[start[i] + j] += w; }}}

    T prefixSum(lng x, lng y) const {
        T res = T(0);
        for (int i = int(lower_bound(xs.begin(), xs.end(), x) - xs.begin()); i > 0; i &= i - 1) {
            auto b = ys.begin() + start[i - 1], e = ys.begin() + start[i];
            for (int j = int(lower_bound(b, e, y) - b); j > 0; j &= j - 1) { res += v[start[i - 1] + j - 1]; }}
        return res;}
    T sum(lng x1, lng y1, lng x2, lng y2) const {
        assert(x1 <= x2 && y1 <= y2);
        return prefixSum(x2, y2) - prefixSum(x1, y2) - prefixSum(x2, y1) + prefixSum(x1, y1);}
};

// f is associative and commutative with identity id; apply sets a[i] = f(a[i], x); no range inverse.
// S: O(n), U: O(log(n)), Q: O(log(n)), M: O(n)
template<typename T, typename F>
struct FenwickPrefixMonoid {
    int n;
    T id;
    F f;
    vector<T> v;

    FenwickPrefixMonoid(int N, T id, F f) : n(N), id(id), f(f) { assert(n >= 0); v.assign(n, id); }
    FenwickPrefixMonoid(const vector<T> &a, T id, F f) : n(int(a.size())), id(id), f(f), v(a) {
        for (int i = 0; i < n; ++i) {
            if (int p = i | (i + 1); p < n) { v[p] = this->f(v[p], v[i]); }}}

    void apply(int i, const T &x) {
        assert(0 <= i && i < n);
        for (; i < n; i |= i + 1) { v[i] = f(v[i], x); }}

    T prefix(int r) const {
        assert(0 <= r && r <= n);
        T res = id;
        for (; r > 0; r &= r - 1) { res = f(res, v[r - 1]); }
        return res;}
};

// Universe [0, n); kth and next return n, prev returns -1 when absent.
// S: O(n / w), U: O(log(n / w)), Q: O(log(n / w) + log(w)), M: O(n / w)
struct Fenwick01 {
    int n, nw, cnt;
    vector<ulng> bits;
    vector<int> c;

    explicit Fenwick01(int N = 0) : n(N), nw((N + 63) >> 6), cnt(0) { assert(n >= 0); bits.assign(nw, 0); c.assign(nw, 0); }
    // T: O(n), M: O(n / w).
    explicit Fenwick01(const vector<bool> &a) : Fenwick01(int(a.size())) {
        for (int i = 0; i < n; ++i) { bits[i >> 6] |= ulng(a[i]) << (i & 63); }
        for (int i = 0; i < nw; ++i) {
            c[i] += std::popcount(bits[i]);
            cnt += std::popcount(bits[i]);
            if (int p = i | (i + 1); p < nw) { c[p] += c[i]; }}}

    bool get(int i) const { assert(0 <= i && i < n); return bits[i >> 6] >> (i & 63) & 1; }
    bool contains(int i) const { return get(i); }
    int size() const { return cnt; }

    void add(int i, int x) {
        assert((x == 1 || x == -1) && get(i) == (x < 0));
        bits[i >> 6] ^= ulng(1) << (i & 63);
        cnt += x;
        for (int k = i >> 6; k < nw; k |= k + 1) { c[k] += x; }}
    bool insert(int i) { return get(i) ? false : (add(i, 1), true); }
    bool erase(int i) { return get(i) ? (add(i, -1), true) : false; }

    int rank(int r) const {
        assert(0 <= r && r <= n);
        int k = r >> 6, res = 0;
        for (int i = k; i > 0; i &= i - 1) { res += c[i - 1]; }
        return k < nw ? res + std::popcount(bits[k] & ((ulng(1) << (r & 63)) - 1)) : res;}
    int sum(int l, int r) const { assert(l <= r); return rank(r) - rank(l); }
    int kth(int k) const {
        assert(k >= 0);
        if (k >= cnt) { return n; }
        int p = 0;
        for (int d = int(std::bit_floor(uint(nw))); d > 0; d >>= 1) {
            if (p + d <= nw && c[p + d - 1] <= k) { k -= c[p + d - 1]; p += d; }}
        ulng x = bits[p];
        int res = p << 6;
        for (int s = 32; s > 0; s >>= 1) {
            if (int low = std::popcount(x & ((ulng(1) << s) - 1)); low <= k) { k -= low; x >>= s; res += s; }}
        return res;}
    int next(int i) const { assert(0 <= i && i <= n); return kth(rank(i)); }
    int prev(int i) const {
        assert(-1 <= i && i < n);
        int k = rank(i + 1);
        return k ? kth(k - 1) : -1;}
};
using FenwickSet = Fenwick01;
