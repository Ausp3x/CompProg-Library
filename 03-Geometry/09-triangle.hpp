#pragma once
#include "../01-Core/01-template.hpp"
#include "01-point.hpp"
#include "02-line_segment.hpp"
#include "06-circle.hpp"

// T: O(1), M: O(1). Exact homogeneous weights for vertices (a,b,c).
// d > 0 and sum(w) == d; not gcd-normalized. Negative weights are allowed.
struct Barycentric2 {
    array<lll, 3> w{1, 0, 0};
    lll d = 1;
    array<long double, 3> approx() const {
        return {static_cast<long double>(w[0]) / d, static_cast<long double>(w[1]) / d, static_cast<long double>(w[2]) / d}; }
};

// Approximate geometry has the same finite-intermediate/computed-sign policy
// as CircleApprox. No hidden epsilon; distinctions and nonzero divisors must
// survive rounding. Nearly collinear triangles can be ill-conditioned.
// Public vertices retain the constructor domain. Failed bool queries leave
// outputs unchanged (including when an output aliases a vertex).
// T: O(1), M: O(1).
struct TriangleApprox {
    dpoint a{}, b{}, c{};
    TriangleApprox(dpoint a = {}, dpoint b = {}, dpoint c = {}) : a(a), b(b), c(c) {
        circle_detail::valid(a); circle_detail::valid(b); circle_detail::valid(c); }

    long double area2() const { return cross(b - a, c - a); } // Signed; CCW positive.
    long double area() const { return std::abs(area2()) / 2; }
    array<long double, 3> sides() const {
        return {circle_detail::length(c - b), circle_detail::length(a - c), circle_detail::length(b - a)}; }
    long double perimeter() const { auto s = sides(); return s[0] + s[1] + s[2]; }
    // The vertex average exists even for a collinear or collapsed triangle.
    dpoint centroid() const { return a + ((b - a) + (c - a)) / 3.L; }

    // Unique normalized signed-area coordinates; false for computed-zero area.
    bool barycentric(dpoint p, array<long double, 3> &out) const {
        circle_detail::valid(p); long double d = area2();
        if (d == 0) { return false; }
        out = {cross(b - p, c - p) / d, cross(c - p, a - p) / d, cross(a - p, b - p) / d};
        return true; }
    // Homogeneous weights, not necessarily summing to one; sum zero is a
    // point at infinity/no affine point, so returns false. Degeneracy is OK.
    bool fromBarycentric(array<long double, 3> w, dpoint &out) const {
        for (long double x : w) { assert(std::isfinite(x)); }
        long double d = w[0] + w[1] + w[2];
        if (d == 0) { return false; }
        out = a + (b - a) * (w[1] / d) + (c - a) * (w[2] / d); return true; }

    // Circle constructors reuse GE02; degenerate triangles have no unique
    // circumcircle or positive incircle. Centers/radii inherit that outcome.
    bool circumcircle(CircleApprox &out) const { return circumcircleApprox(a, b, c, out); }
    bool incircle(CircleApprox &out) const { return incircleApprox(a, b, c, out); }
    bool circumcenter(dpoint &out) const {
        CircleApprox q; if (!circumcircle(q)) { return false; } out = q.c; return true; }
    bool incenter(dpoint &out) const {
        CircleApprox q; if (!incircle(q)) { return false; } out = q.c; return true; }
    bool circumradius(long double &out) const {
        CircleApprox q; if (!circumcircle(q)) { return false; } out = q.r; return true; }
    bool inradius(long double &out) const {
        CircleApprox q; if (!incircle(q)) { return false; } out = q.r; return true; }
    bool orthocenter(dpoint &out) const {
        CircleApprox q; if (!circumcircle(q)) { return false; }
        out = a + ((b - a) + (c - a) - (q.c - a) * 2.L); return true; }

    // Excircle opposite vertex 0,1,2. Its center is also the excenter and r
    // the exradius. The semiperimeter gap must remain strictly positive.
    bool excircle(int opposite, CircleApprox &out) const {
        assert(0 <= opposite && opposite < 3);
        if (area2() == 0) { return false; }
        array<dpoint, 3> p{a, b, c}; dpoint o = p[opposite];
        dpoint u = p[(opposite + 1) % 3] - o, v = p[(opposite + 2) % 3] - o;
        long double x = circle_detail::length(v), y = circle_detail::length(u), z = circle_detail::length(v - u);
        long double d = min(x, y) - (z - max(x, y)); assert(d > 0);
        out = CircleApprox(o + (u * x + v * y) / d, std::abs(cross(u, v)) / d); return true; }
    // Center midway between circumcenter and orthocenter, radius R/2.
    bool ninePointCircle(CircleApprox &out) const {
        CircleApprox q; if (!circumcircle(q)) { return false; }
        dpoint v = q.c - a;
        out = CircleApprox(a + ((b - a) + (c - a) - v) / 2.L, q.r / 2); return true; }
};

// Integral vertices and query points: |coordinate| <= 10^9. Products widen to
// lll before multiplication. Public vertices retain this domain. Collinear
// and repeated vertices have zero area and a vertex-average centroid, but no
// unique barycentric coordinates. Failure leaves out unchanged.
// T: O(1), M: O(1), except centroid's rational normalization below.
struct Triangle2 {
    point a{}, b{}, c{};
    Triangle2(point a = {}, point b = {}, point c = {}) : a(a), b(b), c(c) {
        linear_detail::check(a); linear_detail::check(b); linear_detail::check(c); }

    lll area2() const { return cross(b - a, c - a); } // Signed; CCW positive.
    long double areaApprox() const { return std::abs(static_cast<long double>(area2())) / 2; }
    // T: O(log(C)), C = max(2, |sum(x)|, |sum(y)|) <= 3 * 10^9.
    RationalPoint2 centroid() const { return {lll(a.x) + b.x + c.x, lll(a.y) + b.y + c.y, 3}; }
    bool barycentric(point p, Barycentric2 &out) const {
        linear_detail::check(p); lll d = area2();
        if (d == 0) { return false; }
        array<lll, 3> w{cross(b - p, c - p), cross(c - p, a - p), cross(a - p, b - p)};
        if (d < 0) { d = -d; for (lll &x : w) { x = -x; } }
        out = {w, d}; return true; }
    TriangleApprox approx() const { return {a.cast<long double>(), b.cast<long double>(), c.cast<long double>()}; }
};

// T: O(1), M: O(1). Nonnegative finite side lengths in any order. False for
// an impossible triangle (out unchanged); equality, including zero sides,
// succeeds with area zero. Kahan's parenthesized Heron factors reduce loss for
// thin triangles; exponent accumulation avoids quartic overflow/underflow.
// The four factors must be finite and a positive area must be representable.
// No rescaling by the largest side, which could erase a tiny third side.
inline bool heronAreaApprox(long double a, long double b, long double c, long double &out) {
    assert(std::isfinite(a) && std::isfinite(b) && std::isfinite(c) && a >= 0 && b >= 0 && c >= 0);
    if (a < b) { swap(a, b); } if (b < c) { swap(b, c); } if (a < b) { swap(a, b); }
    long double gap = c - (a - b);
    if (gap < 0) { return false; }
    if (gap == 0) { out = 0; return true; }
    array<long double, 4> f{a + (b + c), gap, c + (a - b), a + (b - c)};
    long double m = 1; int e = 0;
    for (long double x : f) {
        assert(std::isfinite(x) && x > 0); int k;
        m *= std::frexp(x, &k); e += k; }
    if (e % 2) { m *= 2; --e; }
    long double area = std::scalbn(std::sqrt(m), e / 2 - 2);
    assert(std::isfinite(area) && area > 0); out = area; return true; }
