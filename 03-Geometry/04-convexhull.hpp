#pragma once
#include "../01-Core/01-template.hpp"
#include "01-point.hpp"

namespace hull_detail {
    template<typename I, typename F> bool degenerate(vector<I> &q, F at, bool keep_collinear) {
        if (q.size() <= 2) { return true; }
        for (const I &x : q) {
            if (orient(at(q.front()), at(q.back()), at(x))) { return false; }}
        if (!keep_collinear) { q = {q.front(), q.back()}; }
        return true;}
    template<typename T> void dedupe(vector<Point2<T>> &p) {
        static_assert(std::is_integral_v<T>); assert(p.size() <= INT_MAX && std::is_sorted(p.begin(), p.end()));
        p.erase(unique(p.begin(), p.end()), p.end());}
    template<typename T> vector<int> order(const vector<Point2<T>> &p) {
        static_assert(std::is_integral_v<T>); assert(p.size() <= INT_MAX);
        vector<int> q(p.size()); iota(q.begin(), q.end(), 0);
        sort(q.begin(), q.end(), [&](int i, int j) { return p[i] != p[j] ? p[i] < p[j] : i < j; });
        q.erase(unique(q.begin(), q.end(), [&](int i, int j) { return p[i] == p[j]; }), q.end());
        return q;}
    template<typename I, typename F> void chain(const vector<I> &q, F at, bool keep_collinear, bool upper, vector<I> &h) {
        int n = int(q.size()), base = int(h.size());
        for (int i = 0; i < n; ++i) {
            const I &x = q[upper ? n - 1 - i : i];
            while (int(h.size()) - base >= 2) {
                int s = orient(at(h[h.size() - 2]), at(h.back()), at(x));
                if (s > 0 || (keep_collinear && s == 0)) { break; }
                h.pop_back();}
            h.push_back(x);}}
    template<typename I, typename F> vector<I> cycle(vector<I> q, F at, bool keep_collinear) {
        if (degenerate(q, at, keep_collinear)) { return q; }
        vector<I> h; h.reserve(q.size() + 1);
        chain(q, at, keep_collinear, false, h); h.pop_back();
        chain(q, at, keep_collinear, true, h); h.pop_back();
        return h;}
    template<typename I, typename F> vector<I> side(const vector<I> &q, F at, bool keep_collinear, bool upper) {
        vector<I> h; h.reserve(q.size());
        chain(q, at, keep_collinear, upper, h);
        if (upper) { reverse(h.begin(), h.end()); }
        return h;}
    template<typename T> bool boundary(const vector<Point2<T>> &h, Point2<T> x) {
        int lo = 1, hi = int(h.size()) - 1;
        if (orient(h[0], h[lo], x) <= 0 || orient(h[0], h[hi], x) >= 0) { return true; }
        while (hi - lo > 1) {
            int mid = std::midpoint(lo, hi);
            (orient(h[0], h[mid], x) > 0 ? lo : hi) = mid;}
        return orient(h[lo], h[hi], x) == 0;}
} // namespace hull_detail

// T: O(n), M: O(n). As convexHull on lexicographically nondecreasing input (asserted); duplicates allowed.
template<typename T> vector<Point2<T>> convexHullSorted(vector<Point2<T>> p, bool keep_collinear = false) {
    hull_detail::dedupe(p);
    return hull_detail::cycle(std::move(p), std::identity{}, keep_collinear);}
// T: O(n * log(n)), M: O(n). Integral, n <= INT_MAX; distinct CCW cycle from the lexicographic minimum.
// keep_collinear keeps every distinct boundary point; indices name the first duplicate; Graham is the angular scan.
template<typename T> vector<Point2<T>> convexHull(vector<Point2<T>> p, bool keep_collinear = false) {
    sort(p.begin(), p.end());
    return convexHullSorted(std::move(p), keep_collinear);}
template<typename T> vector<int> convexHullIndices(const vector<Point2<T>> &p, bool keep_collinear = false) {
    return hull_detail::cycle(hull_detail::order(p), [&](int i) { return p[i]; }, keep_collinear);}
template<typename T> vector<Point2<T>> convexHullGraham(vector<Point2<T>> p, bool keep_collinear = false) {
    sort(p.begin(), p.end()); hull_detail::dedupe(p);
    if (hull_detail::degenerate(p, std::identity{}, keep_collinear)) { return p; }
    auto o = p.front();
    sort(p.begin() + 1, p.end(), [o](auto a, auto b) {
        int s = orient(o, a, b);
        return s ? s > 0 : dist2(o, a) < dist2(o, b);});
    if (keep_collinear) {
        int i = int(p.size()) - 1;
        while (i > 1 && orient(o, p[i - 1], p.back()) == 0) { --i; }
        reverse(p.begin() + i, p.end());}
    vector<Point2<T>> h; h.reserve(p.size());
    for (const auto &q : p) {
        while (h.size() >= 2) {
            int s = orient(h[h.size() - 2], h.back(), q);
            if (s > 0 || (keep_collinear && s == 0)) { break; }
            h.pop_back();}
        h.push_back(q);}
    return h;}

// T: O(n * log(n)), O(n) for the Sorted forms (nondecreasing input asserted, duplicates allowed), M: O(n). Integral, n <= INT_MAX; distinct points (first-occurrence indices) left to right, both lexicographic extremes included.
template<typename T> vector<Point2<T>> lowerHullSorted(vector<Point2<T>> p, bool keep_collinear = false) {
    hull_detail::dedupe(p);
    return hull_detail::side(p, std::identity{}, keep_collinear, false);}
template<typename T> vector<Point2<T>> upperHullSorted(vector<Point2<T>> p, bool keep_collinear = false) {
    hull_detail::dedupe(p);
    return hull_detail::side(p, std::identity{}, keep_collinear, true);}
template<typename T> vector<Point2<T>> lowerHull(vector<Point2<T>> p, bool keep_collinear = false) {
    sort(p.begin(), p.end());
    return lowerHullSorted(std::move(p), keep_collinear);}
template<typename T> vector<Point2<T>> upperHull(vector<Point2<T>> p, bool keep_collinear = false) {
    sort(p.begin(), p.end());
    return upperHullSorted(std::move(p), keep_collinear);}
template<typename T> vector<int> lowerHullIndices(const vector<Point2<T>> &p, bool keep_collinear = false) {
    return hull_detail::side(hull_detail::order(p), [&](int i) { return p[i]; }, keep_collinear, false);}
template<typename T> vector<int> upperHullIndices(const vector<Point2<T>> &p, bool keep_collinear = false) {
    return hull_detail::side(hull_detail::order(p), [&](int i) { return p[i]; }, keep_collinear, true);}

// T: O(n), keep_collinear O(n * log(out) + k * log(k)) for k boundary points, M: O(n). Integral, n <= INT_MAX / 2; a simple polyline or polygon ring in order without a closing repeat, equal neighbours allowed; output as convexHull(p, keep_collinear).
template<typename T> vector<Point2<T>> convexHullMelkman(const vector<Point2<T>> &p, bool keep_collinear = false) {
    static_assert(std::is_integral_v<T>); assert(p.size() <= INT_MAX / 2);
    int n = int(p.size()), i = 1;
    if (n == 0) { return {}; }
    // A simple polyline runs monotonically along its collinear prefix, so p[0] and its last point b span it.
    Point2<T> a = p[0], b = p[0];
    for (; i < n; ++i) {
        if (orient(a, b, p[i])) { break; }
        b = p[i];}
    if (i == n) { return keep_collinear ? convexHull(p, true) : a == b ? vector{a} : vector{min(a, b), max(a, b)}; }
    if (orient(a, b, p[i]) < 0) { swap(a, b); }
    vector<Point2<T>> d(2 * n + 1);
    int bot = n, top = n + 3;
    d[bot] = d[top] = p[i]; d[bot + 1] = a; d[bot + 2] = b;
    for (++i; i < n; ++i) {
        Point2<T> x = p[i];
        if (orient(d[bot], d[bot + 1], x) > 0 && orient(d[top - 1], d[top], x) > 0) { continue; }
        while (bot + 1 < top && orient(d[bot], d[bot + 1], x) <= 0) { ++bot; }
        d[--bot] = x;
        while (top - 1 > bot && orient(d[top - 1], d[top], x) <= 0) { --top; }
        d[++top] = x;}
    // The deque is a weakly convex CCW cycle; from its lexicographic minimum one chain pass drops collinear vertices.
    vector<Point2<T>> c(d.begin() + bot, d.begin() + top), h;
    std::rotate(c.begin(), std::min_element(c.begin(), c.end()), c.end());
    hull_detail::chain(c, std::identity{}, false, false, h);
    while (h.size() >= 3 && orient(h[h.size() - 2], h.back(), h[0]) <= 0) { h.pop_back(); }
    if (!keep_collinear) { return h; }
    vector<Point2<T>> on;
    for (const auto &x : p) {
        if (hull_detail::boundary(h, x)) { on.push_back(x); }}
    return convexHull(std::move(on), true);}

// T: O(n * out), M: O(out). Integral, n <= INT_MAX; output as convexHull(p, keep_collinear).
template<typename T> vector<Point2<T>> convexHullJarvis(const vector<Point2<T>> &p, bool keep_collinear = false) {
    static_assert(std::is_integral_v<T>); assert(p.size() <= INT_MAX);
    if (p.empty()) { return {}; }
    using W = geometry_detail::Wide<T>;
    Point2<T> s = *std::min_element(p.begin(), p.end()), a = s, prev = s;
    vector<Point2<T>> h;
    do {
        h.push_back(a);
        Point2<T> b = a;
        for (const auto &c : p) {
            // Points on the incoming edge's line behind a are already visited; the rest lie in a cone below pi.
            if (c == a || (!orient(prev, a, c) && dot(a.template cast<W>() - prev.template cast<W>(), c.template cast<W>() - a.template cast<W>()) < 0)) { continue; }
            int o = b == a ? -1 : orient(a, b, c);
            if (o < 0 || (o == 0 && (keep_collinear ? dist2(a, c) < dist2(a, b) : dist2(a, b) < dist2(a, c)))) { b = c; }}
        prev = a; a = b;} while (a != prev && a != s && h.size() < p.size());
    return h;}
