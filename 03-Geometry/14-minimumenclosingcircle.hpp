#pragma once
#include "../01-Core/01-template.hpp"
#include "../01-Core/07-infint.hpp"
#include "../01-Core/09-rational.hpp"
#include "../06-Miscellaneous/02-customhash.hpp"
#include "01-point.hpp"
#include "02-line_segment.hpp"
#include "06-circle.hpp"

namespace enclosing_detail {
    inline int locate(const point *s, int k, point q) {
        if (k <= 1) { return k ? s[0] != q : 1; }
        if (k == 2) { return geometry_detail::sign(dot(s[0] - q, s[1] - q)); }
        point a = s[0] - q, b = s[1] - q, c = s[2] - q;
        lll x = norm2(a) * cross(b, c), y = norm2(b) * cross(c, a), z = norm2(c) * cross(a, b);
        return -orient(s[0], s[1], s[2]) * (x + y > -z ? 1 : x + y < -z ? -1 : 0);}
    // Welzl's three scans over a linked list; mtf moves each new support point to the front. fit(s, k) sets the circle through
    // positions s[0..k), out(i) tests position i; returns k, or 0 once more than budget tests were made.
    template<typename F, typename O> int welzl(int n, array<int, 3> &s, bool mtf, lng budget, F fit, O out) {
        vector<int> nx(n + 1), pv(n + 1);
        for (int i = 0; i <= n; ++i) { nx[i] = i < n ? i + 1 : 0; pv[nx[i]] = i; }
        auto front = [&](int x) {
            if (!mtf || nx[n] == x) { return; }
            nx[pv[x]] = nx[x]; pv[nx[x]] = pv[x]; nx[x] = nx[n]; pv[nx[n]] = x; nx[n] = x; pv[x] = n;};
        auto miss = [&](int x) { return --budget >= 0 && out(x); };
        int k = 1; s = {0, 0, 0}; fit(s, k);
        for (int i = nx[0]; i != n && budget >= 0;) {
            int ni = nx[i];
            if (miss(i)) {
                s = {i, 0, 0}; k = 1; fit(s, k);
                for (int j = nx[n]; j != i && budget >= 0;) {
                    int nj = nx[j];
                    if (miss(j)) {
                        s = {i, j, 0}; k = 2; fit(s, k);
                        for (int t = nx[n]; t != j && budget >= 0;) {
                            int nt = nx[t];
                            if (miss(t)) { s = {i, j, t}; k = 3; fit(s, k); front(t); }
                            t = nt;}
                        front(j);}
                    j = nj;}
                front(i);}
            i = ni;}
        return budget >= 0 ? k : 0;}
    // Move-to-front within budget tests, then plain scans over fresh shuffles; load() permutes the points by o.
    template<typename L, typename F, typename O> int run(int n, ulng seed, lng budget, vector<int> &o, array<int, 3> &s, L load, F fit, O out) {
        for (int pass = 0;; ++pass) {
            o.resize(n); iota(o.begin(), o.end(), 0);
            for (int i = n - 1; i > 0; --i) { swap(o[i], o[CustomHash::splitMix64(seed++) % ulng(i + 1)]); }
            load();
            if (int k = welzl(n, s, !pass, pass ? LLONG_MAX : budget, fit, out)) { return k; }}}
} // namespace enclosing_detail

// T: O(1) per query, M: O(1). support: 1 to 3 distinct input indices, points their coordinates; the circle is their point, diametral circle or circumcircle.
// locate: -1 inside, 0 on, 1 outside, exact; center and radius2 exact; circle is the rounded CircleApprox.
struct EnclosingCircle {
    vector<int> support;
    vector<point> points;
    CircleApprox circle;

    RationalPoint2 center() const {
        int k = int(points.size()); assert(k >= 1 && k <= 3);
        if (k <= 1) { return k ? RationalPoint2(points[0]) : RationalPoint2(); }
        point a = points[0];
        if (k == 2) { return {lll(a.x) + points[1].x, lll(a.y) + points[1].y, 2}; }
        point u = points[1] - a, v = points[2] - a; lll z = 2 * cross(u, v), s = norm2(u), t = norm2(v);
        return {a.x * z + v.y * s - u.y * t, a.y * z - v.x * s + u.x * t, z};}
    Rational<InfInt> radius2() const {
        int k = int(points.size()); assert(k >= 1 && k <= 3);
        if (k <= 1) { return Rational<InfInt>(InfInt(0)); }
        if (k == 2) { return Rational<InfInt>(InfInt(dist2(points[0], points[1])), InfInt(4)); }
        point u = points[1] - points[0], v = points[2] - points[0]; lll z = cross(u, v);
        return Rational<InfInt>(InfInt(norm2(u)) * InfInt(norm2(v)) * InfInt(dist2(points[1], points[2])), InfInt(4) * InfInt(z * z));}
    int locate(point q) const {
        assert(!points.empty() && points.size() <= 3); linear_detail::check(q);
        return enclosing_detail::locate(points.data(), int(points.size()), q);}
    bool contains(point q) const { return locate(q) <= 0; }
};

// T: O(n) expected, M: O(n). Integral |coordinate| <= 10^9, 1 <= n < INT_MAX, duplicates allowed; seeded Las Vegas (the seed affects only time).
template<typename T> EnclosingCircle minimumEnclosingCircle(const vector<Point2<T>> &p, ulng seed = CustomHash::rnd) {
    static_assert(std::is_integral_v<T>);
    assert(!p.empty() && p.size() < INT_MAX);
    int n = int(p.size()), m = 0; vector<int> o; vector<point> q(n); array<point, 3> c; array<int, 3> s;
    if (!n) { return {}; }
    for (const auto &x : p) { linear_detail::check(x.template cast<lng>()); }
    auto load = [&] { for (int i = 0; i < n; ++i) { q[i] = p[o[i]].template cast<lng>(); } };
    auto fit = [&](const array<int, 3> &t, int k) {
        m = k;
        for (int i = 0; i < k; ++i) { c[i] = q[t[i]]; }
        assert(k < 3 || orient(c[0], c[1], c[2]) != 0);};
    int k = enclosing_detail::run(n, seed, 32 * lng(n) + 64, o, s, load, fit, [&](int i) { return enclosing_detail::locate(c.data(), m, q[i]) > 0; });
    EnclosingCircle res;
    for (int t = 0; t < k; ++t) { res.support.push_back(o[s[t]]); res.points.push_back(q[s[t]]); }
    dpoint a = res.center().approx();
    res.circle = CircleApprox(a, distanceApprox(a, res.points[0].cast<long double>()));
    return res;}

// T: O(n) expected, M: O(n). Floating T, finite, 1 <= n < INT_MAX; seeded as minimumEnclosingCircle; rounded long double arithmetic.
// The radius is the largest rounded distance from the rounded centre to the support, so the support and its duplicates test inside.
template<typename T> CircleApprox minimumEnclosingCircleApprox(const vector<Point2<T>> &p, ulng seed = CustomHash::rnd) {
    static_assert(std::is_floating_point_v<T>);
    assert(!p.empty() && p.size() < INT_MAX);
    int n = int(p.size()); vector<int> o; vector<dpoint> q(n); dpoint c; long double r2 = 0; array<int, 3> s;
    if (!n) { return {}; }
    assert(std::all_of(p.begin(), p.end(), [](const Point2<T> &x) { return std::isfinite(x.x) && std::isfinite(x.y); }));
    auto load = [&] { for (int i = 0; i < n; ++i) { q[i] = p[o[i]].template cast<long double>(); } };
    auto fit = [&](const array<int, 3> &t, int k) {
        dpoint a = q[t[0]], u = q[t[1]] - a, v = q[t[2]] - a;
        c = k == 1 ? a : k == 2 ? (a + q[t[1]]) / 2.0L : a + dpoint{v.y * norm2(u) - u.y * norm2(v), u.x * norm2(v) - v.x * norm2(u)} / (2 * cross(u, v));
        r2 = 0;
        for (int i = 0; i < k; ++i) { r2 = max(r2, dist2(q[t[i]], c)); }};
    enclosing_detail::run(n, seed, 32 * lng(n) + 64, o, s, load, fit, [&](int i) { return dist2(q[i], c) > r2; });
    return CircleApprox(c, std::sqrt(r2));}
