#pragma once
#include "../01-Core/01-template.hpp"
#include "01-point.hpp"
#include "02-line_segment.hpp"

// Rings have no repeated closing vertex; n and total ring vertices <= INT_MAX.
// Integral moments are exact when their intermediates fit lll; |coordinate| <=
// 10^9 suffices even at this size bound. Floating arithmetic is approximate,
// with finite inputs/intermediates and computed signs, never epsilon ordering.
// Area/moments/winding also apply algebraically to arbitrary closed walks.
// Geometric area/centroid interpretation requires a simple polygon. Convexity
// assumes simplicity; Pick additionally requires nonzero area. Simplicity
// validation is owned by the segment-intersection sweep.
// A polygon with holes is {outer CCW, holes CW}; rings are simple, disjoint,
// non-touching, holes strictly inside the outer ring and outside one another.

// T: O(1), M: O(1). x6/y6 are six times the signed first area moments.
template<typename T> struct PolygonMoments {
    using W = geometry_detail::Wide<T>;
    W area2{}, x6{}, y6{};
    PolygonMoments &operator+=(const PolygonMoments &m) {
        area2 += m.area2; x6 += m.x6; y6 += m.y6; return *this; }
};

// T: O(n), M: O(1). Empty/degenerate walks have their algebraic moments.
template<typename T> PolygonMoments<T> polygonMoments(const vector<Point2<T>> &p) {
    assert(p.size() <= INT_MAX); int n = int(p.size());
    PolygonMoments<T> m; using W = typename PolygonMoments<T>::W;
    for (int i = 0; i < n; ++i) {
        auto a = p[i], b = p[i + 1 == n ? 0 : i + 1]; W c = cross(a, b);
        m.area2 += c; m.x6 += (W(a.x) + b.x) * c; m.y6 += (W(a.y) + b.y) * c; }
    return m; }

// T: O(n + r), M: O(1). n total vertices, r rings; signed summation removes holes.
template<typename T> PolygonMoments<T> polygonMoments(const vector<vector<Point2<T>>> &rings) {
    PolygonMoments<T> m; lng n = 0;
    for (const auto &p : rings) { assert(p.size() <= INT_MAX - n); n += p.size(); m += polygonMoments(p); }
    return m; }

// T: O(n), M: O(1). Positive for a CCW simple polygon; zero for an empty walk.
template<typename T> auto signedArea2(const vector<Point2<T>> &p) {
    assert(p.size() <= INT_MAX); int n = int(p.size()); geometry_detail::Wide<T> a = 0;
    for (int i = 0; i < n; ++i) { a += cross(p[i], p[i + 1 == n ? 0 : i + 1]); }
    return a; }
// T: O(n + r), M: O(1). Same signed-ring convention as polygonMoments.
template<typename T> auto signedArea2(const vector<vector<Point2<T>>> &rings) {
    geometry_detail::Wide<T> a = 0; lng n = 0;
    for (const auto &p : rings) { assert(p.size() <= INT_MAX - n); n += p.size(); a += signedArea2(p); }
    return a; }
// T: O(n + r), M: O(1). r = 1 for a single ring; root/division output is approximate.
template<typename Polygon> long double signedAreaApprox(const Polygon &p) { return (long double)signedArea2(p) / 2; }

// T: O(n + r + log(n * C^3)), M: O(1), C=max(1, coordinate magnitude).
// False iff signed area is zero; leaves out unchanged. Exact centroid supports
// integral coordinates and signed rings; the logarithm is gcd normalization.
template<typename Polygon> bool centroidExact(const Polygon &p, RationalPoint2 &out) {
    auto m = polygonMoments(p); static_assert(std::is_integral_v<decltype(m.area2)>);
    if (m.area2 == 0) { return false; }
    out = RationalPoint2(m.x6, m.y6, 3 * m.area2); return true; }
// T: O(n + r), M: O(1). Computed-zero signed area returns false; out unchanged.
template<typename Polygon> bool centroidApprox(const Polygon &p, dpoint &out) {
    auto m = polygonMoments(p);
    if (m.area2 == 0) { return false; }
    out = {(long double)m.x6 / (3 * (long double)m.area2), (long double)m.y6 / (3 * (long double)m.area2)};
    return true; }

enum class PolygonLocation { Outside, Boundary, Inside };
struct WindingResult { int winding = 0; bool boundary = false; };

// T: O(n), M: O(1). Winding uses half-open vertical intervals. On boundary,
// winding is unspecified; empty/point/segment walks have no interior.
template<typename T> WindingResult polygonWinding(const vector<Point2<T>> &p, Point2<T> q) {
    assert(p.size() <= INT_MAX); int n = int(p.size()); WindingResult r;
    for (int i = 0; i < n; ++i) {
        auto a = p[i], b = p[i + 1 == n ? 0 : i + 1]; int s = orient(a, b, q);
        if (!s && min(a.x, b.x) <= q.x && q.x <= max(a.x, b.x) && min(a.y, b.y) <= q.y && q.y <= max(a.y, b.y)) {
            r.boundary = true; return r; }
        if (a.y <= q.y && q.y < b.y && s > 0) { ++r.winding; }
        if (b.y <= q.y && q.y < a.y && s < 0) { --r.winding; }}
    return r; }
// T: O(n + r), M: O(1). Nonzero winding fill; any ring boundary wins.
template<typename T> WindingResult polygonWinding(const vector<vector<Point2<T>>> &rings, Point2<T> q) {
    WindingResult r; lng n = 0;
    for (const auto &p : rings) {
        assert(p.size() <= INT_MAX - n); n += p.size(); auto s = polygonWinding(p, q);
        if (s.boundary) { return s; } r.winding += s.winding; }
    return r; }
template<typename Polygon, typename T> PolygonLocation polygonContains(const Polygon &p, Point2<T> q) {
    auto r = polygonWinding(p, q);
    return r.boundary ? PolygonLocation::Boundary : r.winding ? PolygonLocation::Inside : PolygonLocation::Outside; }

// T: O(n), M: O(1). Closed-walk length: empty/point -> 0, two points -> 2*distance.
template<typename T> long double polygonPerimeter(const vector<Point2<T>> &p) {
    assert(p.size() <= INT_MAX); int n = int(p.size()); long double s = 0;
    for (int i = 0; i < n; ++i) { s += distanceApprox(p[i], p[i + 1 == n ? 0 : i + 1]); }
    return s; }
// T: O(n + r), M: O(1). Total perimeter, including hole boundaries.
template<typename T> long double polygonPerimeter(const vector<vector<Point2<T>>> &rings) {
    long double s = 0; lng n = 0;
    for (const auto &p : rings) { assert(p.size() <= INT_MAX - n); n += p.size(); s += polygonPerimeter(p); }
    return s; }

// T: O(n), M: O(1). Input must otherwise be simple: this is not a simplicity test.
// Either orientation; weak allows forward collinear edges, strict does not.
// <3 vertices, zero-area, zero-length edges and backtracking always return false.
template<typename T> bool polygonConvex(const vector<Point2<T>> &p, bool strict = true) {
    assert(p.size() <= INT_MAX); int n = int(p.size()), sign = 0;
    if (n < 3) { return false; }
    for (int i = 0; i < n; ++i) {
        int j = i + 1 == n ? 0 : i + 1, k = j + 1 == n ? 0 : j + 1;
        auto a = p[i], b = p[j], c = p[k];
        if (a == b) { return false; } int s = orient(a, b, c);
        if (!s) {
            using W = geometry_detail::Wide<T>;
            if (strict || dot(b.template cast<W>() - a.template cast<W>(), c.template cast<W>() - b.template cast<W>()) <= 0) {
                return false; }}
        else if (sign && sign != s) { return false; }
        else { sign = s; }}
    return sign != 0; }

// T: O(n * log(C)), M: O(1). C bounds coordinate differences; integer coordinates
// |coordinate| <= 10^9. Sum gcd(|dx|,|dy|); counts distinct boundary points only
// for simple nonzero-area polygons (no duplicated closing vertex).
template<typename T> lll latticeBoundary(const vector<Point2<T>> &p) {
    static_assert(std::is_integral_v<T>); assert(p.size() <= INT_MAX); int n = int(p.size()); lll b = 0;
    for (int i = 0; i < n; ++i) {
        auto a = p[i], c = p[i + 1 == n ? 0 : i + 1];
        assert(abs(lll(a.x)) <= 1000000000 && abs(lll(a.y)) <= 1000000000);
        b += gcd(abs(lng(c.x) - lng(a.x)), abs(lng(c.y) - lng(a.y))); }
    return b; }
// T: O(n * log(C) + r), M: O(1). Sums all ring boundaries.
template<typename T> lll latticeBoundary(const vector<vector<Point2<T>>> &rings) {
    lll b = 0; lng n = 0;
    for (const auto &p : rings) { assert(p.size() <= INT_MAX - n); n += p.size(); b += latticeBoundary(p); }
    return b; }
// T: O(n * log(C)), M: O(1). Pick: strictly interior lattice points, either orientation.
template<typename T> lll latticeInterior(const vector<Point2<T>> &p) {
    assert(p.size() >= 3); lll a = abs(signedArea2(p)); assert(a > 0);
    return (a - latticeBoundary(p)) / 2 + 1; }
// T: O(n * log(C) + r), M: O(1). One CCW outer ring and r-1 CW holes;
// nonempty valid simple disjoint rings required. Euler correction is 1-h.
template<typename T> lll latticeInterior(const vector<vector<Point2<T>>> &rings) {
    assert(!rings.empty() && rings.size() <= INT_MAX); lll a = 0;
    for (int i = 0; i < int(rings.size()); ++i) {
        assert(rings[i].size() >= 3); lll s = signedArea2(rings[i]);
        assert(i == 0 ? s > 0 : s < 0); a += s; }
    assert(a > 0); return (a - latticeBoundary(rings)) / 2 + 2 - lll(rings.size()); }
