#pragma once

#include "../01-Core/01-template.hpp"

// Zero-based half-open ranges/rectangles; coordinates are (row, column).
// Each dimension is in [0, INT_MAX-1]. T is an additive commutative group with
// zero T(0); every stored value and intermediate must fit T, or be modular.
// Data is owned; direct mutation of public storage invalidates its invariants.
namespace prefix_sum_detail {
    inline int checkedSize(size_t n) { assert(n < INT_MAX); return int(n); }
    inline void checkSize(int n) { assert(0 <= n && n < INT_MAX); }
    template<typename T>
    void checkRows(const vector<vector<T>> &a, int m) {
        for (const auto &row : a) { assert(row.size() == size_t(m)); }}
} // namespace prefix_sum_detail

// S: O(n + 1), U: NA, Q: O(1), M: O(n + 1); rebuild has O(n + 1) workspace.
template<typename T>
struct PrefixSum {
    int n;
    vector<T> p;

    explicit PrefixSum(int n = 0) : n(n) {
        prefix_sum_detail::checkSize(n); p.assign(n + 1, T(0));}
    explicit PrefixSum(const vector<T> &a) : PrefixSum(prefix_sum_detail::checkedSize(a.size())) {
        for (int i = 0; i < n; ++i) { p[i + 1] = p[i] + a[i]; }}
    void rebuild(const vector<T> &a) { *this = PrefixSum(a); }

    T prefixSum(int r) const { assert(0 <= r && r <= n); return p[r]; }
    T sum(int l, int r) const {
        assert(0 <= l && l <= r && r <= n); return p[r] - p[l];}
};

// S: O((n + 1) * (m + 1)), U: NA, Q: O(1), M: O((n + 1) * (m + 1)).
// rebuild uses the same additional workspace; n rows, m columns. Empty vector
// input means 0x0; the dimension constructor also represents 0xm and nx0.
template<typename T>
struct PrefixSum2D {
    int n, m;
    vector<vector<T>> p;

    explicit PrefixSum2D(int n = 0, int m = 0) : n(n), m(m) {
        prefix_sum_detail::checkSize(n); prefix_sum_detail::checkSize(m);
        p.assign(n + 1, vector<T>(m + 1, T(0)));}
    explicit PrefixSum2D(const vector<vector<T>> &a) :
        PrefixSum2D(prefix_sum_detail::checkedSize(a.size()), a.empty() ? 0 : prefix_sum_detail::checkedSize(a[0].size())) {
        prefix_sum_detail::checkRows(a, m);
        for (int i = 0; i < n; ++i) {
            T row = T(0);
            for (int j = 0; j < m; ++j) { row += a[i][j]; p[i + 1][j + 1] = p[i][j + 1] + row; }}}
    void rebuild(const vector<vector<T>> &a) { *this = PrefixSum2D(a); }

    T prefixSum(int r, int c) const {
        assert(0 <= r && r <= n && 0 <= c && c <= m); return p[r][c];}
    T sum(int x1, int y1, int x2, int y2) const {
        assert(0 <= x1 && x1 <= x2 && x2 <= n && 0 <= y1 && y1 <= y2 && y2 <= m);
        return (p[x2][y2] - p[x1][y2]) - (p[x2][y1] - p[x1][y1]);}
};

// S: O(n), U: O(1), Q: O(n) to materialize, M: O(n).
// Offline range additions (imos). values is non-destructive, returns O(n) data;
// rebuild replaces initial values, clear resets to zeros. Both take O(n).
template<typename T>
struct DifferenceArray {
    int n;
    vector<T> d;

    explicit DifferenceArray(int n = 0) : n(n) {
        prefix_sum_detail::checkSize(n); d.assign(n, T(0));}
    explicit DifferenceArray(const vector<T> &a) : DifferenceArray(prefix_sum_detail::checkedSize(a.size())) {
        if (n) { d[0] = a[0]; }
        for (int i = 1; i < n; ++i) { d[i] = a[i] - a[i - 1]; }}
    void rebuild(const vector<T> &a) { *this = DifferenceArray(a); }
    void clear() { fill(d.begin(), d.end(), T(0)); }

    void add(int l, int r, T x) {
        assert(0 <= l && l <= r && r <= n);
        if (l == r) { return; }
        d[l] += x; if (r < n) { d[r] -= x; }}
    vector<T> values() const {
        vector<T> a = d;
        for (int i = 1; i < n; ++i) { a[i] += a[i - 1]; }
        return a;}
};

// S: O((n + 1) * (m + 1)), U: O(1), Q: O((n + 1) * (m + 1)) to materialize,
// M: O((n + 1) * (m + 1)); returned values and rebuild workspace have this bound.
// Offline rectangle additions; empty rectangles are no-ops. Same reset/ownership
// rules as DifferenceArray. No unused far-border subtraction is performed.
template<typename T>
struct DifferenceArray2D {
    int n, m;
    vector<vector<T>> d;

    explicit DifferenceArray2D(int n = 0, int m = 0) : n(n), m(m) {
        prefix_sum_detail::checkSize(n); prefix_sum_detail::checkSize(m);
        d.assign(n, vector<T>(m, T(0)));}
    explicit DifferenceArray2D(const vector<vector<T>> &a) :
        DifferenceArray2D(prefix_sum_detail::checkedSize(a.size()), a.empty() ? 0 : prefix_sum_detail::checkedSize(a[0].size())) {
        prefix_sum_detail::checkRows(a, m);
        for (int i = 0; i < n; ++i) { for (int j = 0; j < m; ++j) {
            d[i][j] = a[i][j];
            if (i) { d[i][j] -= a[i - 1][j]; }
            if (j) { d[i][j] -= a[i][j - 1]; }
            if (i && j) { d[i][j] += a[i - 1][j - 1]; }}}}
    void rebuild(const vector<vector<T>> &a) { *this = DifferenceArray2D(a); }
    void clear() { for (auto &row : d) { fill(row.begin(), row.end(), T(0)); }}

    void add(int x1, int y1, int x2, int y2, T x) {
        assert(0 <= x1 && x1 <= x2 && x2 <= n && 0 <= y1 && y1 <= y2 && y2 <= m);
        if (x1 == x2 || y1 == y2) { return; }
        d[x1][y1] += x;
        if (x2 < n) { d[x2][y1] -= x; }
        if (y2 < m) { d[x1][y2] -= x; }
        if (x2 < n && y2 < m) { d[x2][y2] += x; }}
    vector<vector<T>> values() const {
        auto a = d;
        for (int i = 0; i < n; ++i) {
            for (int j = 1; j < m; ++j) { a[i][j] += a[i][j - 1]; }}
        for (int i = 1; i < n; ++i) {
            for (int j = 0; j < m; ++j) { a[i][j] += a[i - 1][j]; }}
        return a;}
};
