#pragma once
#include "../01-Core/01-template.hpp"
#include "../06-Miscellaneous/02-customhash.hpp"
#include "01-point.hpp"
#include "02-line_segment.hpp"
#include "06-circle.hpp"

namespace enclosing_detail {
    inline int locate(const point *s, int k, point q) {
        if (k == 1) { return s[0] != q; }
        if (k == 2) { return geometry_detail::sign(dot(s[0] - q, s[1] - q)); }
        point a = s[0] - q, b = s[1] - q, c = s[2] - q;
        lll x = norm2(a) * cross(b, c), y = norm2(b) * cross(c, a), z = norm2(c) * cross(a, b);
        return -orient(s[0], s[1], s[2]) * (x + y > -z ? 1 : x + y < -z ? -1 : 0);}
} // namespace enclosing_detail

// T: O(1) per query, M: O(1). support: 1 to 3 distinct input indices, points their coordinates; the circle is their point, diametral circle or circumcircle.
// locate: -1 inside, 0 on, 1 outside, exact; center exact; circle is the rounded CircleApprox.
struct EnclosingCircle {
    vector<int> support;
    vector<point> points;
    CircleApprox circle;

    RationalPoint2 center() const {
        int k = int(points.size()); assert(k >= 1 && k <= 3);
        point a = points[0];
        if (k == 1) { return a; }
        if (k == 2) { return {lll(a.x) + points[1].x, lll(a.y) + points[1].y, 2}; }
        point u = points[1] - a, v = points[2] - a; lll z = 2 * cross(u, v), s = norm2(u), t = norm2(v);
        return {a.x * z + v.y * s - u.y * t, a.y * z - v.x * s + u.x * t, z};}
    int locate(point q) const {
        assert(!points.empty() && points.size() <= 3); linear_detail::check(q);
        return enclosing_detail::locate(points.data(), int(points.size()), q);}
    bool contains(point q) const { return locate(q) <= 0; }
};

// T: O(n) expected, M: O(n). Integral |coordinate| <= 10^9, 1 <= n <= INT_MAX, duplicates allowed; seeded Las Vegas (the seed affects only time).
// Randomized incremental form of Welzl's algorithm: three nested scans over a seeded shuffle.
template<typename T> EnclosingCircle minimumEnclosingCircle(const vector<Point2<T>> &p, ulng seed = CustomHash::rnd) {
    static_assert(std::is_integral_v<T>);
    assert(!p.empty() && p.size() <= INT_MAX);
    int n = int(p.size()); vector<int> o(n); vector<point> q(n);
    if (!n) { return {}; }
    iota(o.begin(), o.end(), 0);
    for (int i = n - 1; i > 0; --i) { swap(o[i], o[CustomHash::splitMix64(seed++) % ulng(i + 1)]); }
    for (int i = 0; i < n; ++i) { q[i] = p[o[i]].template cast<lng>(); linear_detail::check(q[i]); }
    array<int, 3> s{0, 0, 0}; array<point, 3> c{q[0], {}, {}}; int k = 1;
    for (int i = 1; i < n; ++i) {
        if (enclosing_detail::locate(c.data(), k, q[i]) <= 0) { continue; }
        s[0] = i; c[0] = q[i]; k = 1;
        for (int j = 0; j < i; ++j) {
            if (enclosing_detail::locate(c.data(), k, q[j]) <= 0) { continue; }
            s = {i, j, 0}; c = {q[i], q[j], {}}; k = 2;
            for (int t = 0; t < j; ++t) {
                if (enclosing_detail::locate(c.data(), k, q[t]) <= 0) { continue; }
                s = {i, j, t}; c = {q[i], q[j], q[t]}; k = 3;
                assert(orient(q[i], q[j], q[t]) != 0);}}}
    EnclosingCircle res;
    for (int t = 0; t < k; ++t) { res.support.push_back(o[s[t]]); res.points.push_back(c[t]); }
    dpoint m = res.center().approx();
    res.circle = CircleApprox(m, distanceApprox(m, c[0].cast<long double>()));
    return res;}
