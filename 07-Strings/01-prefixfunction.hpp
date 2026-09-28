#pragma once
#include "../01-Core/01-template.hpp"

// Sequences have random access/equality and length < INT_MAX; no sentinel symbol.
// T: O(n), M: O(n) returned; pi[i] is the longest proper border of s[0..i].
template<class S> vector<int> prefixFunction(const S &s) {
    assert(s.size() < INT_MAX); int n = int(s.size());
    vector<int> pi(n);
    for (int i = 1, j = 0; i < n; ++i) {
        while (j && !(s[i] == s[j])) { j = pi[j - 1]; }
        if (s[i] == s[j]) { ++j; }
        pi[i] = j; }
    return pi;
}

namespace prefix_detail {
    // Fresh symbols at zero entries separate all unconstrained equivalence classes.
    inline bool witness(const vector<int> &pi, vector<int> &s) {
        assert(pi.size() < INT_MAX); int n = int(pi.size()); s.resize(n);
        for (int i = 0; i < n; ++i) {
            if (pi[i] < 0 || pi[i] > i) { s.clear(); return false; }
            s[i] = pi[i] ? s[pi[i] - 1] : i; }
        if (prefixFunction(s) == pi) { return true; }
        s.clear(); return false;
    }
} // namespace prefix_detail

// T: O(n), M: O(n); exact feasibility over an unrestricted integer alphabet.
inline bool validPrefixFunction(const vector<int> &pi) {
    vector<int> s; return prefix_detail::witness(pi, s);
}

// S: O(m), U: O(1) amortized per symbol (O(m) worst), Q: O(1), M: O(m).
// Owns pattern and pi. state is the longest matching suffix length, including m.
// Empty pattern matches the initial boundary and every step; processed counts steps.
// reset() retains the pattern. At most INT64_MAX steps between resets.
template<class T = char> struct KmpMatcher {
    vector<T> pattern;
    vector<int> pi;
    int state = 0;
    lng processed = 0;

    KmpMatcher() = default;
    template<class S> explicit KmpMatcher(const S &s) {
        assert(s.size() < INT_MAX); pattern.assign(s.begin(), s.end());
        pi = prefixFunction(pattern);
    }
    void reset() { state = 0; processed = 0; }

    bool matched() const { return state == int(pattern.size()); }
    template<class U> bool step(const U &c) {
        assert(processed < std::numeric_limits<lng>::max()); ++processed;
        int m = int(pattern.size());
        if (!m) { return true; }
        if (state == m) { state = pi[m - 1]; }
        while (state && !(c == pattern[state])) { state = pi[state - 1]; }
        if (c == pattern[state]) { ++state; }
        return matched();
    }
};
template<class S> KmpMatcher(const S &) -> KmpMatcher<typename S::value_type>;

// T: O(n + m), M: O(m + k); ascending starts, overlaps included; empty gives 0..n.
template<class P, class S> vector<int> kmpOccurrences(const P &pattern, const S &text) {
    assert(text.size() < INT_MAX); KmpMatcher matcher(pattern);
    int n = int(text.size()), m = int(matcher.pattern.size()); vector<int> ans;
    if (matcher.matched()) { ans.push_back(0); }
    for (int i = 0; i < n; ++i) {
        if (matcher.step(text[i])) { ans.push_back(i + 1 - m); } }
    return ans;
}

// T: O(n), M: O(n) returned; pi must be valid. Counts include overlapping copies.
// Answer[k] counts prefix length k in the original string; answer[0] = n + 1.
inline vector<lng> prefixOccurrences(const vector<int> &pi) {
    assert(pi.size() < INT_MAX); int n = int(pi.size()); vector<lng> ans(n + 1, 1);
    for (int i = n; i > 0; --i) { ans[pi[i - 1]] += ans[i]; }
    return ans;
}

// T: O(n + m), M: O(m); answer[k] counts pattern prefix k in text, answer[0]=n+1.
template<class P, class S> vector<lng> prefixOccurrences(const P &pattern, const S &text) {
    assert(text.size() < INT_MAX); KmpMatcher matcher(pattern);
    int m = int(matcher.pattern.size()); vector<lng> ans(m + 1);
    for (const auto &c : text) { matcher.step(c); ++ans[matcher.state]; }
    for (int i = m; i > 0; --i) { ans[matcher.pi[i - 1]] += ans[i]; }
    ++ans[0]; return ans;
}

// T: O(k), M: O(k) returned; valid pi, prefix length len (-1 means whole string).
// Ascending positive border lengths; proper borders unless include_full is true.
inline vector<int> prefixBorders(const vector<int> &pi, int len = -1, bool include_full = false) {
    assert(pi.size() < INT_MAX); if (len == -1) { len = int(pi.size()); }
    assert(0 <= len && len <= int(pi.size())); vector<int> ans;
    for (int k = include_full ? len : (len ? pi[len - 1] : 0); k; k = pi[k - 1]) {
        ans.push_back(k); }
    reverse(ans.begin(), ans.end()); return ans;
}

// T: O(1), M: O(1); valid pi. Empty has period 0; whole=true requires divisibility.
inline int prefixPeriod(const vector<int> &pi, bool whole = false) {
    assert(pi.size() < INT_MAX); int n = int(pi.size());
    if (!n) { return 0; }
    int p = n - pi.back(); return whole && n % p ? n : p;
}

// T: O(k), M: O(k) returned; valid pi. All positive periods ascending, including n.
inline vector<int> prefixPeriods(const vector<int> &pi) {
    assert(pi.size() < INT_MAX); int n = int(pi.size()); vector<int> ans;
    if (!n) { return ans; }
    for (int k = pi.back(); k; k = pi[k - 1]) { ans.push_back(n - k); }
    ans.push_back(n); return ans;
}

// T: O(m + (m + 1) * sigma), M: O((m + 1) * (sigma + 1)) returned plus O(m).
// Encoded pattern symbols in [0,sigma); sigma>=0. Rows 0..m, columns 0..sigma-1.
// aut[q][c] is the longest pattern-prefix suffix after c; row m handles overlaps.
inline vector<vector<int>> prefixAutomaton(const vector<int> &pattern, int sigma) {
    assert(pattern.size() < INT_MAX && sigma >= 0); int m = int(pattern.size());
    for (int c : pattern) { assert(0 <= c && c < sigma); }
    auto pi = prefixFunction(pattern); vector<vector<int>> aut(m + 1, vector<int>(sigma));
    for (int q = 0; q <= m; ++q) {
        for (int c = 0; c < sigma; ++c) {
            aut[q][c] = q < m && c == pattern[q] ? q + 1 : (q ? aut[pi[q - 1]][c] : 0); } }
    return aut;
}
