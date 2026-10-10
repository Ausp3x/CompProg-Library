#pragma once
#include "../01-Core/01-template.hpp"
#include "07-suffixarray.hpp"

namespace lyndon_detail {
    template<class S> auto at(const S &s, int i) {
        if constexpr (std::is_same_v<std::decay_t<decltype(s[0])>, char>) { return uint8_t(s[i]); }
        else { return s[i]; }}
    template<class S> vector<int> cuts(const S &s, int l) {
        int n = int(s.size());
        vector<int> res{l};
        for (int i = l; i < n;) {
            int j = i + 1, k = i;
            while (j < n && !(at(s, j) < at(s, k))) {
                k = at(s, k) < at(s, j) ? i : k + 1;
                ++j;}
            while (i <= k) { i += j - k; res.push_back(i); }}
        return res;}
} // namespace lyndon_detail

// T: O(n), M: O(k) for duval (returned) and standardFactorization, O(1) otherwise; ordered symbols (char as unsigned), n < INT_MAX; cuts 0 = c[0] < ... < c[k] = n.
template<class S> vector<int> duval(const S &s) { assert(s.size() < INT_MAX); return lyndon_detail::cuts(s, 0); }
template<class S> int longestLyndonPrefix(const S &s) {
    assert(s.size() < INT_MAX);
    int n = int(s.size()), j = 1, k = 0;
    while (j < n && !(lyndon_detail::at(s, j) < lyndon_detail::at(s, k))) {
        k = lyndon_detail::at(s, k) < lyndon_detail::at(s, j) ? 0 : k + 1;
        ++j;}
    return n ? j - k : 0;}
template<class S> bool isLyndon(const S &s) { return !s.empty() && longestLyndonPrefix(s) == int(s.size()); }
template<class S> int standardFactorization(const S &s) {
    assert(s.size() >= 2 && isLyndon(s));
    auto c = lyndon_detail::cuts(s, 1);
    return c[c.size() - 2];}

// T: O(n * log(n + 1)), M: O(n); res[i] is the length of the longest Lyndon prefix of s[i, n).
template<class S> vector<int> lyndonArray(const S &s) {
    assert(s.size() < INT_MAX);
    int n = int(s.size());
    vector<int> id(n), res(n), st;
    iota(id.begin(), id.end(), 0);
    sort(id.begin(), id.end(), [&](int i, int j) { return lyndon_detail::at(s, i) < lyndon_detail::at(s, j); });
    vector<int> code(n);
    for (int t = 1; t < n; ++t) { code[id[t]] = code[id[t - 1]] + (lyndon_detail::at(s, id[t - 1]) < lyndon_detail::at(s, id[t])); }
    SuffixArray<int> sa(std::move(code), false);
    for (int i = n - 1; i >= 0; --i) {
        while (!st.empty() && sa.rank[st.back()] > sa.rank[i]) { st.pop_back(); }
        res[i] = (st.empty() ? n : st.back()) - i;
        st.push_back(i);}
    return res;}

// T: NA, M: O(n); node v spans [l[v], r[v]) with children left/right (-1 for leaves).
struct LyndonTree {
    vector<int> l, r, left, right, roots;
};
// T: O(n), M: O(n); lambda = lyndonArray(s); leaves i are [i, i + 1), roots are the Lyndon factors left to right.
inline LyndonTree lyndonTree(const vector<int> &lambda) {
    int n = int(lambda.size());
    LyndonTree t{vector<int>(n), vector<int>(n), vector<int>(n, -1), vector<int>(n, -1), {}};
    for (int i = n - 1; i >= 0; --i) {
        assert(1 <= lambda[i] && lambda[i] <= n - i);
        int u = i;
        t.l[i] = i;
        t.r[i] = i + 1;
        while (!t.roots.empty() && t.r[t.roots.back()] <= i + lambda[i]) {
            t.l.push_back(i); t.r.push_back(t.r[t.roots.back()]);
            t.left.push_back(u); t.right.push_back(t.roots.back());
            t.roots.pop_back();
            u = int(t.l.size()) - 1;}
        t.roots.push_back(u);}
    reverse(t.roots.begin(), t.roots.end());
    return t;}
// T: O(out), M: O(out) returned; cuts of the Lyndon factorization of s[i, n).
inline vector<int> suffixFactorization(const vector<int> &lambda, int i) {
    int n = int(lambda.size());
    assert(0 <= i && i <= n);
    vector<int> res{i};
    while (i < n) {
        assert(1 <= lambda[i] && lambda[i] <= n - i);
        res.push_back(i += max(lambda[i], 1));}
    return res;}

// S: O(1), U: O(1) amortized, Q: O(1) minSuffix, O(out) factorize, M: O(n); ordered T, n < INT_MAX - 1.
template<class T = int> struct IncrementalLyndon {
    vector<T> s;
    vector<int> mn{0};
    int i = 0, j = 0, k = 0;

    void add(T c) {
        assert(s.size() < INT_MAX - 1);
        s.push_back(c);
        mn.push_back(0);
        for (int n = int(s.size()); j < n;) {
            if (j == i) { k = i; mn[++j] = 1; }
            else if (s[k] < s[j]) { k = i; ++j; mn[j] = j - i; }
            else if (s[j] < s[k]) { i += ((k - i) / (j - k) + 1) * (j - k); j = i; }
            else { ++k; ++j; mn[j] = (j - i) % (j - k) ? mn[k] : j - k; }}}

    int size() const { return int(s.size()); }
    int minSuffix(int m) const { assert(0 <= m && m <= size()); return mn[m]; }
    vector<int> factorize(int m) const {
        assert(0 <= m && m <= size());
        vector<int> res{m};
        while (m) { res.push_back(m -= mn[m]); }
        reverse(res.begin(), res.end());
        return res;}
};

// T: O(n) per call, O(1) amortized over a full enumeration, M: O(1); w is a Lyndon word over [0, k) with |w| <= n; false clears w after the last word.
inline bool nextLyndonWord(vector<int> &w, int n, int k) {
    int m = int(w.size());
    assert(1 <= m && m <= n && k >= 1);
    for (int i = m; m && i < n; ++i) { w.push_back(w[i - m]); }
    while (!w.empty() && w.back() == k - 1) { w.pop_back(); }
    if (w.empty()) { return false; }
    ++w.back();
    return true;}
// T: O(n + out) amortized, out = Lyndon words of length <= n, M: O(n); f(const vector<int> &) in lexicographic order, only length n if exact.
template<class F> void lyndonWords(int n, int k, F f, bool exact = false) {
    assert(n >= 0 && k >= 0);
    for (vector<int> w{0}; n > 0 && k > 0;) {
        if (!exact || int(w.size()) == n) { f(std::as_const(w)); }
        if (!nextLyndonWord(w, n, k)) { return; }}}
