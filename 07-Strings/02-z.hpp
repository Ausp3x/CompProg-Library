#pragma once
#include "01-prefixfunction.hpp"

// T: O(n), M: O(n) returned; random-access/equality sequence, n < INT_MAX.
// z[i]=LCP(s,s[i..n)); z[0]=n, and empty input returns an empty array.
template<class S> vector<int> zFunction(const S &s) {
    assert(s.size() < INT_MAX); int n = int(s.size()); vector<int> z(n);
    if (n) { z[0] = n; }
    for (int i = 1, l = 0, r = 0; i < n; ++i) {
        if (i < r) { z[i] = min(r - i, z[i - l]); }
        while (z[i] < n - i && s[z[i]] == s[i + z[i]]) { ++z[i]; }
        if (i + z[i] > r) { l = i; r = i + z[i]; } }
    return z;
}

// T: O(n + m), M: O(m + n); result[i]=LCP(pattern,text[i..n)), result has n entries.
// Sentinel-free extended KMP; empty pattern produces n zeros, including all bytes.
template<class P, class S> vector<int> extendedZ(const P &pattern, const S &text) {
    assert(text.size() < INT_MAX); auto z = zFunction(pattern);
    int n = int(text.size()), m = int(pattern.size()); vector<int> ans(n);
    for (int i = 0, l = 0, r = 0; i < n; ++i) {
        if (i < r) { ans[i] = min(r - i, z[i - l]); }
        while (ans[i] < m && ans[i] < n - i && pattern[ans[i]] == text[i + ans[i]]) { ++ans[i]; }
        if (i + ans[i] > r) { l = i; r = i + ans[i]; } }
    return ans;
}

// T: O(n + m), M: O(n + m + k); ascending starts with overlaps, empty gives 0..n.
template<class P, class S> vector<int> zOccurrences(const P &pattern, const S &text) {
    auto lcp = extendedZ(pattern, text); int n = int(text.size()), m = int(pattern.size());
    vector<int> ans;
    for (int i = 0; i < n; ++i) { if (lcp[i] == m) { ans.push_back(i); } }
    if (!m) { ans.push_back(n); }
    return ans;
}

// T: O(n), M: O(k) returned; valid z with z[0]=n; ascending positive borders.
inline vector<int> zBorders(const vector<int> &z, bool include_full = false) {
    assert(z.size() < INT_MAX); int n = int(z.size()); vector<int> ans;
    for (int k = 1; k < n; ++k) { if (z[n - k] == k) { ans.push_back(k); } }
    if (n && include_full) { ans.push_back(n); }
    return ans;
}

// T: O(n), M: O(k) returned; valid z with z[0]=n. Positive periods, including n.
inline vector<int> zPeriods(const vector<int> &z) {
    assert(z.size() < INT_MAX); int n = int(z.size()); vector<int> ans;
    for (int p = 1; p < n; ++p) { if (z[p] == n - p) { ans.push_back(p); } }
    if (n) { ans.push_back(n); }
    return ans;
}

// T: O(n), M: O(1); valid z. Empty has period 0; whole=true requires divisibility.
inline int zPeriod(const vector<int> &z, bool whole = false) {
    assert(z.size() < INT_MAX); int n = int(z.size());
    for (int p = 1; p < n; ++p) {
        if (z[p] == n - p && (!whole || n % p == 0)) { return p; } }
    return n;
}

// T: O(n), M: O(n); exact conversion/validation over an unrestricted alphabet.
// z[0]=n strictly. Output may alias input; invalid input clears output, returns false.
inline bool prefixToZ(const vector<int> &pi, vector<int> &z) {
    vector<int> s;
    if (!prefix_detail::witness(pi, s)) { z.clear(); return false; }
    z = zFunction(s); return true;
}

// T: O(n), M: O(n); same status/aliasing contract as prefixToZ. Empty is valid.
inline bool zToPrefix(const vector<int> &z, vector<int> &pi) {
    assert(z.size() < INT_MAX); int n = int(z.size()); vector<int> p(n), s;
    if (n && z[0] != n) { pi.clear(); return false; }
    for (int i = 1; i < n; ++i) {
        if (z[i] < 0 || z[i] > n - i) { pi.clear(); return false; }
        for (int j = z[i] - 1; j >= 0 && !p[i + j]; --j) { p[i + j] = j + 1; } }
    if (!prefix_detail::witness(p, s) || zFunction(s) != z) { pi.clear(); return false; }
    pi = std::move(p); return true;
}

// T: O(n), M: O(n); exact validation, z[0]=n. Does not impose a fixed alphabet size.
inline bool validZFunction(const vector<int> &z) { vector<int> pi; return zToPrefix(z, pi); }
