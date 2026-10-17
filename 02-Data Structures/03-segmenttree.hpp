#pragma once

#include "../01-Core/01-template.hpp"

// 0 <= n <= 2^29; M is a monoid (S, op associative, e two-sided identity); search predicates are true on e and stay false as the range grows.
// S: O(n + 1), U: O(log(n + 1)), Q: O(log(n + 1)), M: O(n + 1)
template<typename M>
struct SegmentTree {
    using S = typename M::S;
    static constexpr int MAX_SIZE = 1 << 29;
    [[no_unique_address]] M m;
    int n, base;
    vector<S> tree;

    static int checkedSize(size_t n) { assert(n <= MAX_SIZE); return int(n); }
    explicit SegmentTree(int N = 0, M mon = {}) : m(std::move(mon)), n(N), base(1) {
        assert(0 <= n && n <= MAX_SIZE);
        while (base < n) { base *= 2; }
        tree.assign(2 * base, m.e());}
    explicit SegmentTree(const vector<S> &a, M mon = {}) : SegmentTree(checkedSize(a.size()), std::move(mon)) {
        std::copy(a.begin(), a.end(), tree.begin() + base);
        for (int i = base - 1; i; --i) { tree[i] = m.op(tree[2 * i], tree[2 * i + 1]); }}
    SegmentTree(std::initializer_list<S> a, M mon = {}) : SegmentTree(vector<S>(a), std::move(mon)) {}

    // Q: O(1).
    typename vector<S>::const_reference get(int p) const { assert(0 <= p && p < n); return tree[base + p]; }
    typename vector<S>::const_reference allProd() const { return tree[1]; }
    // T: O(n), M: O(n).
    vector<S> values() const { return vector<S>(tree.begin() + base, tree.begin() + base + n); }

    void set(int p, const S &x) {
        assert(0 <= p && p < n);
        p += base; tree[p] = x;
        while (p >>= 1) { tree[p] = m.op(tree[2 * p], tree[2 * p + 1]); }}
    void apply(int p, const S &x) { assert(0 <= p && p < n); set(p, m.op(tree[base + p], x)); }

    S prod(int l, int r) const {
        assert(0 <= l && l <= r && r <= n);
        S left = m.e(), right = m.e();
        for (l += base, r += base; l < r; l >>= 1, r >>= 1) {
            if (l & 1) { left = m.op(left, tree[l++]); }
            if (r & 1) { right = m.op(tree[--r], right); }}
        return m.op(left, right);}

    template<typename G>
    int maxRight(int l, G pred) const {
        assert(0 <= l && l <= n && pred(m.e()));
        if (l == n) { return n; }
        S acc = m.e(); l += base;
        do {
            while (!(l & 1)) { l >>= 1; }
            S next = m.op(acc, tree[l]);
            if (!pred(next)) {
                while (l < base) {
                    l *= 2; next = m.op(acc, tree[l]);
                    if (pred(next)) { acc = std::move(next); ++l; }}
                return l - base;}
            acc = std::move(next); ++l;} while ((l & -l) != l);
        return n;}
    template<typename G>
    int minLeft(int r, G pred) const {
        assert(0 <= r && r <= n && pred(m.e()));
        if (r == 0) { return 0; }
        S acc = m.e(); r += base;
        do {
            --r;
            while (r > 1 && (r & 1)) { r >>= 1; }
            S next = m.op(tree[r], acc);
            if (!pred(next)) {
                while (r < base) {
                    r = 2 * r + 1; next = m.op(tree[r], acc);
                    if (pred(next)) { acc = std::move(next); --r; }}
                return r + 1 - base;}
            acc = std::move(next);} while ((r & -r) != r);
        return 0;}
};
