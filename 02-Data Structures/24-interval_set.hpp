#pragma once

#include "../01-Core/01-template.hpp"

// Disjoint half-open integer intervals that merge when they touch; find returns {x, x} when x is uncovered, next returns numeric max when nothing follows.
// S: O(1), U: O(log(n)) amortized, Q: O(log(n)), M: O(n) for n stored intervals
template<typename T = lng>
struct IntervalSet {
    map<T, T> m;
    T total = T(0);

    int count() const { return int(m.size()); }
    T unionLength() const { return total; }
    // T: O(n), M: O(n).
    vector<pair<T, T>> intervals() const { return vector<pair<T, T>>(m.begin(), m.end()); }
    pair<T, T> find(T x) const {
        auto it = m.upper_bound(x);
        if (it == m.begin() || (--it)->second <= x) { return {x, x}; }
        return *it;}
    bool contains(T x) const { return find(x).second != x; }
    bool covers(T l, T r) const { assert(l <= r); return l == r || find(l).second >= r; }
    T mex(T x) const { return find(x).second; }
    T next(T x) const {
        if (contains(x)) { return x; }
        auto it = m.lower_bound(x);
        return it == m.end() ? std::numeric_limits<T>::max() : it->first;}

    T insert(T l, T r) {
        assert(l <= r);
        if (l == r) { return T(0); }
        auto it = m.upper_bound(l);
        if (it != m.begin() && std::prev(it)->second >= l) { --it; }
        T covered = T(0);
        while (it != m.end() && it->first <= r) {
            l = min(l, it->first); r = max(r, it->second);
            covered += it->second - it->first;
            it = m.erase(it);}
        m.emplace_hint(it, l, r);
        total += r - l - covered;
        return r - l - covered;}
    T erase(T l, T r) {
        assert(l <= r);
        if (l == r) { return T(0); }
        auto it = m.upper_bound(l);
        if (it != m.begin() && std::prev(it)->second > l) { --it; }
        T removed = T(0);
        while (it != m.end() && it->first < r) {
            auto [a, b] = *it;
            removed += min(b, r) - max(a, l);
            it = m.erase(it);
            if (a < l) { m.emplace_hint(it, a, l); }
            if (r < b) { m.emplace_hint(it, r, b); break; }}
        total -= removed;
        return removed;}
};

// Piecewise-constant map on [lo, hi); assign merges equal neighbours, apply does not; k is the number of pieces met.
// S: O(1), U: O((k + 1) * log(n)) with k amortized O(1) for assign, Q: O((k + 1) * log(n)), M: O(n) for n pieces
template<typename K, typename V>
struct IntervalMap {
    K lo, hi;
    map<K, V> m;

    IntervalMap(K lo, K hi, V v) : lo(lo), hi(hi) { assert(lo <= hi); if (lo < hi) { m.emplace(lo, std::move(v)); }}

    int count() const { return int(m.size()); }
    K endOf(typename map<K, V>::const_iterator it) const { return std::next(it) == m.end() ? hi : std::next(it)->first; }
    tuple<K, K, V> get(K x) const {
        assert(lo <= x && x < hi);
        auto it = std::prev(m.upper_bound(x));
        return {it->first, endOf(it), it->second};}
    template<typename G>
    void enumerate(K l, K r, G visit) const {
        assert(lo <= l && l <= r && r <= hi);
        for (auto it = l < r ? std::prev(m.upper_bound(l)) : m.end(); it != m.end() && it->first < r; ++it) { visit(max(l, it->first), min(r, endOf(it)), it->second); }}
    template<typename R, typename G>
    R fold(K l, K r, R res, G step) const {
        enumerate(l, r, [&](K a, K b, const V &v) { res = step(std::move(res), a, b, v); });
        return res;}

    typename map<K, V>::iterator split(K x) {
        assert(lo <= x && x <= hi);
        if (x == hi) { return m.end(); }
        auto it = std::prev(m.upper_bound(x));
        return it->first == x ? it : m.emplace_hint(std::next(it), x, it->second);}
    template<typename G>
    void assign(K l, K r, V v, G onRemoved) {
        assert(lo <= l && l <= r && r <= hi);
        if (l == r) { return; }
        auto end = split(r), it = split(l);
        for (auto p = it; p != end; ++p) { onRemoved(p->first, endOf(p), p->second); }
        it = m.erase(it, end);
        it = m.emplace_hint(it, l, std::move(v));
        if (std::next(it) != m.end() && std::next(it)->second == it->second) { m.erase(std::next(it)); }
        if (it != m.begin() && std::prev(it)->second == it->second) { m.erase(it); }}
    void assign(K l, K r, V v) { assign(l, r, std::move(v), [](K, K, const V &) {}); }
    template<typename G>
    void apply(K l, K r, G f) {
        assert(lo <= l && l <= r && r <= hi);
        if (l == r) { return; }
        auto end = split(r);
        for (auto it = split(l); it != end; ++it) { f(it->first, endOf(it), it->second); }}
    void merge(K l, K r) {
        assert(lo <= l && l <= r && r <= hi);
        for (auto it = m.lower_bound(l); it != m.end() && it->first <= r;) {
            if (it != m.begin() && std::prev(it)->second == it->second) { it = m.erase(it); }
            else { ++it; }}}
};
template<typename K, typename V>
using ChthollyTree = IntervalMap<K, V>;
