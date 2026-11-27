#pragma once
#include "../01-Core/01-template.hpp"
#include "006-bit_operations.hpp"

// T: O(k + 1 + out * (k + 1)), M: O(k); n,k >= 0, lexicographic increasing indices.
template<class F>
bool forEachCombination(int n, int k, F &&visit) {
    assert(n >= 0 && k >= 0);
    if (k > n) { return true; }
    vector<int> a(k); iota(a.begin(), a.end(), 0);
    while (true) {
        if (!visit(std::as_const(a))) { return false; }
        int i = k - 1;
        while (i >= 0 && a[i] == n - k + i) { --i; }
        if (i < 0) { return true; }
        ++a[i];
        for (int j = i + 1; j < k; ++j) { a[j] = a[j - 1] + 1; }}}

// T: O(k + 1 + out * (k + 1)), M: O(k); n,k >= 0, nondecreasing lexicographic indices.
template<class F>
bool forEachMulticombination(int n, int k, F &&visit) {
    assert(n >= 0 && k >= 0);
    if (n <= 0 && k) { return true; }
    vector<int> a(k);
    while (true) {
        if (!visit(std::as_const(a))) { return false; }
        int i = k - 1;
        while (i >= 0 && a[i] == n - 1) { --i; }
        if (i < 0) { return true; }
        int x = a[i] + 1;
        fill(a.begin() + i, a.end(), x);}}

// T: O(k + 1 + out) amortized, O(1) delay when k > 3 * (n - k + 1), M: O(k); n,k >= 0, revolving door order.
// visit(indices, removed, added), (-1,-1) first.
template<class F>
bool forEachGrayCombination(int n, int k, F &&visit) {
    assert(n >= 0 && k >= 0);
    if (k > n) { return true; }
    vector<int> a(k); iota(a.begin(), a.end(), 0);
    if (k > 3 * (lng(n) - k + 1)) {
        // Bitner-Ehrlich-Reingold loopless generator, zero-based: g holds the bits, tau the focus pointers with end n, t the selected count below the focus.
        vector<char> g(n);
        vector<int> tau(n);
        for (int j = 0; j < n; ++j) { g[j] = j < k; tau[j] = j + 1; }
        int t = k; tau[0] = k;
        if (!visit(std::as_const(a), -1, -1)) { return false; }
        while (true) {
            int i = tau[0], removed, added;
            if (i >= n) { return true; }
            tau[0] = tau[i]; tau[i] = i + 1;
            if (g[i]) {
                if (t) { g[t - 1] ^= 1; a[t - 1] = added = t - 1; a[t] = i - 1; }
                else { g[i - 1] ^= 1; added = a[0] = i - 1; }
                removed = i; ++t;}
            else {
                if (t != 1) { g[t - 2] ^= 1; a[t - 2] = i - 1; a[t - 1] = i; removed = t - 2; }
                else { g[i - 1] ^= 1; a[0] = i; removed = i - 1; }
                added = i; --t;}
            g[i] ^= 1;
            if (t == i || t == 0) { ++t; }
            else {
                t -= g[i - 1]; tau[i - 1] = tau[0];
                tau[0] = t ? t : i - 1;}
            if (!visit(std::as_const(a), removed, added)) { return false; }}}
    auto above = [&](int j) { return j < k ? a[j] : n; };
    int removed = -1, added = -1;
    while (true) {
        if (!visit(std::as_const(a), removed, added)) { return false; }
        if (!k) { return true; }
        if (k & 1 ? a[0] + 1 < above(1) : a[0] > 0) {
            removed = a[0]; added = a[0] += k & 1 ? 1 : -1;
            continue;}
        if (k == 1) { return true; }
        // Knuth Algorithm R; its j is one-based: step R4 (decrease) runs where j and k differ in parity, R5 (increase) elsewhere.
        for (int j = 2; ; ++j) {
            if ((j ^ k) & 1) {
                if (a[j - 1] >= j) {
                    removed = a[j - 1]; a[j - 1] = a[j - 2]; added = a[j - 2] = j - 2;
                    break;}}
            else if (a[j - 1] + 1 < above(j)) {
                removed = a[j - 2]; a[j - 2] = a[j - 1]; added = ++a[j - 1];
                break;}
            if (j == k) { return true; }}}}

// T: O(n + out), M: O(n); n,k >= 0, visit(bits) with bits[i] = position i + 1 of the cool-lex string (Ruskey-Williams), loopless.
template<class F>
bool forEachCombinationCoolLex(int n, int k, F &&visit) {
    if (n < 0 || k < 0) { assert(n >= 0 && k >= 0); return true; }
    if (k > n) { return true; }
    vector<char> b(n);
    fill(b.begin(), b.begin() + k, char(1));
    if (!visit(std::as_const(b))) { return false; }
    if (!k) { return true; }
    // Their Loopless(s, t) with x, y shifted to zero-based i, j (both start at k - 1); from the second string on, j is the first 0.
    for (int i = k - 1, j = k - 1; i + 1 < n; ) {
        b[i++] = 0; b[j++] = 1;
        if (!b[i]) {
            b[i] = 1; b[0] = 0;
            if (j > 1) { i = 1; }
            j = 0;}
        if (!visit(std::as_const(b))) { return false; }}
    return true;}

// T: O(out) amortized, M: O(n); n >= 0, binary order, selected indices DECREASING.
template<class F>
bool forEachSubset(int n, F &&visit) {
    assert(n >= 0);
    vector<int> a;
    while (true) {
        if (!visit(std::as_const(a))) { return false; }
        int i = 0;
        while (!a.empty() && a.back() == i) { a.pop_back(); ++i; }
        if (i >= n) { return true; }
        a.push_back(i);}}

namespace enumeration_detail {
    // T: O(d), M: O(d) returned; d=radices.size() <= INT_MAX, radices >= 0.
    inline vector<int> activeDigits(const vector<int> &radices) {
        assert(radices.size() <= INT_MAX);
        vector<int> active;
        for (int i = 0; i < int(radices.size()); ++i) {
            assert(radices[i] >= 0);
            if (radices[i] > 1) { active.push_back(i); }}
        return active;}

    // T: O(min(r, n - r)), M: O(1); exact C(n, r), or some value >= 2^64 if it overflows.
    inline ulll binom(int n, int r) {
        if (r < 0 || r > n) { return 0; }
        r = min(r, n - r);
        ulll res = 1;
        for (int j = 1; j <= r && res <= ULLONG_MAX; ++j) { res = res * ulll(n - r + j) / j; }
        return res;}

    // T: O(1), M: O(1); asserts 0 <= n <= width of U and reports it, so a violated width stays bounded under NDEBUG.
    template<std::unsigned_integral U>
    constexpr bool validWidth(int n) {
        assert(0 <= n && n <= BitOps<U>::W);
        return 0 <= n && n <= BitOps<U>::W;}

    // T: O(k + out), M: O(k); compositions of n into k parts >= lo in lexicographic order, loopless.
    template<class F>
    bool compositions(int n, int k, int lo, F &&visit) {
        if (n < 0 || k < 0) { assert(n >= 0 && k >= 0); return true; }
        if (lng(k) * lo > n || (!k && n)) { return true; }
        vector<int> a(k, lo);
        if (!k) { return visit(std::as_const(a)); }
        a[k - 1] = n - (k - 1) * lo;
        // h is the rightmost position below k - 1 holding more than lo, or -1.
        for (int h = -1; ; ) {
            if (!visit(std::as_const(a))) { return false; }
            if (k > 1 && a[k - 1] > lo) { ++a[k - 2]; --a[k - 1]; h = k - 2; }
            else if (h > 0) { a[k - 1] = a[h] - 1; a[h] = lo; ++a[--h]; }
            else { return true; }}}
} // namespace enumeration_detail

// T: O(k + out), M: O(k); n,k >= 0, parts >= 1, lexicographic, loopless.
template<class F>
bool forEachComposition(int n, int k, F &&visit) { return enumeration_detail::compositions(n, k, 1, visit); }
// T: O(k + out), M: O(k); n,k >= 0, parts >= 0, lexicographic, loopless.
template<class F>
bool forEachWeakComposition(int n, int k, F &&visit) { return enumeration_detail::compositions(n, k, 0, visit); }

// T: O(d + out) amortized, M: O(d); d=radices.size(), last digit fastest.
template<class F>
bool forEachProduct(const vector<int> &radices, F &&visit) {
    auto active = enumeration_detail::activeDigits(radices);
    if (std::find(radices.begin(), radices.end(), 0) != radices.end()) { return true; }
    vector<int> a(radices.size());
    while (true) {
        if (!visit(std::as_const(a))) { return false; }
        int j = int(active.size()) - 1;
        while (j >= 0 && a[active[j]] == radices[active[j]] - 1) { a[active[j--]] = 0; }
        if (j < 0) { return true; }
        ++a[active[j]];}}

// T: O(d + out) amortized, M: O(d); d=radices.size(), visit(digits, changed), changed=-1 first.
template<class F>
bool forEachGrayProduct(const vector<int> &radices, F &&visit) {
    auto active = enumeration_detail::activeDigits(radices);
    if (std::find(radices.begin(), radices.end(), 0) != radices.end()) { return true; }
    vector<int> a(radices.size()), dir(active.size(), 1);
    int changed = -1;
    while (true) {
        if (!visit(std::as_const(a), changed)) { return false; }
        int j = int(active.size()) - 1;
        while (j >= 0 && (dir[j] > 0 ? a[active[j]] == radices[active[j]] - 1 : !a[active[j]])) {
            dir[j] = -dir[j]; --j;}
        if (j < 0) { return true; }
        changed = active[j]; a[changed] += dir[j];}}

// T: O(out), M: O(1); submasks of mask descending; supermasks of mask < 2^n inside the low n bits ascending.
template<std::unsigned_integral U, class F>
bool forEachSubmask(U mask, F &&visit) {
    U sub = mask;
    do { if (!visit(U(sub))) { return false; } } while (BitOps<U>::prevSubmask(sub, mask));
    return true;}
template<std::unsigned_integral U, class F>
bool forEachSupermask(U mask, int n, F &&visit) {
    if (!enumeration_detail::validWidth<U>(n)) { return true; }
    U full = BitOps<U>::lowMask(n), sup = mask;
    if (mask & U(~full)) { assert((mask & U(~full)) == 0); return true; }
    do { if (!visit(U(sup))) { return false; } } while (BitOps<U>::nextSupermask(sup, mask, full));
    return true;}

// T: O(popcount(x) + 1), M: O(1); visit(i) for each set bit of x, ascending.
template<std::unsigned_integral U, class F>
bool forEachSetBit(U x, F &&visit) {
    for (; x; x = BitOps<U>::clearLowBit(x)) {
        if (!visit(std::countr_zero(x))) { return false; }}
    return true;}

// T: O(out + 1), M: O(1); ascending k-bit submasks of mask, k >= 0.
template<std::unsigned_integral U, class F>
bool forEachCombinationSubmask(U mask, int k, F &&visit) {
    assert(k >= 0);
    int p = std::popcount(mask);
    if (k < 0 || k > p) { return true; }
    U sub = k == p ? mask : U(mask & BitOps<U>::lowMask(BitOps<U>::select(mask, k)));
    while (true) {
        if (!visit(U(sub))) { return false; }
        // Adding the low bit inside mask moves the lowest run's top past it; the rest of the run refills the low bits of mask.
        U next = U(U(U(sub | U(~mask)) + BitOps<U>::lowBit(sub)) & mask);
        if (!U(next & U(~sub))) { return true; }
        int run = std::popcount(U(sub & U(~next)));
        sub = U(next | U(mask & BitOps<U>::lowMask(BitOps<U>::select(mask, run - 1))));}}

// T: O(out), M: O(1); ascending masks, 0 <= n <= word width (up to 128).
template<std::unsigned_integral U = ulng, class F>
bool forEachSubsetMask(int n, F &&visit) {
    if (!enumeration_detail::validWidth<U>(n)) { return true; }
    U last = BitOps<U>::lowMask(n), x = 0;
    while (true) {
        if (!visit(U(x))) { return false; }
        if (x == last) { return true; }
        ++x;}}

// T: O(out + 1), M: O(1); ascending k-bit masks, 0 <= n <= word width, k >= 0.
template<std::unsigned_integral U = ulng, class F>
bool forEachCombinationMask(int n, int k, F &&visit) {
    assert(k >= 0);
    if (!enumeration_detail::validWidth<U>(n) || k < 0 || k > n) { return true; }
    U last = BitOps<U>::lowMask(n), x = BitOps<U>::lowMask(k);
    do { if (!visit(U(x))) { return false; } }
    while (BitOps<U>::nextCombination(x) && x <= last);
    return true;}

// T: O(out + 1), M: O(1); k-bit masks below 2^n in cool-lex order (Ruskey-Williams, bit i = string position i + 1), loopless.
template<std::unsigned_integral U = ulng, class F>
bool forEachCombinationMaskCoolLex(int n, int k, F &&visit) {
    assert(k >= 0);
    if (!enumeration_detail::validWidth<U>(n) || k < 0 || k > n) { return true; }
    if (!k) { return visit(U(0)); }
    U x = BitOps<U>::lowMask(k), last = U(BitOps<U>::lowMask(k - 1) | U(U(1) << (n - 1)));
    while (true) {
        if (!visit(U(x))) { return false; }
        if (x == last) { return true; }
        // p masks the shortest low prefix ending in 01 (the whole word when none); the bit d after it picks Lemma 6 (add) or Lemma 7 (subtract).
        U r = U(x & U(x + 1)), p = U(r ^ U(r - 1)), d = U(U(p + 1) & x);
        x = U(x + U(p & x) - U(d - U(d != 0)));}}

// T: O(out), M: O(1); reflected Gray masks, visit(mask, changed_bit), (0,-1) first.
template<std::unsigned_integral U = ulng, class F>
bool forEachGrayMask(int n, F &&visit) {
    if (!enumeration_detail::validWidth<U>(n)) { return true; }
    U last = BitOps<U>::lowMask(n), i = 0, x = 0;
    int changed = -1;
    while (true) {
        if (!visit(U(x), changed)) { return false; }
        if (i == last) { return true; }
        changed = std::countr_zero(++i); x = BitOps<U>::flip(x, changed);}}

// T: O(3^n), M: O(1); visit(mask, sub) for sub inside mask < 2^n, masks ascending, submasks descending.
template<std::unsigned_integral U = ulng, class F>
bool forEachSubmaskPair(int n, F &&visit) {
    return forEachSubsetMask<U>(n, [&](U mask) {
        return forEachSubmask(mask, [&](U sub) { return visit(U(mask), U(sub)); });});}

// T: O(n + out) amortized, M: O(n); parts <= max_part, nonincreasing, reverse lexicographic.
template<class F>
bool forEachIntegerPartition(int n, int max_part, F &&visit) {
    if (n < 0) { assert(n >= 0); return true; }
    assert(max_part >= (n > 0));
    max_part = min(max_part, n);
    vector<int> a(max_part ? n / max_part : 0, max_part);
    if (max_part && n % max_part) { a.push_back(n % max_part); }
    int h = int(a.size()) - 1;
    while (h >= 0 && a[h] == 1) { --h; }
    while (true) {
        if (!visit(std::as_const(a))) { return false; }
        if (h < 0) { return true; }
        if (a[h] == 2) { a[h] = 1; a.push_back(1); --h; continue; }
        int r = --a[h], t = int(a.size()) - h;
        a.resize(h + 1);
        for (; t >= r; t -= r) { a.push_back(r); }
        if (t) { a.push_back(t); }
        h = int(a.size()) - 1 - (t == 1);}}
template<class F>
bool forEachIntegerPartition(int n, F &&visit) { return forEachIntegerPartition(n, n, visit); }

// T: O(m + out) amortized, M: O(m); n,m >= 0, exactly m parts, nonincreasing, colexicographic (Knuth Algorithm H).
template<class F>
bool forEachIntegerPartitionParts(int n, int m, F &&visit) {
    if (n < 0 || m < 0) { assert(n >= 0 && m >= 0); return true; }
    if (m > n || (!m && n)) { return true; }
    vector<int> a(m, 1);
    if (m) { a[0] = n - m + 1; }
    if (m < 2) { return visit(std::as_const(a)); }
    while (true) {
        if (!visit(std::as_const(a))) { return false; }
        if (a[1] < a[0] - 1) { --a[0]; ++a[1]; continue; }
        // Smallest j >= 2 with a[j] < a[0] - 1; s = a[0] + ... + a[j - 1] - 1.
        int j = 2, s = a[0] + a[1] - 1;
        while (j < m && a[j] >= a[0] - 1) { s += a[j++]; }
        if (j == m) { return true; }
        int x = ++a[j];
        while (--j > 0) { a[j] = x; s -= x; }
        a[0] = s;}}

// T: O(n + out) amortized, M: O(n); n >= 0, restricted growth strings in lexicographic order.
template<class F>
bool forEachSetPartition(int n, F &&visit) {
    assert(n >= 0);
    vector<int> a(n), top(n);
    while (true) {
        if (!visit(std::as_const(a))) { return false; }
        int i = n - 1;
        while (i > 0 && a[i] == top[i - 1] + 1) { --i; }
        if (i <= 0) { return true; }
        top[i] = max(top[i - 1], ++a[i]);
        for (int j = i + 1; j < n; ++j) { a[j] = 0; top[j] = top[i]; }}}
// T: O(n + out) amortized, M: O(n); n,k >= 0, exactly k blocks, restricted growth strings in lexicographic order.
template<class F>
bool forEachSetPartition(int n, int k, F &&visit) {
    assert(n >= 0 && k >= 0);
    if (k < 0 || k > n || (!k && n)) { return true; }
    vector<int> a(n), top(n);
    // Smallest completion after i: zeros, then the forced tail a[j] = j - n + k from tail on, rewritten only where it moved.
    int tail = n;
    auto complete = [&](int i) {
        int t = n - k + 1 + top[i];
        for (int j = i + 1; j < t; ++j) { a[j] = 0; top[j] = top[i]; }
        for (int j = t; j < tail; ++j) { a[j] = top[j] = j - n + k; }
        tail = t;};
    if (n) { complete(0); }
    while (true) {
        if (!visit(std::as_const(a))) { return false; }
        int i = tail - 1;
        while (i > 0 && (a[i] > top[i - 1] || a[i] >= k - 1)) { --i; }
        if (i <= 0) { return true; }
        top[i] = max(top[i - 1], ++a[i]);
        complete(i);}}

// T: O(n + out) amortized, M: O(n); 0 <= n < INT_MAX, visit(mate), mate[u] = partner or -1 (odd n); lexicographic, unpaired last.
template<class F>
bool forEachPairing(int n, F &&visit) {
    if (n < 0 || n == INT_MAX) { assert(0 <= n && n < INT_MAX); return true; }
    // Odd n pairs with a phantom element n; a sorted dancing-links list at sentinel m holds the free elements.
    int m = n + (n & 1), d = 0;
    vector<int> mate(n, -1), nxt(m + 1), prv(m + 1), us(m / 2), vs(m / 2);
    for (int i = 0; i <= m; ++i) { nxt[i] = i == m ? 0 : i + 1; prv[i] = i ? i - 1 : m; }
    auto unlink = [&](int x) { nxt[prv[x]] = nxt[x]; prv[nxt[x]] = prv[x]; };
    auto relink = [&](int x) { nxt[prv[x]] = x; prv[nxt[x]] = x; };
    auto join = [&](int u, int v) {
        if (v < n) { mate[u] = v; mate[v] = u; }
        else { mate[u] = -1; }};
    while (true) {
        for (; d < m / 2; ++d) {
            us[d] = nxt[m]; unlink(us[d]);
            vs[d] = nxt[m]; unlink(vs[d]);
            join(us[d], vs[d]);}
        if (!visit(std::as_const(mate))) { return false; }
        while (true) {
            if (!d) { return true; }
            --d; relink(vs[d]);
            if (int v = nxt[vs[d]]; v != m) {
                unlink(v); vs[d] = v; join(us[d], v); ++d;
                break;}
            relink(us[d]);}}}

// T: O(n * out) for out = n^(n-2) trees, M: O(n); n >= 1, Pruefer codes lexicographic, visit(edges), (child, parent) toward root n-1.
template<class F>
bool forEachLabeledTree(int n, F &&visit) {
    assert(n >= 1);
    vector<pair<int, int>> edges(n - 1);
    vector<int> degree(n);
    return forEachProduct(vector<int>(max(n - 2, 0), n), [&](const vector<int> &code) {
        fill(degree.begin(), degree.end(), 1);
        for (int v : code) { ++degree[v]; }
        // Linear decoding: ptr scans for the next unused leaf; a vertex freed below ptr is the next leaf at once.
        int ptr = 0;
        while (degree[ptr] != 1) { ++ptr; }
        int leaf = ptr;
        for (int i = 0; i < n - 2; ++i) {
            int v = code[i];
            edges[i] = {leaf, v};
            if (--degree[v] == 1 && v < ptr) { leaf = v; }
            else {
                do { ++ptr; } while (degree[ptr] != 1);
                leaf = ptr;}}
        if (n > 1) { edges[n - 2] = {leaf, n - 1}; }
        return visit(std::as_const(edges));});}

// T: O(k * (min(k, n - k) + 1)) rank, times log(n) unrank, M: O(k); forEachCombination order, C(n, k) < 2^64.
inline ulng combinationRank(int n, const vector<int> &a) {
    int k = int(a.size());
    assert(k <= n && enumeration_detail::binom(n, k) <= ULLONG_MAX);
    ulng res = 0;
    for (int i = 0, p = -1; i < k; p = a[i++]) {
        assert(p < a[i] && a[i] < n);
        res += ulng(enumeration_detail::binom(n - 1 - p, k - i) - enumeration_detail::binom(n - a[i], k - i));}
    return res;}
inline vector<int> combinationUnrank(int n, int k, ulng rank) {
    using enumeration_detail::binom;
    assert(0 <= k && k <= n && binom(n, k) <= ULLONG_MAX && rank < binom(n, k));
    vector<int> a(k);
    for (int i = 0, p = -1; i < k; p = a[i++]) {
        ulll total = binom(n - 1 - p, k - i);
        int lo = p + 1, hi = n - k + i;
        while (lo < hi) {
            int mid = lo + (hi - lo + 1) / 2;
            if (total - binom(n - mid, k - i) <= rank) { lo = mid; }
            else { hi = mid - 1; }}
        a[i] = lo; rank -= ulng(total - binom(n - lo, k - i));}
    return a;}

// T: O(k * (min(k, n - k) + 1)) rank, times log(n) unrank, M: O(k); forEachGrayCombination order (Knuth eq. 30), C(n, k) < 2^64.
inline ulng grayCombinationRank(int n, const vector<int> &a) {
    int k = int(a.size());
    assert(k <= n && enumeration_detail::binom(n, k) <= ULLONG_MAX);
    ulng res = 0;
    for (int i = 0; i < k; ++i) {
        assert((i ? a[i - 1] : -1) < a[i] && a[i] < n);
        res = ulng(enumeration_detail::binom(min(a[i], n - 1) + 1, i + 1)) - 1 - res;}
    return res;}
inline vector<int> grayCombinationUnrank(int n, int k, ulng rank) {
    using enumeration_detail::binom;
    assert(0 <= k && k <= n && binom(n, k) <= ULLONG_MAX && rank < binom(n, k));
    vector<int> a(k);
    // Top element c holds ranks [C(c, i + 1), C(c + 1, i + 1)), the reverse of the order below it.
    for (int i = k - 1, hi = n - 1; i >= 0; hi = a[i--] - 1) {
        int lo = i;
        while (lo < hi) {
            int mid = lo + (hi - lo + 1) / 2;
            if (binom(mid, i + 1) <= rank) { lo = mid; }
            else { hi = mid - 1; }}
        a[i] = lo; rank = ulng(binom(lo, i)) - 1 - (rank - ulng(binom(lo, i + 1)));}
    return a;}

// T: O(k * (min(k, n - k) + 1)), M: O(1); rank in forEachCombinationCoolLex order (Ruskey-Williams Theorem 2), C(n, k) < 2^64.
inline ulng combinationRankCoolLex(int n, const vector<int> &a) {
    int k = int(a.size());
    assert(k <= n && enumeration_detail::binom(n, k) <= ULLONG_MAX);
    int r = 0;
    while (r < k && a[r] == r) { ++r; }
    if (r == k) { return 0; }
    // The first element above its position fixes C(a[r] + 1, r + 1) - 1; every later one adds C(a[j], j + 1) - 1.
    ulng res = ulng(enumeration_detail::binom(min(a[r], n - 1) + 1, r + 1)) - 1;
    for (int j = r; j < k; ++j) {
        assert((j ? a[j - 1] : -1) < a[j] && a[j] < n);
        if (j > r) { res += ulng(enumeration_detail::binom(a[j], j + 1)) - 1; }}
    return res;}
