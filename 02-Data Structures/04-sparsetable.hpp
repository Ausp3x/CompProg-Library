#pragma once

#include "../01-Core/01-template.hpp"

// 0 <= n <= INT_MAX; M supplies S and an associative op (prod also needs it idempotent); products take nonempty [l, r) and never call e.
// S: O(n * log(n + 1)), U: NA, Q: O(1), M: O(n * log(n + 1))
template<typename M>
struct SparseTable {
    using S = typename M::S;
    [[no_unique_address]] M m;
    int n, h;
    vector<vector<S>> v;

    explicit SparseTable(vector<S> a = {}, M mon = {}) : m(std::move(mon)), n(0), h(0) {
        assert(a.size() <= INT_MAX);
        n = int(a.size()); h = std::bit_width(uint(n));
        if (!n) { return; }
        v.reserve(h); v.push_back(std::move(a));
        for (int i = 1; i < h; ++i) {
            int len = 1 << i;
            vector<S> level; level.reserve(n - len + 1);
            for (int j = 0; j <= n - len; ++j) {
                level.push_back(m.op(v[i - 1][j], v[i - 1][j + len / 2]));}
            v.push_back(std::move(level));}}

    S prod(int l, int r) const {
        assert(0 <= l && l < r && r <= n);
        int i = std::bit_width(uint(r - l)) - 1;
        return m.op(v[i][l], v[i][r - (1 << i)]);}
    // Q: O(log(n + 1)); associativeProd needs only associativity.
    S associativeProd(int l, int r) const {
        assert(0 <= l && l < r && r <= n);
        int i = std::bit_width(uint(r - l)) - 1;
        S acc = v[i][l]; l += 1 << i;
        while (l < r) {
            i = std::bit_width(uint(r - l)) - 1;
            acc = m.op(acc, v[i][l]); l += 1 << i;}
        return acc;}

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

// rows x cols grid, rows * cols <= INT_MAX; M supplies S and an associative, commutative, idempotent op; prod takes nonempty [x1, x2) x [y1, y2).
// S: O(n * m * log(n + 1) * log(m + 1)), U: NA, Q: O(1), M: O(n * m * log(n + 1) * log(m + 1)) for n rows and m cols
template<typename M>
struct SparseTable2D {
    using S = typename M::S;
    [[no_unique_address]] M m;
    int rows, cols, hr, hc;
    vector<vector<S>> v;

    explicit SparseTable2D(const vector<vector<S>> &a = {}, M mon = {}) : m(std::move(mon)), rows(0), cols(0), hr(0), hc(0) {
        assert(a.size() <= INT_MAX && (a.empty() || a[0].size() <= INT_MAX / a.size()));
        rows = int(a.size()); cols = rows ? int(a[0].size()) : 0;
        assert(std::ranges::all_of(a, [this](const vector<S> &row) { return row.size() == size_t(cols); }));
        hr = std::bit_width(uint(rows)); hc = std::bit_width(uint(cols));
        if (!rows || !cols) { return; }
        v.resize(hr * hc);
        v[0].reserve(rows * cols);
        for (const auto &row : a) { v[0].insert(v[0].end(), row.begin(), row.end()); }
        for (int i = 0; i < hr; ++i) {
            for (int j = !i; j < hc; ++j) {
                int w = cols - (1 << j) + 1, h = rows - (1 << i) + 1;
                const vector<S> &src = j ? v[i * hc + j - 1] : v[(i - 1) * hc];
                int sw = j ? cols - (1 << (j - 1)) + 1 : w, di = j ? 0 : 1 << (i - 1), dj = j ? 1 << (j - 1) : 0;
                vector<S> &dst = v[i * hc + j]; dst.reserve(h * w);
                for (int x = 0; x < h; ++x) {
                    for (int y = 0; y < w; ++y) { dst.push_back(m.op(src[x * sw + y], src[(x + di) * sw + y + dj])); }}}}}

    S prod(int x1, int y1, int x2, int y2) const {
        assert(0 <= x1 && x1 < x2 && x2 <= rows && 0 <= y1 && y1 < y2 && y2 <= cols);
        int i = std::bit_width(uint(x2 - x1)) - 1, j = std::bit_width(uint(y2 - y1)) - 1, w = cols - (1 << j) + 1;
        const vector<S> &t = v[i * hc + j];
        x2 -= 1 << i; y2 -= 1 << j;
        return m.op(m.op(t[x1 * w + y1], t[x1 * w + y2]), m.op(t[x2 * w + y1], t[x2 * w + y2]));}
};
