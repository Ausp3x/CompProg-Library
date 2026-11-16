#pragma once
#include "../01-Core/01-template.hpp"
#include "../02-Data Structures/07-ordered_set.hpp"
#include "02-customhash.hpp"

// T: O(m * log(m + 1)), M: O(m); m <= INT_MAX / 2 intervals with !comp(r, l), both endpoints kept as given.
template<typename T, typename Compare = std::less<T>>
CoordinateCompression<T, Compare> compressEndpoints(const vector<pair<T, T>> &intervals, Compare comp = {}) {
    assert(intervals.size() <= INT_MAX / 2);
    vector<T> values; values.reserve(2 * intervals.size());
    for (const auto &[l, r] : intervals) {
        assert(!comp(r, l));
        values.push_back(l); values.push_back(r);}
    return CoordinateCompression<T, Compare>(std::move(values), comp);}

// T: O(n * log(n + 1)), M: O(n); n <= INT_MAX, permutations of [0, n) ordering equivalent values by index: argsort lists indices in sorted order, stableRanks is its inverse.
template<typename T, typename Compare = std::less<T>>
vector<int> argsort(const vector<T> &a, Compare cmp = {}) {
    assert(a.size() <= size_t(INT_MAX));
    vector<int> ord(a.size());
    iota(ord.begin(), ord.end(), 0);
    std::stable_sort(ord.begin(), ord.end(), [&](int i, int j) { return cmp(a[i], a[j]); });
    return ord;}
template<typename T, typename Compare = std::less<T>>
vector<int> stableRanks(const vector<T> &a, Compare cmp = {}) {
    vector<int> ord = argsort(a, cmp), res(ord.size());
    for (int i = 0; i < int(ord.size()); ++i) { res[ord[i]] = i; }
    return res;}

namespace compression_detail {
    // T: O(n), M: O(1); the minimum of a nonempty a and max - min, without overflow.
    inline pair<lng, ulng> valueSpan(const vector<lng> &a) {
        auto [lo, hi] = std::minmax_element(a.begin(), a.end());
        return {*lo, ulng(*hi) - ulng(*lo)};}
} // namespace compression_detail

// T: O(n + V), M: O(n + V), V = max - min + 1, max - min < INT_MAX; dense ids by value with the sorted distinct values, and stableRanks, by counting.
inline pair<vector<int>, vector<lng>> compressCounting(const vector<lng> &a) {
    assert(a.size() <= size_t(INT_MAX));
    if (a.empty()) { return {}; }
    auto [mn, span] = compression_detail::valueSpan(a);
    if (span >= ulng(INT_MAX)) { assert(span < ulng(INT_MAX)); return {}; }
    vector<int> id(span + 1, -1), res; vector<lng> values;
    for (lng x : a) { id[ulng(x) - ulng(mn)] = 0; }
    for (ulng v = 0; v <= span; ++v) {
        if (!id[v]) { id[v] = int(values.size()); values.push_back(lng(ulng(mn) + v)); }}
    res.reserve(a.size());
    for (lng x : a) { res.push_back(id[ulng(x) - ulng(mn)]); }
    return {std::move(res), std::move(values)};}
inline vector<int> stableRanksCounting(const vector<lng> &a) {
    assert(a.size() <= size_t(INT_MAX));
    if (a.empty()) { return {}; }
    auto [mn, span] = compression_detail::valueSpan(a);
    if (span >= ulng(INT_MAX)) { assert(span < ulng(INT_MAX)); return {}; }
    vector<int> start(span + 2), res; res.reserve(a.size());
    for (lng x : a) { ++start[ulng(x) - ulng(mn) + 1]; }
    for (ulng v = 0; v <= span; ++v) { start[v + 1] += start[v]; }
    for (lng x : a) { res.push_back(start[ulng(x) - ulng(mn)]++); }
    return res;}

// T: NA, M: O(n); the compression of each axis and each point's (x id, y id).
template<typename T, typename Compare = std::less<T>>
struct PointCompression {
    CoordinateCompression<T, Compare> xs, ys;
    vector<pair<int, int>> ids;
};
// T: O(n * log(n + 1)), M: O(n); each axis compressed apart, ids[i] = (x id, y id) of points[i].
template<typename T, typename Compare = std::less<T>>
PointCompression<T, Compare> compressPoints(const vector<pair<T, T>> &points, Compare cmp = {}) {
    assert(points.size() <= size_t(INT_MAX));
    vector<T> x, y; x.reserve(points.size()); y.reserve(points.size());
    for (const auto &[a, b] : points) { x.push_back(a); y.push_back(b); }
    PointCompression<T, Compare> res{CoordinateCompression<T, Compare>(std::move(x), cmp), CoordinateCompression<T, Compare>(std::move(y), cmp), {}};
    res.ids.reserve(points.size());
    for (const auto &[a, b] : points) { res.ids.emplace_back(res.xs.index(a), res.ys.index(b)); }
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

// S: O(1), U: O(1) expected amortized, Q: O(1) expected, M: O(peak n); first-encounter ids by == over Map (Data Structures HashMap<T, int, Hash> is faster), -1 when absent.
template<typename T, typename Hash = CustomHash, typename Map = unordered_map<T, int, Hash>>
struct HashedEncounterCompression {
    Map ids;
    vector<T> values;
    explicit HashedEncounterCompression(Hash hash = {}) : ids(0, hash) {}

    int size() const { return int(values.size()); }
    int add(const T &x) {
        // ids holds id + 1, so the 0 that operator[] inserts marks a new key.
        int &i = ids[x];
        if (!i) { assert(values.size() < INT_MAX); values.push_back(x); i = size(); }
        return i - 1;}
    int id(const T &x) const { auto it = ids.find(x); return it == ids.end() ? -1 : it->second - 1; }
    typename vector<T>::const_reference value(int i) const { assert(0 <= i && i < size()); return values[i]; }
    void clear() { ids.clear(); values.clear(); }
};
