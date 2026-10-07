#pragma once
#include "../01-Core/01-template.hpp"
#include "01-point.hpp"
#include "02-line_segment.hpp"

// T: O(1), M: O(1). x6/y6 are six times the signed first area moments.
template<typename T> struct PolygonMoments {
    using W = geometry_detail::Wide<T>;
    W area2{}, x6{}, y6{};
    PolygonMoments &operator+=(const PolygonMoments &m) {
        area2 += m.area2; x6 += m.x6; y6 += m.y6;
        return *this;}
};

// T: O(n + r), M: O(1), n total vertices, r rings (1 for a ring). Rings omit the closing vertex; outer CCW, holes CW; exact for integral T.
template<typename T> PolygonMoments<T> polygonMoments(const vector<Point2<T>> &p) {
    assert(p.size() <= INT_MAX); int n = int(p.size());
    PolygonMoments<T> m; using W = typename PolygonMoments<T>::W;
    for (int i = 0; i < n; ++i) {
        auto a = p[i], b = p[i + 1 == n ? 0 : i + 1]; W c = cross(a, b);
        m.area2 += c; m.x6 += (W(a.x) + b.x) * c; m.y6 += (W(a.y) + b.y) * c;}
    return m;}
template<typename T> PolygonMoments<T> polygonMoments(const vector<vector<Point2<T>>> &rings) {
    PolygonMoments<T> m; lng n = 0;
    for (const auto &p : rings) { assert(lng(p.size()) <= INT_MAX - n); n += p.size(); m += polygonMoments(p); }
    return m;}

template<typename T> auto signedArea2(const vector<Point2<T>> &p) {
    assert(p.size() <= INT_MAX); int n = int(p.size()); geometry_detail::Wide<T> a = 0;
    for (int i = 0; i < n; ++i) { a += cross(p[i], p[i + 1 == n ? 0 : i + 1]); }
    return a;}
template<typename T> auto signedArea2(const vector<vector<Point2<T>>> &rings) {
    geometry_detail::Wide<T> a = 0; lng n = 0;
    for (const auto &p : rings) { assert(lng(p.size()) <= INT_MAX - n); n += p.size(); a += signedArea2(p); }
    return a;}
template<typename Polygon> long double signedAreaApprox(const Polygon &p) { return static_cast<long double>(signedArea2(p)) / 2; }

// T: O(n + r), plus O(log(n * C^3)) exact normalization, M: O(1), n total vertices, r rings, C = max(1, |coordinate|). False on zero area, out unchanged.
template<typename Polygon> bool centroidExact(const Polygon &p, RationalPoint2 &out) {
    auto m = polygonMoments(p); static_assert(std::is_integral_v<decltype(m.area2)>);
    if (m.area2 == 0) { return false; }
    out = RationalPoint2(m.x6, m.y6, 3 * m.area2);
    return true;}
template<typename Polygon> bool centroidApprox(const Polygon &p, dpoint &out) {
    auto m = polygonMoments(p);
    if (m.area2 == 0) { return false; }
    long double d = 3 * static_cast<long double>(m.area2);
    out = {static_cast<long double>(m.x6) / d, static_cast<long double>(m.y6) / d};
    return true;}

enum class PolygonLocation { Outside, Boundary, Inside };

// T: O(1), M: O(1). winding is unspecified when boundary is true.
struct WindingResult {
    int winding = 0;
    bool boundary = false;
};

// T: O(n + r), M: O(1), n total vertices, r rings (1 for a ring). Nonzero fill, half-open vertical crossings; any boundary wins.
template<typename T> WindingResult polygonWinding(const vector<Point2<T>> &p, Point2<T> q) {
    assert(p.size() <= INT_MAX); int n = int(p.size()); WindingResult r;
    for (int i = 0; i < n; ++i) {
        auto a = p[i], b = p[i + 1 == n ? 0 : i + 1]; int s = orient(a, b, q);
        if (!s && min(a.x, b.x) <= q.x && q.x <= max(a.x, b.x) && min(a.y, b.y) <= q.y && q.y <= max(a.y, b.y)) {
            r.boundary = true;
            return r;}
        if (a.y <= q.y && q.y < b.y && s > 0) { ++r.winding; }
        if (b.y <= q.y && q.y < a.y && s < 0) { --r.winding; }}
    return r;}
template<typename T> WindingResult polygonWinding(const vector<vector<Point2<T>>> &rings, Point2<T> q) {
    WindingResult r; lng n = 0;
    for (const auto &p : rings) {
        assert(lng(p.size()) <= INT_MAX - n); n += p.size();
        auto s = polygonWinding(p, q);
        if (s.boundary) { return s; }
        r.winding += s.winding;}
    return r;}
template<typename Polygon, typename T> PolygonLocation polygonContains(const Polygon &p, Point2<T> q) {
    auto r = polygonWinding(p, q);
    return r.boundary ? PolygonLocation::Boundary : r.winding ? PolygonLocation::Inside : PolygonLocation::Outside;}

// T: O(n + r), M: O(1), n total vertices, r rings (1 for a ring). Closed-walk length including holes; two points count twice.
template<typename T> long double polygonPerimeter(const vector<Point2<T>> &p) {
    assert(p.size() <= INT_MAX); int n = int(p.size()); long double s = 0;
    for (int i = 0; i < n; ++i) { s += distanceApprox(p[i], p[i + 1 == n ? 0 : i + 1]); }
    return s;}
template<typename T> long double polygonPerimeter(const vector<vector<Point2<T>>> &rings) {
    long double s = 0; lng n = 0;
    for (const auto &p : rings) { assert(lng(p.size()) <= INT_MAX - n); n += p.size(); s += polygonPerimeter(p); }
    return s;}

// T: O(n), M: O(1). Simple ring assumed, either orientation; weak allows forward collinear vertices.
template<typename T> bool polygonConvex(const vector<Point2<T>> &p, bool strict = true) {
    assert(p.size() <= INT_MAX); int n = int(p.size()), sign = 0;
    if (n < 3) { return false; }
    for (int i = 0; i < n; ++i) {
        int j = i + 1 == n ? 0 : i + 1, k = j + 1 == n ? 0 : j + 1;
        auto a = p[i], b = p[j], c = p[k];
        if (a == b) { return false; }
        int s = orient(a, b, c);
        if (!s) {
            using W = geometry_detail::Wide<T>;
            if (strict || dot(b.template cast<W>() - a.template cast<W>(), c.template cast<W>() - b.template cast<W>()) <= 0) { return false; }}
        else if (sign && sign != s) { return false; }
        else { sign = s; }}
    return sign != 0;}

// T: O(n * log(C) + r), M: O(1), n total vertices, r rings (1 for a ring), C = max coordinate difference. Integral |coordinate| <= 10^9; Pick needs simple nonzero-area rings.
template<typename T> lll latticeBoundary(const vector<Point2<T>> &p) {
    static_assert(std::is_integral_v<T>);
    assert(p.size() <= INT_MAX); int n = int(p.size()); lll b = 0;
    for (int i = 0; i < n; ++i) {
        auto a = p[i], c = p[i + 1 == n ? 0 : i + 1];
        assert(abs(lll(a.x)) <= 1000000000 && abs(lll(a.y)) <= 1000000000);
        b += gcd(abs(lng(c.x) - lng(a.x)), abs(lng(c.y) - lng(a.y)));}
    return b;}
template<typename T> lll latticeBoundary(const vector<vector<Point2<T>>> &rings) {
    lll b = 0; lng n = 0;
    for (const auto &p : rings) { assert(lng(p.size()) <= INT_MAX - n); n += p.size(); b += latticeBoundary(p); }
    return b;}
template<typename T> lll latticeInterior(const vector<Point2<T>> &p) {
    assert(p.size() >= 3); lll a = abs(signedArea2(p)); assert(a > 0);
    return (a - latticeBoundary(p)) / 2 + 1;}
template<typename T> lll latticeInterior(const vector<vector<Point2<T>>> &rings) {
    assert(!rings.empty() && rings.size() <= INT_MAX); lll a = 0;
    for (int i = 0; i < int(rings.size()); ++i) {
        assert(rings[i].size() >= 3); lll s = signedArea2(rings[i]);
        assert(i == 0 ? s > 0 : s < 0); a += s;}
    assert(a > 0);
    return (a - latticeBoundary(rings)) / 2 + 2 - lll(rings.size());}

// T: O(n), M: O(n), output size <= 2 * n. Keeps p within the closed left half-plane of a -> b, a != b.
// The result may hold collinear or zero-width pieces; its signed area is that of the clipped region.
template<typename T> vector<dpoint> polygonCutApprox(const vector<Point2<T>> &p, Point2<T> a, Point2<T> b) {
    assert(p.size() <= INT_MAX && a != b); int n = int(p.size()); vector<dpoint> res;
    dpoint s = a.template cast<long double>(), u = b.template cast<long double>() - s;
    for (int i = 0; i < n; ++i) {
        auto c = p[i], d = p[i + 1 == n ? 0 : i + 1]; int x = orient(a, b, c), y = orient(a, b, d);
        dpoint q = c.template cast<long double>(), v = d.template cast<long double>() - q;
        if (x >= 0) { res.push_back(q); }
        if (x * y < 0) { res.push_back(q + v * (cross(u, s - q) / cross(u, v))); }}
    return res;}
