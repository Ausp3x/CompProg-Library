#pragma once
#include "../01-Core/01-template.hpp"
#include "06-bit_operations.hpp"

// Visitors return true to continue; false stops immediately (also at the last
// output). Traversals return true only on completion. State is borrowed const
// data, valid only during the callback; copy it to retain it. Callback costs are
// additional. No traversal counts all outputs in a fixed-width integer.
// A visitor must not mutate the radices through an alias during traversal.

// T: O(k + 1 + outputs * (k + 1)), M: O(k); lexicographic increasing indices.
// n,k >= 0; k>n emits nothing, k=0 emits one empty combination.
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
        for (int j = i + 1; j < k; ++j) { a[j] = a[j - 1] + 1; }}
}

// T: O(k + 1 + outputs * (k + 1)), M: O(k); combinations with replacement.
// n,k >= 0; nondecreasing lexicographic indices. n=0<k emits nothing.
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
        fill(a.begin() + i, a.end(), x);}
}

// T: O(outputs) amortized, M: O(n); n >= 0, binary subset order.
// Selected indices are DECREASING so binary carries pop from the back.
// n=0 emits one empty subset; indices are not limited to a machine word.
template<class F>
bool forEachSubset(int n, F &&visit) {
    assert(n >= 0);
    vector<int> a;
    while (true) {
        if (!visit(std::as_const(a))) { return false; }
        int i = 0;
        while (!a.empty() && a.back() == i) { a.pop_back(); ++i; }
        if (i == n) { return true; }
        a.push_back(i);}
}

namespace enumeration_detail {
    // T: O(d), M: O(d) returned; validate even after a zero radix.
    inline vector<int> activeDigits(const vector<int> &radices) {
        assert(radices.size() <= INT_MAX);
        vector<int> active;
        for (int i = 0; i < int(radices.size()); ++i) {
            assert(radices[i] >= 0);
            if (radices[i] > 1) { active.push_back(i); }}
        return active;}
} // namespace enumeration_detail

// T: O(d + outputs) amortized, M: O(d); d=radices.size() <= INT_MAX.
// Nonnegative radices; a zero means no outputs, d=0 means one empty output.
// Lexicographic order, last digit fastest. Unit radices do not slow each step.
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
        ++a[active[j]];}
}

// T: O(d + outputs) amortized, M: O(d); same domains as forEachProduct.
// Reflected Gray order changes one digit by +/-1. visit(digits, changed_index)
// receives -1 initially. Unit radices are skipped during direction reversals.
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
            dir[j] = -dir[j]; --j; }
        if (j < 0) { return true; }
        changed = active[j]; a[changed] += dir[j];}
}

// T: O(outputs), M: O(1); descending submasks, including mask and zero.
template<std::unsigned_integral U, class F>
bool forEachSubmask(U mask, F &&visit) {
    U sub = mask;
    do { if (!visit(U(sub))) { return false; } } while (BitOps<U>::prevSubmask(sub, mask));
    return true;}

// T: O(outputs), M: O(1); ascending masks, 0 <= n <= word width (up to 128).
template<std::unsigned_integral U = ulng, class F>
bool forEachSubsetMask(int n, F &&visit) {
    U last = BitOps<U>::lowMask(n), x = 0;
    while (true) {
        if (!visit(U(x))) { return false; }
        if (x == last) { return true; }
        ++x;}
}

// T: O(outputs + 1), M: O(1); ascending k-bit masks; 0 <= n <= word width.
// k >= 0; k>n emits nothing.
template<std::unsigned_integral U = ulng, class F>
bool forEachCombinationMask(int n, int k, F &&visit) {
    assert(k >= 0);
    U last = BitOps<U>::lowMask(n);
    if (k > n) { return true; }
    U x = BitOps<U>::lowMask(k);
    do { if (!visit(U(x))) { return false; } }
    while (BitOps<U>::nextCombination(x) && x <= last);
    return true;}

// T: O(outputs), M: O(1); binary reflected Gray masks, 0 <= n <= word width.
// visit(mask, changed_bit) receives (0,-1) initially. Full-width n is safe.
template<std::unsigned_integral U = ulng, class F>
bool forEachGrayMask(int n, F &&visit) {
    U last = BitOps<U>::lowMask(n), i = 0, x = 0;
    int changed = -1;
    while (true) {
        if (!visit(U(x), changed)) { return false; }
        if (i == last) { return true; }
        changed = BitOps<U>::trailingZeros(++i); x = BitOps<U>::flip(x, changed);}
}
