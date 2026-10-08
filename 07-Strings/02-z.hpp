#pragma once
#include "01-prefixfunction.hpp"

// T: O(n), M: O(n) returned; random-access/equality sequence, n < INT_MAX; z[0] = n.
template<class S>
vector<int> zFunction(const S &s) {
    assert(s.size() < INT_MAX);
    int n = int(s.size());
    vector<int> z(n);
    if (n) { z[0] = n; }
    for (int i = 1, l = 0, r = 0; i < n; ++i) {
        if (i < r) { z[i] = min(r - i, z[i - l]); }
        while (z[i] < n - i && s[z[i]] == s[i + z[i]]) { ++z[i]; }
        if (i + z[i] > r) { l = i; r = i + z[i]; }}
    return z;}

// T: O(n + m), M: O(n + m + out); res[i] = LCP(pattern, text[i, n)); occurrences ascending, empty pattern gives 0..n.
template<class P, class S>
vector<int> extendedZ(const P &pattern, const S &text) {
    assert(text.size() < INT_MAX);
    auto z = zFunction(pattern);
    int n = int(text.size()), m = int(pattern.size());
    vector<int> res(n);
    for (int i = 0, l = 0, r = 0; i < n; ++i) {
        if (i < r) { res[i] = min(r - i, z[i - l]); }
        while (res[i] < m && res[i] < n - i && KmpMatcher<>::same(pattern[res[i]], text[i + res[i]])) { ++res[i]; }
        if (i + res[i] > r) { l = i; r = i + res[i]; }}
    return res;}
template<class P, class S>
vector<int> zOccurrences(const P &pattern, const S &text) {
    auto lcp = extendedZ(pattern, text);
    int n = int(text.size()), m = int(pattern.size());
    vector<int> res;
    for (int i = 0; i < n; ++i) {
        if (lcp[i] == m) { res.push_back(i); }}
    if (!m) { res.push_back(n); }
    return res;}

// T: O(n), M: O(out) returned; valid z; ascending positive borders (n only if include_full) and periods (with n).
inline vector<int> zBorders(const vector<int> &z, bool include_full = false) {
    assert(z.size() < INT_MAX);
    int n = int(z.size());
    vector<int> res;
    for (int k = 1; k < n; ++k) {
        if (z[n - k] == k) { res.push_back(k); }}
    if (n && include_full) { res.push_back(n); }
    return res;}
inline vector<int> zPeriods(const vector<int> &z) {
    assert(z.size() < INT_MAX);
    int n = int(z.size());
    vector<int> res;
    for (int p = 1; p < n; ++p) {
        if (z[p] == n - p) { res.push_back(p); }}
    if (n) { res.push_back(n); }
    return res;}

// T: O(n), M: O(1); valid z; empty gives 0, whole = true returns the smallest period dividing n.
inline int zPeriod(const vector<int> &z, bool whole = false) {
    assert(z.size() < INT_MAX);
    int n = int(z.size());
    for (int p = 1; p < n; ++p) {
        if (z[p] == n - p && (!whole || n % p == 0)) { return p; }}
    return n;}

// T: O(n), M: O(n); exact validation and conversion; false clears z, which may alias pi.
inline bool prefixToZ(const vector<int> &pi, vector<int> &z) {
    vector<int> s;
    if (!validPrefixFunction(pi, s)) { z.clear(); return false; }
    z = zFunction(s);
    return true;}

// T: O(n), M: O(n); exact validation (z[0] = n) and conversion; false clears pi, which may alias z.
inline bool zToPrefix(const vector<int> &z, vector<int> &pi) {
    assert(z.size() < INT_MAX);
    int n = int(z.size());
    vector<int> p(n), s;
    if (n && z[0] != n) { pi.clear(); return false; }
    for (int i = 1; i < n; ++i) {
        if (z[i] < 0 || z[i] > n - i) { pi.clear(); return false; }
        for (int j = z[i] - 1; j >= 0 && !p[i + j]; --j) { p[i + j] = j + 1; }}
    if (!validPrefixFunction(p, s) || zFunction(s) != z) { pi.clear(); return false; }
    pi = std::move(p);
    return true;}
inline bool validZFunction(const vector<int> &z) { vector<int> pi; return zToPrefix(z, pi); }
