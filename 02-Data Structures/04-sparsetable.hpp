#pragma once

#include "../01-Core/01-template.hpp"

// Immutable sequence, associative f. query additionally requires idempotence.
// 0 <= n <= INT_MAX. Canonical query/fold use nonempty half-open [l, r).
// Empty construction is valid; no identity is assumed. No default T is needed.
// Costs count calls/copies of T and f as O(1); their arithmetic must be valid.
// S: O(n * log(n + 1)), U: NA, Q: O(1), M: O(n * log(n + 1))
template<typename T, typename F>
struct SparseTable {
    int n, h;
    vector<vector<T>> v;
    F f;

    SparseTable(vector<T> a, const F &fnc) : n(0), h(0), f(fnc) {
        assert(a.size() <= INT_MAX);
        n = int(a.size()); h = std::bit_width(uint(n));
        if (!n) { return; }
        v.reserve(h); v.push_back(std::move(a));
        for (int i = 1; i < h; ++i) {
            int len = 1 << i;
            vector<T> level; level.reserve(n - len + 1);
            for (int j = 0; j <= n - len; ++j) {
                level.push_back(this->f(v[i - 1][j], v[i - 1][j + len / 2])); }
            v.push_back(std::move(level)); }}

    T query(int l, int r) const {
        assert(0 <= l && l < r && r <= n);
        int i = std::bit_width(uint(r - l)) - 1;
        return f(v[i][l], v[i][r - (1 << i)]);}
    // Q: O(log(n + 1)); associative, possibly nonidempotent/noncommutative f.
    T fold(int l, int r) const {
        assert(0 <= l && l < r && r <= n);
        int i = std::bit_width(uint(r - l)) - 1;
        T acc = v[i][l]; l += 1 << i;
        while (l < r) {
            i = std::bit_width(uint(r - l)) - 1;
            acc = f(acc, v[i][l]); l += 1 << i;}
        return acc;}

    // Legacy inclusive [l, r] adapters. queryFast also requires idempotence.
    T queryFast(int l, int r) const {
        assert(0 <= l && l <= r && r < n); return query(l, r + 1);}
    // Q: O(log(n + 1)).
    T querySlow(int l, int r) const {
        assert(0 <= l && l <= r && r < n); return fold(l, r + 1);}
    // T: O(n * log(n + 1)); legacy diagnostic format with inclusive blocks.
    friend ostream &operator<<(ostream &os, const SparseTable &a) {
        if (!a.n) { return os << "[]"; }
        os << "\n[\n";
        for (int i = a.h - 1; i >= 0; --i) {
            os << "  [";
            int len = 1 << i;
            for (int j = 0; j <= a.n - len; ++j) {
                os << "[" << j << ", " << j + len - 1 << "]: " << a.v[i][j]
                   << (j < a.n - len ? ", " : ""); }
            os << "]" << (i ? ",\n" : "\n"); }
        return os << "]\n";}
};
