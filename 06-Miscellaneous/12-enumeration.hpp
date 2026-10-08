#pragma once
#include "../01-Core/01-template.hpp"
#include "06-bit_operations.hpp"

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
    if (!n && k) { return true; }
    vector<int> a(k);
    while (true) {
        if (!visit(std::as_const(a))) { return false; }
        int i = k - 1;
        while (i >= 0 && a[i] == n - 1) { --i; }
        if (i < 0) { return true; }
        int x = a[i] + 1;
        fill(a.begin() + i, a.end(), x);}}

// T: O(out) amortized, M: O(n); n >= 0, binary order, selected indices DECREASING.
template<class F>
bool forEachSubset(int n, F &&visit) {
    assert(n >= 0);
    vector<int> a;
    while (true) {
        if (!visit(std::as_const(a))) { return false; }
        int i = 0;
        while (!a.empty() && a.back() == i) { a.pop_back(); ++i; }
        if (i == n) { return true; }
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
} // namespace enumeration_detail

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
    U full = BitOps<U>::lowMask(n), sup = mask;
    assert((mask & U(~full)) == 0);
    do { if (!visit(U(sup))) { return false; } } while (BitOps<U>::nextSupermask(sup, mask, full));
    return true;}

// T: O(out), M: O(1); ascending masks, 0 <= n <= word width (up to 128).
template<std::unsigned_integral U = ulng, class F>
bool forEachSubsetMask(int n, F &&visit) {
    U last = BitOps<U>::lowMask(n), x = 0;
    while (true) {
        if (!visit(U(x))) { return false; }
        if (x == last) { return true; }
        ++x;}}

// T: O(out + 1), M: O(1); ascending k-bit masks, 0 <= n <= word width, k >= 0.
template<std::unsigned_integral U = ulng, class F>
bool forEachCombinationMask(int n, int k, F &&visit) {
    assert(k >= 0);
    U last = BitOps<U>::lowMask(n);
    if (k > n) { return true; }
    U x = BitOps<U>::lowMask(k);
    do { if (!visit(U(x))) { return false; } }
    while (BitOps<U>::nextCombination(x) && x <= last);
    return true;}

// T: O(out), M: O(1); reflected Gray masks, visit(mask, changed_bit), (0,-1) first.
template<std::unsigned_integral U = ulng, class F>
bool forEachGrayMask(int n, F &&visit) {
    U last = BitOps<U>::lowMask(n), i = 0, x = 0;
    int changed = -1;
    while (true) {
        if (!visit(U(x), changed)) { return false; }
        if (i == last) { return true; }
        changed = BitOps<U>::trailingZeros(++i); x = BitOps<U>::flip(x, changed);}}

// T: O(n + out) amortized, M: O(n); parts <= max_part, nonincreasing, reverse lexicographic.
template<class F>
bool forEachIntegerPartition(int n, int max_part, F &&visit) {
    assert(n >= 0 && max_part >= (n > 0));
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

// T: O(k * min(k, n - k)) rank, times log(n) unrank, M: O(k); forEachCombination order, C(n, k) < 2^64.
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
