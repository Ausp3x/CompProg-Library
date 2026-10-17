#pragma once

#include "../01-Core/01-template.hpp"
#include "00-monoids.hpp"

// 0 <= n <= 2^29; A is an acted monoid (op, e, mapping, composition = new after old, id); search predicates hold on e and stay false as the range grows.
// S: O(n + 1), U: O(log(n + 1)), Q: O(log(n + 1)), M: O(n + 1)
template<typename A>
struct LazySegmentTree {
    using S = typename A::S;
    using F = typename A::F;
    static constexpr int MAX_SIZE = 1 << 29;
    [[no_unique_address]] A m;
    int n, lg, base;
    vector<S> d;
    vector<F> lz;

    static int checkedSize(size_t n) { assert(n <= MAX_SIZE); return int(n); }
    explicit LazySegmentTree(int N = 0, A mon = {}) : m(std::move(mon)), n(N) { assert(0 <= n && n <= MAX_SIZE); init(); }
    explicit LazySegmentTree(const vector<S> &v, A mon = {}) : m(std::move(mon)), n(checkedSize(v.size())) {
        init();
        std::copy(v.begin(), v.end(), d.begin() + base);
        for (int k = base - 1; k; --k) { pull(k); }}
    void init() {
        for (lg = 0; (1 << lg) < n; ++lg) {}
        base = 1 << lg;
        d.assign(2 * base, m.e());
        lz.assign(base, m.id());}

    void pull(int k) { d[k] = m.op(d[2 * k], d[2 * k + 1]); }
    void put(int k, const F &f) {
        d[k] = m.mapping(f, d[k]);
        if (k < base) { lz[k] = m.composition(f, lz[k]); }}
    void push(int k) { put(2 * k, lz[k]); put(2 * k + 1, lz[k]); lz[k] = m.id(); }
    void pushPath(int p) { for (int i = lg; i >= 1; --i) { push(p >> i); }}
    void pullPath(int p) { for (int i = 1; i <= lg; ++i) { pull(p >> i); }}

    S get(int p) { assert(0 <= p && p < n); p += base; pushPath(p); return d[p]; }
    const S &allProd() const { return d[1]; }
    // T: O(n), M: O(n).
    vector<S> values() {
        for (int k = 1; k < base; ++k) { push(k); }
        return vector<S>(d.begin() + base, d.begin() + base + n);}

    void set(int p, const S &x) { assert(0 <= p && p < n); p += base; pushPath(p); d[p] = x; pullPath(p); }
    void apply(int p, const F &f) { assert(0 <= p && p < n); p += base; pushPath(p); d[p] = m.mapping(f, d[p]); pullPath(p); }
    void apply(int l, int r, const F &f) {
        assert(0 <= l && l <= r && r <= n);
        if (l == r) { return; }
        l += base; r += base;
        for (int i = lg; i >= 1; --i) {
            if (((l >> i) << i) != l) { push(l >> i); }
            if (((r >> i) << i) != r) { push((r - 1) >> i); }}
        for (int a = l, b = r; a < b; a >>= 1, b >>= 1) {
            if (a & 1) { put(a++, f); }
            if (b & 1) { put(--b, f); }}
        for (int i = 1; i <= lg; ++i) {
            if (((l >> i) << i) != l) { pull(l >> i); }
            if (((r >> i) << i) != r) { pull((r - 1) >> i); }}}

    S prod(int l, int r) {
        assert(0 <= l && l <= r && r <= n);
        if (l == r) { return m.e(); }
        l += base; r += base;
        for (int i = lg; i >= 1; --i) {
            if (((l >> i) << i) != l) { push(l >> i); }
            if (((r >> i) << i) != r) { push((r - 1) >> i); }}
        S left = m.e(), right = m.e();
        for (; l < r; l >>= 1, r >>= 1) {
            if (l & 1) { left = m.op(left, d[l++]); }
            if (r & 1) { right = m.op(d[--r], right); }}
        return m.op(left, right);}
    template<typename G>
    int maxRight(int l, G pred) {
        assert(0 <= l && l <= n && pred(m.e()));
        if (l == n) { return n; }
        l += base; pushPath(l);
        S acc = m.e();
        do {
            while (!(l & 1)) { l >>= 1; }
            if (!pred(m.op(acc, d[l]))) {
                while (l < base) {
                    push(l); l *= 2;
                    if (S next = m.op(acc, d[l]); pred(next)) { acc = std::move(next); ++l; }}
                return l - base;}
            acc = m.op(acc, d[l++]);} while ((l & -l) != l);
        return n;}
    template<typename G>
    int minLeft(int r, G pred) {
        assert(0 <= r && r <= n && pred(m.e()));
        if (r == 0) { return 0; }
        r += base; pushPath(r - 1);
        S acc = m.e();
        do {
            --r;
            while (r > 1 && (r & 1)) { r >>= 1; }
            if (!pred(m.op(d[r], acc))) {
                while (r < base) {
                    push(r); r = 2 * r + 1;
                    if (S next = m.op(d[r], acc); pred(next)) { acc = std::move(next); --r; }}
                return r + 1 - base;}
            acc = m.op(d[r], acc);} while ((r & -r) != r);
        return 0;}
};

// 0 <= n <= 2^29; A supplies F, composition (new after old) and id; get returns the composed action at p.
// S: O(n + 1), U: O(log(n + 1)), Q: O(log(n + 1)), M: O(n + 1)
template<typename A>
struct DualSegmentTree {
    using F = typename A::F;
    static constexpr int MAX_SIZE = 1 << 29;
    [[no_unique_address]] A m;
    int n, lg, base;
    vector<F> lz;

    static int checkedSize(size_t n) { assert(n <= MAX_SIZE); return int(n); }
    explicit DualSegmentTree(int N = 0, A mon = {}) : m(std::move(mon)), n(N) { assert(0 <= n && n <= MAX_SIZE); init(); }
    explicit DualSegmentTree(const vector<F> &v, A mon = {}) : m(std::move(mon)), n(checkedSize(v.size())) { init(); std::copy(v.begin(), v.end(), lz.begin() + base); }
    void init() {
        for (lg = 0; (1 << lg) < n; ++lg) {}
        base = 1 << lg;
        lz.assign(2 * base, m.id());}

    void push(int k) {
        lz[2 * k] = m.composition(lz[k], lz[2 * k]);
        lz[2 * k + 1] = m.composition(lz[k], lz[2 * k + 1]);
        lz[k] = m.id();}

    F get(int p) const {
        assert(0 <= p && p < n);
        F res = lz[p += base];
        while (p >>= 1) { res = m.composition(lz[p], res); }
        return res;}
    // T: O(n), M: O(n).
    vector<F> values() {
        for (int k = 1; k < base; ++k) { push(k); }
        return vector<F>(lz.begin() + base, lz.begin() + base + n);}

    void set(int p, const F &f) {
        assert(0 <= p && p < n);
        p += base;
        for (int i = lg; i >= 1; --i) { push(p >> i); }
        lz[p] = f;}
    void apply(int l, int r, const F &f) {
        assert(0 <= l && l <= r && r <= n);
        if (l == r) { return; }
        l += base; r += base;
        for (int i = lg; i >= 1; --i) {
            if (((l >> i) << i) != l) { push(l >> i); }
            if (((r >> i) << i) != r) { push((r - 1) >> i); }}
        for (; l < r; l >>= 1, r >>= 1) {
            if (l & 1) { lz[l] = m.composition(f, lz[l]); ++l; }
            if (r & 1) { --r; lz[r] = m.composition(f, lz[r]); }}}
};

// 0 <= n <= 2^29; A supplies F, a commutative composition and id; get returns the composed action at p.
// S: O(n + 1), U: O(log(n + 1)), Q: O(log(n + 1)), M: O(n + 1)
template<typename A>
struct CommutativeDualSegmentTree {
    using F = typename A::F;
    static constexpr int MAX_SIZE = 1 << 29;
    [[no_unique_address]] A m;
    int n;
    vector<F> lz;

    static int checkedSize(size_t n) { assert(n <= MAX_SIZE); return int(n); }
    explicit CommutativeDualSegmentTree(int N = 0, A mon = {}) : m(std::move(mon)), n(N) { assert(0 <= n && n <= MAX_SIZE); lz.assign(2 * n, m.id()); }
    explicit CommutativeDualSegmentTree(const vector<F> &v, A mon = {}) : m(std::move(mon)), n(checkedSize(v.size())) {
        lz.assign(2 * n, m.id());
        std::copy(v.begin(), v.end(), lz.begin() + n);}

    F get(int p) const {
        assert(0 <= p && p < n);
        F res = lz[p += n];
        while (p >>= 1) { res = m.composition(lz[p], res); }
        return res;}
    // T: O(n), M: O(n).
    vector<F> values() const {
        vector<F> c = lz;
        for (int k = 1; k < n; ++k) { c[2 * k] = m.composition(c[k], c[2 * k]); c[2 * k + 1] = m.composition(c[k], c[2 * k + 1]); }
        return vector<F>(c.begin() + n, c.end());}

    void apply(int l, int r, const F &f) {
        assert(0 <= l && l <= r && r <= n);
        for (l += n, r += n; l < r; l >>= 1, r >>= 1) {
            if (l & 1) { lz[l] = m.composition(f, lz[l]); ++l; }
            if (r & 1) { --r; lz[r] = m.composition(f, lz[r]); }}}
};
