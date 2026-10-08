#pragma once
#include "../01-Core/01-template.hpp"
#include "03-stringhash.hpp"
#include "05-manacher.hpp"

// T: O(1), M: O(1); [l,r) palindrome tests: exact on equality-built Manacher radii, Monte Carlo on hashes (true may collide).
inline bool isPalindrome(const Manacher &m, int l, int r) {
    assert(0 <= l && l <= r && r <= m.size());
    int len = r - l, i = l + len / 2;
    return len % 2 ? m.odd[i] >= len / 2 + 1 : m.even[i] >= len / 2;}
template<int KIND> bool maybePalindrome(const StringHash<KIND> &h, int l, int r) {
    return h.get(l, r) == h.reverseGet(l, r);}

// T: O(n), M: O(1); leftmost longest palindrome [l,r) ([0,0) if n = 0) and the number of nonempty palindromic substrings by position.
inline pair<int, int> longestPalindrome(const Manacher &m) {
    pair<int, int> res{0, 0};
    auto take = [&](pair<int, int> p) {
        if (p.second - p.first > res.second - res.first) { res = p; }};
    for (int i = 0; i < m.size(); ++i) {
        take(m.oddInterval(i));
        take(m.evenInterval(i));}
    return res;}
inline lng countPalindromes(const Manacher &m) {
    lng res = 0;
    for (int k : m.odd) { res += k; }
    for (int k : m.even) { res += k; }
    return res;}
