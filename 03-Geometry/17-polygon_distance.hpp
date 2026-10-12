#pragma once
#include "../01-Core/01-template.hpp"
#include "01-point.hpp"
#include "02-line_segment.hpp"
#include "03-polygon.hpp"
#include "07-rotatingcalipers.hpp"
#include "11-minkowskisum.hpp"

// T: O(1), M: O(1). distance >= 0 is rounded; p in the first and q in the second polygon realise it (p == q when they meet).
struct PolygonDistanceApprox {
    long double distance = 0;
    dpoint p{}, q{};
};

namespace polygon_distance_detail {
    // Vertex k of P + (-Q) is a[i] + b[j] with (i, j) = ij[k]; sa, sb are the canonical start offsets in the inputs.
    struct Difference {
        vector<point> a, b, v;
        vector<pair<int, int>> ij;
        int sa = 0, sb = 0;
    };
    template<typename T> Difference difference(const vector<Point2<T>> &p, const vector<Point2<T>> &q) {
        static_assert(std::is_integral_v<T>);
        assert(!p.empty() && !q.empty());
        Difference d;
        for (auto x : p) { d.a.push_back(x.template cast<lng>()); linear_detail::check(d.a.back()); }
        for (auto x : q) { d.b.push_back(-x.template cast<lng>()); linear_detail::check(d.b.back()); }
        d.sa = int(std::min_element(d.a.begin(), d.a.end()) - d.a.begin()); d.sb = int(std::min_element(d.b.begin(), d.b.end()) - d.b.begin());
        d.a = minkowski_detail::canonical(d.a); d.b = minkowski_detail::canonical(d.b);
        d.ij = minkowski_detail::merge(d.a, d.b);
        for (auto [i, j] : d.ij) { d.v.push_back(d.a[i] + d.b[j]); }
        return d;}
    // Index k of a vertex of D, or of an edge v[k] -> v[k + 1] with the origin strictly right; -1 when the origin is in D.
    inline int outside(const Difference &d) {
        int n = int(d.v.size());
        if (n == 1) { return d.v[0] == point{} ? -1 : 0; }
        if (n == 2) { return !orient(d.v[0], d.v[1], point{}) && dot(d.v[0], d.v[1]) <= 0 ? -1 : 0; }
        for (int k = 0; k < n; ++k) { if (orient(d.v[k], d.v[(k + 1) % n], point{}) < 0) { return k; } }
        return -1;}
    inline bool touch(point a, point b, point c, point e) {
        int x = orient(a, b, c), y = orient(a, b, e), z = orient(c, e, a), w = orient(c, e, b);
        Linear2 s = Linear2::segment(a, b), t = Linear2::segment(c, e);
        return (x * y < 0 && z * w < 0) || (!x && contains(s, c)) || (!y && contains(s, e)) || (!z && contains(t, a)) || (!w && contains(t, b));}
    // Closest point of segment a -> b to p as a parameter in [0, 1] and its rounded distance.
    inline pair<long double, long double> nearest(point p, point a, point b) {
        point e = b - a; lll n = dot(p - a, e), d = norm2(e);
        if (d == 0 || n <= 0) { return {0, distanceApprox(p, a)}; }
        if (n >= d) { return {1, distanceApprox(p, b)}; }
        return {static_cast<long double>(n) / static_cast<long double>(d), abs(static_cast<long double>(cross(e, p - a))) / std::sqrt(static_cast<long double>(d))};}
    inline dpoint along(point a, point b, long double t) { return a.cast<long double>() + (b - a).cast<long double>() * t; }
    template<typename T> long double edges(const vector<Point2<T>> &p, Point2<T> q, long double res) {
        assert(p.size() <= INT_MAX); int n = int(p.size());
        for (int i = 0; i < n; ++i) { res = min(res, pointSegmentDistanceApprox(q, p[i], p[i + 1 == n ? 0 : i + 1])); }
        return res;}
} // namespace polygon_distance_detail

// T: O(n + m), M: O(n + m). Strictly convex CCW inputs (any start, a point or two points allowed), integral |coordinate| <= 10^9; exact topology.
template<typename T> PolygonDistanceApprox convexPolygonDistanceApprox(const vector<Point2<T>> &p, const vector<Point2<T>> &q) {
    using namespace polygon_distance_detail;
    Difference d = difference(p, q); int n = int(d.v.size()); PolygonDistanceApprox res;
    auto pa = [&](int k, int l, long double t) { return along(d.a[d.ij[k].first], d.a[d.ij[l].first], t); };
    auto pb = [&](int k, int l, long double t) { return dpoint{} - along(d.b[d.ij[k].second], d.b[d.ij[l].second], t); };
    if (outside(d) < 0) {
        int k = 1;
        while (k + 2 < n && orient(d.v[0], d.v[k + 1], point{}) > 0) { ++k; }
        if (n >= 3) {
            lll w0 = cross(d.v[k], d.v[k + 1]), w1 = cross(d.v[k + 1], d.v[0]), w2 = cross(d.v[0], d.v[k]);
            long double s = static_cast<long double>(w0 + w1 + w2);
            res.p = (d.a[d.ij[0].first].cast<long double>() * static_cast<long double>(w0) + d.a[d.ij[k].first].cast<long double>() * static_cast<long double>(w1)
                   + d.a[d.ij[k + 1].first].cast<long double>() * static_cast<long double>(w2)) / s;}
        else { res.p = pa(0, n - 1, n == 1 ? 0 : nearest(point{}, d.v[0], d.v[1]).first); }
        res.q = res.p;
        return res;}
    res.distance = -1;
    for (int k = 0; k < n; ++k) {
        int l = (k + 1) % n; auto [t, dist] = nearest(point{}, d.v[k], d.v[l]);
        if (res.distance < 0 || dist < res.distance) { res = {dist, pa(k, l, t), pb(k, l, t)}; }}
    return res;}

// T: O(n + m), M: O(n + m). Same domain; true iff the closed polygons are disjoint, then min over P of dot(axis, x) > max over Q.
template<typename T> bool convexSeparated(const vector<Point2<T>> &p, const vector<Point2<T>> &q, point &axis) {
    using namespace polygon_distance_detail;
    Difference d = difference(p, q); int k = outside(d), n = int(d.v.size());
    if (k < 0) { return false; }
    point a = d.v[k], b = d.v[(k + 1) % n], u = n >= 3 || orient(a, b, point{}) ? perp(b - a) : a;
    if (dot(u, a) < 0) { u = -u; }
    axis = u / gcd(abs(u.x), abs(u.y));
    return true;}

// T: O(n + m), M: O(n + m). Same domain; exact farthest pair (first in p, second in q, input indices), lexicographically smallest.
template<typename T> ConvexDiameter convexPolygonMaxDistance(const vector<Point2<T>> &p, const vector<Point2<T>> &q) {
    polygon_distance_detail::Difference d = polygon_distance_detail::difference(p, q); ConvexDiameter res;
    int n = int(d.a.size()), m = int(d.b.size());
    for (int k = 0; k < int(d.v.size()); ++k) {
        int i = (d.ij[k].first + d.sa) % n, j = (d.ij[k].second + d.sb) % m; lll s = norm2(d.v[k]);
        if (res.first < 0 || s > res.squared || (s == res.squared && pair{i, j} < pair{res.first, res.second})) { res = {i, j, s}; }}
    return res;}

// T: O(n + r), M: O(1), n total vertices, r rings (1 for a ring). Nonempty rings; distance to the closed region, 0 inside; exact topology for integral T.
template<typename T> long double pointPolygonDistanceApprox(const vector<Point2<T>> &p, Point2<T> q) {
    assert(!p.empty());
    return polygonContains(p, q) != PolygonLocation::Outside ? 0 : polygon_distance_detail::edges(p, q, distanceApprox(q, p[0]));}
template<typename T> long double pointPolygonDistanceApprox(const vector<vector<Point2<T>>> &rings, Point2<T> q) {
    assert(!rings.empty() && !rings[0].empty());
    if (polygonContains(rings, q) != PolygonLocation::Outside) { return 0; }
    long double res = distanceApprox(q, rings[0][0]);
    for (const auto &p : rings) { res = polygon_distance_detail::edges(p, q, res); }
    return res;}

// T: O(n * m), M: O(1). Simple rings (either orientation, a point or segment allowed), integral |coordinate| <= 10^9; closed regions, touching counts.
template<typename T> bool polygonsIntersect(const vector<Point2<T>> &p, const vector<Point2<T>> &q) {
    static_assert(std::is_integral_v<T>);
    assert(!p.empty() && !q.empty() && p.size() <= INT_MAX && q.size() <= INT_MAX);
    int n = int(p.size()), m = int(q.size());
    for (int i = 0; i < n; ++i) { for (int j = 0; j < m; ++j) {
        if (polygon_distance_detail::touch(p[i].template cast<lng>(), p[(i + 1) % n].template cast<lng>(), q[j].template cast<lng>(), q[(j + 1) % m].template cast<lng>())) { return true; }}}
    return polygonContains(p, q[0]) != PolygonLocation::Outside || polygonContains(q, p[0]) != PolygonLocation::Outside;}

// T: O(n * m), M: O(1). Same domain; rounded distance between the closed regions with witnesses, exact intersection topology.
template<typename T> PolygonDistanceApprox polygonDistanceApprox(const vector<Point2<T>> &p, const vector<Point2<T>> &q) {
    using namespace polygon_distance_detail;
    static_assert(std::is_integral_v<T>);
    assert(!p.empty() && !q.empty() && p.size() <= INT_MAX && q.size() <= INT_MAX);
    int n = int(p.size()), m = int(q.size()); PolygonDistanceApprox res{-1, {}, {}};
    auto at = [](const vector<Point2<T>> &r, int i) { return r[i % r.size()].template cast<lng>(); };
    for (int i = 0; i < n; ++i) { for (int j = 0; j < m; ++j) {
        point a = at(p, i), b = at(p, i + 1), c = at(q, j), e = at(q, j + 1);
        if (touch(a, b, c, e)) {
            auto x = intersect(Linear2::segment(a, b), Linear2::segment(c, e));
            res.p = x.kind == IntersectionKind::Point ? x.p.approx() : x.overlap.a.cast<long double>();
            res.distance = 0; res.q = res.p;
            return res;}
        for (int s = 0; s < 2; ++s) {
            auto [t, dist] = s ? nearest(c, a, b) : nearest(a, c, e);
            if (res.distance < 0 || dist < res.distance) { res = {dist, s ? along(a, b, t) : a.cast<long double>(), s ? c.cast<long double>() : along(c, e, t)}; }}}}
    for (int s = 0; s < 2; ++s) {
        point x = s ? at(p, 0) : at(q, 0);
        if (polygonContains(s ? q : p, s ? p[0] : q[0]) != PolygonLocation::Outside) { return {0, x.cast<long double>(), x.cast<long double>()}; }}
    return res;}
