#pragma once
#include "../01-Core/01-template.hpp"
#include "01-point.hpp"
#include "03-polygon.hpp"
#include "06-circle.hpp"

namespace circle_polygon_detail {
    // Signed area of the disk of radius r at the origin intersected with triangle (0, a, b).
    inline long double fan(long double r, dpoint a, dpoint b) {
        dpoint d = b - a; long double dd = norm2(d), r2 = r * r;
        auto sector = [&](dpoint u, dpoint v) { return r2 / 2 * std::atan2(cross(u, v), dot(u, v)); };
        if (dd == 0 || r == 0) { return 0; }
        long double h = cross(a, d), m = -dot(a, d) / dd, g = r2 * dd - h * h;
        if (g <= 0) { return sector(a, b); }
        long double s = std::sqrt(g) / dd, lo = max(0.L, m - s), hi = min(1.L, m + s);
        if (lo >= hi) { return sector(a, b); }
        dpoint u = a + d * lo, v = a + d * hi;
        return sector(a, u) + cross(u, v) / 2 + sector(v, b);}
    template<typename T, typename F> void edges(const vector<Point2<T>> &p, F f) {
        assert(p.size() <= INT_MAX); int n = int(p.size());
        for (int i = 0; i < n; ++i) { f(p[i].template cast<long double>(), p[i + 1 == n ? 0 : i + 1].template cast<long double>()); }}
    template<typename T, typename F> void edges(const vector<vector<Point2<T>>> &rings, F f) {
        for (const auto &p : rings) { edges(p, f); }}
    template<typename T> vector<dpoint> approx(const vector<Point2<T>> &p) {
        vector<dpoint> res;
        for (auto q : p) { res.push_back(q.template cast<long double>()); }
        return res;}
    template<typename T> vector<vector<dpoint>> approx(const vector<vector<Point2<T>>> &rings) {
        vector<vector<dpoint>> res;
        for (const auto &p : rings) { res.push_back(approx(p)); }
        return res;}
} // namespace circle_polygon_detail

// T: O(1), M: O(1). Rounded signed area of the disk intersected with triangle (c.c, a, b), positive when c.c, a, b turn CCW.
template<typename T> long double diskTriangleAreaApprox(CircleApprox c, Point2<T> a, Point2<T> b) {
    return circle_polygon_detail::fan(c.r, a.template cast<long double>() - c.c, b.template cast<long double>() - c.c);}

// T: O(n + r), M: O(1), n total vertices, r rings (1 for a ring). Rounded signed area of disk and closed walk; outer CCW and holes CW for rings.
template<typename T> long double diskPolygonAreaApprox(CircleApprox c, const vector<Point2<T>> &p) {
    long double s = 0;
    circle_polygon_detail::edges(p, [&](dpoint a, dpoint b) { s += circle_polygon_detail::fan(c.r, a - c.c, b - c.c); });
    return s;}
template<typename T> long double diskRingsAreaApprox(CircleApprox c, const vector<vector<Point2<T>>> &rings) {
    long double s = 0;
    for (const auto &p : rings) { s += diskPolygonAreaApprox(c, p); }
    return s;}

// T: O(n * log(n)), M: O(n), n total vertices. r > 0; nonzero winding counts as inside; CCW arcs (begin, end), begin in [-pi, pi), begin < end <= begin + 2 * pi.
template<typename Polygon> vector<pair<long double, long double>> circleArcsInPolygonApprox(CircleApprox c, const Polygon &p) {
    assert(c.r > 0);
    const long double PI = std::numbers::pi_v<long double>, r2 = c.r * c.r;
    // Winding steps from shared vertex statuses (on the circle counts as inside r + eps); touches are zero steps.
    vector<pair<long double, int>> ev;
    auto angle = [](dpoint z) { return std::atan2(z.y, z.x); };
    circle_polygon_detail::edges(p, [&](dpoint a, dpoint b) {
        dpoint A = a - c.c, B = b - c.c, D = B - A; long double dd = norm2(D), na = norm2(A), nb = norm2(B);
        if (dd == 0) { return; }
        if (na == r2) { ev.push_back({angle(A), 0}); }
        long double h = cross(A, D), m = -dot(A, D) / dd, g = r2 * dd - h * h, s = std::sqrt(max(0.L, g)) / dd;
        auto at = [&](long double t) { return angle(A + D * std::clamp(t, 0.L, 1.L)); };
        if (na <= r2 && nb > r2) { ev.push_back({na == r2 && dot(D, A) >= 0 ? angle(A) : at(m + s), 1}); }
        if (na > r2 && nb <= r2) { ev.push_back({nb == r2 && dot(D, B) <= 0 ? angle(B) : at(m - s), -1}); }
        if (na > r2 && nb > r2 && m > 0 && m < 1 && g >= 0) {
            ev.push_back({at(m - s), g > 0 ? -1 : 0});
            if (g > 0) { ev.push_back({at(m + s), 1}); }}});
    sort(ev.begin(), ev.end());
    int m = int(ev.size()); long double ref = 0, gap = -1;
    for (int i = 0; i < m; ++i) {
        long double l = ev[i].first, r = i + 1 < m ? ev[i + 1].first : ev[0].first + 2 * PI;
        if (r - l > gap) { gap = r - l; ref = (l + r) / 2; }}
    int w = polygonWinding(circle_polygon_detail::approx(p), c.c + dpoint{std::cos(ref), std::sin(ref)} * c.r).winding;
    // Sweep from the reference angle; arcs keep the original event angles, and the one open at ref closes last.
    auto rel = [&](long double x) { return x > ref ? x - ref : x - ref + 2 * PI; };
    vector<tuple<long double, long double, int>> order;
    for (auto [x, d] : ev) { order.push_back({rel(x), x, d}); }
    sort(order.begin(), order.end());
    vector<pair<long double, long double>> res;
    bool open = w != 0; int slot = open ? -2 : -1; long double from = 0;
    for (int i = 0; i < m; ++i) {
        auto [t, x, d] = order[i]; w += d;
        if (i + 1 < m && std::get<1>(order[i + 1]) == x) { continue; }
        if (w && !open) { open = true; from = x; }
        if (!w && open) {
            if (slot == -2) { slot = int(res.size()); }
            res.push_back({from, x}); open = false;}}
    if (slot == -2) { return {{-PI, PI}}; }
    if (slot >= 0) { res[slot].first = from; }
    for (auto &[l, r] : res) {
        if (r <= l) { r += 2 * PI; }
        if (l == PI) { l -= 2 * PI; r -= 2 * PI; }}
    sort(res.begin(), res.end());
    return res;}
