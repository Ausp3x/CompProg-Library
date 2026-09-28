#pragma once
#include "../01-Core/01-template.hpp"

// Prefer std::sort/stable_sort/nth_element for ordinary comparison workloads.
// These explicit integer sorts trade extra memory for dense/fixed-width domains;
// no machine-dependent dispatch threshold is hidden here. All n <= INT_MAX.
// Projections/predicates are pure and value-based; comparisons are strict weak orders. Exceptions
// propagate, with no rollback guarantee after mutation has begun.
namespace sorting_detail {
    template<typename I>
    constexpr void checkInteger() {
        static_assert(std::is_integral_v<I> && !std::is_same_v<I, bool> && sizeof(I) <= 16); }
    template<typename I>
    std::make_unsigned_t<I> rank(I x) {
        checkInteger<I>(); using U = std::make_unsigned_t<I>;
        constexpr U BIAS = std::is_signed_v<I> ? U(U(1) << (8 * sizeof(I) - 1)) : U(0);
        return U(U(x) ^ BIAS); }
} // namespace sorting_detail

// Non-bool signed/unsigned integers through 128 bits, inclusive lo <= value <= hi.
// k=hi-lo+1 must be <=INT_MAX, including for empty input. No signed subtraction
// or increment beyond hi. Select only when O(k) setup/storage is appropriate.
// T: O(n + k), M: O(k); values sorted in place (integer duplicates indistinguishable).
template<typename I>
void countingSort(vector<I> &a, I lo, I hi) {
    sorting_detail::checkInteger<I>(); assert(a.size() <= size_t(INT_MAX) && lo <= hi);
    ulll low = sorting_detail::rank(lo), gap = ulll(sorting_detail::rank(hi)) - low;
    assert(gap < INT_MAX); int k = int(gap) + 1;
    if (a.empty()) { return; }
    vector<int> counts(k);
    for (I x : a) { assert(lo <= x && x <= hi); ++counts[int(ulll(sorting_detail::rank(x)) - low)]; }
    int p = 0; I value = lo;
    for (int i = 0; i < k; ++i) {
        for (int j = 0; j < counts[i]; ++j) { a[p++] = value; }
        if (i + 1 < k) { ++value; } }}

// Stable records by a non-bool integer key in [0,alphabet); alphabet>=0, with
// alphabet=0 valid for empty input only. key is called exactly once per element.
// T is copy-constructible and move-assignable (default construction not required).
// Replaces vector storage; existing references/iterators must not be retained.
// T: O(n + alphabet), M: O(n + alphabet), counting projection/copy/move as O(1).
template<typename T, typename Key>
void stableCountingSort(vector<T> &a, int alphabet, Key key) {
    using I = std::remove_cvref_t<std::invoke_result_t<Key, const T &>>;
    sorting_detail::checkInteger<I>(); assert(a.size() <= size_t(INT_MAX) && alphabet >= 0);
    int n = int(a.size()); if (!n) { return; }
    vector<int> counts(alphabet), keys; keys.reserve(n);
    for (const T &x : a) {
        I value = key(x); assert(value >= 0 && value < alphabet);
        int id = int(value); ++counts[id]; keys.push_back(id); }
    int p = 0;
    for (int &count : counts) { int old = count; count = p; p += old; }
    vector<T> out(a);
    for (int i = 0; i < n; ++i) { out[counts[keys[i]]++] = std::move(a[i]); }
    a.swap(out); }

// Stable LSD radix sort of full signed/unsigned 8..128-bit non-bool keys.
// Signed order is mapped to unsigned order by flipping the sign bit. Constant
// bytes are skipped; equal keys return after one scan. key may be called multiple
// times; T is copy-constructible and move-assignable, no default construction.
// Replaces vector storage when a digit pass runs; do not retain references/iterators.
// b=sizeof(key), d<=b varying bytes. T: O(n + d * (n + 256)), M: O(n + 256).
// Projection/copy/move costs counted as O(1); alphabet tables have 256 int slots.
template<typename T, typename Key = std::identity>
void radixSort(vector<T> &a, Key key = {}) {
    using I = std::remove_cvref_t<std::invoke_result_t<Key, const T &>>;
    sorting_detail::checkInteger<I>(); using U = std::make_unsigned_t<I>;
    assert(a.size() <= size_t(INT_MAX)); int n = int(a.size()); if (n < 2) { return; }
    U first = sorting_detail::rank(I(key(std::as_const(a)[0]))), different = 0;
    for (const T &x : a) { different |= U(sorting_detail::rank(I(key(x))) ^ first); }
    if (!different) { return; }
    vector<T> out(a);
    for (int byte = 0; byte < int(sizeof(I)); ++byte) {
        int shift = 8 * byte; if (!((different >> shift) & U(255))) { continue; }
        array<int, 256> counts{};
        for (const T &x : a) { ++counts[int((sorting_detail::rank(I(key(x))) >> shift) & U(255))]; }
        int p = 0;
        for (int &count : counts) { int old = count; count = p; p += old; }
        for (int i = 0; i < n; ++i) {
            int id = int((sorting_detail::rank(I(key(std::as_const(a)[i]))) >> shift) & U(255));
            out[counts[id]++] = std::move(a[i]); }
        a.swap(out); }}

// Standard stable partition: true records before false records, order retained
// within both groups; returns the first false index, including 0/n. Standard
// MoveConstructible/MoveAssignable/ValueSwappable element requirements apply.
// T: O(n * log(n + 1)) moves worst-case, exactly n predicate calls; O(n) moves
// when the standard library obtains sufficient buffer. M: O(n + log(n + 1)).
template<typename T, typename Pred>
int stablePartition(vector<T> &a, Pred pred) {
    assert(a.size() <= size_t(INT_MAX));
    return int(std::stable_partition(a.begin(), a.end(), pred) - a.begin()); }

// Standard nth_element selection, 0<=k<n. Returns a COPY of rank k; permutes a
// so no right element compares before a left element across position k. Order
// among equivalent values and within partitions is unspecified. Movable/swappable
// elements plus copy construction of the result are required; duplicates allowed.
// T: O(n) comparisons on average; GCC introselect heap fallback O(n * log(n + 1))
// worst-case. M: O(1) for the GNU implementation. No worst-case linear-time claim.
template<typename T, typename Compare = std::less<T>>
T quickSelect(vector<T> &a, int k, Compare cmp = {}) {
    assert(a.size() <= size_t(INT_MAX) && 0 <= k && size_t(k) < a.size());
    std::nth_element(a.begin(), a.begin() + k, a.end(), cmp); return a[k]; }
