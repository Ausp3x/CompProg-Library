#pragma once

#include "../01-Core/01-template.hpp"
#include "00-monoids.hpp"
#include "02-fenwick.hpp"
#include "03-segmenttree.hpp"
#include "17-waveletmatrix.hpp"
#include "18-persistentsegmenttree.hpp"

namespace static_range_queries_detail {
    template<typename T>
    vector<pair<T, int>> occurrences(const vector<T> &a) {
        assert(a.size() < (1u << 31));
        vector<pair<T, int>> s;
        s.reserve(a.size());
        for (int i = 0; i < int(a.size()); ++i) { s.push_back({a[i], i}); }
        std::stable_sort(s.begin(), s.end(), [](const auto &x, const auto &y) { return x.first < y.first; });
        return s;}

    inline vector<int> byRight(const vector<pair<int, int>> &qs, int n) {
        int q = int(qs.size());
        vector<int> start(size_t(n) + 2), res(q);
        for (auto [l, r] : qs) { assert(0 <= l && l <= r && r <= n); ++start[r + 1]; }
        for (int i = 0; i <= n; ++i) { start[i + 1] += start[i]; }
        for (int j = 0; j < q; ++j) { res[start[qs[j].second]++] = j; }
        return res;}
} // namespace static_range_queries_detail

// T: O((n + q) * log(n)), M: O(n + q)
template<typename T>
vector<int> staticRangeCountDistinct(const vector<T> &a, const vector<pair<int, int>> &qs) {
    int n = int(a.size());
    auto s = static_range_queries_detail::occurrences(a);
    vector<int> prv(n, -1), res(qs.size());
    for (int k = 1; k < n; ++k) {
        if (!(s[k - 1].first < s[k].first)) { prv[s[k].second] = s[k - 1].second; }}
    Fenwick<int> fw(n);
    int i = 0;
    for (int j : static_range_queries_detail::byRight(qs, n)) {
        auto [l, r] = qs[j];
        for (; i < r; ++i) {
            fw.add(i, 1);
            if (prv[i] >= 0) { fw.add(prv[i], -1); }}
        res[j] = fw.sum(l, r);}
    return res;}

// S: O(n * log(n)), U: NA, Q: O(log(n)), M: O(n)
template<typename T>
struct StaticRangeFrequency {
    int n;
    vector<pair<T, int>> s;

    explicit StaticRangeFrequency(const vector<T> &a = {}) : n(int(a.size())), s(static_range_queries_detail::occurrences(a)) {}

    int lowerIndex(const T &x, int i) const {
        auto less = [](const pair<T, int> &p, const pair<T, int> &k) { return p.first < k.first || (!(k.first < p.first) && p.second < k.second); };
        return int(lower_bound(s.begin(), s.end(), pair<T, int>{x, i}, less) - s.begin());}
    int count(int l, int r, const T &x) const { assert(0 <= l && l <= r && r <= n); return lowerIndex(x, r) - lowerIndex(x, l); }
};

// T is a signed or unsigned integer type, not bool or a character type; values outside [0, n) never matter; n <= 2^29.
// T: O((n + q) * log(n)), M: O(n + q)
template<typename T>
vector<int> rangeMex(const vector<T> &a, const vector<pair<int, int>> &qs) {
    assert(a.size() <= size_t(1 << 29));
    int n = int(a.size());
    vector<int> res(qs.size());
    SegmentTree<Min<int>> last(vector<int>(n, -1));
    int i = 0;
    for (int j : static_range_queries_detail::byRight(qs, n)) {
        auto [l, r] = qs[j];
        for (; i < r; ++i) {
            if (std::cmp_less_equal(0, a[i]) && std::cmp_less(a[i], n)) { last.set(int(a[i]), i); }}
        res[j] = last.maxRight(0, [&](int x) { return x >= l; });}
    return res;}

// S: O(n * log(n)), U: NA, Q: O(log(n)), M: O(n * log(n))
struct PersistentRangeMex {
    int n;
    PersistentSegmentTree<Min<int>> tree;
    vector<int> ver;

    PersistentRangeMex() : PersistentRangeMex(vector<int>{}) {}
    template<typename T>
    explicit PersistentRangeMex(const vector<T> &a) : n(int(a.size())), tree(lng(a.size()), -1), ver(a.size() + 1) {
        assert(a.size() < (1u << 26));
        auto in = [&](const T &x) { return std::cmp_less_equal(0, x) && std::cmp_less(x, n); };
        tree.reserve(int(std::count_if(a.begin(), a.end(), in) + 2) * (std::bit_width(uint(n)) + 1));
        for (int i = 0; i < n; ++i) { ver[i + 1] = in(a[i]) ? tree.set(ver[i], lng(a[i]), i) : ver[i]; }}

    int mex(int l, int r) const { assert(0 <= l && l <= r && r <= n); return int(tree.maxRight(ver[r], 0, [&](int x) { return x >= l; })); }
};

// T is default-constructible, totally ordered by < and comparable by ==; 0 <= n <= 2^29; majority(l, r, res) is false when no value fills more than half of [l, r).
// S: O(n * log(n)), U: NA, Q: O(log(n)), M: O(n)
template<typename T>
struct RangeMajority {
    StaticRangeFrequency<T> freq;
    SegmentTree<MajorityVote<T>> votes;

    explicit RangeMajority(const vector<T> &a = {}) : freq(a), votes(leaves(a)) {}
    static vector<typename MajorityVote<T>::S> leaves(const vector<T> &a) {
        vector<typename MajorityVote<T>::S> res;
        res.reserve(a.size());
        for (const T &x : a) { res.push_back(MajorityVote<T>::leaf(x)); }
        return res;}

    bool majority(int l, int r, T &res) const {
        assert(0 <= l && l <= r && r <= freq.n);
        if (l == r) { return false; }
        T x = votes.prod(l, r).x;
        if (2 * freq.count(l, r, x) <= r - l) { return false; }
        res = std::move(x);
        return true;}
};

// T is totally ordered by < and comparable by ==; 0 <= n < 2^31; frequent(l, r, t) lists (value, count) with count >= t >= 1 in [l, r), by increasing value.
// S: O(n * log(n)), U: NA, Q: O((1 + len / t) * log(n)) for len = r - l, M: O(n * log(n))
template<typename T>
struct RangeThresholdFrequency {
    WaveletMatrix<T> wm;

    explicit RangeThresholdFrequency(const vector<T> &a = {}) : wm(a) {}

    vector<pair<T, int>> frequent(int l, int r, int t) const {
        assert(0 <= l && l <= r && r <= wm.n && t >= 1);
        vector<pair<T, int>> res;
        for (lng k = lng(t) - 1; t > 0 && k < r - l;) {
            const T &x = wm.kth(l, r, int(k));
            int a = l, b = r, less = wm.descend(a, b, wm.lowerRank(x));
            if (b - a >= t) { res.push_back({x, b - a}); }
            k = (lng(less) + (b - a) + t) / t * t - 1;}
        return res;}
};
