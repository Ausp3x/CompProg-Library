#pragma once
#include "../01-Core/01-template.hpp"
#include "01-point.hpp"

namespace circle_detail {
    inline void valid(dpoint p) { assert(std::isfinite(p.x) && std::isfinite(p.y)); }
    inline void tolerance(long double e) { assert(std::isfinite(e) && e >= 0); }
    inline long double length(dpoint p) { return std::hypot(p.x, p.y); }
    inline long double chord(long double r, long double x) { return std::sqrt(max(0.L, (r - x) * (r + x))); }
    inline long double angleMinusSin(long double x) {
        if (x >= .5L) { return x - std::sin(x); }
        long double y = x * x;
        return x * y / 6 * (1 - y / 20 * (1 - y / 42 * (1 - y / 72 * (1 - y / 110 * (1 - y / 156 * (1 - y / 210 * (1 - y / 272)))))));}
} // namespace circle_detail

// T: O(1), M: O(1). Finite c and r >= 0 (radius zero is the point c); tolerances are absolute lengths >= 0.
struct CircleApprox {
    dpoint c{};
    long double r{};
    CircleApprox() = default;
    CircleApprox(dpoint c, long double r) : c(c), r(r) {
        circle_detail::valid(c); assert(std::isfinite(r) && r >= 0);}

    int locate(dpoint p, long double tolerance = 0) const {
        circle_detail::valid(p); circle_detail::tolerance(tolerance);
        long double d = circle_detail::length(p - c) - r;
        return d < -tolerance ? -1 : d > tolerance ? 1 : 0;}
    bool onBoundary(dpoint p, long double tolerance = 0) const { return locate(p, tolerance) == 0; }
    bool contains(dpoint p, long double tolerance = 0) const { return locate(p, tolerance) <= 0; }
    bool contains(CircleApprox b, long double tolerance = 0) const {
        circle_detail::tolerance(tolerance);
        return circle_detail::length(b.c - c) + b.r - r <= tolerance;}

    long double area() const { return std::numbers::pi_v<long double> * r * r; }
    long double perimeter() const { return 2 * std::numbers::pi_v<long double> * r; }
    long double arcLength(long double sweep) const {
        assert(std::isfinite(sweep));
        return r * abs(sweep);}
    long double sectorArea(long double sweep) const {
        assert(std::isfinite(sweep));
        return r * r * sweep / 2;}
    long double segmentArea(long double sweep) const {
        assert(std::isfinite(sweep) && sweep >= 0 && sweep <= 2 * std::numbers::pi_v<long double>);
        return r * r / 2 * circle_detail::angleMinusSin(sweep);}
};

// T: O(1), M: O(1). count = 0, 1, 2, or -1 for infinitely many; only points[0..count) are meaningful.
struct CircleIntersectionApprox {
    int count{};
    array<dpoint, 2> points{};
};

// T: O(1), M: O(1). power = |p - c|^2 - r^2; the radical axis is false for concentric circles, else a line with a's side on the left.
inline long double powerApprox(CircleApprox c, dpoint p) {
    circle_detail::valid(p);
    return norm2(p - c.c) - c.r * c.r;}
inline bool radicalAxisApprox(CircleApprox a, CircleApprox b, array<dpoint, 2> &out) {
    dpoint v = b.c - a.c; long double d = norm2(v);
    if (d == 0) { return false; }
    dpoint p = a.c + v * ((d + (a.r - b.r) * (a.r + b.r)) / (2 * d));
    out = {p, p + perp(v)}; return true;}

// T: O(1), M: O(1). Boundary versus the line through a != b or the closed segment [a, b] (a == b allowed); points in a-to-b order.
inline CircleIntersectionApprox circleLineIntersectionApprox(CircleApprox c, dpoint a, dpoint b) {
    circle_detail::valid(a); circle_detail::valid(b); assert(a != b);
    dpoint w = b - a; long double l = norm2(w), z = cross(w, a - c.c), d = c.r * c.r * l - z * z;
    if (d < 0) { return {}; }
    dpoint p = c.c + perp(w) * (z / l);
    if (d == 0) { return {1, {p, {}}}; }
    dpoint v = w * (std::sqrt(d) / l);
    return {2, {p - v, p + v}};}
inline CircleIntersectionApprox circleSegmentIntersectionApprox(CircleApprox c, dpoint a, dpoint b) {
    if (a == b) { return powerApprox(c, a) != 0 ? CircleIntersectionApprox{} : CircleIntersectionApprox{1, {a, {}}}; }
    auto z = circleLineIntersectionApprox(c, a, b);
    dpoint w = b - a; long double b0 = dot(a - c.c, w), c0 = powerApprox(c, a), b1 = dot(b - c.c, w), c1 = powerApprox(c, b);
    bool first = b0 <= 0 && c0 >= 0 && (b1 >= 0 || c1 <= 0), second = (b0 <= 0 || c0 <= 0) && b1 >= 0 && c1 >= 0;
    CircleIntersectionApprox res;
    if (z.count >= 1 && first) { res.points[res.count++] = z.points[0]; }
    if (z.count == 2 && second) { res.points[res.count++] = z.points[1]; }
    return res;}

// T: O(1), M: O(1). Two boundaries (coincident point circles meet once); centers of radius-r circles through a, b; filled-disk overlap area.
inline CircleIntersectionApprox circleIntersectionApprox(CircleApprox a, CircleApprox b) {
    dpoint v = b.c - a.c; long double d = circle_detail::length(v);
    if (d == 0) {
        return a.r != b.r ? CircleIntersectionApprox{} : a.r == 0 ? CircleIntersectionApprox{1, {a.c, {}}} : CircleIntersectionApprox{-1, {}};}
    if (d > a.r + b.r || d < abs(a.r - b.r)) { return {}; }
    long double x = (d + (a.r - b.r) * (a.r + b.r) / d) / 2;
    dpoint u = v / d, p = a.c + u * x;
    if (d == a.r + b.r || d == abs(a.r - b.r)) { return {1, {p, {}}}; }
    dpoint h = perp(u) * circle_detail::chord(a.r, x);
    return {2, {p - h, p + h}};}
inline CircleIntersectionApprox circleCentersApprox(dpoint a, dpoint b, long double r) {
    return circleIntersectionApprox(CircleApprox(a, r), CircleApprox(b, r));}
inline long double diskOverlapAreaApprox(CircleApprox a, CircleApprox b) {
    long double d = circle_detail::length(b.c - a.c);
    if (d >= a.r + b.r) { return 0; }
    if (d <= abs(a.r - b.r)) { return min(a.area(), b.area()); }
    long double x = (d + (a.r - b.r) * (a.r + b.r) / d) / 2;
    long double h = circle_detail::chord(a.r, x);
    return a.segmentArea(2 * std::atan2(h, x)) + b.segmentArea(2 * std::atan2(h, d - x));}

// T: O(1), M: O(1). Exact for integral T, |coordinates|, radii <= 10^18, radii > 0: common tangent count 4..0, -1 if identical.
template<typename T> int circleRelation(Point2<T> a, T ra, Point2<T> b, T rb) {
    static_assert(std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) <= sizeof(lng));
    auto ok = [](T v) { return lll(v) >= -1000000000000000000LL && lll(v) <= 1000000000000000000LL; };
    assert(ra > 0 && rb > 0 && ok(a.x) && ok(a.y) && ok(b.x) && ok(b.y) && ok(ra) && ok(rb));
    lll d = dist2(a, b), s = (lll(ra) + rb) * (lll(ra) + rb), t = (lll(ra) - rb) * (lll(ra) - rb);
    if (d == 0 && t == 0) { return -1; }
    return d > s ? 4 : d == s ? 3 : d > t ? 2 : d == t ? 1 : 0;}

// T: O(1), M: O(1). Contact points a, b and the unit direction of the tangent line.
struct CircleTangentApprox { dpoint a, b, direction; };
// T: O(1), M: O(1). count = 0, 1, 2, or -1 for infinitely many distinct lines.
struct CircleTangentsApprox {
    int count{};
    array<CircleTangentApprox, 2> lines{};
};

// T: O(1), M: O(1). inner: centers on opposite sides; a zero radius is a point; point tangents put the contact on c in .a.
inline CircleTangentsApprox commonTangentsApprox(CircleApprox a, CircleApprox b, bool inner = false) {
    dpoint v = b.c - a.c; long double d = circle_detail::length(v);
    long double rb = inner ? -b.r : b.r, dr = a.r - rb;
    if (d == 0) { return {dr == 0 ? -1 : 0, {}}; }
    if (d < abs(dr)) { return {}; }
    dpoint u = v / d; long double x = dr / d;
    long double h = std::sqrt(max(0.L, (1 - x) * (1 + x)));
    CircleTangentsApprox res;
    res.count = h == 0 || (a.r == 0 && b.r == 0) ? 1 : 2;
    for (int i = 0; i < res.count; ++i) {
        dpoint n = u * x + perp(u) * (i ? h : -h);
        res.lines[i] = {a.c + n * a.r, b.c + n * rb, perp(n)};}
    return res;}
inline CircleTangentsApprox pointTangentsApprox(CircleApprox c, dpoint p) { return commonTangentsApprox(c, CircleApprox(p, 0)); }

// T: O(1), M: O(1). Computed-collinear or repeated vertices return false and leave out unchanged.
inline bool circumcircleApprox(dpoint a, dpoint b, dpoint c, CircleApprox &out) {
    circle_detail::valid(a); circle_detail::valid(b); circle_detail::valid(c);
    dpoint u = b - a, v = c - a; long double z = 2 * cross(u, v);
    if (z == 0) { return false; }
    dpoint w = (perp(v) * -norm2(u) + perp(u) * norm2(v)) / z;
    out = CircleApprox(a + w, circle_detail::length(w)); return true;}
inline bool incircleApprox(dpoint a, dpoint b, dpoint c, CircleApprox &out) {
    circle_detail::valid(a); circle_detail::valid(b); circle_detail::valid(c);
    dpoint u = b - a, v = c - a; long double z = abs(cross(u, v));
    if (z == 0) { return false; }
    long double x = circle_detail::length(v), y = circle_detail::length(u);
    long double p = circle_detail::length(c - b) + x + y;
    out = CircleApprox(a + (u * x + v * y) / p, z / p); return true;}
