#pragma once
#include "../01-Core/01-template.hpp"
#include "01-point.hpp"
#include "02-line_segment.hpp"
#include "06-circle.hpp"

// T: O(1), M: O(1). Exact homogeneous weights for vertices (a,b,c): d > 0, sum(w) == d, not gcd-normalized.
struct Barycentric2 {
    array<lll, 3> w{1, 0, 0};
    lll d = 1;
    array<long double, 3> approx() const {
        long double q = static_cast<long double>(d);
        return {static_cast<long double>(w[0]) / q, static_cast<long double>(w[1]) / q, static_cast<long double>(w[2]) / q};}
};

// T: O(1), M: O(1). Finite vertices, computed signs, no epsilon; bool queries return false on computed-zero area and leave out unchanged.
struct TriangleApprox {
    dpoint a{}, b{}, c{};
    TriangleApprox(dpoint a = {}, dpoint b = {}, dpoint c = {}) : a(a), b(b), c(c) {
        circle_detail::valid(a); circle_detail::valid(b); circle_detail::valid(c);}

    long double area2() const { return cross(b - a, c - a); }
    long double area() const { return std::abs(area2()) / 2; }
    array<long double, 3> sides() const {
        return {circle_detail::length(c - b), circle_detail::length(a - c), circle_detail::length(b - a)};}
    long double perimeter() const { auto s = sides(); return s[0] + s[1] + s[2]; }
    dpoint centroid() const { return a + ((b - a) + (c - a)) / 3.L; }

    bool barycentric(dpoint p, array<long double, 3> &out) const {
        circle_detail::valid(p); long double d = area2();
        if (d == 0) { return false; }
        out = {cross(b - p, c - p) / d, cross(c - p, a - p) / d, cross(a - p, b - p) / d};
        return true;}
    bool fromBarycentric(array<long double, 3> w, dpoint &out) const {
        for (long double x : w) { assert(std::isfinite(x)); }
        long double d = w[0] + w[1] + w[2];
        if (d == 0) { return false; }
        out = a + (b - a) * (w[1] / d) + (c - a) * (w[2] / d); return true;}

    bool circumcircle(CircleApprox &out) const { return circumcircleApprox(a, b, c, out); }
    bool incircle(CircleApprox &out) const { return incircleApprox(a, b, c, out); }
    bool circumcenter(dpoint &out) const {
        CircleApprox q; if (!circumcircle(q)) { return false; } out = q.c; return true;}
    bool incenter(dpoint &out) const {
        CircleApprox q; if (!incircle(q)) { return false; } out = q.c; return true;}
    bool circumradius(long double &out) const {
        CircleApprox q; if (!circumcircle(q)) { return false; } out = q.r; return true;}
    bool inradius(long double &out) const {
        CircleApprox q; if (!incircle(q)) { return false; } out = q.r; return true;}
    bool orthocenter(dpoint &out) const {
        CircleApprox q; if (!circumcircle(q)) { return false; }
        out = a + ((b - a) + (c - a) - (q.c - a) * 2.L); return true;}
    bool excircle(int opposite, CircleApprox &out) const {
        assert(0 <= opposite && opposite < 3);
        array<dpoint, 3> p{a, b, c}; dpoint o = p[opposite];
        dpoint u = p[(opposite + 1) % 3] - o, v = p[(opposite + 2) % 3] - o;
        long double s = cross(u, v), w = dot(u, v);
        if (s == 0) { return false; }
        long double x = circle_detail::length(v), y = circle_detail::length(u), z = circle_detail::length(v - u);
        // x + y - z = 2 * g / (x + y + z) with g = x * y + w = s^2 / (x * y - w), cancellation-free by sign of w.
        long double g = w >= 0 ? x * y + w : s / (x * y - w) * s, k = (x + y + z) / (2 * g);
        out = CircleApprox(o + (u * x + v * y) * k, std::abs(s) * k); return true;}
    bool ninePointCircle(CircleApprox &out) const {
        CircleApprox q; if (!circumcircle(q)) { return false; }
        dpoint v = q.c - a;
        out = CircleApprox(a + ((b - a) + (c - a) - v) / 2.L, q.r / 2); return true;}
};

// T: O(1), M: O(1); centroid normalization O(log(C)), C <= 3 * 10^9.
// |coordinate| <= 10^9; zero area makes barycentric return false (out unchanged) and angleKind assert.
struct Triangle2 {
    point a{}, b{}, c{};
    Triangle2(point a = {}, point b = {}, point c = {}) : a(a), b(b), c(c) {
        linear_detail::check(a); linear_detail::check(b); linear_detail::check(c);}

    lll area2() const { return cross(b - a, c - a); }
    long double areaApprox() const { return std::abs(static_cast<long double>(area2())) / 2; }
    RationalPoint2 centroid() const { return {lll(a.x) + b.x + c.x, lll(a.y) + b.y + c.y, 3}; }
    int angleKind() const {
        assert(area2() != 0);
        lll m = min({dot(b - a, c - a), dot(c - b, a - b), dot(a - c, b - c)});
        return (m > 0) - (m < 0);}
    bool barycentric(point p, Barycentric2 &out) const {
        linear_detail::check(p); lll d = area2();
        if (d == 0) { return false; }
        array<lll, 3> w{cross(b - p, c - p), cross(c - p, a - p), cross(a - p, b - p)};
        if (d < 0) { d = -d; for (lll &x : w) { x = -x; } }
        out = {w, d}; return true;}
    TriangleApprox approx() const { return {a.cast<long double>(), b.cast<long double>(), c.cast<long double>()}; }
};

// T: O(1), M: O(1). Finite sides >= 0 in any order; false (out unchanged) for an impossible triangle, area 0 on equality.
inline bool heronAreaApprox(long double a, long double b, long double c, long double &out) {
    assert(std::isfinite(a) && std::isfinite(b) && std::isfinite(c) && a >= 0 && b >= 0 && c >= 0);
    if (a < b) { swap(a, b); } if (b < c) { swap(b, c); } if (a < b) { swap(a, b); }
    long double gap = c - (a - b);
    if (gap < 0) { return false; }
    if (gap == 0) { out = 0; return true; }
    // Kahan's parenthesized factors; frexp keeps the quartic product in range.
    array<long double, 4> f{a + (b + c), gap, c + (a - b), a + (b - c)};
    long double m = 1; int e = 0;
    for (long double x : f) {
        assert(std::isfinite(x) && x > 0); int k;
        m *= std::frexp(x, &k); e += k;}
    if (e % 2) { m *= 2; --e; }
    long double area = std::scalbn(std::sqrt(m), e / 2 - 2);
    assert(std::isfinite(area) && area > 0); out = area; return true;}
