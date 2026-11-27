#pragma once

#include "../01-Core/01-template.hpp"

namespace prefix_sum_detail {
    // T: O(1), M: O(1)
    inline int checkedSize(size_t n) { assert(n < size_t(INT_MAX)); return int(n); }

    // T: O(n * m * k), M: O(1); prefix sums along every axis of a flat n x m x k array, in place.
    template<typename T>
    void integrate3D(vector<T> &a, int n, int m, int k) {
        if (!n || !m || !k) { return; }
        int plane = m * k, total = n * plane;
        for (int i = plane; i < total; ++i) { a[i] += a[i - plane]; }
        for (int x = 0; x < n; ++x) {
            for (int i = x * plane + k; i < (x + 1) * plane; ++i) { a[i] += a[i - k]; }}
        for (int row = 0; row < n * m; ++row) {
            for (int i = row * k + 1; i < (row + 1) * k; ++i) { a[i] += a[i - 1]; }}}

    // T: O(n * m * k), M: O(1); the inverse of integrate3D, in place.
    template<typename T>
    void differentiate3D(vector<T> &a, int n, int m, int k) {
        int plane = m * k, total = n * plane;
        for (int row = 0; row < n * m; ++row) {
            for (int i = (row + 1) * k - 1; i > row * k; --i) { a[i] -= a[i - 1]; }}
        for (int x = 0; x < n; ++x) {
            for (int i = (x + 1) * plane - 1; i >= x * plane + k; --i) { a[i] -= a[i - k]; }}
        for (int i = total - 1; i >= plane; --i) { a[i] -= a[i - plane]; }}

    // T: O(n * m), M: O(1); dimensions of a 3D vector, asserting equal plane and row lengths.
    template<typename T>
    array<int, 3> shape3D(const vector<vector<vector<T>>> &a) {
        int n = checkedSize(a.size()), m = n ? checkedSize(a[0].size()) : 0, k = m ? checkedSize(a[0][0].size()) : 0;
        assert(std::ranges::all_of(a, [&](const vector<vector<T>> &plane) {
            return plane.size() == size_t(m) && std::ranges::all_of(plane, [&](const vector<T> &row) { return row.size() == size_t(k); });}));
        return {n, m, k};}
} // namespace prefix_sum_detail

// 0 <= n < INT_MAX; T is an additive commutative group with zero T(0); ranges [l, r).
// S: O(n + 1), U: NA, Q: O(1), M: O(n + 1)
template<typename T>
struct PrefixSum {
    int n;
    vector<T> p;

    explicit PrefixSum(int n = 0) : n(n) {
        assert(0 <= n && n < INT_MAX); p.assign(n + 1, T(0));}
    explicit PrefixSum(const vector<T> &a) : PrefixSum(prefix_sum_detail::checkedSize(a.size())) {
        for (int i = 0; i < n; ++i) { p[i + 1] = p[i] + a[i]; }}
    PrefixSum(std::initializer_list<T> a) : PrefixSum(vector<T>(a)) {}

    T prefixSum(int r) const { assert(0 <= r && r <= n); return p[r]; }
    T sum(int l, int r) const { assert(0 <= l && l <= r && r <= n); return p[r] - p[l]; }
};

// n rows and m columns, each in [0, INT_MAX); rectangles [x1, x2) x [y1, y2); empty vector input is 0 x 0.
// S: O((n + 1) * (m + 1)), U: NA, Q: O(1), M: O((n + 1) * (m + 1))
template<typename T>
struct PrefixSum2D {
    int n, m;
    vector<vector<T>> p;

    explicit PrefixSum2D(int n = 0, int m = 0) : n(n), m(m) {
        assert(0 <= n && n < INT_MAX && 0 <= m && m < INT_MAX);
        p.assign(n + 1, vector<T>(m + 1, T(0)));}
    explicit PrefixSum2D(const vector<vector<T>> &a) :
        PrefixSum2D(prefix_sum_detail::checkedSize(a.size()), a.empty() ? 0 : prefix_sum_detail::checkedSize(a[0].size())) {
        assert(std::ranges::all_of(a, [this](const vector<T> &row) { return row.size() == size_t(m); }));
        for (int i = 0; i < n; ++i) {
            T row = T(0);
            for (int j = 0; j < m; ++j) { row += a[i][j]; p[i + 1][j + 1] = p[i][j + 1] + row; }}}

    T prefixSum(int x, int y) const { assert(0 <= x && x <= n && 0 <= y && y <= m); return p[x][y]; }
    T sum(int x1, int y1, int x2, int y2) const {
        assert(0 <= x1 && x1 <= x2 && x2 <= n && 0 <= y1 && y1 <= y2 && y2 <= m);
        return (p[x2][y2] - p[x1][y2]) - (p[x2][y1] - p[x1][y1]);}
};

// addArithmetic(l, r, x, step) adds x + (i - l) * step at each i in [l, r), needs T(int) and *, and (r - 1 - l) * step must fit T.
// S: O(n), U: O(1) (O(n) for the first addArithmetic after construction or clear), Q: O(n), M: O(n)
template<typename T>
struct DifferenceArray {
    int n;
    vector<T> d, s;

    explicit DifferenceArray(int n = 0) : n(n) {
        assert(0 <= n && n < INT_MAX); d.assign(n, T(0));}
    explicit DifferenceArray(const vector<T> &a) : DifferenceArray(prefix_sum_detail::checkedSize(a.size())) {
        if (n) { d[0] = a[0]; }
        for (int i = 1; i < n; ++i) { d[i] = a[i] - a[i - 1]; }}
    DifferenceArray(std::initializer_list<T> a) : DifferenceArray(vector<T>(a)) {}

    void clear() { fill(d.begin(), d.end(), T(0)); s.clear(); }
    void add(int l, int r, T x) {
        assert(0 <= l && l <= r && r <= n);
        if (l == r) { return; }
        d[l] += x;
        if (r < n) { d[r] -= x; }}
    void addArithmetic(int l, int r, T x, T step) {
        assert(0 <= l && l <= r && r <= n);
        if (l == r) { return; }
        if (s.empty()) { s.assign(n, T(0)); }
        d[l] += x;
        if (r - l > 1) {
            s[l + 1] += step;
            if (r < n) { s[r] -= step; }}
        if (r < n) { d[r] -= x + T(r - 1 - l) * step; }}

    vector<T> values() const {
        vector<T> a = d;
        if (!s.empty()) {
            T slope = T(0);
            for (int i = 0; i < n; ++i) { slope += s[i]; a[i] += slope; }}
        for (int i = 1; i < n; ++i) { a[i] += a[i - 1]; }
        return a;}
};

// S: O((n + 1) * (m + 1)), U: O(1), Q: O((n + 1) * (m + 1)), M: O((n + 1) * (m + 1))
template<typename T>
struct DifferenceArray2D {
    int n, m;
    vector<vector<T>> d;

    explicit DifferenceArray2D(int n = 0, int m = 0) : n(n), m(m) {
        assert(0 <= n && n < INT_MAX && 0 <= m && m < INT_MAX);
        d.assign(n, vector<T>(m, T(0)));}
    explicit DifferenceArray2D(const vector<vector<T>> &a) :
        DifferenceArray2D(prefix_sum_detail::checkedSize(a.size()), a.empty() ? 0 : prefix_sum_detail::checkedSize(a[0].size())) {
        assert(std::ranges::all_of(a, [this](const vector<T> &row) { return row.size() == size_t(m); }));
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < m; ++j) {
                d[i][j] = a[i][j];
                if (i) { d[i][j] -= a[i - 1][j]; }
                if (j) { d[i][j] -= a[i][j - 1]; }
                if (i && j) { d[i][j] += a[i - 1][j - 1]; }}}}

    void clear() { for (auto &row : d) { fill(row.begin(), row.end(), T(0)); }}
    void add(int x1, int y1, int x2, int y2, T w) {
        assert(0 <= x1 && x1 <= x2 && x2 <= n && 0 <= y1 && y1 <= y2 && y2 <= m);
        if (x1 == x2 || y1 == y2) { return; }
        d[x1][y1] += w;
        if (x2 < n) { d[x2][y1] -= w; }
        if (y2 < m) { d[x1][y2] -= w; }
        if (x2 < n && y2 < m) { d[x2][y2] += w; }}

    vector<vector<T>> values() const {
        auto a = d;
        for (int i = 0; i < n; ++i) {
            for (int j = 1; j < m; ++j) { a[i][j] += a[i][j - 1]; }}
        for (int i = 1; i < n; ++i) {
            for (int j = 0; j < m; ++j) { a[i][j] += a[i - 1][j]; }}
        return a;}
};

// G is a group (monoid with inv; op need not commute); prod(l, r) is a[l] * ... * a[r - 1] in order.
// S: O(n + 1), U: NA, Q: O(1), M: O(n + 1)
template<typename G>
struct PrefixProduct {
    using S = typename G::S;
    [[no_unique_address]] G m;
    int n;
    vector<S> p;

    explicit PrefixProduct(const vector<S> &a = {}, G mon = {}) : m(std::move(mon)), n(prefix_sum_detail::checkedSize(a.size())) {
        p.reserve(n + 1); p.push_back(m.e());
        for (const S &x : a) { p.push_back(m.op(p.back(), x)); }}

    S prod(int l, int r) const { assert(0 <= l && l <= r && r <= n); return m.op(m.inv(p[l]), p[r]); }
};

// n x m x k cells with (n + 1) * (m + 1) * (k + 1) <= INT_MAX; boxes [x1, x2) x [y1, y2) x [z1, z2); T as in PrefixSum.
// S: O((n + 1) * (m + 1) * (k + 1)), U: NA, Q: O(1), M: O((n + 1) * (m + 1) * (k + 1))
template<typename T>
struct PrefixSum3D {
    int n, m, k;
    vector<T> p;

    explicit PrefixSum3D(int n = 0, int m = 0, int k = 0) : n(n), m(m), k(k) {
        assert(0 <= n && 0 <= m && 0 <= k && (lll(n) + 1) * (lll(m) + 1) * (lll(k) + 1) <= INT_MAX);
        p.assign(size_t(n + 1) * size_t(m + 1) * size_t(k + 1), T(0));}
    explicit PrefixSum3D(const vector<vector<vector<T>>> &a) : PrefixSum3D(0, 0, 0) {
        auto dims = prefix_sum_detail::shape3D(a);
        *this = PrefixSum3D(dims[0], dims[1], dims[2]);
        for (int x = 0; x < n; ++x) {
            for (int y = 0; y < m; ++y) {
                for (int z = 0; z < k; ++z) { p[id(x + 1, y + 1, z + 1)] = a[x][y][z]; }}}
        prefix_sum_detail::integrate3D(p, n + 1, m + 1, k + 1);}

    int id(int x, int y, int z) const { return (x * (m + 1) + y) * (k + 1) + z; }
    T prefixSum(int x, int y, int z) const { assert(0 <= x && x <= n && 0 <= y && y <= m && 0 <= z && z <= k); return p[id(x, y, z)]; }
    T sum(int x1, int y1, int z1, int x2, int y2, int z2) const {
        assert(0 <= x1 && x1 <= x2 && x2 <= n && 0 <= y1 && y1 <= y2 && y2 <= m && 0 <= z1 && z1 <= z2 && z2 <= k);
        auto column = [&](int x, int y) { return p[id(x, y, z2)] - p[id(x, y, z1)]; };
        return (column(x2, y2) - column(x2, y1)) - (column(x1, y2) - column(x1, y1));}
};

// S: O(n * m * k + n * m + n), U: O(1), Q: O(n * m * k + n * m + n), M: O(n * m * k)
template<typename T>
struct DifferenceArray3D {
    int n, m, k;
    vector<T> d;

    explicit DifferenceArray3D(int n = 0, int m = 0, int k = 0) : n(n), m(m), k(k) {
        assert(0 <= n && 0 <= m && 0 <= k && lll(n) * max(m, 1) * max(k, 1) <= INT_MAX);
        d.assign(size_t(n) * size_t(m) * size_t(k), T(0));}
    explicit DifferenceArray3D(const vector<vector<vector<T>>> &a) : DifferenceArray3D(0, 0, 0) {
        auto dims = prefix_sum_detail::shape3D(a);
        *this = DifferenceArray3D(dims[0], dims[1], dims[2]);
        for (int x = 0; x < n; ++x) {
            for (int y = 0; y < m; ++y) { std::ranges::copy(a[x][y], d.begin() + id(x, y, 0)); }}
        prefix_sum_detail::differentiate3D(d, n, m, k);}

    int id(int x, int y, int z) const { return (x * m + y) * k + z; }
    void clear() { fill(d.begin(), d.end(), T(0)); }
    void add(int x1, int y1, int z1, int x2, int y2, int z2, T w) {
        assert(0 <= x1 && x1 <= x2 && x2 <= n && 0 <= y1 && y1 <= y2 && y2 <= m && 0 <= z1 && z1 <= z2 && z2 <= k);
        if (x1 == x2 || y1 == y2 || z1 == z2) { return; }
        for (int c = 0; c < 8; ++c) {
            int x = c & 1 ? x2 : x1, y = c & 2 ? y2 : y1, z = c & 4 ? z2 : z1;
            if (x == n || y == m || z == k) { continue; }
            if (std::popcount(uint(c)) & 1) { d[id(x, y, z)] -= w; }
            else { d[id(x, y, z)] += w; }}}

    vector<vector<vector<T>>> values() const {
        vector<T> a = d;
        prefix_sum_detail::integrate3D(a, n, m, k);
        vector<vector<vector<T>>> res(n);
        for (int x = 0; x < n; ++x) {
            res[x].resize(m);
            for (int y = 0; y < m; ++y) { res[x][y].assign(a.begin() + id(x, y, 0), a.begin() + id(x, y, 0) + k); }}
        return res;}
};

// S: O(n + 1), U: NA, Q: O(1), M: O(n + 1); sum(k, l, r) adds a[i] over i in [l, r) with i % step == k.
template<typename T>
struct StepSum {
    int n, step;
    vector<PrefixSum<T>> chains;

    explicit StepSum(const vector<T> &a, int step) : n(prefix_sum_detail::checkedSize(a.size())), step(step) {
        assert(step >= 1);
        vector<vector<T>> parts(min(step, n));
        for (int i = 0; i < n; ++i) { parts[i % step].push_back(a[i]); }
        chains.reserve(parts.size());
        for (const auto &part : parts) { chains.emplace_back(part); }}

    T sum(int k, int l, int r) const {
        assert(0 <= k && k < step && 0 <= l && l <= r && r <= n);
        if (k >= int(chains.size())) { return T(0); }
        auto before = [&](int x) { return x <= k ? 0 : (x - k - 1) / step + 1; };
        return chains[k].sum(before(l), before(r));}
};
