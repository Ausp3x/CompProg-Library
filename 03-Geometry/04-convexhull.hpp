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
    template<typename T> bool prepare(vector<Point2<T>> &p, bool keep_collinear) {
        static_assert(std::is_integral_v<T>); assert(p.size() <= INT_MAX);
        sort(p.begin(), p.end()); p.erase(unique(p.begin(), p.end()), p.end());
        return degenerate(p, [](Point2<T> x) { return x; }, keep_collinear);}
    template<typename I, typename F> vector<I> chain(const vector<I> &q, F at, bool keep_collinear) {
        int n = int(q.size()); vector<I> h; h.reserve(n + 1);
        for (int pass = 0; pass < 2; ++pass) {
            int base = int(h.size());
            for (int i = 0; i < n; ++i) {
                const I &x = q[pass ? n - 1 - i : i];
                while (int(h.size()) - base >= 2) {
                    int s = orient(at(h[h.size() - 2]), at(h.back()), at(x));
                    if (s > 0 || (keep_collinear && s == 0)) { break; }
                    h.pop_back();}
                h.push_back(x);}
            h.pop_back();}
        return h;}
} // namespace hull_detail

// T: O(n * log(n)), M: O(n). Integral, n <= INT_MAX; distinct CCW cycle from the lexicographic minimum.
// keep_collinear keeps every distinct boundary point; indices name the first duplicate; Graham is the angular scan.
template<typename T> vector<Point2<T>> convexHull(vector<Point2<T>> p, bool keep_collinear = false) {
    if (hull_detail::prepare(p, keep_collinear)) { return p; }
    return hull_detail::chain(p, [](Point2<T> x) { return x; }, keep_collinear);}
template<typename T> vector<int> convexHullIndices(const vector<Point2<T>> &p, bool keep_collinear = false) {
    static_assert(std::is_integral_v<T>); assert(p.size() <= INT_MAX);
    vector<int> q(p.size()); iota(q.begin(), q.end(), 0);
    sort(q.begin(), q.end(), [&](int i, int j) { return p[i] != p[j] ? p[i] < p[j] : i < j; });
    q.erase(unique(q.begin(), q.end(), [&](int i, int j) { return p[i] == p[j]; }), q.end());
    auto at = [&](int i) { return p[i]; };
    if (hull_detail::degenerate(q, at, keep_collinear)) { return q; }
    return hull_detail::chain(q, at, keep_collinear);}
template<typename T> vector<Point2<T>> convexHullGraham(vector<Point2<T>> p, bool keep_collinear = false) {
    if (hull_detail::prepare(p, keep_collinear)) { return p; }
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
