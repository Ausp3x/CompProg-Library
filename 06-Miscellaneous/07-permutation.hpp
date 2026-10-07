#pragma once
#include "../01-Core/01-template.hpp"
#include "../02-Data Structures/02-fenwick.hpp"

// T: O(n), M: O(1); strict weak comparator, equivalent values are duplicates; false wraps to the first/last order.
template<typename T, typename Compare = std::less<T>>
bool nextPermutation(vector<T> &a, Compare cmp = {}) { return std::next_permutation(a.begin(), a.end(), cmp); }
template<typename T, typename Compare = std::less<T>>
bool previousPermutation(vector<T> &a, Compare cmp = {}) { return std::prev_permutation(a.begin(), a.end(), cmp); }

// T: O(n), M: O(n); a permutation is a bijection of [0, n), compose(p, q)[i] = p[q[i]]; inverse/compose assert it.
inline bool isPermutation(const vector<int> &p) {
    if (p.size() > size_t(INT_MAX)) { return false; }
    int n = int(p.size()); vector<bool> seen(n);
    for (int x : p) {
        if (x < 0 || x >= n || seen[x]) { return false; }
        seen[x] = true;}
    return true;}
inline vector<int> permutationInverse(const vector<int> &p) {
    assert(isPermutation(p)); int n = int(p.size()); vector<int> q(n);
    for (int i = 0; i < n; ++i) { q[p[i]] = i; }
    return q;}
inline vector<int> permutationCompose(const vector<int> &p, const vector<int> &q) {
    assert(p.size() == q.size() && isPermutation(p) && isPermutation(q));
    int n = int(p.size()); vector<int> res(n);
    for (int i = 0; i < n; ++i) { res[i] = p[q[i]]; }
    return res;}

// T: O(n), M: O(n); every lng exponent, negative powers use the inverse, exponent zero is the identity.
inline vector<int> permutationPower(const vector<int> &p, lng k) {
    assert(isPermutation(p)); int n = int(p.size()); vector<int> res(n, -1), cycle;
    for (int i = 0; i < n; ++i) {
        if (res[i] != -1) { continue; }
        cycle.clear(); int v = i;
        do { cycle.push_back(v); v = p[v]; } while (v != i);
        int m = int(cycle.size()), j = int(k % m);
        if (j < 0) { j += m; }
        for (int x : cycle) {
            res[x] = cycle[j];
            if (++j == m) { j = 0; }}}
    return res;}

// T: O(n), M: O(n); cycles ordered by smallest element, each starting there and following p; sign is (-1)^(n - cycles).
inline vector<vector<int>> permutationCycles(const vector<int> &p) {
    assert(isPermutation(p)); int n = int(p.size()); vector<bool> seen(n); vector<vector<int>> res;
    for (int i = 0; i < n; ++i) {
        if (seen[i]) { continue; }
        auto &c = res.emplace_back();
        for (int v = i; !seen[v]; v = p[v]) { seen[v] = true; c.push_back(v); }}
    return res;}
inline int permutationSign(const vector<int> &p) {
    assert(isPermutation(p)); int n = int(p.size()), odd = 0; vector<bool> seen(n);
    for (int i = 0; i < n; ++i) {
        for (int v = i; !seen[v]; v = p[v]) { seen[v] = true; odd ^= v != i; }}
    return odd ? -1 : 1;}

// T: O(n + min(n, 21)^2), M: O(min(n, 21)); p becomes rank (rank(p) + k) mod n!, false iff that wrapped.
inline bool kthNextPermutation(vector<int> &p, lng k) {
    assert(isPermutation(p));
    int n = int(p.size()), m = min(n, 21), s = n - m, c = 0, dir = k < 0 ? -1 : 1;
    ulng a = k < 0 ? 0 - ulng(k) : ulng(k);
    vector<int> d(m), vals(p.end() - m, p.end());
    for (int i = 0; i < m; ++i) {
        for (int j = i + 1; j < m; ++j) { d[i] += vals[j] < vals[i]; }}
    for (int i = m - 1; i >= 0; --i) {
        int r = m - i, x = d[i] + c + dir * int(a % ulng(r));
        a /= ulng(r); c = x < 0 ? -1 : x >= r ? 1 : 0; d[i] = x - c * r;}
    bool ok = !c && !a;
    if (!ok && dir > 0) {
        sort(p.begin() + s, p.end(), std::greater<>()); ok = std::next_permutation(p.begin(), p.end());}
    else if (!ok) {
        sort(p.begin() + s, p.end()); ok = std::prev_permutation(p.begin(), p.end());}
    vals.assign(p.begin() + s, p.end()); sort(vals.begin(), vals.end());
    for (int i = 0; i < m; ++i) {
        p[s + i] = vals[d[i]]; vals.erase(vals.begin() + d[i]);}
    return ok;}

// T: O(n * log(n + 1)), M: O(n); d[i] counts j > i with p[j] < p[i], 0 <= d[i] < n - i, any n <= INT_MAX.
inline vector<int> permutationLehmer(const vector<int> &p) {
    assert(isPermutation(p)); int n = int(p.size()); vector<int> d(n);
    Fenwick<int> live(vector<int>(n, 1));
    for (int i = 0; i < n; ++i) { d[i] = live.prefixSum(p[i]); live.add(p[i], -1); }
    return d;}
inline vector<int> permutationFromLehmer(const vector<int> &d) {
    assert(d.size() <= size_t(INT_MAX)); int n = int(d.size()); vector<int> p(n);
    Fenwick<int> live(vector<int>(n, 1));
    for (int i = 0; i < n; ++i) {
        assert(0 <= d[i] && d[i] < n - i);
        p[i] = live.lowerBound(d[i] + 1); live.add(p[i], -1);}
    return p;}

// T: O(n * log(n + 1)), M: O(n); zero-based lexicographic rank for n <= 20, unrank false iff rank >= n! leaving out.
inline ulng permutationRank(const vector<int> &p) {
    assert(p.size() <= 20); vector<int> d = permutationLehmer(p); ulng rank = 0;
    int n = int(p.size());
    for (int i = 0; i < n; ++i) { rank = rank * ulng(n - i) + ulng(d[i]); }
    return rank;}
inline bool permutationUnrank(int n, ulng rank, vector<int> &out) {
    assert(0 <= n && n <= 20); vector<int> d(n);
    for (int i = n - 1; i >= 0; --i) { d[i] = int(rank % ulng(n - i)); rank /= ulng(n - i); }
    if (rank) { return false; }
    out = permutationFromLehmer(d); return true;}

// T: O(n + m), M: O(1); m = count.size(), n = sum(count) <= INT_MAX, exact n! / prod(count[i]!), false on ulng overflow.
inline bool multisetPermutationCount(const vector<int> &count, ulng &out) {
    int n = 0;
    for (int c : count) { assert(c >= 0 && c <= INT_MAX - n); n += c; }
    ulll ways = 1; int prefix = 0;
    for (int c : count) {
        int k = min(prefix, c), total = prefix + c;
        for (int j = 1; j <= k; ++j) {
            ways = ways * ulll(total - k + j) / ulll(j);
            if (ways > std::numeric_limits<ulng>::max()) { return false; }}
        prefix = total;}
    out = ulng(ways); return true;}

namespace permutation_detail {
    // T: O(n), M: O(n); distinct cycle lengths in increasing order.
    inline vector<int> cycleLengths(const vector<int> &p) {
        assert(isPermutation(p)); int n = int(p.size()); vector<bool> seen(n), has(n + 1); vector<int> res;
        for (int i = 0; i < n; ++i) {
            int m = 0;
            for (int v = i; !seen[v]; v = p[v]) { seen[v] = true; ++m; }
            has[m] = true;}
        for (int m = 1; m <= n; ++m) {
            if (has[m]) { res.push_back(m); }}
        return res;}

    // T: O(n * log(m + 1)), M: O(m), m distinct labels; total = 0 means the multinomial exceeds ulng.
    struct Multiset {
        vector<int> keys, count;
        int n;
        ulng total = 0;
        explicit Multiset(const vector<int> &a) : n(0) {
            assert(a.size() <= size_t(INT_MAX)); n = int(a.size()); map<int, int> freq;
            for (int x : a) { ++freq[x]; }
            keys.reserve(freq.size()); count.reserve(freq.size());
            for (auto [x, c] : freq) { keys.push_back(x); count.push_back(c); }
            multisetPermutationCount(count, total);}
    };
} // namespace permutation_detail

// T: O(n), M: O(n); order = lcm of cycle lengths, false on ulng overflow leaving out; Mod reduces it into ring T.
inline bool permutationOrder(const vector<int> &p, ulng &out) {
    ulng res = 1;
    for (int m : permutation_detail::cycleLengths(p)) {
        ulll x = ulll(res / gcd(res, ulng(m))) * ulng(m);
        if (x > std::numeric_limits<ulng>::max()) { return false; }
        res = ulng(x);}
    out = res; return true;}
template<typename T>
T permutationOrderMod(const vector<int> &p) {
    vector<int> best(p.size() + 1, 1);
    for (int m : permutation_detail::cycleLengths(p)) {
        for (int q = 2; q <= m / q; ++q) {
            int e = 1;
            while (m % q == 0) { m /= q; e *= q; }
            best[q] = max(best[q], e);}
        if (m > 1) { best[m] = max(best[m], m); }}
    T res = T(1);
    for (int e : best) {
        if (e > 1) { res *= T(e); }}
    return res;}

// T: O(n * log(m + 1)), M: O(m + n), m distinct labels; lexicographic rank of distinct arrangements, false when the total exceeds ulng.
inline bool multisetPermutationRank(const vector<int> &p, ulng &out) {
    permutation_detail::Multiset a(p);
    if (!a.total) { return false; }
    Fenwick<int> live(a.count); ulng ways = a.total, rank = 0; int left = a.n;
    for (int x : p) {
        int j = int(lower_bound(a.keys.begin(), a.keys.end(), x) - a.keys.begin());
        rank += ulng(ulll(ways) * ulll(live.prefixSum(j)) / ulll(left));
        ways = ulng(ulll(ways) * ulll(a.count[j]) / ulll(left--));
        --a.count[j]; live.add(j, -1);}
    out = rank; return true;}
inline bool multisetPermutationUnrank(const vector<int> &values, ulng rank, vector<int> &out) {
    permutation_detail::Multiset a(values);
    if (!a.total || rank >= a.total) { return false; }
    Fenwick<int> live(a.count); ulng ways = a.total; vector<int> p; p.reserve(a.n);
    for (int left = a.n; left > 0; --left) {
        int j = live.lowerBound(int(ulll(rank) * ulll(left) / ways) + 1);
        rank -= ulng(ulll(ways) * ulll(live.prefixSum(j)) / ulll(left));
        ways = ulng(ulll(ways) * ulll(a.count[j]) / ulll(left));
        p.push_back(a.keys[j]); --a.count[j]; live.add(j, -1);}
    out = std::move(p); return true;}
