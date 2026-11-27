#pragma once
#include "../01-Core/01-template.hpp"

// T: O(n + upper), M: O(n + upper); sentinel-free suffix array (n entries, empty suffix omitted).
// Symbols in [0, upper], n < INT_MAX; comparison sort below NAIVE symbols (measured default).
template<int NAIVE = 32> vector<int> sais(const vector<int> &s, int upper) {
    static_assert(NAIVE >= 1);
    assert(upper >= 0 && s.size() < INT_MAX);
    assert(std::ranges::all_of(s, [&](int c) { return 0 <= c && c <= upper; }));
    int n = int(s.size());
    vector<int> sa(n);
    iota(sa.begin(), sa.end(), 0);
    if (n < NAIVE) {
        sort(sa.begin(), sa.end(), [&](int i, int j) { return std::lexicographical_compare(s.begin() + i, s.end(), s.begin() + j, s.end()); });
        return sa;}
    vector<char> stype(n);
    for (int i = n - 2; i >= 0; --i) { stype[i] = s[i] < s[i + 1] || (s[i] == s[i + 1] && stype[i + 1]); }
    auto lms = [&](int i) { return i > 0 && stype[i] && !stype[i - 1]; };
    vector<int> head(upper + 2), tail(upper + 1), at(upper + 1);
    for (int c : s) { ++head[c + 1]; }
    for (int c = 0; c <= upper; ++c) { head[c + 1] += head[c]; tail[c] = head[c + 1]; }
    // Seeds keep their order at bucket tails; L-types fill heads left to right, then S-types refill tails right to left.
    auto induce = [&](const vector<int> &seeds) {
        fill(sa.begin(), sa.end(), -1);
        std::copy(tail.begin(), tail.end(), at.begin());
        for (int k = int(seeds.size()) - 1; k >= 0; --k) { sa[--at[s[seeds[k]]]] = seeds[k]; }
        std::copy(head.begin(), head.end() - 1, at.begin());
        sa[at[s[n - 1]]++] = n - 1;
        for (int i = 0; i < n; ++i) {
            int p = sa[i] - 1;
            if (p >= 0 && !stype[p]) { sa[at[s[p]]++] = p; }}
        std::copy(tail.begin(), tail.end(), at.begin());
        for (int i = n - 1; i >= 0; --i) {
            int p = sa[i] - 1;
            if (p >= 0 && stype[p]) { sa[--at[s[p]]] = p; }}};
    vector<int> pos, id(n, -1);
    for (int i = 1; i < n; ++i) {
        if (lms(i)) { id[i] = int(pos.size()); pos.push_back(i); }}
    induce(pos);
    int m = int(pos.size());
    if (!m) { return sa; }
    vector<int> sorted, name(m);
    sorted.reserve(m);
    for (int i : sa) {
        if (id[i] >= 0) { sorted.push_back(i); }}
    auto end = [&](int i) { return id[i] + 1 < m ? pos[id[i] + 1] : n; };
    int names = 0;
    for (int k = 1; k < m; ++k) {
        int a = sorted[k - 1], b = sorted[k], ea = end(a), eb = end(b);
        bool same = ea - a == eb - b && ea < n && eb < n && std::equal(s.begin() + a, s.begin() + ea + 1, s.begin() + b);
        name[id[b]] = names += !same;}
    if (names + 1 < m) {
        vector<int> order = sais<NAIVE>(name, names);
        for (int k = 0; k < m; ++k) { sorted[k] = pos[order[k]]; }}
    induce(sorted);
    return sa;}
// T: O(n), M: O(n); unsigned byte order, embedded NUL allowed.
inline vector<int> sais(string_view s) {
    assert(s.size() < INT_MAX);
    vector<int> t(s.begin(), s.end());
    for (int &c : t) { c = uint8_t(c); }
    return sais(t, 255);}
// T: O(n * log(n + 1)), M: O(n); dense ranks in [0, k) preserving order, k distinct symbols.
template<class T> pair<vector<int>, int> compressAlphabet(const vector<T> &s) {
    assert(s.size() < INT_MAX);
    int n = int(s.size()), k = 0;
    vector<int> order(n), res(n);
    iota(order.begin(), order.end(), 0);
    sort(order.begin(), order.end(), [&](int i, int j) { return s[i] < s[j]; });
    for (int t = 0; t < n; ++t) {
        k += t && s[order[t - 1]] < s[order[t]];
        res[order[t]] = k;}
    return {res, n ? k + 1 : 0};}
// T: O(n * log(n + 1)), M: O(n); any totally ordered T via compressAlphabet.
template<class T> vector<int> sais(const vector<T> &s) {
    auto [codes, k] = compressAlphabet(s);
    return sais(codes, max(k - 1, 0));}
// T: O(n), M: O(n); lcp[i] = LCP(sa[i], sa[i + 1]), n - 1 entries (none when n <= 1); sa must be the suffix array of s.
template<class S> vector<int> lcpArray(const S &s, const vector<int> &sa) {
    int n = int(s.size());
    assert(int(sa.size()) == n);
    vector<int> rank(n), lcp(max(n - 1, 0));
    for (int i = 0; i < n; ++i) { rank[sa[i]] = i; }
    for (int i = 0, k = 0; i < n; ++i) {
        if (rank[i] == n - 1) { k = 0; continue; }
        int j = sa[rank[i] + 1];
        while (k < n - i && k < n - j && s[i + k] == s[j + k]) { ++k; }
        lcp[rank[i]] = k;
        k -= k > 0;}
    return lcp;}
