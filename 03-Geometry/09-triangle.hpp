#pragma once
#include "../01-Core/01-template.hpp"
#include "../01-Core/08-infintmini.hpp"
#include "../01-Core/09-rational.hpp"
#include "01-point.hpp"
#include "02-line_segment.hpp"
#include "06-circle.hpp"
#include "08-coordinate_transform.hpp"

// T: O(1), M: O(1). Exact homogeneous weights for vertices (a,b,c): d > 0, sum(w) == d, not gcd-normalized.
struct Barycentric2 {
    array<lll, 3> w{1, 0, 0};
    lll d = 1;
    array<long double, 3> approx() const {
        long double q = static_cast<long double>(d);
        return {static_cast<long double>(w[0]) / q, static_cast<long double>(w[1]) / q, static_cast<long double>(w[2]) / q};}
};

// T: O(1), M: O(1). Finite vertices, computed signs, no epsilon; bool queries return false on computed-zero area (fromBarycentric: zero weight sum) and leave out unchanged.
// angles: vertex angles at (a, b, c) in [0, pi], distinct vertices asserted.
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
    array<long double, 3> angles() const { return {angleApprox(b - a, c - a), angleApprox(c - b, a - b), angleApprox(a - c, b - c)}; }
    TriangleApprox transform(const Affine2Approx &f) const { return {f.applyPoint(a), f.applyPoint(b), f.applyPoint(c)}; }

    bool barycentric(dpoint p, array<long double, 3> &out) const {
        circle_detail::valid(p); long double d = area2();
        if (d == 0) { return false; }
        out = {cross(b - p, c - p) / d, cross(c - p, a - p) / d, cross(a - p, b - p) / d};
        return true;}
    bool fromBarycentric(array<long double, 3> w, dpoint &out) const {
        for ([[maybe_unused]] long double x : w) { assert(std::isfinite(x)); }
        long double d = w[0] + w[1] + w[2];
        if (d == 0) { return false; }
        out = a + (b - a) * (w[1] / d) + (c - a) * (w[2] / d); return true;}

    bool circumcircleApprox(CircleApprox &out) const { return ::circumcircleApprox(a, b, c, out); }
    bool incircleApprox(CircleApprox &out) const { return ::incircleApprox(a, b, c, out); }
    bool orthocenter(dpoint &out) const {
        CircleApprox q; if (!circumcircleApprox(q)) { return false; }
        out = a + ((b - a) + (c - a) - (q.c - a) * 2.L); return true;}
    bool excircle(int opposite, CircleApprox &out) const {
        assert(0 <= opposite && opposite < 3);
        array<dpoint, 3> p{a, b, c}; dpoint o = p[opposite];
        dpoint u = p[(opposite + 1) % 3] - o, v = p[(opposite + 2) % 3] - o;
        long double s = cross(u, v), w = dot(u, v);
        if (s == 0) { return false; }
        long double x = circle_detail::length(v), y = circle_detail::length(u), z = circle_detail::length(v - u);
        // x + y - z = 2 * g / (x + y + z) with g = x * y + w = s^2 / (x * y - w), cancellation-free by sign of w.
        long double g = w >= 0 ? x * y + w : s / (x * y - w) * s;
        assert(std::isnormal(g)); long double k = (x + y + z) / (2 * g);
        out = CircleApprox(o + (u * x + v * y) * k, std::abs(s) * k); return true;}
    bool ninePointCircle(CircleApprox &out) const {
        CircleApprox q; if (!circumcircleApprox(q)) { return false; }
        dpoint v = q.c - a;
        out = CircleApprox(a + ((b - a) + (c - a) - v) / 2.L, q.r / 2); return true;}
};

// T: O(1), M: O(1); centroid, circumcenter, orthocenter gcd O(log(C)), C <= 1.5 * 10^29; circumradius2 O(1) big-integer words, allocates.
// |coordinate| <= 10^9; zero area makes barycentric return false (out unchanged) and angleKind, circumcenter, circumradius2, orthocenter assert; locate: -1/0/1 interior/boundary/outside (zero area: its edges).
struct Triangle2 {
    point a{}, b{}, c{};
    Triangle2(point a = {}, point b = {}, point c = {}) : a(a), b(b), c(c) {
        linear_detail::check(a); linear_detail::check(b); linear_detail::check(c);}

    lll area2() const { return cross(b - a, c - a); }
    int orient() const { return ::orient(a, b, c); }
    long double areaApprox() const { return std::abs(static_cast<long double>(area2())) / 2; }
    RationalPoint2 centroid() const { return {lll(a.x) + b.x + c.x, lll(a.y) + b.y + c.y, 3}; }
    int angleKind() const {
        assert(area2() != 0);
        lll m = min({dot(b - a, c - a), dot(c - b, a - b), dot(a - c, b - c)});
        return (m > 0) - (m < 0);}
    RationalPoint2 circumcenter() const {
        point u = b - a, v = c - a; lll z = 2 * cross(u, v), s = norm2(u), t = norm2(v);
        assert(z != 0);
        return {a.x * z + v.y * s - u.y * t, a.y * z - v.x * s + u.x * t, z};}
    template<typename I = InfIntMini> Rational<I> circumradius2() const {
        static_assert(!std::is_integral_v<I>, "circumradius2 needs a big integer type");
        lll z = area2(); assert(z != 0);
        return Rational<I>(I(norm2(b - a)) * I(norm2(c - a)) * I(norm2(c - b)), I(4) * I(z * z));}
    RationalPoint2 orthocenter() const {
        RationalPoint2 o = circumcenter();
        return {(lll(a.x) + b.x + c.x) * o.d - 2 * o.x, (lll(a.y) + b.y + c.y) * o.d - 2 * o.y, o.d};}
    bool barycentric(point p, Barycentric2 &out) const {
        linear_detail::check(p); lll d = area2();
        if (d == 0) { return false; }
        array<lll, 3> w{cross(b - p, c - p), cross(c - p, a - p), cross(a - p, b - p)};
        if (d < 0) { d = -d; for (lll &x : w) { x = -x; } }
        out = {w, d}; return true;}
    int locate(point p) const {
        linear_detail::check(p); int o = orient();
        if (!o) { return !contains(Linear2::segment(a, b), p) && !contains(Linear2::segment(b, c), p); }
        int ab = ::orient(a, b, p) * o, bc = ::orient(b, c, p) * o, ca = ::orient(c, a, p) * o;
        return min({ab, bc, ca}) < 0 ? 1 : ab && bc && ca ? -1 : 0;}
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
