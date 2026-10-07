#pragma once
#include "../01-Core/01-template.hpp"
#include "01-point.hpp"

// T: O(log(max(|x|, |y|, |d|))) construction, O(1) otherwise; M: O(1). Exact (x/d, y/d) with d > 0 and gcd 1; d != 0 and no component equals the lll minimum.
struct RationalPoint2 {
    lll x = 0, y = 0, d = 1;
    RationalPoint2() = default;
    RationalPoint2(lll x, lll y, lll d = 1) {
        assert(d && x != std::numeric_limits<lll>::min() && y != std::numeric_limits<lll>::min() && d != std::numeric_limits<lll>::min());
        if (d < 0) { x = -x; y = -y; d = -d; }
        lll g = gcd(gcd(x < 0 ? -x : x, y < 0 ? -y : y), d);
        this->x = x / g; this->y = y / g; this->d = d / g;}
    RationalPoint2(point p) : RationalPoint2(p.x, p.y) {}
    dpoint approx() const { return {static_cast<long double>(x) / static_cast<long double>(d), static_cast<long double>(y) / static_cast<long double>(d)}; }
    friend bool operator==(const RationalPoint2 &, const RationalPoint2 &) = default;
};

enum class LinearKind { Segment, Ray, Line };

// T: O(1), M: O(1). Integral endpoints with |coordinate| <= 10^9, all closed; ray from a through b; a == b is a singleton.
struct Linear2 {
    point a{}, b{};
    LinearKind kind = LinearKind::Segment;
    static Linear2 segment(point a, point b) { return {a, b, LinearKind::Segment}; }
    static Linear2 ray(point a, point through) { return {a, through, LinearKind::Ray}; }
    static Linear2 line(point a, point through) { return {a, through, LinearKind::Line}; }
};

enum class IntersectionKind { Empty, Point, Segment, Ray, Line };

// T: O(1), M: O(1). Only the field matching kind is meaningful: p for Point, overlap otherwise.
struct LinearIntersection2 {
    IntersectionKind kind = IntersectionKind::Empty;
    RationalPoint2 p{};
    Linear2 overlap{};
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
        else {
            assert(std::isfinite(p.x) && std::isfinite(p.y));}
        return p.template cast<long double>();}
} // namespace linear_detail

// T: O(1), M: O(1). Exact; zero directions are parallel and orthogonal to every direction.
inline bool contains(Linear2 s, point p) {
    linear_detail::check(s); linear_detail::check(p);
    if (s.a == s.b) { return p == s.a; }
    if (orient(s.a, s.b, p)) { return false; }
    return linear_detail::parameter(s.kind, dot(p - s.a, s.b - s.a), norm2(s.b - s.a));}
inline bool onSegment(point p, point a, point b) { return contains(Linear2::segment(a, b), p); }
inline bool parallel(Linear2 s, Linear2 t) {
    linear_detail::check(s); linear_detail::check(t);
    return cross(s.b - s.a, t.b - t.a) == 0;}
inline bool orthogonal(Linear2 s, Linear2 t) {
    linear_detail::check(s); linear_detail::check(t);
    return dot(s.b - s.a, t.b - t.a) == 0;}
inline bool collinear(Linear2 s, Linear2 t) {
    linear_detail::check(s); linear_detail::check(t);
    if (s.a != s.b) { return !orient(s.a, s.b, t.a) && !orient(s.a, s.b, t.b); }
    return !orient(t.a, t.b, s.a);}

// T: O(log(C)), M: O(1), C = 24 * 10^27 bounds the degree-3 rational numerators. All nine kind pairs; crossings are exact rationals, overlaps reuse input endpoints (segments sorted).
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

// T: O(log(C)), M: O(1), C = max coordinate difference. Supporting line A x + B y = K of s.a != s.b; gcd(A, B) = 1 with B > 0 or (B == 0, A > 0); equal keys iff equal supporting lines.
inline array<lng, 3> canonicalLine(Linear2 s) {
    linear_detail::check(s); assert(s.a != s.b);
    point n = canonicalDirection(perp(s.b - s.a));
    return {n.x, n.y, n.x * s.a.x + n.y * s.a.y};}

// T: O(log(C)), M: O(1), C = max coordinate difference. Lattice endpoints; segments may be singletons.
inline lng latticeOnSegment(point a, point b) {
    linear_detail::check(a); linear_detail::check(b);
    return gcd(b.x - a.x, b.y - a.y) + 1;}
inline lng gridCellsCrossed(point a, point b) {
    linear_detail::check(a); linear_detail::check(b);
    lng x = abs(b.x - a.x), y = abs(b.y - a.y);
    return x + y - gcd(x, y);}

// T: O(1), M: O(1). Rounded long double, no tolerance; |coordinate| <= 10^9 or finite; line endpoints a != b; constructions return a line as two points; left of a line is positive.
template<typename T> dpoint projectLineApprox(Point2<T> p, Point2<T> a, Point2<T> b) {
    dpoint origin = linear_detail::approximate(a), u = linear_detail::approximate(b) - origin;
    dpoint v = linear_detail::approximate(p) - origin;
    long double d = norm2(u); assert(d > 0 && std::isfinite(d));
    return origin + u * (dot(v, u) / d);}
template<typename T> dpoint reflectLineApprox(Point2<T> p, Point2<T> a, Point2<T> b) {
    return projectLineApprox(p, a, b) * 2.L - p.template cast<long double>();}
template<typename T> dpoint reflectDirectionApprox(Point2<T> v, Point2<T> a, Point2<T> b) {
    dpoint u = linear_detail::approximate(b) - linear_detail::approximate(a), w = linear_detail::approximate(v);
    long double d = norm2(u); assert(d > 0 && std::isfinite(d));
    return u * (2 * dot(w, u) / d) - w;}
template<typename T> long double signedLineDistanceApprox(Point2<T> p, Point2<T> a, Point2<T> b) {
    dpoint origin = linear_detail::approximate(a), u = linear_detail::approximate(b) - origin;
    dpoint v = linear_detail::approximate(p) - origin;
    long double d = norm2(u); assert(d > 0 && std::isfinite(d));
    return cross(u, v) / std::sqrt(d);}
template<typename T> long double lineDistanceApprox(Point2<T> a, Point2<T> b, Point2<T> c, Point2<T> d) {
    long double h = abs(signedLineDistanceApprox(c, a, b));
    dpoint u = linear_detail::approximate(b) - linear_detail::approximate(a), v = linear_detail::approximate(d) - linear_detail::approximate(c);
    return cross(u, v) != 0 ? 0 : h;}
template<typename T> array<dpoint, 2> perpendicularBisectorApprox(Point2<T> a, Point2<T> b) {
    dpoint p = linear_detail::approximate(a), q = linear_detail::approximate(b), m = (p + q) / 2.L;
    assert(p != q);
    return {m, m + perp(q - p)};}
template<typename T> array<dpoint, 2> angleBisectorApprox(Point2<T> a, Point2<T> o, Point2<T> b) {
    dpoint p = linear_detail::approximate(o), x = linear_detail::approximate(a) - p, y = linear_detail::approximate(b) - p;
    long double s = normApprox(x), t = normApprox(y); assert(s > 0 && t > 0);
    dpoint u = x / s, v = y / t;
    // Sum of units for angles <= pi/2, else the perpendicular of their well-conditioned difference.
    dpoint w = dot(x, y) >= 0 ? u + v : perp(cross(x, y) >= 0 ? u - v : v - u);
    return {p, p + w * max(s, t)};}

// T: O(1), M: O(1). Singleton segments allowed; perpendicular height avoids projection cancellation.
template<typename T> long double pointSegmentDistanceApprox(Point2<T> p, Point2<T> a, Point2<T> b) {
    dpoint origin = linear_detail::approximate(a), u = linear_detail::approximate(b) - origin;
    dpoint v = linear_detail::approximate(p) - origin;
    long double d = norm2(u), n = dot(v, u); assert(std::isfinite(d) && (d > 0 || a == b));
    if (d == 0 || n <= 0) { return normApprox(v); }
    if (n >= d) { return normApprox(v - u); }
    return abs(cross(u, v)) / std::sqrt(d);}
// T: O(1), M: O(1). Integral endpoints; exact intersection topology, approximate distance.
inline long double segmentDistanceApprox(point a, point b, point c, point d) {
    for (point p : {a, b, c, d}) { linear_detail::check(p); }
    int x = orient(a, b, c), y = orient(a, b, d), z = orient(c, d, a), w = orient(c, d, b);
    if ((x * y < 0 && z * w < 0) || (!x && onSegment(c, a, b)) || (!y && onSegment(d, a, b)) ||
        (!z && onSegment(a, c, d)) || (!w && onSegment(b, c, d))) { return 0; }
    return min({pointSegmentDistanceApprox(a, c, d), pointSegmentDistanceApprox(b, c, d),
                pointSegmentDistanceApprox(c, a, b), pointSegmentDistanceApprox(d, a, b)});}
