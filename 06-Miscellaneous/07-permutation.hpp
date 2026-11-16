#pragma once
#include "../01-Core/01-template.hpp"
#include "../02-Data Structures/02-fenwick.hpp"
#include "../05-Mathematics/05-combinatorics.hpp"
#include "06-bit_operations.hpp"

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
// T: O(n), M: O(n); a[k] counts the k-cycles of p for 0 <= k <= n, so a[0] = 0 and the sum of k * a[k] is n.
inline vector<int> cycleType(const vector<int> &p) {
    assert(isPermutation(p)); int n = int(p.size()); vector<bool> seen(n); vector<int> a(n + 1);
    for (int i = 0; i < n; ++i) {
        if (seen[i]) { continue; }
        int m = 0;
        for (int v = i; !seen[v]; v = p[v]) { seen[v] = true; ++m; }
        ++a[m];}
    return a;}

// T: O(n), M: O(n); cycles partition [0, n) and each follows p, p[c[j]] = c[j + 1] wrapping, as permutationCycles returns them.
inline vector<int> permutationFromCycles(int n, const vector<vector<int>> &cycles) {
    assert(n >= 0); vector<int> p(n, -1);
    for (const auto &c : cycles) {
        int m = int(c.size());
        for (int j = 0; j < m; ++j) {
            assert(0 <= c[j] && c[j] < n && p[c[j]] < 0);
            p[c[j]] = c[j + 1 < m ? j + 1 : 0];}}
    assert(isPermutation(p)); return p;}

// T: O(n), M: O(n); p a permutation of a's size: permute gives b[i] = a[p[i]], inversePermute b[p[i]] = a[i].
template<typename T>
vector<T> permute(const vector<T> &a, const vector<int> &p) {
    assert(a.size() == p.size() && isPermutation(p)); vector<T> b; b.reserve(a.size());
    for (int i : p) { b.push_back(a[i]); }
    return b;}
template<typename T>
vector<T> inversePermute(const vector<T> &a, const vector<int> &p) {
    assert(a.size() == p.size() && isPermutation(p)); int n = int(a.size()); vector<T> b(a);
    for (int i = 0; i < n; ++i) { b[p[i]] = a[i]; }
    return b;}

// T: O(n), M: O(n); fewest rounds of disjoint position swaps sorting p (0, 1 or 2), each round's swaps (i < j) in order (CSES 1698).
inline vector<vector<pair<int, int>>> swapRoundSort(const vector<int> &p) {
    vector<vector<pair<int, int>>> rounds(2);
    auto add = [&](int r, int i, int j) { rounds[r].emplace_back(min(i, j), max(i, j)); };
    for (const auto &c : permutationCycles(p)) {
        int m = int(c.size());
        if (m == 2) { add(0, c[0], c[1]); }
        if (m < 3) { continue; }
        for (int j = 1; 2 * j < m; ++j) { add(0, c[j], c[m - j]); }
        add(1, c[0], c[1]);
        for (int j = 2; 2 * j < m + 1; ++j) { add(1, c[j], c[m + 1 - j]); }}
    while (!rounds.empty() && rounds.back().empty()) { rounds.pop_back(); }
    return rounds;}
// T: O(n), M: O(n); rounds of one left-to-right pass collecting 0, 1, ..., n - 1 in order: 1 + #{v : pos(v + 1) < pos(v)}, 0 when empty (CSES 2216).
inline int collectingNumbers(const vector<int> &p) {
    vector<int> pos = permutationInverse(p); int n = int(p.size()), res = n > 0;
    for (int v = 0; v + 1 < n; ++v) { res += pos[v + 1] < pos[v]; }
    return res;}

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

namespace permutation_detail {
    // T: O(n), M: O(n); distinct cycle lengths in increasing order.
    inline vector<int> cycleLengths(const vector<int> &p) {
        vector<int> a = cycleType(p), res;
        for (int m = 1; m < int(a.size()); ++m) {
            if (a[m]) { res.push_back(m); }}
        return res;}

    // T: O(n + log(mod)), M: O(n); 0! .. n! and their inverses in a field M where 1..n are units.
    template<typename M>
    pair<vector<M>, vector<M>> factorialTables(int n) {
        vector<M> fac(n + 1, M(1)), inv_fac(n + 1, M(1));
        for (int i = 1; i <= n; ++i) { fac[i] = fac[i - 1] * M(i); }
        inv_fac[n] = M(1) / fac[n];
        for (int i = n; i > 0; --i) { inv_fac[i - 1] = inv_fac[i] * M(i); }
        return {std::move(fac), std::move(inv_fac)};}

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
            multinomialExact(vector<ulng>(count.begin(), count.end()), total);}
    };

    // T: O(n), M: O(1); sum of d[i] * (n - 1 - i)! in ring T, the factorial running from the back.
    template<typename T>
    T rankFromDigits(const vector<int> &d) {
        T rank = T(0), f = T(1);
        for (int i = int(d.size()) - 1; i >= 0; --i) { rank += T(d[i]) * f; f *= T(int(d.size()) - i); }
        return rank;}

    // T: O(k), M: O(1), k = a.size(); the sum of i * a[i] over a cycle type a, clamped to INT_MAX + 1, also returned for a negative count; callers assert below it.
    inline lng cycleTypeSize(const vector<int> &a) {
        assert(a.empty() || a[0] == 0); lng n = 0;
        for (int k = 1; k < int(a.size()); ++k) {
            if (a[k] < 0) { return lng(INT_MAX) + 1; }
            n = std::min(n + lng(k) * a[k], lng(INT_MAX) + 1);}
        return n;}
} // namespace permutation_detail

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

// T: O(n^2), M: O(n); the same digits by direct scans, without Fenwick.
inline vector<int> permutationLehmerNaive(const vector<int> &p) {
    assert(isPermutation(p)); int n = int(p.size()); vector<int> d(n);
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) { d[i] += p[j] < p[i]; }}
    return d;}
inline vector<int> permutationFromLehmerNaive(const vector<int> &d) {
    assert(d.size() <= size_t(INT_MAX)); int n = int(d.size()); vector<int> p(n), rest(n);
    iota(rest.begin(), rest.end(), 0);
    for (int i = 0; i < n; ++i) {
        assert(0 <= d[i] && d[i] < n - i);
        p[i] = rest.at(d[i]); rest.erase(rest.begin() + d[i]);}
    return p;}

// T: O(n), M: O(n); zero-based lexicographic rank for n <= 20 over a used-value mask, unrank false iff rank >= n! leaving out.
inline ulng permutationRank(const vector<int> &p) {
    assert(p.size() <= 20 && isPermutation(p)); int n = int(p.size()); uint used = 0; ulng rank = 0;
    for (int i = 0; i < n; ++i) {
        rank = rank * ulng(n - i) + ulng(p[i] - std::popcount(used & ((1u << p[i]) - 1)));
        used |= 1u << p[i];}
    return rank;}
inline bool permutationUnrank(int n, ulng rank, vector<int> &out) {
    if (n < 0 || n > 20) { assert(0 <= n && n <= 20); return false; }
    vector<int> p(n);
    for (int i = n - 1; i >= 0; --i) { p[i] = int(rank % ulng(n - i)); rank /= ulng(n - i); }
    if (rank) { return false; }
    uint unused = (1u << n) - 1;
    for (int &x : p) { x = BitOps<uint>::select(unused, x); unused ^= 1u << x; }
    out = std::move(p); return true;}

// T: O(n * log(n + 1)) (Naive O(n^2)), M: O(n); lexicographic rank of any length reduced into ring T (needs T(int), +=, *, *=).
template<typename T>
T permutationRankMod(const vector<int> &p) { return permutation_detail::rankFromDigits<T>(permutationLehmer(p)); }
template<typename T>
T permutationRankModNaive(const vector<int> &p) { return permutation_detail::rankFromDigits<T>(permutationLehmerNaive(p)); }

// T: O(n), M: O(n); Myrvold-Ruskey order, not lexicographic: rank of any length in ring T (needs T(int), +=, *, *=), unrank any n >= 0, false iff rank >= n! leaving out.
template<typename T>
T permutationRankLinear(const vector<int> &p) {
    vector<int> a = p, inv = permutationInverse(p); T rank = T(0), f = T(1);
    for (int k = int(a.size()); k > 1; --k) {
        int s = a[k - 1];
        std::swap(a[k - 1], a[inv[k - 1]]); std::swap(inv[s], inv[k - 1]);
        rank += T(s) * f; f *= T(k);}
    return rank;}
inline bool permutationUnrankLinear(int n, ulng rank, vector<int> &out) {
    assert(n >= 0); vector<int> p(n);
    iota(p.begin(), p.end(), 0);
    for (int k = n; k > 1; --k) { std::swap(p[k - 1], p[rank % ulng(k)]); rank /= ulng(k); }
    if (rank) { return false; }
    out = std::move(p); return true;}

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

// T: O(n * log(m + 1)), M: O(m), m distinct labels; number of distinct arrangements of values, false when it exceeds ulng leaving out.
inline bool distinctPermutationCount(const vector<int> &values, ulng &out) {
    ulng total = permutation_detail::Multiset(values).total;
    if (!total) { return false; }
    out = total; return true;}

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

// T: O(n * log(m + 1) + log(mod)), M: O(n + m), mod the modulus of M; arrangements of values below target (a proper prefix is smaller), in a field M where 1..n are units.
template<typename M>
M multisetPermutationRankMod(const vector<int> &values, const vector<int> &target) {
    permutation_detail::Multiset a(values); auto [fac, inv_fac] = permutation_detail::factorialTables<M>(a.n);
    M ways = fac[a.n], rank = M(0);
    for (int c : a.count) { ways *= inv_fac[c]; }
    Fenwick<int> live(a.count); int left = a.n;
    for (int x : target) {
        if (!left) { rank += M(1); return rank; }
        int j = int(lower_bound(a.keys.begin(), a.keys.end(), x) - a.keys.begin());
        M step = ways * inv_fac[left] * fac[left - 1];
        rank += step * M(live.prefixSum(j));
        if (j == int(a.keys.size()) || a.keys[j] != x || !a.count[j]) { return rank; }
        ways = step * M(a.count[j]); --a.count[j]; live.add(j, -1); --left;}
    return rank;}

// T: O(n + k) (Mod O(n + k + log(mod))), M: O(1) (Mod O(n)), n the sum of i * a[i], k = a.size(); permutations of [0, n) with a[i] i-cycles (a[0] = 0, as cycleType gives), false above ulng leaving out; Mod in a field M where 1..n are units.
inline bool cycleTypeCount(const vector<int> &a, ulng &out) {
    lng n = permutation_detail::cycleTypeSize(a);
    if (n > INT_MAX) { assert(n <= INT_MAX); return false; }
    ulng res = 1, rem = ulng(n), f;
    for (int k = 2; k < int(a.size()); ++k) {
        if (a[k] <= 0) { continue; }
        ulng block = ulng(k) * ulng(a[k]);
        if (!combiExact(rem, block, f) || __builtin_mul_overflow(res, f, &res)) { return false; }
        for (ulng j = 1; j <= ulng(a[k]); ++j) {
            if (!permuExact(j * ulng(k) - 1, ulng(k - 1), f) || __builtin_mul_overflow(res, f, &res)) { return false; }}
        rem -= block;}
    out = res; return true;}
template<typename M>
M cycleTypeCountMod(const vector<int> &a) {
    lng n = permutation_detail::cycleTypeSize(a);
    if (n > INT_MAX) { assert(n <= INT_MAX); return M(0); }
    auto [fac, inv_fac] = permutation_detail::factorialTables<M>(int(n));
    M res = fac[n];
    for (int k = 1; k < int(a.size()); ++k) {
        if (a[k] <= 0) { continue; }
        M inv_k = inv_fac[k] * fac[k - 1];
        res *= inv_fac[a[k]];
        for (int j = 0; j < a[k]; ++j) { res *= inv_k; }}
    return res;}
