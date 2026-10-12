#pragma once
#include "../01-Core/01-template.hpp"
#include "../02-Data Structures/07-ordered_set.hpp"

// T: O(m * log(m + 1)), M: O(m); m <= INT_MAX / 2 intervals with !comp(r, l), both endpoints kept as given.
template<typename T, typename Compare = std::less<T>>
CoordinateCompression<T, Compare> compressEndpoints(const vector<pair<T, T>> &intervals, Compare comp = {}) {
    assert(intervals.size() <= INT_MAX / 2);
    vector<T> values; values.reserve(2 * intervals.size());
    for (const auto &[l, r] : intervals) {
        assert(!comp(r, l));
        values.push_back(l); values.push_back(r);}
    return CoordinateCompression<T, Compare>(std::move(values), comp);}

// T: O(n * log(n + 1)), M: O(n); n <= INT_MAX, returns a permutation of [0, n) ordering equivalent values by index.
template<typename T, typename Compare = std::less<T>>
vector<int> stableRanks(const vector<T> &a, Compare cmp = {}) {
    assert(a.size() <= size_t(INT_MAX));
    int n = int(a.size()); vector<int> ord(n), res(n);
    iota(ord.begin(), ord.end(), 0);
    std::stable_sort(ord.begin(), ord.end(), [&](int i, int j) { return cmp(a[i], a[j]); });
    for (int i = 0; i < n; ++i) { res[ord[i]] = i; }
    return res;}

// S: O(1), U: O(log(n + 1)) amortized, Q: O(log(n + 1)), M: O(peak n); first-encounter ids, -1 when absent.
template<typename T, typename Compare = std::less<T>>
struct EncounterCompression {
    map<T, int, Compare> ids;
    vector<T> values;
    explicit EncounterCompression(Compare comp = {}) : ids(comp) {}

    int size() const { return int(values.size()); }
    int add(const T &x) {
        auto it = ids.lower_bound(x);
        if (it != ids.end() && !ids.key_comp()(x, it->first)) { return it->second; }
        assert(values.size() < INT_MAX);
        int i = size(); values.push_back(x); ids.emplace_hint(it, x, i);
        return i;}
    int id(const T &x) const { auto it = ids.find(x); return it == ids.end() ? -1 : it->second; }
    typename vector<T>::const_reference value(int i) const { assert(0 <= i && i < size()); return values[i]; }
    void clear() { ids.clear(); values.clear(); }
};
