#pragma once

#include "../01-Core/01-template.hpp"
#include "00-monoids.hpp"

// Keys in [lo, hi) with hi - lo <= 2^62; roots are pool handles and 0 is the empty tree; unset keys hold M::e(); PERSISTENT copies update paths and forbids meld and split.
// S: O(1), U: O(log(n)), Q: O(log(n)), M: O(k * log(n)) for n = hi - lo and k set keys, O(q * log(n)) for q updates when PERSISTENT; meld is O(nodes freed) with total work O(nodes ever allocated)
template<typename M, bool PERSISTENT = false>
struct DynamicSegmentTree {
    using S = typename M::S;
    struct Node { S x; int l, r; };
    lng lo, hi;
    vector<Node> t;
    vector<int> spare;

    DynamicSegmentTree(lng lo, lng hi) : lo(lo), hi(hi) { assert(lo <= hi && ulng(hi) - ulng(lo) <= ulng(1) << 62); t.push_back({M::e(), 0, 0}); }
    void reserve(int nodes) { t.reserve(size_t(nodes) + 1); }
    int make(const S &x, int l, int r) {
        if (spare.empty()) { t.push_back({x, l, r}); return int(t.size()) - 1; }
        int k = spare.back(); spare.pop_back();
        t[k] = {x, l, r};
        return k;}
    int own(int k) { return k ? (PERSISTENT ? make(t[k].x, t[k].l, t[k].r) : k) : make(M::e(), 0, 0); }
    void pull(int k) { t[k].x = M::op(t[t[k].l].x, t[t[k].r].x); }

    int nodeCount() const { return int(t.size()) - 1 - int(spare.size()); }
    S get(int k, lng i) const {
        assert(lo <= i && i < hi);
        for (lng a = lo, b = hi; k && b - a > 1;) {
            lng m = a + (b - a) / 2;
            if (i < m) { k = t[k].l; b = m; }
            else { k = t[k].r; a = m; }}
        return t[k].x;}
    S allProd(int k) const { return t[k].x; }
    // T: O(k * log(n)) for k set keys.
    template<typename G>
    void enumerate(int k, G visit) const { walk(k, lo, hi, visit); }
    template<typename G>
    void walk(int k, lng a, lng b, G &visit) const {
        if (!k) { return; }
        if (b - a == 1) { visit(a, t[k].x); return; }
        lng m = a + (b - a) / 2;
        walk(t[k].l, a, m, visit); walk(t[k].r, m, b, visit);}

    int set(int k, lng i, const S &x) { assert(lo <= i && i < hi); return point(k, lo, hi, i, x, false); }
    int apply(int k, lng i, const S &x) { assert(lo <= i && i < hi); return point(k, lo, hi, i, x, true); }
    int point(int k, lng a, lng b, lng i, const S &x, bool combine) {
        k = own(k);
        if (b - a == 1) { t[k].x = combine ? M::op(t[k].x, x) : x; return k; }
        lng m = a + (b - a) / 2;
        if (i < m) { int c = point(t[k].l, a, m, i, x, combine); t[k].l = c; }
        else { int c = point(t[k].r, m, b, i, x, combine); t[k].r = c; }
        pull(k);
        return k;}
    int meld(int a, int b) { static_assert(!PERSISTENT); assert(!a || a != b); return melded(a, b, hi - lo); }
    int melded(int a, int b, lng len) {
        if (!a || !b) { return a ? a : b; }
        if (len == 1) { t[a].x = M::op(t[a].x, t[b].x); }
        else {
            int l = melded(t[a].l, t[b].l, len / 2), r = melded(t[a].r, t[b].r, len - len / 2);
            t[a].l = l; t[a].r = r; pull(a);}
        spare.push_back(b);
        return a;}
    pair<int, int> split(int k, lng i) { static_assert(!PERSISTENT); assert(lo <= i && i <= hi); return cut(k, lo, hi, i); }
    pair<int, int> cut(int k, lng a, lng b, lng i) {
        if (!k || i <= a) { return {0, k}; }
        if (b <= i) { return {k, 0}; }
        lng m = a + (b - a) / 2;
        int q = make(M::e(), 0, 0);
        if (i < m) { auto [x, y] = cut(t[k].l, a, m, i); t[k].l = x; t[q].l = y; t[q].r = t[k].r; t[k].r = 0; }
        else { auto [x, y] = cut(t[k].r, m, b, i); t[k].r = x; t[q].r = y; }
        return {trim(k), trim(q)};}
    int trim(int k) {
        if (t[k].l || t[k].r) { pull(k); return k; }
        spare.push_back(k);
        return 0;}

    S prod(int k, lng l, lng r) const { assert(lo <= l && l <= r && r <= hi); return fold(k, lo, hi, l, r); }
    S fold(int k, lng a, lng b, lng l, lng r) const {
        if (!k || r <= a || b <= l) { return M::e(); }
        if (l <= a && b <= r) { return t[k].x; }
        lng m = a + (b - a) / 2;
        return M::op(fold(t[k].l, a, m, l, r), fold(t[k].r, m, b, l, r));}
    template<typename G>
    lng maxRight(int k, lng l, G pred) const {
        assert(lo <= l && l <= hi && pred(M::e()));
        S acc = M::e();
        lng res = hi;
        descendRight(k, lo, hi, l, pred, acc, res);
        return res;}
    template<typename G>
    bool descendRight(int k, lng a, lng b, lng l, G &pred, S &acc, lng &res) const {
        if (!k || b <= l) { return false; }
        if (l <= a) {
            S next = M::op(acc, t[k].x);
            if (pred(next)) { acc = std::move(next); return false; }
            if (b - a == 1) { res = a; return true; }}
        lng m = a + (b - a) / 2;
        return descendRight(t[k].l, a, m, l, pred, acc, res) || descendRight(t[k].r, m, b, l, pred, acc, res);}
    template<typename G>
    lng minLeft(int k, lng r, G pred) const {
        assert(lo <= r && r <= hi && pred(M::e()));
        S acc = M::e();
        lng res = lo;
        descendLeft(k, lo, hi, r, pred, acc, res);
        return res;}
    template<typename G>
    bool descendLeft(int k, lng a, lng b, lng r, G &pred, S &acc, lng &res) const {
        if (!k || r <= a) { return false; }
        if (b <= r) {
            S next = M::op(t[k].x, acc);
            if (pred(next)) { acc = std::move(next); return false; }
            if (b - a == 1) { res = b; return true; }}
        lng m = a + (b - a) / 2;
        return descendLeft(t[k].r, m, b, r, pred, acc, res) || descendLeft(t[k].l, a, m, r, pred, acc, res);}
};

// Keys in [lo, hi) with hi - lo <= 2^62; fill(l, r) is the product of an untouched range and must split as op(fill(l, m), fill(m, r)).
// S: O(1), U: O(log(n)), Q: O(log(n)), M: O(q * log(n)) for n = hi - lo and q operations of any kind
template<typename A, typename D>
struct DynamicLazySegmentTree {
    using S = typename A::S;
    using F = typename A::F;
    struct Node { S x; F f; int l, r; };
    lng lo, hi;
    D fill;
    vector<Node> t;

    DynamicLazySegmentTree(lng lo, lng hi, D fill) : lo(lo), hi(hi), fill(std::move(fill)) {
        assert(lo <= hi && ulng(hi) - ulng(lo) <= ulng(1) << 62);
        t.push_back({A::e(), A::id(), 0, 0});
        if (lo < hi) { make(lo, hi); }}
    void reserve(int nodes) { t.reserve(size_t(nodes) + 1); }
    int make(lng a, lng b) { t.push_back({fill(a, b), A::id(), 0, 0}); return int(t.size()) - 1; }
    void put(int k, const F &f) { t[k].x = A::mapping(f, t[k].x); t[k].f = A::composition(f, t[k].f); }
    void push(int k, lng a, lng m, lng b) {
        if (!t[k].l) { int c = make(a, m); t[k].l = c; }
        if (!t[k].r) { int c = make(m, b); t[k].r = c; }
        put(t[k].l, t[k].f); put(t[k].r, t[k].f);
        t[k].f = A::id();}
    void pull(int k) { t[k].x = A::op(t[t[k].l].x, t[t[k].r].x); }

    int nodeCount() const { return int(t.size()) - 1; }
    S get(lng i) { assert(lo <= i && i < hi); return fold(1, lo, hi, i, i + 1); }
    S allProd() const { return t[lo < hi].x; }

    void set(lng i, const S &x) { assert(lo <= i && i < hi); point(1, lo, hi, i, x); }
    void point(int k, lng a, lng b, lng i, const S &x) {
        if (b - a == 1) { t[k].x = x; return; }
        lng m = a + (b - a) / 2;
        push(k, a, m, b);
        point(i < m ? t[k].l : t[k].r, i < m ? a : m, i < m ? m : b, i, x);
        pull(k);}
    void apply(lng l, lng r, const F &f) { assert(lo <= l && l <= r && r <= hi); if (l < r) { act(1, lo, hi, l, r, f); }}
    void act(int k, lng a, lng b, lng l, lng r, const F &f) {
        if (l <= a && b <= r) { put(k, f); return; }
        lng m = a + (b - a) / 2;
        push(k, a, m, b);
        if (l < m) { act(t[k].l, a, m, l, r, f); }
        if (m < r) { act(t[k].r, m, b, l, r, f); }
        pull(k);}

    S prod(lng l, lng r) { assert(lo <= l && l <= r && r <= hi); return l < r ? fold(1, lo, hi, l, r) : A::e(); }
    S fold(int k, lng a, lng b, lng l, lng r) {
        if (l <= a && b <= r) { return t[k].x; }
        lng m = a + (b - a) / 2;
        push(k, a, m, b);
        if (r <= m) { return fold(t[k].l, a, m, l, r); }
        if (m <= l) { return fold(t[k].r, m, b, l, r); }
        return A::op(fold(t[k].l, a, m, l, r), fold(t[k].r, m, b, l, r));}
    template<typename G>
    lng maxRight(lng l, G pred) {
        assert(lo <= l && l <= hi && pred(A::e()));
        S acc = A::e();
        lng res = hi;
        if (l < hi) { descendRight(1, lo, hi, l, pred, acc, res); }
        return res;}
    template<typename G>
    bool descendRight(int k, lng a, lng b, lng l, G &pred, S &acc, lng &res) {
        if (b <= l) { return false; }
        if (l <= a) {
            S next = A::op(acc, t[k].x);
            if (pred(next)) { acc = std::move(next); return false; }
            if (b - a == 1) { res = a; return true; }}
        lng m = a + (b - a) / 2;
        push(k, a, m, b);
        return descendRight(t[k].l, a, m, l, pred, acc, res) || descendRight(t[k].r, m, b, l, pred, acc, res);}
    template<typename G>
    lng minLeft(lng r, G pred) {
        assert(lo <= r && r <= hi && pred(A::e()));
        S acc = A::e();
        lng res = lo;
        if (lo < r) { descendLeft(1, lo, hi, r, pred, acc, res); }
        return res;}
    template<typename G>
    bool descendLeft(int k, lng a, lng b, lng r, G &pred, S &acc, lng &res) {
        if (r <= a) { return false; }
        if (b <= r) {
            S next = A::op(t[k].x, acc);
            if (pred(next)) { acc = std::move(next); return false; }
            if (b - a == 1) { res = b; return true; }}
        lng m = a + (b - a) / 2;
        push(k, a, m, b);
        return descendLeft(t[k].r, m, b, r, pred, acc, res) || descendLeft(t[k].l, a, m, r, pred, acc, res);}
};

// Keys in [lo, hi) with hi - lo <= 2^62; A supplies F, composition (new after old) and id; get returns the composed action at i.
// S: O(1), U: O(log(n)), Q: O(log(n)), M: O(q * log(n)) for n = hi - lo and q range applications
template<typename A>
struct DynamicDualSegmentTree {
    using F = typename A::F;
    struct Node { F f; int l, r; };
    lng lo, hi;
    vector<Node> t;

    DynamicDualSegmentTree(lng lo, lng hi) : lo(lo), hi(hi) {
        assert(lo <= hi && ulng(hi) - ulng(lo) <= ulng(1) << 62);
        t.push_back({A::id(), 0, 0});
        if (lo < hi) { t.push_back({A::id(), 0, 0}); }}
    void reserve(int nodes) { t.reserve(size_t(nodes) + 1); }
    int make() { t.push_back({A::id(), 0, 0}); return int(t.size()) - 1; }
    void push(int k) {
        if (!t[k].l) { int c = make(); t[k].l = c; }
        if (!t[k].r) { int c = make(); t[k].r = c; }
        t[t[k].l].f = A::composition(t[k].f, t[t[k].l].f);
        t[t[k].r].f = A::composition(t[k].f, t[t[k].r].f);
        t[k].f = A::id();}

    int nodeCount() const { return int(t.size()) - 1; }
    F get(lng i) const {
        assert(lo <= i && i < hi);
        F res = A::id();
        lng a = lo, b = hi;
        for (int k = 1; k;) {
            res = A::composition(res, t[k].f);
            if (b - a == 1) { break; }
            lng m = a + (b - a) / 2;
            if (i < m) { k = t[k].l; b = m; }
            else { k = t[k].r; a = m; }}
        return res;}

    void apply(lng l, lng r, const F &f) { assert(lo <= l && l <= r && r <= hi); if (l < r) { act(1, lo, hi, l, r, f); }}
    void act(int k, lng a, lng b, lng l, lng r, const F &f) {
        if (l <= a && b <= r) { t[k].f = A::composition(f, t[k].f); return; }
        push(k);
        lng m = a + (b - a) / 2;
        if (l < m) { act(t[k].l, a, m, l, r, f); }
        if (m < r) { act(t[k].r, m, b, l, r, f); }}
};
