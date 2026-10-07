#pragma once

#include "../01-Core/01-template.hpp"

// 0 <= n <= INT_MAX; f is associative (query also idempotent); queries take nonempty [l, r); no identity is needed.
// S: O(n * log(n + 1)), U: NA, Q: O(1), M: O(n * log(n + 1))
template<typename T, typename F>
struct SparseTable {
    int n, h;
    vector<vector<T>> v;
    F f;

    SparseTable(vector<T> a, const F &fnc) : n(0), h(0), f(fnc) {
        assert(a.size() <= INT_MAX);
        n = int(a.size()); h = std::bit_width(uint(n));
        if (!n) { return; }
        v.reserve(h); v.push_back(std::move(a));
        for (int i = 1; i < h; ++i) {
            int len = 1 << i;
            vector<T> level; level.reserve(n - len + 1);
            for (int j = 0; j <= n - len; ++j) {
                level.push_back(this->f(v[i - 1][j], v[i - 1][j + len / 2]));}
            v.push_back(std::move(level));}}

    T query(int l, int r) const {
        assert(0 <= l && l < r && r <= n);
        int i = std::bit_width(uint(r - l)) - 1;
        return f(v[i][l], v[i][r - (1 << i)]);}
    T queryFast(int l, int r) const { assert(0 <= l && l <= r && r < n); return query(l, r + 1); }
    // Q: O(log(n + 1)); fold needs only associativity, querySlow is its inclusive [l, r] form.
    T fold(int l, int r) const {
        assert(0 <= l && l < r && r <= n);
        int i = std::bit_width(uint(r - l)) - 1;
        T acc = v[i][l]; l += 1 << i;
        while (l < r) {
            i = std::bit_width(uint(r - l)) - 1;
            acc = f(acc, v[i][l]); l += 1 << i;}
        return acc;}
    T querySlow(int l, int r) const { assert(0 <= l && l <= r && r < n); return fold(l, r + 1); }

    // T: O(n * log(n + 1)); legacy diagnostic format with inclusive blocks.
    friend ostream &operator<<(ostream &os, const SparseTable &a) {
        if (!a.n) { return os << "[]"; }
        os << "\n[\n";
        for (int i = a.h - 1; i >= 0; --i) {
            os << "  [";
            int len = 1 << i;
            for (int j = 0; j <= a.n - len; ++j) {
                os << "[" << j << ", " << j + len - 1 << "]: " << a.v[i][j]
                   << (j < a.n - len ? ", " : "");}
            os << "]" << (i ? ",\n" : "\n");}
        return os << "]\n";}
};

// n x m grid, n * m <= INT_MAX; f is associative, commutative and idempotent; queries take nonempty [x1, x2) x [y1, y2).
// S: O(n * m * log(n + 1) * log(m + 1)), U: NA, Q: O(1), M: O(n * m * log(n + 1) * log(m + 1))
template<typename T, typename F>
struct SparseTable2D {
    int n, m, hn, hm;
    vector<vector<T>> v;
    F f;

    SparseTable2D(const vector<vector<T>> &a, const F &fnc) : n(0), m(0), hn(0), hm(0), f(fnc) {
        assert(a.size() <= INT_MAX && (a.empty() || a[0].size() <= INT_MAX / a.size()));
        n = int(a.size()); m = n ? int(a[0].size()) : 0;
        assert(std::ranges::all_of(a, [this](const vector<T> &row) { return row.size() == size_t(m); }));
        hn = std::bit_width(uint(n)); hm = std::bit_width(uint(m));
        if (!n || !m) { return; }
        v.resize(hn * hm);
        v[0].reserve(n * m);
        for (const auto &row : a) { v[0].insert(v[0].end(), row.begin(), row.end()); }
        for (int i = 0; i < hn; ++i) {
            for (int j = !i; j < hm; ++j) {
                int w = m - (1 << j) + 1, h = n - (1 << i) + 1;
                const vector<T> &src = j ? v[i * hm + j - 1] : v[(i - 1) * hm];
                int sw = j ? m - (1 << (j - 1)) + 1 : w, di = j ? 0 : 1 << (i - 1), dj = j ? 1 << (j - 1) : 0;
                vector<T> &dst = v[i * hm + j]; dst.reserve(h * w);
                for (int x = 0; x < h; ++x) {
                    for (int y = 0; y < w; ++y) { dst.push_back(this->f(src[x * sw + y], src[(x + di) * sw + y + dj])); }}}}}

    T query(int x1, int y1, int x2, int y2) const {
        assert(0 <= x1 && x1 < x2 && x2 <= n && 0 <= y1 && y1 < y2 && y2 <= m);
        int i = std::bit_width(uint(x2 - x1)) - 1, j = std::bit_width(uint(y2 - y1)) - 1, w = m - (1 << j) + 1;
        const vector<T> &t = v[i * hm + j];
        x2 -= 1 << i; y2 -= 1 << j;
        return f(f(t[x1 * w + y1], t[x1 * w + y2]), f(t[x2 * w + y1], t[x2 * w + y2]));}
};
