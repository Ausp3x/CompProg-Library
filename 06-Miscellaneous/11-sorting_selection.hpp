#pragma once
#include "../01-Core/01-template.hpp"

namespace sorting_detail {
    // T: O(1) rank and partition step, O(n) select worst case, M: O(log(n + 1)); rank maps signed order to unsigned order.
    template<typename I>
    constexpr void checkInteger() {
        static_assert(std::is_integral_v<I> && !std::is_same_v<I, bool> && sizeof(I) <= 16);}
    template<typename I>
    std::make_unsigned_t<I> rank(I x) {
        checkInteger<I>(); using U = std::make_unsigned_t<I>;
        constexpr U BIAS = std::is_signed_v<I> ? U(U(1) << (8 * sizeof(I) - 1)) : U(0);
        return U(U(x) ^ BIAS);}
    template<typename T, typename Compare>
    pair<int, int> partition(vector<T> &a, int l, int r, const T &pivot, Compare &cmp) {
        int lt = l, i = l, gt = r;
        while (i < gt) {
            if (cmp(a[i], pivot)) { swap(a[lt++], a[i++]); }
            else if (cmp(pivot, a[i])) { swap(a[i], a[--gt]); }
            else { ++i; }}
        return {lt, gt};}
    template<typename T, typename Compare>
    void select(vector<T> &a, int l, int r, int k, Compare &cmp) {
        while (r - l > 5) {
            int g = l;
            for (int i = l; i < r; i += 5) {
                int e = min(i + 5, r);
                for (int j = i + 1; j < e; ++j) {
                    for (int t = j; t > i && cmp(a[t], a[t - 1]); --t) { swap(a[t], a[t - 1]); }}
                swap(a[g++], a[i + (e - i - 1) / 2]);}
            int m = l + (g - l - 1) / 2;
            select(a, l, g, m, cmp);
            T pivot = a[m]; auto [lt, gt] = partition(a, l, r, pivot, cmp);
            if (k < lt) { r = lt; }
            else if (k >= gt) { l = gt; }
            else { return; }}
        for (int j = l + 1; j < r; ++j) {
            for (int t = j; t > l && cmp(a[t], a[t - 1]); --t) { swap(a[t], a[t - 1]); }}}
} // namespace sorting_detail

// T: O(n + k), M: O(k); non-bool integers through 128 bits in [lo, hi], k = hi - lo + 1 <= INT_MAX even for empty input.
template<typename I>
void countingSort(vector<I> &a, I lo, I hi) {
    sorting_detail::checkInteger<I>(); assert(a.size() <= size_t(INT_MAX) && lo <= hi);
    ulll low = sorting_detail::rank(lo), gap = ulll(sorting_detail::rank(hi)) - low;
    assert(gap < INT_MAX); int k = int(gap) + 1;
    if (a.empty()) { return; }
    assert(lo <= *std::min_element(a.begin(), a.end()) && *std::max_element(a.begin(), a.end()) <= hi);
    vector<int> counts(k);
    for (I x : a) { ++counts[int(ulll(sorting_detail::rank(x)) - low)]; }
    int p = 0; I value = lo;
    for (int i = 0; i < k; ++i) {
        for (int j = 0; j < counts[i]; ++j) { a[p++] = value; }
        if (i + 1 < k) { ++value; }}}

// T: O(n + S), M: O(n + S); stable by an integer key in [0, S), S = alphabet >= 0, key called once per record.
template<typename T, typename Key>
void stableCountingSort(vector<T> &a, int alphabet, Key key) {
    using I = std::remove_cvref_t<std::invoke_result_t<Key, const T &>>;
    sorting_detail::checkInteger<I>(); assert(a.size() <= size_t(INT_MAX) && alphabet >= 0);
    int n = int(a.size());
    if (!n) { return; }
    vector<int> counts(alphabet), keys; keys.reserve(n); bool bad = false;
    for (const T &x : a) {
        I value = key(x); bad |= ulll(value) >= ulll(alphabet);
        keys.push_back(int(value));}
    assert(!bad);
    for (int id : keys) { ++counts[id]; }
    int p = 0;
    for (int &count : counts) { int old = count; count = p; p += old; }
    vector<T> out(a);
    for (int i = 0; i < n; ++i) { out[counts[keys[i]]++] = std::move(a[i]); }
    a.swap(out);}

// T: O(n + d * (n + 256)), M: O(n); d <= 16 key bytes that vary, stable LSD over non-bool 8..128-bit keys.
template<typename T, typename Key = std::identity>
void radixSort(vector<T> &a, Key key = {}) {
    using I = std::remove_cvref_t<std::invoke_result_t<Key, const T &>>;
    sorting_detail::checkInteger<I>(); using U = std::make_unsigned_t<I>;
    assert(a.size() <= size_t(INT_MAX)); int n = int(a.size());
    if (n < 2) { return; }
    U first = sorting_detail::rank(I(key(std::as_const(a)[0]))), different = 0;
    for (const T &x : a) { different |= U(sorting_detail::rank(I(key(x))) ^ first); }
    if (!different) { return; }
    vector<T> out(a);
    for (int byte = 0; byte < int(sizeof(I)); ++byte) {
        int shift = 8 * byte;
        if (!((different >> shift) & U(255))) { continue; }
        array<int, 256> counts{};
        for (const T &x : a) { ++counts[int((sorting_detail::rank(I(key(x))) >> shift) & U(255))]; }
        int p = 0;
        for (int &count : counts) { int old = count; count = p; p += old; }
        for (int i = 0; i < n; ++i) {
            int id = int((sorting_detail::rank(I(key(std::as_const(a)[i]))) >> shift) & U(255));
            out[counts[id]++] = std::move(a[i]);}
        a.swap(out);}}

// T: O(n * log(n + 1)) moves worst case (O(n) with a full buffer), M: O(n); n predicate calls, returns the first false index.
template<typename T, typename Pred>
int stablePartition(vector<T> &a, Pred pred) {
    assert(a.size() <= size_t(INT_MAX));
    return int(std::stable_partition(a.begin(), a.end(), pred) - a.begin());}
// T: O(n), M: O(1); returns {lt, gt}: [0, lt) before pivot, [lt, gt) equivalent, [gt, n) after, order within groups unspecified.
template<typename T, typename Compare = std::less<T>>
pair<int, int> threeWayPartition(vector<T> &a, T pivot, Compare cmp = {}) {
    assert(a.size() <= size_t(INT_MAX));
    return sorting_detail::partition(a, 0, int(a.size()), pivot, cmp);}

// T: O(n * log(n + 1)) worst case (O(n) on average), M: O(1); 0 <= k < n, returns a copy of rank k, a partitioned around k.
template<typename T, typename Compare = std::less<T>>
T quickSelect(vector<T> &a, int k, Compare cmp = {}) {
    assert(a.size() <= size_t(INT_MAX) && 0 <= k && size_t(k) < a.size());
    std::nth_element(a.begin(), a.begin() + k, a.end(), cmp); return a[k];}
// T: O(n) worst case, M: O(log(n + 1)); same contract as quickSelect with groups of five and a three-way split.
template<typename T, typename Compare = std::less<T>>
T medianOfMedians(vector<T> &a, int k, Compare cmp = {}) {
    assert(a.size() <= size_t(INT_MAX) && 0 <= k && size_t(k) < a.size());
    sorting_detail::select(a, 0, int(a.size()), k, cmp); return a[k];}
