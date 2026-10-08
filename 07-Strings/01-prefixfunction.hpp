#pragma once
#include "../01-Core/01-template.hpp"

// T: O(n), M: O(n) returned; random-access/equality sequence, n < INT_MAX.
template<class S>
vector<int> prefixFunction(const S &s) {
    assert(s.size() < INT_MAX);
    int n = int(s.size());
    vector<int> pi(n);
    for (int i = 1, j = 0; i < n; ++i) {
        while (j && !(s[i] == s[j])) { j = pi[j - 1]; }
        if (s[i] == s[j]) { ++j; }
        pi[i] = j;}
    return pi;}

// T: O(n), M: O(n); exact over an unrestricted alphabet; s receives a witness or is cleared.
inline bool validPrefixFunction(const vector<int> &pi, vector<int> &s) {
    assert(pi.size() < INT_MAX);
    int n = int(pi.size());
    s.resize(n);
    for (int i = 0; i < n; ++i) {
        if (pi[i] < 0 || pi[i] > i) { s.clear(); return false; }
        s[i] = pi[i] ? s[pi[i] - 1] : i;}
    if (prefixFunction(s) == pi) { return true; }
    s.clear(); return false;}
inline bool validPrefixFunction(const vector<int> &pi) { vector<int> s; return validPrefixFunction(pi, s); }

// T: O(m), M: O(m) returned; m + 1 entries, next[i] = -1 when no border of p[0, i) fits.
template<class P>
vector<int> kmpNext(const P &p, const vector<int> &pi) {
    int m = int(pi.size());
    vector<int> next(m + 1, -1);
    for (int i = 1; i <= m; ++i) {
        int b = pi[i - 1];
        next[i] = i < m && p[i] == p[b] ? next[b] : b;}
    return next;}
template<class P>
vector<int> kmpNext(const P &p) { return kmpNext(p, prefixFunction(p)); }

// S: O(m), U: O(1) amortized (O(log(m)) worst), Q: O(1), M: O(m); at most INT64_MAX steps between resets.
template<class T = char>
struct KmpMatcher {
    vector<T> pattern;
    vector<int> pi, next;
    int state = 0;
    lng processed = 0;

    KmpMatcher() : next{-1} {}
    template<class S> explicit KmpMatcher(const S &s) {
        assert(s.size() < INT_MAX);
        pattern.assign(s.begin(), s.end());
        pi = prefixFunction(pattern);
        next = kmpNext(pattern, pi);}
    void reset() { state = 0; processed = 0; }
    template<class A, class B> static bool same(const A &a, const B &b) {
        if constexpr (std::is_same_v<A, B> || !std::is_integral_v<A> || !std::is_integral_v<B>) { return a == b; }
        else {
            auto negative = [](auto x) { if constexpr (std::is_signed_v<decltype(x)>) { return x < 0; } else { return false; } };
            return negative(a) == negative(b) && ulll(a) == ulll(b);}}

    bool matched() const { return state == int(pattern.size()); }
    template<class U> bool step(const U &c) {
        assert(processed < std::numeric_limits<lng>::max());
        ++processed;
        int m = int(pattern.size());
        if (!m) { return true; }
        if (state == m) { state = next[m]; }
        while (state >= 0 && !same(c, pattern[state])) { state = next[state]; }
        return ++state == m;}
};
template<class S> KmpMatcher(const S &) -> KmpMatcher<typename S::value_type>;

// T: O(n + m), M: O(m + out); ascending starts with overlaps; empty pattern gives 0..n.
template<class P, class S>
vector<int> kmpOccurrences(const P &pattern, const S &text) {
    assert(text.size() < INT_MAX);
    KmpMatcher matcher(pattern);
    int n = int(text.size()), m = int(matcher.pattern.size());
    vector<int> res;
    if (matcher.matched()) { res.push_back(0); }
    for (int i = 0; i < n; ++i) {
        if (matcher.step(text[i])) { res.push_back(i + 1 - m); }}
    return res;}

// T: O(n + m), M: O(m + n); res[k] counts prefix k of the pattern (or of s for pi) in the text; res[0] = n + 1.
inline vector<lng> prefixOccurrences(const vector<int> &pi) {
    assert(pi.size() < INT_MAX);
    int n = int(pi.size());
    vector<lng> res(n + 1, 1);
    for (int i = n; i > 0; --i) { res[pi[i - 1]] += res[i]; }
    return res;}
template<class P, class S>
vector<lng> prefixOccurrences(const P &pattern, const S &text) {
    assert(text.size() < INT_MAX);
    KmpMatcher matcher(pattern);
    int n = int(text.size()), m = int(matcher.pattern.size());
    vector<lng> res(m + 1);
    for (int i = 0; i < n; ++i) {
        matcher.step(text[i]); ++res[matcher.state];}
    for (int i = m; i > 0; --i) { res[matcher.pi[i - 1]] += res[i]; }
    ++res[0];
    return res;}

// T: O(out), M: O(out) returned; valid pi, len = -1 means n; ascending, full length only if include_full.
inline vector<int> prefixBorders(const vector<int> &pi, int len = -1, bool include_full = false) {
    assert(pi.size() < INT_MAX);
    if (len == -1) { len = int(pi.size()); }
    assert(0 <= len && len <= int(pi.size()));
    vector<int> res;
    for (int k = include_full ? len : (len ? pi[len - 1] : 0); k; k = pi[k - 1]) { res.push_back(k); }
    reverse(res.begin(), res.end());
    return res;}

// T: O(1), M: O(1); valid pi; empty gives 0, whole = true returns the smallest period dividing n.
inline int prefixPeriod(const vector<int> &pi, bool whole = false) {
    assert(pi.size() < INT_MAX);
    int n = int(pi.size());
    if (!n) { return 0; }
    int p = n - pi.back();
    return whole && n % p ? n : p;}

// T: O(out), M: O(out) returned; valid pi; all positive periods ascending, including n.
inline vector<int> prefixPeriods(const vector<int> &pi) {
    assert(pi.size() < INT_MAX);
    int n = int(pi.size());
    vector<int> res;
    if (!n) { return res; }
    for (int k = pi.back(); k; k = pi[k - 1]) { res.push_back(n - k); }
    res.push_back(n);
    return res;}

// T: O((m + 1) * S), M: O((m + 1) * S) returned; symbols in [0, S); aut[q][c] for states 0..m.
inline vector<vector<int>> prefixAutomaton(const vector<int> &pattern, int sigma) {
    assert(pattern.size() < INT_MAX && sigma >= 0);
    int m = int(pattern.size());
    assert(std::ranges::all_of(pattern, [&](int c) { return 0 <= c && c < sigma; }));
    auto pi = prefixFunction(pattern);
    vector<vector<int>> aut(m + 1, vector<int>(sigma));
    for (int q = 0; q <= m; ++q) {
        for (int c = 0; c < sigma; ++c) { aut[q][c] = q < m && c == pattern[q] ? q + 1 : (q ? aut[pi[q - 1]][c] : 0); }}
    return aut;}
