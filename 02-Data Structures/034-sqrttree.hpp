#pragma once

#include "../01-Core/01-template.hpp"

// n < 2^30; M is a monoid (S, op, e), op associative, stored as m; prod(l, r) returns e() on l == r.
// S: O(n * log(log(n))), U: O(sqrt(n)), Q: O(1), M: O(n * log(log(n)))
template<typename M>
struct SqrtTree {
    using S = typename M::S;
    [[no_unique_address]] M m;
    int n, lg, nb, cut;
    vector<int> lay, on;
    vector<S> v;
    vector<vector<S>> pre, suf, btw;

    explicit SqrtTree(const vector<S> &a = {}, M mon = {}) : m(std::move(mon)), n(int(a.size())), lg(n > 1 ? int(std::bit_width(uint(n - 1))) : 0), v(a) {
        assert(a.size() < size_t(1 << 30));
        for (int t = lg; t > 3; t = (t + 1) / 2) { lay.push_back(t); }
        cut = lay.empty() ? lg : (lay.back() + 1) / 2;
        on.assign(lg + 1, 0);
        for (int i = 0; i < int(lay.size()); ++i) { on[lay[i]] = i; }
        for (int b = lg - 1; b >= 0; --b) { on[b] = max(on[b], on[b + 1]); }
        int bs = (lg + 1) / 2;
        nb = lay.empty() ? 0 : (n + (1 << bs) - 1) >> bs;
        v.resize(n + nb, m.e());
        pre.assign(lay.size(), vector<S>(n + nb, m.e()));
        suf = pre;
        btw.assign(max(int(lay.size()) - 1, 0), vector<S>((1 << lg) + (1 << bs), m.e()));
        build(0, 0, n, 0);}
    // Layer i splits [lb, rb) into blocks of 2^bs; the index (block totals of layer 0) is a tree at base n from layer 1.
    void build(int i, int lb, int rb, int off) {
        if (i == int(lay.size())) { return; }
        int bs = (lay[i] + 1) / 2;
        for (int l = lb; l < rb; l += 1 << bs) {
            block(i, l, min(l + (1 << bs), rb));
            build(i + 1, l, min(l + (1 << bs), rb), off);}
        if (i) { between(i, lb, rb, off); return; }
        for (int b = 0; b < nb; ++b) { v[n + b] = suf[0][b << bs]; }
        build(1, n, n + nb, (1 << lg) - n);}
    void block(int i, int l, int r) {
        pre[i][l] = v[l];
        for (int j = l + 1; j < r; ++j) { pre[i][j] = m.op(pre[i][j - 1], v[j]); }
        suf[i][r - 1] = v[r - 1];
        for (int j = r - 2; j >= l; --j) { suf[i][j] = m.op(v[j], suf[i][j + 1]); }}
    void between(int i, int lb, int rb, int off) {
        int bs = (lay[i] + 1) / 2, bc = lay[i] / 2, k = (rb - lb + (1 << bs) - 1) >> bs;
        for (int a = 0; a < k; ++a) {
            int o = off + lb + (a << bc);
            btw[i - 1][o + a] = suf[i][lb + (a << bs)];
            for (int b = a + 1; b < k; ++b) { btw[i - 1][o + b] = m.op(btw[i - 1][o + b - 1], suf[i][lb + (b << bs)]); }}}

    void set(int p, const S &x) {
        assert(0 <= p && p < n);
        v[p] = x;
        update(0, 0, n, 0, p);}
    void update(int i, int lb, int rb, int off, int p) {
        if (i == int(lay.size())) { return; }
        int bs = (lay[i] + 1) / 2, l = lb + ((p - lb) >> bs << bs), r = min(l + (1 << bs), rb);
        block(i, l, r);
        if (i) { between(i, lb, rb, off); }
        else { v[n + (p >> bs)] = suf[0][l]; update(1, n, n + nb, (1 << lg) - n, n + (p >> bs)); }
        update(i + 1, l, r, off, p);}

    S prod(int l, int r) const {
        assert(0 <= l && l <= r && r <= n);
        return l == r ? m.e() : inner(l, r - 1, 0, 0);}
    // Closed [l, r] of the tree at base: a direct fold below 2^cut elements, else suffix, blocks between, prefix.
    S inner(int l, int r, int off, int base) const {
        int d = int(std::bit_width(uint((l - base) ^ (r - base))));
        if (d <= cut) {
            S res = v[l];
            for (int j = l + 1; j <= r; ++j) { res = m.op(res, v[j]); }
            return res;}
        int i = on[d], bs = (lay[i] + 1) / 2, bc = lay[i] / 2;
        int lb = (((l - base) >> lay[i]) << lay[i]) + base, a = ((l - lb) >> bs) + 1, b = ((r - lb) >> bs) - 1;
        S res = suf[i][l];
        if (a <= b) { res = m.op(res, i ? btw[i - 1][off + lb + (a << bc) + b] : inner(n + a, n + b, (1 << lg) - n, n)); }
        return m.op(res, pre[i][r]);}
};
