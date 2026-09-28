#pragma once

#include "../01-Core/01-template.hpp"

// 0 <= n <= INT_MAX; zero-based add, prefix [0,r), sum [l,r).
// T supplies an additive commutative group with identity T(0); every intermediate
// addition/subtraction must fit T (or use an explicitly modular type). Public state
// must not be edited directly. lowerBound additionally requires ordered, nonnegative
// point values and exact non-overflowing sums; updates may be negative if this holds.
// S: O(n), U: O(log(n)), Q: O(log(n)), M: O(n), in T arithmetic operations.
template<typename T>
struct Fenwick {
    int n;
    vector<T> v;

    explicit Fenwick(int N = 0) : n(N) {
        assert(n >= 0); v.assign(n, T(0));
    }
    explicit Fenwick(vector<T> a) : n(0), v(std::move(a)) {
        assert(v.size() <= size_t(INT_MAX)); n = int(v.size());
        for (int i = 0; i < n; ++i) {
            int p = i | (i + 1);
            if (p < n) { v[p] += v[i]; }}
    }

    void add(int i, T x) {
        assert(0 <= i && i < n);
        for (; i < n; i |= i + 1) { v[i] += x; }
    }
    T prefixSum(int r) const {
        assert(0 <= r && r <= n);
        T res = T(0);
        for (; r > 0; r &= r - 1) { res += v[r - 1]; }
        return res;
    }
    T sum(int l, int r) const {
        assert(0 <= l && l <= r && r <= n);
        return prefixSum(r) - prefixSum(l);
    }

    // First i with prefixSum(i+1) >= x, n if none; x <= 0 returns 0.
    // Nonnegative point values are a caller precondition, not checked by scanning.
    int lowerBound(T x) const {
        if (x <= T(0)) { return 0; }
        int i = 0;
        for (int d = int(std::bit_floor(uint(n))); d > 0; d >>= 1) {
            if (d <= n - i && v[i + d - 1] < x) { x -= v[i + d - 1]; i += d; }}
        return i;
    }

    // One-based adapters: add1 uses [1,n], prefixSum1(r) sums inclusive [1,r].
    // sum1(l,r) is inclusive [l,r], permitting empty l == r+1 without computing r+1.
    // lowerBound1 returns [1,n], or 0 if none; x <= 0 returns 1 when n > 0.
    void add1(int i, T x) { assert(1 <= i && i <= n); add(i - 1, x); }
    T prefixSum1(int r) const { return prefixSum(r); }
    T sum1(int l, int r) const {
        assert(1 <= l && l - 1 <= r && r <= n); return sum(l - 1, r);
    }
    int lowerBound1(T x) const { int i = lowerBound(x); return i == n ? 0 : i + 1; }
};
