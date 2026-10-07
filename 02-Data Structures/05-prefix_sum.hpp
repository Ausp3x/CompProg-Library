#pragma once

#include "../01-Core/01-template.hpp"

namespace prefix_sum_detail {
    // T: O(1), M: O(1)
    inline int checkedSize(size_t n) { assert(n < size_t(INT_MAX)); return int(n); }
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

    void rebuild(const vector<T> &a) { *this = PrefixSum(a); }

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

    void rebuild(const vector<vector<T>> &a) { *this = PrefixSum2D(a); }

    T prefixSum(int x, int y) const { assert(0 <= x && x <= n && 0 <= y && y <= m); return p[x][y]; }
    T sum(int x1, int y1, int x2, int y2) const {
        assert(0 <= x1 && x1 <= x2 && x2 <= n && 0 <= y1 && y1 <= y2 && y2 <= m);
        return (p[x2][y2] - p[x1][y2]) - (p[x2][y1] - p[x1][y1]);}
};

// S: O(n), U: O(1), Q: O(n), M: O(n)
template<typename T>
struct DifferenceArray {
    int n;
    vector<T> d;

    explicit DifferenceArray(int n = 0) : n(n) {
        assert(0 <= n && n < INT_MAX); d.assign(n, T(0));}
    explicit DifferenceArray(const vector<T> &a) : DifferenceArray(prefix_sum_detail::checkedSize(a.size())) {
        if (n) { d[0] = a[0]; }
        for (int i = 1; i < n; ++i) { d[i] = a[i] - a[i - 1]; }}
    DifferenceArray(std::initializer_list<T> a) : DifferenceArray(vector<T>(a)) {}

    void rebuild(const vector<T> &a) { *this = DifferenceArray(a); }
    void clear() { fill(d.begin(), d.end(), T(0)); }
    void add(int l, int r, T x) {
        assert(0 <= l && l <= r && r <= n);
        if (l == r) { return; }
        d[l] += x;
        if (r < n) { d[r] -= x; }}

    vector<T> values() const {
        vector<T> a = d;
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

    void rebuild(const vector<vector<T>> &a) { *this = DifferenceArray2D(a); }
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
