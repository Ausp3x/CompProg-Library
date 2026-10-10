#pragma once
#include "../01-Core/01-template.hpp"
#include "07-suffixarray.hpp"

namespace rotation_detail {
    template<class S> auto at(const S &s, int i) {
        if constexpr (std::is_same_v<std::decay_t<decltype(s[0])>, char>) { return uint8_t(s[i]); }
        else { return s[i]; }}
    template<class S> pair<int, int> extreme(const S &s, bool largest) {
        assert(s.size() <= INT_MAX / 2);
        int n = int(s.size()), i = 0, j = 1, k = 0;
        while (i < n && j < n && k < n) {
            auto a = at(s, i + k < n ? i + k : i + k - n), b = at(s, j + k < n ? j + k : j + k - n);
            if (!(a < b) && !(b < a)) { ++k; continue; }
            ((largest ? a < b : b < a) ? i : j) += k + 1;
            j += i == j;
            k = 0;}
        return {min(i, j), k == n && n ? abs(i - j) : n};}
} // namespace rotation_detail

// T: O(n), M: O(1); ordered symbols (char as unsigned), n <= INT_MAX / 2; smallest start, Indices adds the gap p between equal starts (p = n when unique).
template<class S> pair<int, int> minRotationIndices(const S &s) { return rotation_detail::extreme(s, false); }
template<class S> int minRotation(const S &s) { return rotation_detail::extreme(s, false).first; }
template<class S> int maxRotation(const S &s) { return rotation_detail::extreme(s, true).first; }
template<class S> S canonicalRotation(S s) { std::rotate(s.begin(), s.begin() + minRotation(s), s.end()); return s; }
// T: O(n), M: O(1); smallest k with a[k, n) + a[0, k) == b, -1 when b is not a rotation of a.
template<class S> int rotationOffset(const S &a, const S &b) {
    int n = int(a.size());
    if (b.size() != a.size()) { return -1; }
    auto [x, p] = minRotationIndices(a);
    int y = minRotation(b);
    for (int t = 0; t < n; ++t) {
        if (!(rotation_detail::at(a, (x + t) % n) == rotation_detail::at(b, (y + t) % n))) { return -1; }}
    return n ? ((x - y) % p + p) % p : 0;}
template<class S> bool rotationEquivalent(const S &a, const S &b) { return rotationOffset(a, b) >= 0; }

// T: O(n), M: O(1); doubled = SuffixArray of s + s; smallest start of the minimal rotation.
template<class T> int minRotation(const SuffixArray<T> &doubled) {
    int m = doubled.size(), n = m / 2, at = 0;
    assert(m % 2 == 0 && std::equal(doubled.text.begin(), doubled.text.begin() + n, doubled.text.begin() + n));
    if (!n) { return 0; }
    while (doubled.sa[at] >= n) { ++at; }
    int res = doubled.sa[at];
    while (at + 1 < m && doubled.lcp[at] >= n) { res = min(res, doubled.sa[++at]); }
    return res;}
