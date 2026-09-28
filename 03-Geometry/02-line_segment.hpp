#pragma once
#include "../01-Core/01-template.hpp"
#include "01-point.hpp"

// Exact homogeneous (x/d, y/d), gcd-normalized with d>0; no rational ordering.
// Inputs exclude the minimum lll value; d!=0. Public fields retain this invariant.
// T: O(log(max(|x|, |y|, |d|))) construction, O(1) otherwise; M: O(1).
struct RationalPoint2 {
    lll x = 0, y = 0, d = 1;
    RationalPoint2() = default;
    RationalPoint2(lll X, lll Y, lll D = 1): x(X), y(Y), d(D) {
        assert(d && x != std::numeric_limits<lll>::min() && y != std::numeric_limits<lll>::min() && d != std::numeric_limits<lll>::min());
        if (d < 0) { x = -x; y = -y; d = -d; }
        lll g = std::gcd(std::gcd(x < 0 ? -x : x, y < 0 ? -y : y), d);
        x /= g; y /= g; d /= g;}
    RationalPoint2(point p): RationalPoint2(p.x, p.y) {}
    dpoint approx() const { return {static_cast<long double>(x) / d, static_cast<long double>(y) / d}; }
    friend bool operator==(const RationalPoint2 &, const RationalPoint2 &) = default;
};

enum class LinearKind { Segment, Ray, Line };

// Original integral endpoints have |coordinate|<=1e9. Ray uses a as its origin
// and b as a point in its forward direction; all endpoints are closed.
// a==b is a singleton for every kind, including line/ray.
// T: O(1), M: O(1).
struct Linear2 {
    point a{}, b{};
    LinearKind kind = LinearKind::Segment;
    static Linear2 segment(point a, point b) { return {a, b, LinearKind::Segment}; }
    static Linear2 ray(point a, point through) { return {a, through, LinearKind::Ray}; }
    static Linear2 line(point a, point through) { return {a, through, LinearKind::Line}; }
};

enum class IntersectionKind { Empty, Point, Segment, Ray, Line };
struct LinearIntersection2 {
    IntersectionKind kind = IntersectionKind::Empty;
    RationalPoint2 p{}; // Valid exactly for Point.
    Linear2 overlap{}; // Valid for Segment/Ray/Line; segment endpoints sorted.
};

namespace linear_detail {
    inline void check(point p) { assert(-1000000000 <= p.x && p.x <= 1000000000 && -1000000000 <= p.y && p.y <= 1000000000); }
    inline void check(Linear2 s) {
        check(s.a); check(s.b);
        assert(s.kind == LinearKind::Segment || s.kind == LinearKind::Ray || s.kind == LinearKind::Line);}
    inline bool parameter(LinearKind kind, lll n, lll d) {
        return kind == LinearKind::Line || (n >= 0 && (kind == LinearKind::Ray || n <= d));}
    template<typename T> dpoint approximate(Point2<T> p) {
        if constexpr (std::is_integral_v<T>) {
            assert(-1000000000 <= p.x && p.x <= 1000000000 && -1000000000 <= p.y && p.y <= 1000000000);}
        else { assert(std::isfinite(p.x) && std::isfinite(p.y)); }
        return p.template cast<long double>();}
} // namespace linear_detail

// Exact membership; includes singleton degeneracies. T: O(1), M: O(1).
inline bool contains(Linear2 s, point p) {
    linear_detail::check(s); linear_detail::check(p);
    if (s.a == s.b) { return p == s.a; }
    if (orient(s.a, s.b, p)) { return false; }
    return linear_detail::parameter(s.kind, dot(p - s.a, s.b - s.a), norm2(s.b - s.a));}
inline bool onSegment(point p, point a, point b) { return contains(Linear2::segment(a, b), p); }

// Zero directions are parallel to every direction; collinear means that all
// endpoints lie on a common line (two singleton objects are always collinear).
// T: O(1), M: O(1).
inline bool parallel(Linear2 s, Linear2 t) {
    linear_detail::check(s); linear_detail::check(t); return cross(s.b - s.a, t.b - t.a) == 0;}
inline bool collinear(Linear2 s, Linear2 t) {
    linear_detail::check(s); linear_detail::check(t);
    if (s.a != s.b) { return !orient(s.a, s.b, t.a) && !orient(s.a, s.b, t.b); }
    return !orient(t.a, t.b, s.a);}

// All nine line/segment/ray combinations. Nonparallel intersections are exact
// rational points. Predicates use degree-2 products; point numerators degree-3
// (<=24 * 1e27 on the input domain), fitting lll. Never multiply rationals.
// Overlaps reuse original integral endpoints, so they remain valid Linear2s.
// T: O(log(C)) for rational normalization, O(1) otherwise; M: O(1), C<=24 * 1e27.
inline LinearIntersection2 intersect(Linear2 s, Linear2 t) {
    linear_detail::check(s); linear_detail::check(t);
    if (s.a == s.b) { return contains(t, s.a) ? LinearIntersection2{IntersectionKind::Point, s.a, {}} : LinearIntersection2{}; }
    if (t.a == t.b) { return intersect(t, s); }
    point u = s.b - s.a, v = t.b - t.a, q = t.a - s.a;
    lll d = cross(u, v), n = cross(q, v), m = cross(q, u);
    if (d) {
        if (d < 0) { d = -d; n = -n; m = -m; }
        if (!linear_detail::parameter(s.kind, n, d) || !linear_detail::parameter(t.kind, m, d)) { return {}; }
        return {IntersectionKind::Point, {lll(s.a.x) * d + lll(u.x) * n, lll(s.a.y) * d + lll(u.y) * n, d}, {}};}
    if (m) { return {}; }
    point lo{}, hi{}; bool has_lo = false, has_hi = false;
    for (Linear2 z : {s, t}) {
        if (z.kind == LinearKind::Line) { continue; }
        if (z.kind == LinearKind::Segment || z.a < z.b) {
            point p = z.kind == LinearKind::Segment ? min(z.a, z.b) : z.a;
            if (!has_lo || lo < p) { lo = p; has_lo = true; }}
        if (z.kind == LinearKind::Segment || z.b < z.a) {
            point p = z.kind == LinearKind::Segment ? max(z.a, z.b) : z.a;
            if (!has_hi || p < hi) { hi = p; has_hi = true; }}}
    if (has_lo && has_hi) {
        if (hi < lo) { return {}; }
        if (lo == hi) { return {IntersectionKind::Point, lo, {}}; }
        return {IntersectionKind::Segment, {}, Linear2::segment(lo, hi)};}
    if (!has_lo && !has_hi) { return {IntersectionKind::Line, {}, s}; }
    point origin = has_lo ? lo : hi;
    Linear2 ray = s.kind == LinearKind::Ray && s.a == origin ? s : t;
    return {IntersectionKind::Ray, {}, ray};}

// Approximate metric/construction APIs use ordinary long-double arithmetic:
// no tolerance-based topology or certified error bound. Coordinates and all
// arithmetic intermediates/results must be finite; line endpoints differ and
// direction norm squared must be nonzero (also excludes squared underflow).
// Integral inputs obey the same |coordinate|<=1e9 domain. T: O(1), M: O(1).
template<typename T>
dpoint projectLineApprox(Point2<T> p, Point2<T> a, Point2<T> b) {
    dpoint origin = linear_detail::approximate(a), u = linear_detail::approximate(b) - origin;
    dpoint v = linear_detail::approximate(p) - origin;
    long double d = norm2(u); assert(d > 0 && std::isfinite(d));
    return origin + u * (dot(v, u) / d);}
template<typename T>
dpoint reflectLineApprox(Point2<T> p, Point2<T> a, Point2<T> b) {
    return projectLineApprox(p, a, b) * 2.L - dpoint{static_cast<long double>(p.x), static_cast<long double>(p.y)};}
template<typename T>
long double signedLineDistanceApprox(Point2<T> p, Point2<T> a, Point2<T> b) {
    dpoint origin = linear_detail::approximate(a), u = linear_detail::approximate(b) - origin;
    dpoint v = linear_detail::approximate(p) - origin;
    long double d = norm2(u); assert(d > 0 && std::isfinite(d));
    return cross(u, v) / std::sqrt(d);}

// Singleton segments allowed. Interior distance uses perpendicular height,
// avoiding subtraction of nearly equal projected coordinates.
// T: O(1), M: O(1).
template<typename T>
long double pointSegmentDistanceApprox(Point2<T> p, Point2<T> a, Point2<T> b) {
    dpoint origin = linear_detail::approximate(a), u = linear_detail::approximate(b) - origin;
    dpoint v = linear_detail::approximate(p) - origin;
    long double d = norm2(u), n = dot(v, u); assert(std::isfinite(d) && (d > 0 || a == b));
    if (!d || n <= 0) { return normApprox(v); }
    if (n >= d) { return normApprox(v - u); }
    return std::abs(cross(u, v)) / std::sqrt(d);}

// Integral endpoints; exact intersection topology, approximate distance.
// T: O(1), M: O(1).
inline long double segmentDistanceApprox(point a, point b, point c, point d) {
    for (point p : {a, b, c, d}) { linear_detail::check(p); }
    int x = orient(a, b, c), y = orient(a, b, d), z = orient(c, d, a), w = orient(c, d, b);
    if ((x * y < 0 && z * w < 0) || (!x && onSegment(c, a, b)) || (!y && onSegment(d, a, b)) ||
        (!z && onSegment(a, c, d)) || (!w && onSegment(b, c, d))) { return 0; }
    return min({pointSegmentDistanceApprox(a, c, d), pointSegmentDistanceApprox(b, c, d),
                pointSegmentDistanceApprox(c, a, b), pointSegmentDistanceApprox(d, a, b)});}
