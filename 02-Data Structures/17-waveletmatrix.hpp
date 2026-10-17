#pragma once

#include "../01-Core/01-template.hpp"

// T is totally ordered by <, W() is zero of an additive group, 0 <= n < 2^31; sums need weights; kth/kthMax are 0-based; select gives -1 and maxLeq/minGeq false when absent.
// S: O(n * log(n)), U: NA, Q: O(log(k)) for k distinct values, M: O(n * log(k))
template<typename T, typename W = lng>
struct WaveletMatrix {
    int n, h;
    vector<T> vals;
    vector<int> mid, pos;
    vector<vector<pair<ulng, int>>> bits;
    vector<vector<W>> sum;

    explicit WaveletMatrix(const vector<T> &a = {}, const vector<W> &w = {}) : n(int(a.size())), vals(a) {
        assert(a.size() < (1u << 31) && (w.empty() || w.size() == a.size()));
        sort(vals.begin(), vals.end());
        vals.erase(unique(vals.begin(), vals.end()), vals.end());
        int k = int(vals.size());
        h = int(std::bit_width(uint(k)));
        vector<int> cur(n), nxt(n), nidx(n);
        pos.resize(n);
        for (int i = 0; i < n; ++i) { cur[i] = lowerRank(a[i]); pos[i] = i; }
        mid.assign(h, 0);
        bits.assign(h, vector<pair<ulng, int>>(n / 64 + 1));
        if (!w.empty()) { sum.assign(h, vector<W>(n + 1)); }
        for (int d = h - 1; d >= 0; --d) {
            auto &b = bits[d];
            for (int i = 0; i < n; ++i) {
                if (cur[i] >> d & 1) { b[i >> 6].first |= ulng(1) << (i & 63); } else { ++mid[d]; }}
            for (int j = 1; j < int(b.size()); ++j) { b[j].second = b[j - 1].second + std::popcount(b[j - 1].first); }
            for (int i = 0, p0 = 0, p1 = mid[d]; i < n; ++i) {
                int t = cur[i] >> d & 1 ? p1++ : p0++;
                nxt[t] = cur[i]; nidx[t] = pos[i];}
            swap(cur, nxt); swap(pos, nidx);
            if (!w.empty()) {
                for (int i = 0; i < n; ++i) { sum[d][i + 1] = sum[d][i] + w[pos[i]]; }}}}

    bool bit(int d, int i) const { return bits[d][i >> 6].first >> (i & 63) & 1; }
    int rank1(int d, int i) const { auto &[b, c] = bits[d][i >> 6]; return c + std::popcount(b & ((ulng(1) << (i & 63)) - 1)); }
    // Level d maps position i of its order to the next level's order.
    int step(int d, int i) const { return bit(d, i) ? mid[d] + rank1(d, i) : i - rank1(d, i); }
    int lowerRank(const T &x) const { return int(lower_bound(vals.begin(), vals.end(), x) - vals.begin()); }
    int upperRank(const T &x) const { return int(upper_bound(vals.begin(), vals.end(), x) - vals.begin()); }
    bool present(int p, const T &x) const { return p < int(vals.size()) && !(x < vals[p]); }
    // Counts ranks below p in [l, r) and adds their weights to *s; [l, r) ends as the run of rank p.
    int descend(int &l, int &r, int p, W *s = nullptr) const {
        int res = 0;
        for (int d = h - 1; d >= 0; --d) {
            int l1 = rank1(d, l), r1 = rank1(d, r);
            if (p >> d & 1) {
                res += (r - l) - (r1 - l1);
                if (s) { *s += sum[d][r - r1] - sum[d][l - l1]; }
                l = mid[d] + l1; r = mid[d] + r1;}
            else { l -= l1; r -= r1; }}
        return res;}

    const T &access(int i) const {
        assert(0 <= i && i < n);
        int p = 0;
        for (int d = h - 1; d >= 0; --d) {
            if (bit(d, i)) { p |= 1 << d; }
            i = step(d, i);}
        return vals[p];}
    int rank(int r, const T &x) const { return countEqual(0, r, x); }
    int select(const T &x, int k) const {
        assert(k >= 0);
        int p = lowerRank(x), l = 0, r = n;
        if (!present(p, x)) { return -1; }
        descend(l, r, p);
        return k < r - l ? pos[l + k] : -1;}

    int countLess(int l, int r, const T &x) const { assert(0 <= l && l <= r && r <= n); return descend(l, r, lowerRank(x)); }
    int countRange(int l, int r, const T &lo, const T &hi) const { assert(0 <= l && l <= r && r <= n); return lo < hi ? countLess(l, r, hi) - countLess(l, r, lo) : 0; }
    int countEqual(int l, int r, const T &x) const {
        assert(0 <= l && l <= r && r <= n);
        int p = lowerRank(x);
        if (!present(p, x)) { return 0; }
        descend(l, r, p);
        return r - l;}
    const T &kth(int l, int r, int k) const {
        assert(0 <= l && l <= r && r <= n && 0 <= k && k < r - l);
        int p = 0;
        for (int d = h - 1; d >= 0; --d) {
            int l1 = rank1(d, l), r1 = rank1(d, r), z = (r - l) - (r1 - l1);
            if (k < z) { l -= l1; r -= r1; }
            else { k -= z; p |= 1 << d; l = mid[d] + l1; r = mid[d] + r1; }}
        return vals[p];}
    const T &kthMax(int l, int r, int k) const { assert(0 <= l && l <= r && r <= n && 0 <= k && k < r - l); return kth(l, r, r - l - 1 - k); }
    bool maxLeq(int l, int r, const T &x, T &res) const {
        assert(0 <= l && l <= r && r <= n);
        int a = l, b = r, c = descend(a, b, upperRank(x));
        if (!c) { return false; }
        res = kth(l, r, c - 1);
        return true;}
    bool minGeq(int l, int r, const T &x, T &res) const {
        assert(0 <= l && l <= r && r <= n);
        int a = l, b = r, c = descend(a, b, lowerRank(x));
        if (c == r - l) { return false; }
        res = kth(l, r, c);
        return true;}
    W sumLess(int l, int r, const T &x) const {
        assert(0 <= l && l <= r && r <= n && (n == 0 || !sum.empty()));
        W res = W();
        descend(l, r, lowerRank(x), &res);
        return res;}
    W kthSum(int l, int r, int k) const {
        assert(0 <= l && l <= r && r <= n && 0 <= k && k <= r - l && (n == 0 || !sum.empty()));
        W res = W();
        for (int d = h - 1; d >= 0; --d) {
            int l1 = rank1(d, l), r1 = rank1(d, r), z = (r - l) - (r1 - l1);
            if (k < z) { l -= l1; r -= r1; }
            else { res += sum[d][r - r1] - sum[d][l - l1]; k -= z; l = mid[d] + l1; r = mid[d] + r1; }}
        if (h) { res += sum[0][l + k] - sum[0][l]; }
        return res;}
};

// T is totally ordered by <; points (x[i], y[i]) weigh w[i] (W() zero of an additive group; rectangleSum needs weights); rectangles [xl, xr) x [yl, yr) are empty when a side is.
// S: O(n * log(n)), U: NA, Q: O(log(n)), M: O(n * log(n))
template<typename T, typename W = lng>
struct RectangleWaveletMatrix {
    vector<T> xs;
    WaveletMatrix<T, W> wm;

    RectangleWaveletMatrix(const vector<T> &x, const vector<T> &y, const vector<W> &w = {}) {
        assert(x.size() == y.size() && (w.empty() || w.size() == x.size()));
        int n = int(x.size());
        vector<int> ord(n);
        std::iota(ord.begin(), ord.end(), 0);
        std::stable_sort(ord.begin(), ord.end(), [&](int i, int j) { return x[i] < x[j]; });
        vector<T> ys;
        vector<W> ws;
        for (int i : ord) {
            xs.push_back(x[i]); ys.push_back(y[i]);
            if (!w.empty()) { ws.push_back(w[i]); }}
        wm = WaveletMatrix<T, W>(ys, ws);}

    pair<int, int> span(const T &xl, const T &xr) const {
        int l = int(lower_bound(xs.begin(), xs.end(), xl) - xs.begin());
        return {l, max(l, int(lower_bound(xs.begin(), xs.end(), xr) - xs.begin()))};}
    int rectangleCount(const T &xl, const T &xr, const T &yl, const T &yr) const { auto [l, r] = span(xl, xr); return wm.countRange(l, r, yl, yr); }
    W rectangleSum(const T &xl, const T &xr, const T &yl, const T &yr) const {
        assert(wm.n == 0 || !wm.sum.empty());
        auto [l, r] = span(xl, xr);
        return yl < yr ? wm.sumLess(l, r, yr) - wm.sumLess(l, r, yl) : W();}
};

// T is totally ordered by <; M is a commutative monoid (S, op, e); point i is (x[i], y[i]) holding v[i] (e() when v is empty); rectangles [xl, xr) x [yl, yr) fold to e() when empty.
// S: O(n * log(n)), U: O(log(n)^2), Q: O(log(n)^2), M: O(n * log(n))
template<typename T, typename M>
struct WaveletMatrixMonoid {
    using S = typename M::S;
    [[no_unique_address]] M m;
    int n;
    vector<T> xs;
    vector<int> at;
    vector<S> v;
    WaveletMatrix<T> wm;
    vector<vector<S>> seg;

    WaveletMatrixMonoid(const vector<T> &x, const vector<T> &y, const vector<S> &v0 = {}, M mon = {}) : m(std::move(mon)), n(int(x.size())), at(n), v(v0) {
        assert(x.size() == y.size() && (v0.empty() || v0.size() == x.size()));
        if (v.empty()) { v.assign(n, m.e()); }
        vector<int> ord(n);
        std::iota(ord.begin(), ord.end(), 0);
        std::stable_sort(ord.begin(), ord.end(), [&](int i, int j) { return x[i] < x[j]; });
        vector<T> ys;
        for (int k = 0; k < n; ++k) { xs.push_back(x[ord[k]]); ys.push_back(y[ord[k]]); at[ord[k]] = k; }
        wm = WaveletMatrix<T>(ys);
        seg.assign(wm.h, vector<S>(2 * n, m.e()));
        for (int i = 0; i < n; ++i) {
            for (int d = wm.h - 1, p = at[i]; d >= 0; --d) { p = wm.step(d, p); seg[d][n + p] = v[i]; }}
        for (auto &s : seg) {
            for (int k = n - 1; k > 0; --k) { s[k] = m.op(s[2 * k], s[2 * k + 1]); }}}

    const S &get(int i) const { assert(0 <= i && i < n); return v[i]; }

    void set(int i, const S &s) {
        assert(0 <= i && i < n);
        v[i] = s;
        for (int d = wm.h - 1, p = at[i]; d >= 0; --d) {
            p = wm.step(d, p);
            auto &t = seg[d];
            int k = n + p;
            t[k] = s;
            while (k >>= 1) { t[k] = m.op(t[2 * k], t[2 * k + 1]); }}}
    void apply(int i, const S &s) { assert(0 <= i && i < n); set(i, m.op(v[i], s)); }

    S fold(int d, int l, int r) const {
        S res = m.e();
        for (l += n, r += n; l < r; l >>= 1, r >>= 1) {
            if (l & 1) { res = m.op(res, seg[d][l++]); }
            if (r & 1) { res = m.op(res, seg[d][--r]); }}
        return res;}
    S prod(const T &xl, const T &xr, const T &yl, const T &yr) const {
        int l = int(lower_bound(xs.begin(), xs.end(), xl) - xs.begin()), r = max(l, int(lower_bound(xs.begin(), xs.end(), xr) - xs.begin()));
        int a = wm.lowerRank(yl), b = max(a, wm.lowerRank(yr)), d = wm.h - 1;
        S res = m.e();
        if (a == b) { return res; }
        // Follow the shared high bits of a and b; below their first difference a's path folds ranks >= a and b's path ranks < b.
        for (; (a >> d & 1) == (b >> d & 1); --d) {
            int l1 = wm.rank1(d, l), r1 = wm.rank1(d, r);
            if (a >> d & 1) { l = wm.mid[d] + l1; r = wm.mid[d] + r1; } else { l -= l1; r -= r1; }}
        int l1 = wm.rank1(d, l), r1 = wm.rank1(d, r), la = l - l1, ra = r - r1, lb = wm.mid[d] + l1, rb = wm.mid[d] + r1;
        while (--d >= 0) {
            l1 = wm.rank1(d, la); r1 = wm.rank1(d, ra);
            if (a >> d & 1) { la = wm.mid[d] + l1; ra = wm.mid[d] + r1; }
            else { res = m.op(res, fold(d, wm.mid[d] + l1, wm.mid[d] + r1)); la -= l1; ra -= r1; }
            l1 = wm.rank1(d, lb); r1 = wm.rank1(d, rb);
            if (b >> d & 1) { res = m.op(res, fold(d, lb - l1, rb - r1)); lb = wm.mid[d] + l1; rb = wm.mid[d] + r1; }
            else { lb -= l1; rb -= r1; }}
        return m.op(res, fold(0, la, ra));}
};
