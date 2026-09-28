#pragma once
#include "../01-Core/01-template.hpp"
#include "03-stringhash.hpp"
#include "05-manacher.hpp"

// T: O(1), M: O(1); exact static [l,r) query on prebuilt Manacher radii.
// Center l+(r-l)/2 is a character for odd lengths, a gap for even lengths.
inline bool isPalindrome(const Manacher &m, int l, int r) {
    assert(0 <= l && l <= r && r <= m.size());
    int len = r - l, i = l + len / 2;
    return len % 2 ? m.odd[i] >= len / 2 + 1 : m.even[i] >= len / 2;
}

// T: O(1), M: O(1); Monte Carlo [l,r) query, false is exact, true may collide.
// Inherits StringHash's alphabet/base/collision contract; empty intervals are true.
template<bool WRAP64> bool maybePalindrome(const StringHash<WRAP64> &h, int l, int r) {
    return h.get(l, r) == h.reverseGet(l, r);
}

// T: O(n), M: O(1); leftmost longest palindrome [l,r), empty input gives [0,0).
// Extract bytes with s.substr(l,r-l), or generic symbols with the iterator range.
inline pair<int, int> longestPalindrome(const Manacher &m) {
    pair<int, int> ans{0, 0};
    auto take = [&](pair<int, int> p) {
        int len = p.second - p.first, best = ans.second - ans.first;
        if (len > best || (len == best && p.first < ans.first)) { ans = p; }
    };
    for (int i = 0; i < m.size(); ++i) { take(m.oddInterval(i)); take(m.evenInterval(i)); }
    return ans;
}
