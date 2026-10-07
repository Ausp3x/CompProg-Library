#pragma once

#include "../01-Core/01-template.hpp"

// 0 <= n <= INT_MAX; T is an additive commutative group; lowerBound/upperBound need nonnegative values and return n when absent.
// S: O(n), U: O(log(n)), Q: O(log(n)), M: O(n)
template<typename T>
struct Fenwick {
    int n;
    vector<T> v;

    explicit Fenwick(int N = 0) : n(N) { assert(n >= 0); v.assign(n, T(0)); }
    explicit Fenwick(vector<T> a) : n(0), v(std::move(a)) {
        assert(v.size() <= size_t(INT_MAX)); n = int(v.size());
        for (int i = 0; i < n; ++i) {
            int p = i | (i + 1);
            if (p < n) { v[p] += v[i]; }}}
    Fenwick(std::initializer_list<T> a) : Fenwick(vector<T>(a)) {}

    T get(int i) const { assert(0 <= i && i < n); return sum(i, i + 1); }
    // T: O(n), M: O(n).
    vector<T> values() const {
        vector<T> a = v;
        for (int i = n - 1; i >= 0; --i) {
            int p = i | (i + 1);
            if (p < n) { a[p] -= a[i]; }}
        return a;}

    void add(int i, T x) {
        assert(0 <= i && i < n);
        for (; i < n; i |= i + 1) { v[i] += x; }}
    void set(int i, T x) { add(i, x - get(i)); }

    T prefixSum(int r) const {
        assert(0 <= r && r <= n);
        T res = T(0);
        for (; r > 0; r &= r - 1) { res += v[r - 1]; }
        return res;}
    T sum(int l, int r) const {
        assert(0 <= l && l <= r && r <= n);
        return prefixSum(r) - prefixSum(l);}
    template<typename G>
    int maxRight(int l, G pred) const {
        assert(0 <= l && l <= n && pred(T(0)));
        T pl = prefixSum(l), acc = T(0);
        int i = 0;
        for (int d = int(std::bit_floor(uint(n))); d > 0; d >>= 1) {
            if (d <= n - i) {
                T next = acc + v[i + d - 1];
                if (i + d <= l || pred(next - pl)) { acc = next; i += d; }}}
        return i;}
    template<typename G>
    int minLeft(int r, G pred) const {
        assert(0 <= r && r <= n && pred(T(0)));
        T pr = prefixSum(r), acc = T(0);
        if (pred(pr)) { return 0; }
        int i = 0;
        for (int d = int(std::bit_floor(uint(n))); d > 0; d >>= 1) {
            if (d < r - i) {
                T next = acc + v[i + d - 1];
                if (!pred(pr - next)) { acc = next; i += d; }}}
        return i + 1;}
    int lowerBound(T x) const { return x <= T(0) ? 0 : descend(std::move(x), std::less<T>{}); }
    int upperBound(T x) const { return x < T(0) ? 0 : descend(std::move(x), std::less_equal<T>{}); }
    template<typename C>
    int descend(T x, C below) const {
        int i = 0;
        for (int d = int(std::bit_floor(uint(n))); d > 0; d >>= 1) {
            if (d <= n - i && below(v[i + d - 1], x)) { x -= v[i + d - 1]; i += d; }}
        return i;}

    void add1(int i, T x) { assert(1 <= i && i <= n); add(i - 1, x); }
    T prefixSum1(int r) const { return prefixSum(r); }
    T sum1(int l, int r) const { assert(1 <= l && l - 1 <= r && r <= n); return sum(l - 1, r); }
    int lowerBound1(T x) const { int i = lowerBound(x); return i == n ? 0 : i + 1; }
};
