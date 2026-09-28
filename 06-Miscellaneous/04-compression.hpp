#pragma once
#include "../01-Core/01-template.hpp"
#include "../02-Data Structures/07-ordered_set.hpp"

// S: O(n * log(n + 1)), U: rebuild O(n * log(n + 1)), Q: O(log(n + 1)), M: O(n).
// n <= INT_MAX input values; T is copyable, Compare is a strict weak ordering.
// IDs are sorted equivalence-class ranks, fixed until assign()/rebuild(); no metric.
// Reuses CoordinateCompression's sorted unique storage v and comparator cmp.
// Comparator-equivalent values share an arbitrary original representative.
template<typename T, typename Compare = std::less<T>>
struct Compression : CoordinateCompression<T, Compare> {
    using Base = CoordinateCompression<T, Compare>;
    using Base::Base;

    void assign(vector<T> input) { this->rebuild(std::move(input)); }
    int lowerBound(const T &x) const { return this->rank(x); }
    int upperBound(const T &x) const { return this->upperRank(x); }
    int id(const T &x) const { return this->index(x); }
    // Q: O(1); references last until assign()/inherited rebuild()/destruction.
    typename vector<T>::const_reference value(int i) const { assert(0 <= i && i < this->size()); return this->v[i]; }

    // Stored POINT ranks in the interval; [l,r) by default. Require !cmp(r,l).
    // Endpoints need not occur in v. All four open/closed choices support l==r.
    pair<int, int> pointRange(const T &l, const T &r, bool left_closed = true, bool right_closed = false) const {
        assert(!this->cmp(r, l));
        int a = left_closed ? lowerBound(l) : upperBound(l);
        int b = right_closed ? upperBound(r) : lowerBound(r);
        return {a, max(a, b)};}
};

// T: O(m * log(m + 1)), M: O(m) returned; m <= INT_MAX/2 valid ordered intervals.
// Collect both endpoints directly (no r+1). Equal/repeated/empty intervals are valid.
// For known [l,r), slab ranks are [id(l),id(r)); physical widths use original values.
template<typename T, typename Compare = std::less<T>>
Compression<T, Compare> compressEndpoints(const vector<pair<T, T>> &intervals, Compare comp = {}) {
    assert(intervals.size() <= INT_MAX / 2);
    vector<T> values; values.reserve(2 * intervals.size());
    for (const auto &[l, r] : intervals) {
        assert(!comp(r, l)); values.push_back(l); values.push_back(r); }
    return Compression<T, Compare>(std::move(values), comp);}

// S: O(1), U: amortized O(log(n + 1)), Q: O(log(n + 1)), M: O(peak n); n <= INT_MAX.
// Append-only FIRST-ENCOUNTER IDs; no sorted-order promise. clear() invalidates IDs.
// Same comparator equivalence as Compression; preserves first representative.
template<typename T, typename Compare = std::less<T>>
struct EncounterCompression {
    map<T, int, Compare> ids;
    vector<T> values;
    explicit EncounterCompression(Compare comp = {}) : ids(comp) {}

    // Q: O(1).
    int size() const { return int(values.size()); }
    int add(const T &x) {
        auto it = ids.lower_bound(x);
        if (it != ids.end() && !ids.key_comp()(x, it->first)) { return it->second; }
        assert(values.size() < INT_MAX);
        int i = size(); values.push_back(x); ids.emplace_hint(it, x, i); return i;}
    int id(const T &x) const { auto it = ids.find(x); return it == ids.end() ? -1 : it->second; }
    // Q: O(1); add() may invalidate references; clear()/destruction invalidate them.
    typename vector<T>::const_reference value(int i) const { assert(0 <= i && i < size()); return values[i]; }
    // U: O(n); retains vector capacity for reuse, resets IDs to start from zero.
    void clear() { ids.clear(); values.clear(); }
};
