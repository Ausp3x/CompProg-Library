#pragma once
#include "../01-Core/01-template.hpp"
#include "01-point.hpp"
#include "04-convexhull.hpp"

namespace minkowski_detail {
    template<typename T> vector<Point2<T>> canonical(vector<Point2<T>> p) {
        static_assert(std::is_integral_v<T>);
        int n = int(p.size()); assert(p.size() <= INT_MAX && (n != 2 || p[0] != p[1]));
        for (int i = 0; i < n && n >= 3; ++i) { assert(orient(p[i], p[(i + 1) % n], p[(i + 2) % n]) > 0); }
        std::rotate(p.begin(), std::min_element(p.begin(), p.end()), p.end());
        return p;}
    template<typename T> int compare(Point2<T> u, Point2<T> v) {
        bool hu = u.x < 0 || (u.x == 0 && u.y < 0), hv = v.x < 0 || (v.x == 0 && v.y < 0);
        if (hu != hv) { return hu ? 1 : -1; }
        auto c = cross(u, v);
        return c > 0 ? -1 : c < 0 ? 1 : 0;}
    template<typename T> vector<pair<int, int>> merge(const vector<Point2<T>> &p, const vector<Point2<T>> &q) {
        int n = int(p.size()), m = int(q.size()), en = n > 1 ? n : 0, em = m > 1 ? m : 0, i = 0, j = 0;
        vector<pair<int, int>> res{{0, 0}};
        while (i < en || j < em) {
            int s = i == en ? 1 : j == em ? -1 : compare(p[(i + 1) % n] - p[i], q[(j + 1) % m] - q[j]);
            i += s <= 0; j += s >= 0;
            if (i < en || j < em) { res.push_back({i % n, j % m}); }}
        return res;}
} // namespace minkowski_detail

// T: O(n + m), M: O(n + m). Strictly convex CCW inputs, any start, sizes >= 0; integral, sums and edge vectors fit T; from the lexicographic minimum.
template<typename T> vector<Point2<T>> minkowskiSum(const vector<Point2<T>> &p, const vector<Point2<T>> &q) {
    if (p.empty() || q.empty()) { return {}; }
    auto a = minkowski_detail::canonical(p), b = minkowski_detail::canonical(q);
    vector<Point2<T>> res;
    for (auto [i, j] : minkowski_detail::merge(a, b)) { res.push_back(a[i] + b[j]); }
    return res;}
template<typename T> vector<Point2<T>> minkowskiDifference(const vector<Point2<T>> &p, vector<Point2<T>> q) {
    for (auto &x : q) { x = -x; }
    return minkowskiSum(p, q);}

// T: O(n * log(k)), M: O(n), n total vertices of k polygons. Same domain; the empty family sums to the origin.
template<typename T> vector<Point2<T>> minkowskiSum(const vector<vector<Point2<T>>> &ps) {
    assert(ps.size() <= INT_MAX);
    auto rec = [&](auto &&rec, int l, int r) -> vector<Point2<T>> {
        if (r - l == 1) { return minkowski_detail::canonical(ps[l]); }
        int mid = std::midpoint(l, r);
        return minkowskiSum(rec(rec, l, mid), rec(rec, mid, r));};
    return ps.empty() ? vector<Point2<T>>{Point2<T>{}} : rec(rec, 0, int(ps.size()));}

// T: O((n + m) * log(n + m)), M: O(n + m). Any point sets; exact convex hull of the sum, convexHull conventions.
template<typename T> vector<Point2<T>> minkowskiSumHull(const vector<Point2<T>> &p, const vector<Point2<T>> &q) {
    return minkowskiSum(convexHull(p), convexHull(q));}
