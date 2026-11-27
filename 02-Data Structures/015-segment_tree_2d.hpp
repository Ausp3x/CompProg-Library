#pragma once

#include "../01-Core/01-template.hpp"

// M is a commutative monoid (S, op, e); rows * cols <= 2^28 cells; rectangles are [xl, xr) x [yl, yr).
// S: O(n * m), U: O(log(n) * log(m)), Q: O(log(n) * log(m)), M: O(n * m) for n rows and m cols
template<typename M>
struct SegmentTree2DDense {
    using S = typename M::S;
    static constexpr lng MAX_CELLS = 1 << 28;
    [[no_unique_address]] M m;
    int rows, cols;
    vector<S> d;

    explicit SegmentTree2DDense(int R = 0, int C = 0, M mon = {}) : m(std::move(mon)), rows(R), cols(C) {
        assert(0 <= rows && 0 <= cols && lng(rows) * cols <= MAX_CELLS);
        d.assign(4 * size_t(max(rows, 0)) * size_t(max(cols, 0)), m.e());}
    explicit SegmentTree2DDense(const vector<vector<S>> &a, M mon = {}) : SegmentTree2DDense(int(a.size()), a.empty() ? 0 : int(a[0].size()), std::move(mon)) {
        for (int i = 0; i < rows; ++i) {
            assert(int(a[i].size()) == cols);
            std::copy(a[i].begin(), a[i].end(), d.begin() + at(rows + i, cols));
            for (int j = cols - 1; j >= 1; --j) { pullCol(rows + i, j); }}
        for (int i = rows - 1; i >= 1; --i) {
            for (int j = 1; j < 2 * cols; ++j) { pullRow(i, j); }}}

    int at(int i, int j) const { return 2 * cols * i + j; }
    void pullCol(int i, int j) { d[at(i, j)] = m.op(d[at(i, 2 * j)], d[at(i, 2 * j + 1)]); }
    void pullRow(int i, int j) { d[at(i, j)] = m.op(d[at(2 * i, j)], d[at(2 * i + 1, j)]); }

    const S &get(int i, int j) const { assert(0 <= i && i < rows && 0 <= j && j < cols); return d[at(rows + i, cols + j)]; }
    S allProd() const { return rows && cols ? d[at(1, 1)] : m.e(); }

    void set(int i, int j, const S &x) {
        assert(0 <= i && i < rows && 0 <= j && j < cols);
        i += rows; j += cols;
        d[at(i, j)] = x;
        for (int b = j >> 1; b; b >>= 1) { pullCol(i, b); }
        for (int a = i >> 1; a; a >>= 1) {
            for (int b = j; b; b >>= 1) { pullRow(a, b); }}}
    void apply(int i, int j, const S &x) { set(i, j, m.op(get(i, j), x)); }

    S rowProd(int i, int l, int r) const {
        S res = m.e();
        for (l += cols, r += cols; l < r; l >>= 1, r >>= 1) {
            if (l & 1) { res = m.op(res, d[at(i, l++)]); }
            if (r & 1) { res = m.op(res, d[at(i, --r)]); }}
        return res;}
    S prod(int xl, int xr, int yl, int yr) const {
        assert(0 <= xl && xl <= xr && xr <= rows && 0 <= yl && yl <= yr && yr <= cols);
        S res = m.e();
        for (xl += rows, xr += rows; xl < xr; xl >>= 1, xr >>= 1) {
            if (xl & 1) { res = m.op(res, rowProd(xl++, yl, yr)); }
            if (xr & 1) { res = m.op(res, rowProd(--xr, yl, yr)); }}
        return res;}
};

// A supplies F, a commutative composition and id; rows * cols <= 2^28 cells; get returns the composed action at a cell.
// S: O(n * m), U: O(log(n) * log(m)), Q: O(log(n) * log(m)), M: O(n * m) for n rows and m cols
template<typename A>
struct DualSegmentTree2DDense {
    using F = typename A::F;
    static constexpr lng MAX_CELLS = 1 << 28;
    [[no_unique_address]] A m;
    int rows, cols;
    vector<F> lz;

    explicit DualSegmentTree2DDense(int R = 0, int C = 0, A mon = {}) : m(std::move(mon)), rows(R), cols(C) {
        assert(0 <= rows && 0 <= cols && lng(rows) * cols <= MAX_CELLS);
        lz.assign(4 * size_t(max(rows, 0)) * size_t(max(cols, 0)), m.id());}

    int at(int i, int j) const { return 2 * cols * i + j; }

    F get(int i, int j) const {
        assert(0 <= i && i < rows && 0 <= j && j < cols);
        F res = m.id();
        for (int a = i + rows; a; a >>= 1) {
            for (int b = j + cols; b; b >>= 1) { res = m.composition(lz[at(a, b)], res); }}
        return res;}
    // T: O(n * m), M: O(n * m).
    vector<vector<F>> values() const {
        vector<F> c = lz;
        for (int a = 1; a < 2 * rows; ++a) {
            for (int b = 2; b < 2 * cols; ++b) { c[at(a, b)] = m.composition(c[at(a, b >> 1)], c[at(a, b)]); }}
        for (int a = 2; a < 2 * rows; ++a) {
            for (int b = cols; b < 2 * cols; ++b) { c[at(a, b)] = m.composition(c[at(a >> 1, b)], c[at(a, b)]); }}
        vector<vector<F>> res(rows, vector<F>(cols));
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) { res[i][j] = c[at(rows + i, cols + j)]; }}
        return res;}

    void applyRow(int i, int l, int r, const F &f) {
        for (l += cols, r += cols; l < r; l >>= 1, r >>= 1) {
            if (l & 1) { lz[at(i, l)] = m.composition(f, lz[at(i, l)]); ++l; }
            if (r & 1) { --r; lz[at(i, r)] = m.composition(f, lz[at(i, r)]); }}}
    void apply(int xl, int xr, int yl, int yr, const F &f) {
        assert(0 <= xl && xl <= xr && xr <= rows && 0 <= yl && yl <= yr && yr <= cols);
        for (xl += rows, xr += rows; xl < xr; xl >>= 1, xr >>= 1) {
            if (xl & 1) { applyRow(xl++, yl, yr, f); }
            if (xr & 1) { applyRow(--xr, yl, yr, f); }}}
};

// M is a commutative monoid; the k points are fixed at construction (duplicates merge, weights combine) with lng coordinates; rectangles are [xl, xr) x [yl, yr).
// S: O(k * log(k)), U: O(log(k)^2), Q: O(log(k)^2), M: O(k * log(k))
template<typename M>
struct SegmentTree2DSparse {
    using S = typename M::S;
    [[no_unique_address]] M m;
    int k;
    vector<pair<lng, lng>> pts;
    vector<int> off, id;
    vector<lng> ys;
    vector<S> t;

    explicit SegmentTree2DSparse(const vector<pair<lng, lng>> &p = {}, const vector<S> &w = {}, M mon = {}) : m(std::move(mon)), pts(p) {
        assert(w.empty() || w.size() == p.size());
        sort(pts.begin(), pts.end());
        pts.erase(unique(pts.begin(), pts.end()), pts.end());
        k = int(pts.size());
        vector<int> len(2 * k);
        for (int v = 2 * k - 1; v >= 1; --v) { len[v] = v >= k ? 1 : len[2 * v] + len[2 * v + 1]; }
        off.assign(2 * k + 1, 0);
        for (int v = 1; v < 2 * k; ++v) { off[v + 1] = off[v] + len[v]; }
        id.resize(off[2 * k]);
        auto below = [&](int a, int b) { return pair(pts[a].second, a) < pair(pts[b].second, b); };
        for (int v = 2 * k - 1; v >= 1; --v) {
            if (v >= k) { id[off[v]] = v - k; }
            else { std::merge(id.begin() + off[2 * v], id.begin() + off[2 * v + 1], id.begin() + off[2 * v + 1], id.begin() + off[2 * v + 2], id.begin() + off[v], below); }}
        ys.resize(id.size());
        for (int q = 0; q < int(id.size()); ++q) { ys[q] = pts[id[q]].second; }
        vector<S> val(k, m.e());
        for (int i = 0; i < int(w.size()); ++i) { int q = find(p[i].first, p[i].second); val[q] = m.op(val[q], w[i]); }
        t.assign(2 * id.size(), m.e());
        for (int v = 1; v < 2 * k; ++v) {
            int base = 2 * off[v], n = off[v + 1] - off[v];
            for (int q = 0; q < n; ++q) { t[base + n + q] = val[id[off[v] + q]]; }
            for (int j = n - 1; j >= 1; --j) { t[base + j] = m.op(t[base + 2 * j], t[base + 2 * j + 1]); }}}

    int find(lng x, lng y) const {
        int p = int(lower_bound(pts.begin(), pts.end(), pair(x, y)) - pts.begin());
        assert(p < k && pts[p] == pair(x, y));
        return p;}
    int firstX(lng x) const { return int(lower_bound(pts.begin(), pts.end(), pair(x, std::numeric_limits<lng>::min())) - pts.begin()); }
    int firstY(int v, lng y) const { return int(lower_bound(ys.begin() + off[v], ys.begin() + off[v + 1], y) - ys.begin()) - off[v]; }

    const S &get(lng x, lng y) const { return t[2 * off[find(x, y) + k] + 1]; }
    S allProd() const { return k ? t[2 * off[1] + 1] : m.e(); }

    void set(lng x, lng y, const S &s) {
        int p = find(x, y);
        for (int v = p + k; v; v >>= 1) {
            auto b = id.begin() + off[v];
            int base = 2 * off[v], n = off[v + 1] - off[v];
            int j = int(lower_bound(b, b + n, p, [&](int a, int c) { return pair(pts[a].second, a) < pair(pts[c].second, c); }) - b) + n;
            t[base + j] = s;
            for (j >>= 1; j; j >>= 1) { t[base + j] = m.op(t[base + 2 * j], t[base + 2 * j + 1]); }}}
    void apply(lng x, lng y, const S &s) { set(x, y, m.op(get(x, y), s)); }

    S prod(lng xl, lng xr, lng yl, lng yr) const {
        S res = m.e();
        if (xl >= xr || yl >= yr) { return res; }
        for (int a = firstX(xl) + k, b = firstX(xr) + k; a < b; a >>= 1, b >>= 1) {
            if (a & 1) { res = m.op(res, inner(a++, yl, yr)); }
            if (b & 1) { res = m.op(res, inner(--b, yl, yr)); }}
        return res;}
    S inner(int v, lng yl, lng yr) const {
        int base = 2 * off[v], n = off[v + 1] - off[v];
        S res = m.e();
        for (int l = firstY(v, yl) + n, r = firstY(v, yr) + n; l < r; l >>= 1, r >>= 1) {
            if (l & 1) { res = m.op(res, t[base + l++]); }
            if (r & 1) { res = m.op(res, t[base + --r]); }}
        return res;}
};

// A is an acted monoid with commutative op; point i keeps its input index and starts at w[i] (e() when w is empty); rectangles are [xl, xr) x [yl, yr).
// S: O(k * log(k)), U: O(log(k)) point and O(sqrt(k)) rectangle, Q: O(log(k)) point and O(sqrt(k)) rectangle, M: O(k)
template<typename A>
struct LazyKdTree {
    using S = typename A::S;
    using F = typename A::F;
    [[no_unique_address]] A m;
    int k;
    vector<array<lng, 4>> box;
    vector<S> d;
    vector<F> lz;
    vector<int> leaf;

    explicit LazyKdTree(const vector<pair<lng, lng>> &p = {}, const vector<S> &w = {}, A mon = {}) : m(std::move(mon)), k(int(p.size())), leaf(p.size()) {
        assert(w.empty() || w.size() == p.size());
        size_t sz = 2 * std::bit_ceil(max<size_t>(p.size(), 1));
        box.resize(sz); d.assign(sz, m.e()); lz.assign(sz, m.id());
        vector<int> ord(k);
        std::iota(ord.begin(), ord.end(), 0);
        if (k) { build(1, 0, k, false, ord, p, w); }}
    void build(int v, int a, int b, bool vertical, vector<int> &ord, const vector<pair<lng, lng>> &p, const vector<S> &w) {
        box[v] = {p[ord[a]].first, p[ord[a]].first, p[ord[a]].second, p[ord[a]].second};
        for (int i = a + 1; i < b; ++i) {
            auto [x, y] = p[ord[i]];
            box[v] = {min(box[v][0], x), max(box[v][1], x), min(box[v][2], y), max(box[v][3], y)};}
        if (b - a == 1) { leaf[ord[a]] = v; d[v] = w.empty() ? m.e() : w[ord[a]]; return; }
        int mid = std::midpoint(a, b);
        std::nth_element(ord.begin() + a, ord.begin() + mid, ord.begin() + b, [&](int i, int j) { return vertical ? p[i].second < p[j].second : p[i].first < p[j].first; });
        build(2 * v, a, mid, !vertical, ord, p, w); build(2 * v + 1, mid, b, !vertical, ord, p, w);
        pull(v);}

    void pull(int v) { d[v] = m.op(d[2 * v], d[2 * v + 1]); }
    void put(int v, const F &f) { d[v] = m.mapping(f, d[v]); lz[v] = m.composition(f, lz[v]); }
    void push(int v) { put(2 * v, lz[v]); put(2 * v + 1, lz[v]); lz[v] = m.id(); }
    void pushPath(int v) { for (int h = int(std::bit_width(uint(v))) - 1; h >= 1; --h) { push(v >> h); }}
    // 0 when box v misses the rectangle, 2 when it lies inside, 1 otherwise.
    int cover(int v, lng xl, lng xr, lng yl, lng yr) const {
        auto &[x0, x1, y0, y1] = box[v];
        if (x1 < xl || xr <= x0 || y1 < yl || yr <= y0) { return 0; }
        return xl <= x0 && x1 < xr && yl <= y0 && y1 < yr ? 2 : 1;}

    S get(int i) { assert(0 <= i && i < k); pushPath(leaf[i]); return d[leaf[i]]; }
    const S &allProd() const { return d[1]; }

    void set(int i, const S &x) {
        assert(0 <= i && i < k);
        int v = leaf[i];
        pushPath(v); d[v] = x;
        while (v >>= 1) { pull(v); }}
    void apply(lng xl, lng xr, lng yl, lng yr, const F &f) { if (k) { apply(1, xl, xr, yl, yr, f); }}
    void apply(int v, lng xl, lng xr, lng yl, lng yr, const F &f) {
        int c = cover(v, xl, xr, yl, yr);
        if (c == 2) { put(v, f); }
        if (c != 1) { return; }
        push(v);
        apply(2 * v, xl, xr, yl, yr, f); apply(2 * v + 1, xl, xr, yl, yr, f);
        pull(v);}

    S prod(lng xl, lng xr, lng yl, lng yr) { return k ? prod(1, xl, xr, yl, yr) : m.e(); }
    S prod(int v, lng xl, lng xr, lng yl, lng yr) {
        int c = cover(v, xl, xr, yl, yr);
        if (c != 1) { return c ? d[v] : m.e(); }
        push(v);
        return m.op(prod(2 * v, xl, xr, yl, yr), prod(2 * v + 1, xl, xr, yl, yr));}
};
