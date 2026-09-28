#pragma once

#include "../01-Core/01-template.hpp"

// Associative f with two-sided identity id; operand order is preserved.
// 0 <= n <= 2^29; zero-based points and half-open ranges, including empty ones.
// Predicates are deterministic, true on id, and stay false as the range grows.
// Costs count calls/copies of T and f as O(1); their arithmetic must be valid.
// S: O(n + 1), U: O(log(n + 1)), Q: O(log(n + 1)), M: O(n + 1)
template<typename T, typename F>
struct SegmentTree {
    static constexpr int MAX_SIZE = 1 << 29;
    int n, base;
    T id;
    F f;
    vector<T> tree;

    static int checkedSize(size_t n) { assert(n <= MAX_SIZE); return int(n); }
    SegmentTree(int n, T id, F f) : n(n), base(1), id(std::move(id)), f(std::move(f)) {
        assert(0 <= n && n <= MAX_SIZE);
        while (base < n) { base *= 2; }
        tree.assign(2 * base, this->id);}
    SegmentTree(const vector<T> &a, T id, F f) : SegmentTree(checkedSize(a.size()), std::move(id), std::move(f)) {
        std::copy(a.begin(), a.end(), tree.begin() + base);
        for (int i = base - 1; i; --i) { tree[i] = this->f(tree[2 * i], tree[2 * i + 1]); }}

    // Q: O(1). Returned references remain owned by this tree.
    const T &get(int p) const { assert(0 <= p && p < n); return tree[base + p]; }
    const T &allQuery() const { return tree[1]; }
    void set(int p, const T &x) {
        assert(0 <= p && p < n);
        p += base; tree[p] = x;
        while (p >>= 1) { tree[p] = f(tree[2 * p], tree[2 * p + 1]); }}
    T query(int l, int r) const {
        assert(0 <= l && l <= r && r <= n);
        T left = id, right = id;
        for (l += base, r += base; l < r; l >>= 1, r >>= 1) {
            if (l & 1) { left = f(left, tree[l++]); }
            if (r & 1) { right = f(tree[--r], right); }}
        return f(left, right);}

    // Largest r in [l, n] with pred(query(l, r)); n means no failure.
    template<typename G>
    int maxRight(int l, G pred) const {
        assert(0 <= l && l <= n && pred(id));
        if (l == n) { return n; }
        T acc = id; l += base;
        do {
            while (!(l & 1)) { l >>= 1; }
            T next = f(acc, tree[l]);
            if (!pred(next)) {
                while (l < base) {
                    l *= 2; next = f(acc, tree[l]);
                    if (pred(next)) { acc = std::move(next); ++l; }}
                return l - base;}
            acc = std::move(next); ++l;
        } while ((l & -l) != l);
        return n;}
    // Smallest l in [0, r] with pred(query(l, r)); 0 means no failure.
    template<typename G>
    int minLeft(int r, G pred) const {
        assert(0 <= r && r <= n && pred(id));
        if (r == 0) { return 0; }
        T acc = id; r += base;
        do {
            --r;
            while (r > 1 && (r & 1)) { r >>= 1; }
            T next = f(tree[r], acc);
            if (!pred(next)) {
                while (r < base) {
                    r = 2 * r + 1; next = f(tree[r], acc);
                    if (pred(next)) { acc = std::move(next); --r; }}
                return r + 1 - base;}
            acc = std::move(next);
        } while ((r & -r) != r);
        return 0;}
};
