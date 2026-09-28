#pragma once
#include "../01-Core/01-template.hpp"
#include "01-point.hpp"

// Approximate long-double geometry: inputs and every intermediate/output must
// be finite; nonzero divisors and geometric distinctions must survive rounding.
// Constructions use computed comparisons, with no hidden epsilon or certified
// topology near degeneracy. Only membership queries accept an absolute length
// tolerance >= 0. No epsilon relation is used for sorting or exact predicates.
namespace circle_detail {
    inline void valid(dpoint p) { assert(std::isfinite(p.x) && std::isfinite(p.y)); }
    inline void tolerance(long double e) { assert(std::isfinite(e) && e >= 0); }
    inline long double length(dpoint p) { return std::hypot(p.x, p.y); }
    inline long double chord(long double r, long double x) {
        return std::sqrt(max(0.L, (r - x) * (r + x))); }
    // Avoid cancellation of x - sin(x) for a small circular segment.
    inline long double angleMinusSin(long double x) {
        if (x >= .01L) { return x - std::sin(x); }
        long double y = x * x;
        return x * y / 6 * (1 - y / 20 * (1 - y / 42 * (1 - y / 72 * (1 - y / 110)))); }
} // namespace circle_detail

// T: O(1), M: O(1). r >= 0; a zero-radius boundary and disk are both {c}.
// Fields may be changed if the constructor's finite/nonnegative domain is kept.
struct CircleApprox {
    dpoint c{};
    long double r{};
    CircleApprox() = default;
    CircleApprox(dpoint c, long double r) : c(c), r(r) {
        circle_detail::valid(c); assert(std::isfinite(r) && r >= 0); }

    // -1 inside, 0 in the closed tolerance band, +1 outside the disk.
    int locate(dpoint p, long double tolerance = 0) const {
        circle_detail::valid(p); circle_detail::tolerance(tolerance);
        long double d = circle_detail::length(p - c) - r;
        return d < -tolerance ? -1 : d > tolerance ? 1 : 0; }
    bool onBoundary(dpoint p, long double tolerance = 0) const { return locate(p, tolerance) == 0; }
    bool contains(dpoint p, long double tolerance = 0) const { return locate(p, tolerance) <= 0; }
    // Closed disk containment; internal tangency is included.
    bool contains(CircleApprox b, long double tolerance = 0) const {
        circle_detail::tolerance(tolerance);
        return circle_detail::length(b.c - c) + b.r - r <= tolerance; }
    long double area() const { return std::numbers::pi_v<long double> * r * r; }
    long double perimeter() const { return 2 * std::numbers::pi_v<long double> * r; }
    // Signed sweep in radians, including multiple revolutions. Arc length is
    // nonnegative; sector area is oriented (negative for clockwise traversal).
    long double arcLength(long double sweep) const {
        assert(std::isfinite(sweep)); return r * std::abs(sweep); }
    long double sectorArea(long double sweep) const {
        assert(std::isfinite(sweep)); return r * r * sweep / 2; }
    // Area between a CCW arc and its chord, 0 <= sweep <= 2*pi (major allowed).
    long double segmentArea(long double sweep) const {
        assert(std::isfinite(sweep) && sweep >= 0 && sweep <= 2 * std::numbers::pi_v<long double>);
        return r * r / 2 * circle_detail::angleMinusSin(sweep); }
};

// T: O(1), M: O(1). count = 0, 1, 2, or -1 for infinitely many points.
// Only points[0..count) are meaningful; no ordering guarantee.
struct CircleIntersectionApprox {
    int count{};
    array<dpoint, 2> points{};
};

// T: O(1), M: O(1). Circle BOUNDARY versus infinite line through distinct a,b.
inline CircleIntersectionApprox circleLineIntersectionApprox(CircleApprox c, dpoint a, dpoint b) {
    circle_detail::valid(a); circle_detail::valid(b); assert(a != b);
    dpoint u = (b - a) / circle_detail::length(b - a), n = perp(u);
    long double z = dot(a - c.c, n), d = std::abs(z);
    if (d > c.r) { return {}; }
    dpoint p = c.c + n * z;
    if (d == c.r) { return {1, {p, {}}}; }
    dpoint v = u * circle_detail::chord(c.r, d);
    return {2, {p - v, p + v}}; }

// T: O(1), M: O(1). Two BOUNDARIES: nested disjoint circles return zero;
// coincident positive-radius circles return -1, coincident points return one.
inline CircleIntersectionApprox circleIntersectionApprox(CircleApprox a, CircleApprox b) {
    dpoint v = b.c - a.c; long double d = circle_detail::length(v);
    if (d == 0) { return a.r != b.r ? CircleIntersectionApprox{} :
        a.r == 0 ? CircleIntersectionApprox{1, {a.c, {}}} : CircleIntersectionApprox{-1, {}}; }
    if (d > a.r + b.r || d < std::abs(a.r - b.r)) { return {}; }
    long double x = (d + (a.r - b.r) * (a.r + b.r) / d) / 2;
    dpoint u = v / d, p = a.c + u * x;
    if (d == a.r + b.r || d == std::abs(a.r - b.r)) { return {1, {p, {}}}; }
    dpoint h = perp(u) * circle_detail::chord(a.r, x);
    return {2, {p - h, p + h}}; }

// T: O(1), M: O(1). Centers of radius-r circles through a and b. A repeated
// point gives infinitely many centers for r > 0, and one center for r == 0.
inline CircleIntersectionApprox circleCentersApprox(dpoint a, dpoint b, long double r) {
    return circleIntersectionApprox(CircleApprox(a, r), CircleApprox(b, r)); }

// T: O(1), M: O(1). Tangency points plus unit direction of the tangent line.
// Direction remains meaningful when the two contact points coincide.
struct CircleTangentApprox { dpoint a, b, direction; };
struct CircleTangentsApprox {
    int count{}; // 0, 1, 2, or -1 for infinitely many distinct lines.
    array<CircleTangentApprox, 2> lines{};
};

// T: O(1), M: O(1). inner=false: centers on the same side; true: opposite.
// A radius-zero circle denotes a point, with all lines through it tangent.
// Inner/outer families therefore coincide if either radius is zero; two
// distinct points yield one line in either family, identical points yield -1.
inline CircleTangentsApprox commonTangentsApprox(CircleApprox a, CircleApprox b, bool inner = false) {
    dpoint v = b.c - a.c; long double d = circle_detail::length(v);
    long double rb = inner ? -b.r : b.r, dr = a.r - rb;
    if (d == 0) { return {dr == 0 ? -1 : 0, {}}; }
    if (d < std::abs(dr)) { return {}; }
    dpoint u = v / d; long double x = dr / d;
    long double h = std::sqrt(max(0.L, (1 - x) * (1 + x)));
    CircleTangentsApprox result;
    result.count = h == 0 || (a.r == 0 && b.r == 0) ? 1 : 2;
    for (int i = 0; i < result.count; ++i) {
        dpoint n = u * x + perp(u) * (i ? h : -h);
        result.lines[i] = {a.c + n * a.r, b.c + n * rb, perp(n)}; }
    return result; }

// T: O(1), M: O(1). Tangents through p, with contact on c in .a and p in .b.
inline CircleTangentsApprox pointTangentsApprox(CircleApprox c, dpoint p) {
    return commonTangentsApprox(c, CircleApprox(p, 0)); }

// T: O(1), M: O(1). Collinear/repeated vertices return false, leaving out
// unchanged. A nearly collinear circumcircle can be arbitrarily ill-conditioned.
inline bool circumcircleApprox(dpoint a, dpoint b, dpoint c, CircleApprox &out) {
    circle_detail::valid(a); circle_detail::valid(b); circle_detail::valid(c);
    dpoint u = b - a, v = c - a; long double z = 2 * cross(u, v);
    if (z == 0) { return false; }
    dpoint w = (perp(v) * -norm2(u) + perp(u) * norm2(v)) / z;
    out = CircleApprox(a + w, circle_detail::length(w)); return true; }

// T: O(1), M: O(1). Unique incircle of a nondegenerate triangle, independent
// of vertex orientation. Degenerate triangles return false, out unchanged.
inline bool incircleApprox(dpoint a, dpoint b, dpoint c, CircleApprox &out) {
    circle_detail::valid(a); circle_detail::valid(b); circle_detail::valid(c);
    dpoint u = b - a, v = c - a; long double z = std::abs(cross(u, v));
    if (z == 0) { return false; }
    long double x = circle_detail::length(v), y = circle_detail::length(u);
    long double p = circle_detail::length(c - b) + x + y;
    out = CircleApprox(a + (u * x + v * y) / p, z / p); return true; }

// T: O(1), M: O(1). Area of intersection of FILLED disks; contained disks
// contribute their entire area. Small caps use a cancellation-safe sine series.
inline long double diskOverlapAreaApprox(CircleApprox a, CircleApprox b) {
    long double d = circle_detail::length(b.c - a.c);
    if (d >= a.r + b.r) { return 0; }
    if (d <= std::abs(a.r - b.r)) { return min(a.area(), b.area()); }
    long double x = (d + (a.r - b.r) * (a.r + b.r) / d) / 2;
    long double h = circle_detail::chord(a.r, x);
    return a.segmentArea(2 * std::atan2(h, x)) + b.segmentArea(2 * std::atan2(h, d - x)); }
