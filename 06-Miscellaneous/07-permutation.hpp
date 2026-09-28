#pragma once
#include "../01-Core/01-template.hpp"
#include "../02-Data Structures/02-fenwick.hpp"

// T: O(n), M: O(1). Strict weak comparator; equivalent values are duplicates.
// False wraps to first/last order, including empty/singleton ranges.
template<typename T, typename Compare = std::less<T>>
bool nextPermutation(vector<T> &a, Compare cmp = {}) { return std::next_permutation(a.begin(), a.end(), cmp); }
template<typename T, typename Compare = std::less<T>>
bool previousPermutation(vector<T> &a, Compare cmp = {}) { return std::prev_permutation(a.begin(), a.end(), cmp); }

// T: O(n), M: O(n). A permutation is a bijection of [0,n), n <= INT_MAX.
inline bool isPermutation(const vector<int> &p) {
    if (p.size() > size_t(INT_MAX)) { return false; }
    int n = int(p.size()); vector<bool> seen(n);
    for (int x : p) {
        if (x < 0 || x >= n || seen[x]) { return false; }
        seen[x] = true; }
    return true; }

// T: O(n), M: O(n) workspace and returned storage. p[i] maps i to p[i].
// Inverse/compose/power require permutations; compose additionally equal sizes.
inline vector<int> permutationInverse(const vector<int> &p) {
    assert(isPermutation(p)); int n = int(p.size()); vector<int> q(n);
    for (int i = 0; i < n; ++i) { q[p[i]] = i; }
    return q; }
// T: O(n), M: O(n) returned storage. compose(p,q)[i] = p[q[i]] (q first).
inline vector<int> permutationCompose(const vector<int> &p, const vector<int> &q) {
    assert(p.size() == q.size() && isPermutation(p) && isPermutation(q));
    int n = int(p.size()); vector<int> r(n);
    for (int i = 0; i < n; ++i) { r[i] = p[q[i]]; }
    return r; }
// T: O(n), M: O(n) workspace and returned storage. All lng exponents, including
// the minimum: negative powers use the inverse; exponent zero is identity.
inline vector<int> permutationPower(const vector<int> &p, lng k) {
    assert(isPermutation(p)); int n = int(p.size()); vector<int> r(n, -1), cycle;
    for (int i = 0; i < n; ++i) {
        if (r[i] != -1) { continue; }
        cycle.clear(); int v = i;
        do { cycle.push_back(v); v = p[v]; } while (v != i);
        int m = int(cycle.size()), j = int(k % m);
        if (j < 0) { j += m; }
        for (int x : cycle) {
            r[x] = cycle[j]; if (++j == m) { j = 0; }} }
    return r; }

// T: O(n * log(n + 1)), M: O(n) workspace and returned storage.
// Exact scalable rank representation: d[i] counts j>i with p[j]<p[i].
// Digits satisfy 0 <= d[i] < n-i; no factorial-size/n<=20 restriction.
inline vector<int> permutationLehmer(const vector<int> &p) {
    assert(isPermutation(p)); int n = int(p.size()); vector<int> d(n);
    Fenwick<int> live(vector<int>(n, 1));
    for (int i = 0; i < n; ++i) { d[i] = live.prefixSum(p[i]); live.add(p[i], -1); }
    return d; }
// T: O(n * log(n + 1)), M: O(n) workspace and returned storage.
// Requires n<=INT_MAX and valid Lehmer digits; empty digits decode to empty.
inline vector<int> permutationFromLehmer(const vector<int> &d) {
    assert(d.size() <= size_t(INT_MAX)); int n = int(d.size()); vector<int> p(n);
    Fenwick<int> live(vector<int>(n, 1));
    for (int i = 0; i < n; ++i) {
        assert(0 <= d[i] && d[i] < n - i);
        p[i] = live.lowerBound(d[i] + 1); live.add(p[i], -1); }
    return p; }

// T: O(n * log(n + 1)), M: O(n). Distinct [0,n) permutations only, n<=20.
// Zero-based lexicographic rank; the empty permutation has rank zero.
inline ulng permutationRank(const vector<int> &p) {
    assert(p.size() <= 20); vector<int> d = permutationLehmer(p); ulng rank = 0;
    int n = int(p.size());
    for (int i = 0; i < n; ++i) { rank = rank * (n - i) + d[i]; }
    return rank; }
// T: O(n * log(n + 1)), M: O(n) workspace and returned storage. Requires 0<=n<=20.
// False iff rank >= n!, preserving out; n=0,rank=0 succeeds with empty output.
inline bool permutationUnrank(int n, ulng rank, vector<int> &out) {
    assert(0 <= n && n <= 20); vector<int> d(n);
    for (int i = n - 1; i >= 0; --i) { d[i] = int(rank % (n - i)); rank /= n - i; }
    if (rank) { return false; }
    out = permutationFromLehmer(d); return true; }

// T: O(n + m), M: O(1), m=count.size(), n=sum(count)<=INT_MAX.
// Nonnegative multiplicities. Exact multinomial n!/prod(count[i]!), including
// empty count=1. False iff total exceeds ulng, preserving out. No n<=20 limit.
inline bool multisetPermutationCount(const vector<int> &count, ulng &out) {
    int n = 0;
    for (int c : count) { assert(c >= 0 && c <= INT_MAX - n); n += c; }
    ulll ways = 1; int prefix = 0;
    for (int c : count) {
        int k = min(prefix, c), total = prefix + c;
        for (int j = 1; j <= k; ++j) {
            ways = ways * (total - k + j) / j;
            if (ways > std::numeric_limits<ulng>::max()) { return false; }}
        prefix = total; }
    out = ulng(ways); return true; }

namespace permutation_detail {
    // Shared grouping for multiset queries; total=0 means count overflow.
    struct Multiset {
        vector<int> keys, count;
        int n;
        ulng total = 0;
        explicit Multiset(const vector<int> &a) : n(0) {
            assert(a.size() <= size_t(INT_MAX)); n = int(a.size()); map<int, int> freq;
            for (int x : a) { ++freq[x]; }
            keys.reserve(freq.size()); count.reserve(freq.size());
            for (auto [x, c] : freq) { keys.push_back(x); count.push_back(c); }
            multisetPermutationCount(count, total); }
    };
} // namespace permutation_detail

// T: O(n * log(m + 1)), M: O(m), m distinct labels. Arbitrary int values, repeated.
// Rank distinct value sequences lexicographically. False iff their total count
// exceeds ulng (even if this particular rank would fit), preserving out.
inline bool multisetPermutationRank(const vector<int> &p, ulng &out) {
    permutation_detail::Multiset a(p);
    if (!a.total) { return false; }
    Fenwick<int> live(a.count); ulng ways = a.total, rank = 0; int left = a.n;
    for (int x : p) {
        int j = int(lower_bound(a.keys.begin(), a.keys.end(), x) - a.keys.begin());
        rank += ulng(ulll(ways) * live.prefixSum(j) / left);
        ways = ulng(ulll(ways) * a.count[j] / left--);
        --a.count[j]; live.add(j, -1); }
    out = rank; return true; }

// T: O(n * log(m + 1)), M: O(m) workspace, O(n) returned, m distinct labels. Input order
// irrelevant. False for count overflow or rank>=total, preserving out; alias-safe.
inline bool multisetPermutationUnrank(const vector<int> &values, ulng rank, vector<int> &out) {
    permutation_detail::Multiset a(values);
    if (!a.total || rank >= a.total) { return false; }
    Fenwick<int> live(a.count); ulng ways = a.total; vector<int> p; p.reserve(a.n);
    for (int left = a.n; left > 0; --left) {
        int target = int(ulll(rank) * left / ways) + 1;
        int j = live.lowerBound(target);
        rank -= ulng(ulll(ways) * live.prefixSum(j) / left);
        ways = ulng(ulll(ways) * a.count[j] / left);
        p.push_back(a.keys[j]); --a.count[j]; live.add(j, -1); }
    out = std::move(p); return true; }
