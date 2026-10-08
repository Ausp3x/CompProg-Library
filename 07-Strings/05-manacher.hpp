#pragma once
#include "../01-Core/01-template.hpp"

// S: O(n), U: NA, Q: O(1), M: O(n); n < INT_MAX, sentinel-free; odd[i] counts the center, even[g] is the radius at gap g.
// match(i, j), i < j, must be key(i) == key(j) (an involution s[j] == f(s[i]) keeps only even exact); k = -1 is maximal.
struct Manacher {
    vector<int> odd, even;

    Manacher() : even(1) {}
    template<class F> Manacher(int n, F match) {
        assert(0 <= n && n < INT_MAX);
        odd.resize(n); even.resize(n + 1);
        for (int i = 0, l = 0, r = -1; i < n; ++i) {
            int k = i > r ? 1 : min(odd[l + (r - i)], r - i + 1);
            while (k <= i && k < n - i && match(i - k, i + k)) { ++k; }
            odd[i] = k;
            if (k - 1 > r - i) { l = i - k + 1; r = i + k - 1; }}
        for (int i = 0, l = 0, r = -1; i < n; ++i) {
            int k = i > r ? 0 : min(even[l + (r - i) + 1], r - i + 1);
            while (k < i && k < n - i && match(i - k - 1, i + k)) { ++k; }
            even[i] = k;
            if (k - 1 > r - i) { l = i - k; r = i + k - 1; }}}
    template<class S> explicit Manacher(const S &s) : Manacher(int(s.size()), [&](int i, int j) { return s[i] == s[j]; }) {
        assert(s.size() < INT_MAX);}
    int size() const { return int(odd.size()); }

    pair<int, int> oddInterval(int i, int k = -1) const {
        assert(0 <= i && i < size());
        if (k == -1) { k = odd[i]; }
        assert(1 <= k && k <= odd[i]);
        return {i - k + 1, i + k};}
    pair<int, int> evenInterval(int i, int k = -1) const {
        assert(0 <= i && i <= size());
        if (k == -1) { k = even[i]; }
        assert(0 <= k && k <= even[i]);
        return {i - k, i + k};}
    pair<int, int> oddInclusive(int i, int k = -1) const { auto [l, r] = oddInterval(i, k); return {l, r - 1}; }
    pair<int, int> evenInclusive(int i, int k = -1) const { auto [l, r] = evenInterval(i, k); return {l, r - 1}; }

    int centerEnd(int c) const {
        assert(0 <= c && c <= 2 * size() - 2);
        return c % 2 ? (c + 1) / 2 + even[(c + 1) / 2] - 1 : c / 2 + odd[c / 2] - 1;}
    // T: O(n), M: O(n) returned; res[r] (res[l]) is the longest palindrome length ending at r (starting at l).
    vector<int> longestEnding() const {
        int n = size();
        vector<int> res(n);
        for (int c = 0, r = 0; c <= 2 * n - 2; ++c) {
            for (int e = centerEnd(c); r <= e; ++r) { res[r] = 2 * r - c + 1; }}
        return res;}
    vector<int> longestStarting() const {
        int n = size();
        vector<int> res(n);
        for (int c = 2 * n - 2, l = n - 1; c >= 0; --c) {
            for (int b = c - centerEnd(c); l >= b; --l) { res[l] = c - 2 * l + 1; }}
        return res;}
};
