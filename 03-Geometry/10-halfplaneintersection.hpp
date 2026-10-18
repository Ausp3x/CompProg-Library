#pragma once
#include "../01-Core/01-template.hpp"
#include "01-point.hpp"
#include "02-line_segment.hpp"
#include "03-polygon.hpp"

// T: O(1), M: O(1). Closed left side of a -> b, a != b, integral |coordinate| <= 10^9; rational points need |x|, |y| <= 3 * 10^28, 0 < d <= 8 * 10^18.
struct Halfplane {
    point a{}, b{};
    bool contains(point p, bool strict = false) const {
        int s = orient(a, b, p);
        return strict ? s > 0 : s >= 0;}
    bool contains(const RationalPoint2 &p, bool strict = false) const {
        lll s = lll(b.x - a.x) * (p.y - lll(a.y) * p.d) - lll(b.y - a.y) * (p.x - lll(a.x) * p.d);
        return strict ? s > 0 : s >= 0;}
};

enum class HalfplaneKind { Empty, Point, Segment, Ray, Line, Polygon, Unbounded };

// T: O(1), M: O(out). Unbounded is two-dimensional; recession lists CCW generators of the recession cone; planes reproduce the set.
struct HalfplaneResult {
    HalfplaneKind kind = HalfplaneKind::Empty;
    vector<RationalPoint2> vertices;
    vector<point> recession;
    vector<Halfplane> planes;
};

namespace halfplane_detail {
    inline point dir(const Halfplane &h) { return h.b - h.a; }
    inline bool same(point u, point v) { return cross(u, v) == 0 && dot(u, v) > 0; }
    inline point primitive(point u) { return u == point{} ? u : u / gcd(abs(u.x), abs(u.y)); }
    // Angular order of directions; parallel same-direction planes put the innermost first.
    inline bool less(const Halfplane &x, const Halfplane &y) {
        point u = dir(x), v = dir(y);
        return same(u, v) ? orient(y.a, y.b, x.a) > 0 : polarLess(u, v);}
    // Sign of the depth in h of the meeting point of non-parallel boundaries p and q; degree-4 terms fit lll.
    inline int side(const Halfplane &p, const Halfplane &q, const Halfplane &h) {
        point u = dir(p), w = dir(h); lll d = cross(u, dir(q)), n = cross(q.a - p.a, dir(q));
        return geometry_detail::sign(d * cross(w, p.a - h.a) + n * cross(w, u)) * geometry_detail::sign(d);}
    inline RationalPoint2 vertex(const Halfplane &p, const Halfplane &q) {
        point u = dir(p); lll d = cross(u, dir(q)), n = cross(q.a - p.a, dir(q));
        return {p.a.x * d + u.x * n, p.a.y * d + u.y * n, d};}
    // Deque over angle-sorted distinct directions; bad(i, j, k) drops the vertex of i, j for k; apart(i, k) keeps an antiparallel chain end.
    template<typename Bad, typename Apart> vector<int> sweep(const vector<point> &u, bool cyclic, Bad bad, Apart apart) {
        int m = int(u.size()), l = 0, r = 0; vector<int> q(m);
        for (int i = 0; i < m; ++i) {
            while (r - l >= 2 && bad(q[r - 2], q[r - 1], i)) { --r; }
            while (r - l >= 2 && bad(q[l], q[l + 1], i)) { ++l; }
            if (r - l >= 1 && cross(u[q[r - 1]], u[i]) <= 0 && (cyclic || !apart(q[r - 1], i))) { return {}; }
            q[r++] = i;}
        if (cyclic) {
            while (r - l >= 3 && bad(q[r - 2], q[r - 1], q[l])) { --r; }
            while (r - l >= 3 && bad(q[l], q[l + 1], q[r - 1])) { ++l; }
            if (r - l < 3 || cross(u[q[r - 1]], u[q[l]]) <= 0) { return {}; }}
        return vector<int>(q.begin() + l, q.begin() + r);}
    // The intersection restricted to the boundary of h[i], whose antiparallel partner h[j] shares that line.
    inline HalfplaneResult onLine(const vector<Halfplane> &h, int i, int j) {
        const Halfplane &s = h[i]; point w = dir(s); HalfplaneResult res;
        lll ln = 0, ld = 0, hn = 0, hd = 0; int lo = -1, hi = -1;
        for (int k = 0; k < int(h.size()); ++k) {
            lll c1 = cross(dir(h[k]), w), c0 = cross(dir(h[k]), s.a - h[k].a);
            if (c1 == 0 && c0 < 0) { return res; }
            if (c1 > 0 && (lo < 0 || -c0 * ld > ln * c1)) { ln = -c0; ld = c1; lo = k; }
            if (c1 < 0 && (hi < 0 || c0 * hd < hn * -c1)) { hn = c0; hd = -c1; hi = k; }}
        if (lo >= 0 && hi >= 0 && ln * hd > hn * ld) { return res; }
        auto at = [&](lll n, lll d) { return RationalPoint2(s.a.x * d + w.x * n, s.a.y * d + w.y * n, d); };
        res.planes = {s, h[j]};
        if (lo >= 0) { res.planes.push_back(h[lo]); res.vertices.push_back(at(ln, ld)); }
        if (hi >= 0) { res.planes.push_back(h[hi]); res.vertices.push_back(at(hn, hd)); }
        if (lo >= 0 && hi >= 0) {
            if (ln * hd == hn * ld) { res.vertices.pop_back(); }
            res.kind = res.vertices.size() == 1 ? HalfplaneKind::Point : HalfplaneKind::Segment;}
        else if (lo >= 0 || hi >= 0) { res.kind = HalfplaneKind::Ray; res.recession = {primitive(lo >= 0 ? w : -w)}; }
        else { res.kind = HalfplaneKind::Line; res.recession = {primitive(w), primitive(-w)}; }
        return res;}
    // Input sorted by less; validates each plane once.
    inline HalfplaneResult solve(vector<Halfplane> h) {
        int m = 0;
        for (const Halfplane &x : h) {
            linear_detail::check(x.a); linear_detail::check(x.b); assert(x.a != x.b);
            if (!m || !same(dir(h[m - 1]), dir(x))) { h[m++] = x; }}
        h.resize(m); HalfplaneResult res;
        if (!m) {
            res.kind = HalfplaneKind::Unbounded; res.recession = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
            return res;}
        int g = -1;
        for (int i = 0; i < m && g < 0; ++i) {
            point x = dir(h[i]), y = dir(h[(i + 1) % m]);
            if (m == 1 || cross(x, y) < 0 || (cross(x, y) == 0 && dot(x, y) < 0)) { g = i; }}
        vector<Halfplane> c = h;
        if (g >= 0) { std::rotate(c.begin(), c.begin() + (g + 1) % m, c.end()); }
        vector<point> u(m);
        for (int i = 0; i < m; ++i) { u[i] = dir(c[i]); }
        auto apart = [&](int i, int k) { return orient(c[i].a, c[i].b, c[k].a) > 0; };
        auto q = sweep(u, g < 0, [&](int i, int j, int k) { return side(c[i], c[j], c[k]) <= 0; }, apart);
        int k = int(q.size());
        for (int t = 0; t < k; ++t) { res.planes.push_back(c[q[t]]); }
        if (g < 0 && k) {
            res.kind = HalfplaneKind::Polygon;
            for (int t = 0; t < k; ++t) { res.vertices.push_back(vertex(res.planes[(t + k - 1) % k], res.planes[t])); }
            return res;}
        if (k) {
            res.kind = HalfplaneKind::Unbounded;
            for (int t = 0; t + 1 < k; ++t) {
                if (cross(u[q[t]], u[q[t + 1]]) > 0) { res.vertices.push_back(vertex(res.planes[t], res.planes[t + 1])); }}
            point x = u[m - 1], y = -u[0];
            if (m == 1) { res.recession = {primitive(x), primitive(perp(x)), primitive(y)}; }
            else if (m == 2 && same(x, y)) { res.recession = {primitive(x), primitive(-x)}; }
            else if (same(x, y)) { res.recession = {primitive(x)}; }
            else { res.recession = {primitive(x), primitive(y)}; }
            return res;}
        int s = 0;
        for (int i = 1; i < m; ++i) { s = polarLess(-dir(h[i]), -dir(h[s])) ? i : s; }
        // Negated directions are a rotation of a sorted list, so one merge finds every antiparallel partner.
        for (int t = 0, j = 0; t < m; ++t) {
            int i = (s + t) % m; point v = -dir(h[i]);
            while (j < m && !same(dir(h[j]), v) && polarLess(dir(h[j]), v)) { ++j; }
            if (j < m && same(dir(h[j]), v) && orient(h[i].a, h[i].b, h[j].a) == 0) { return onLine(h, i, j); }}
        if (g >= 0) { return {}; }
        // Empty interior without a doubled line leaves at most a point on three or more boundaries; the closed sweep proposes it.
        q = sweep(u, true, [&](int i, int j, int k) { return side(c[i], c[j], c[k]) < 0; }, apart);
        if (q.empty()) { return {}; }
        for (int t = 0; t < m; ++t) {
            if (side(c[q[0]], c[q[1]], c[t]) < 0) { return {}; }}
        res.kind = HalfplaneKind::Point; res.vertices = {vertex(c[q[0]], c[q[1]])};
        for (int t = 0; t < m; ++t) {
            if (side(c[q[0]], c[q[1]], c[t]) == 0) { res.planes.push_back(c[t]); }}
        return res;}
    // Edges of a convex CCW ring as halfplanes, rotated into the order of less.
    template<typename T> vector<Halfplane> edges(const vector<Point2<T>> &p) {
        int n = int(p.size()), s = 0; vector<Halfplane> e(n);
        for (int i = 0; i < n; ++i) { e[i] = {p[i].template cast<lng>(), p[(i + 1) % n].template cast<lng>()}; }
        for (int i = 1; i < n; ++i) { s = less(e[i], e[s]) ? i : s; }
        for (int t = 0; t < n && same(dir(e[(s + n - 1) % n]), dir(e[s])); ++t) { s = (s + n - 1) % n; }
        std::rotate(e.begin(), e.begin() + s, e.end());
        return e;}
} // namespace halfplane_detail

// T: O(n * log(n)), M: O(n). Exact; kinds Empty, Point, Segment, Ray, Line, Polygon (CCW from the smallest edge angle) or Unbounded (no bounding box).
inline HalfplaneResult halfplaneIntersection(vector<Halfplane> h) {
    sort(h.begin(), h.end(), halfplane_detail::less);
    return halfplane_detail::solve(std::move(h));}

// T: O(n + m * log(m)), M: O(n + m). Convex CCW polygon, n >= 3, positive area, collinear vertices allowed; integral |coordinate| <= 10^9.
template<typename T> HalfplaneResult clipConvex(const vector<Point2<T>> &p, vector<Halfplane> h) {
    static_assert(std::is_integral_v<T>);
    assert(p.size() >= 3 && p.size() <= INT_MAX && polygonConvex(p, false) && signedArea2(p) > 0);
    auto e = halfplane_detail::edges(p);
    sort(h.begin(), h.end(), halfplane_detail::less);
    vector<Halfplane> all(e.size() + h.size());
    std::merge(e.begin(), e.end(), h.begin(), h.end(), all.begin(), halfplane_detail::less);
    return halfplane_detail::solve(std::move(all));}

// T: O(n * log(n)), M: O(n). Simple polygon, either orientation, nonzero area, repeated vertices allowed; integral |coordinate| <= 10^9.
template<typename T> HalfplaneResult polygonKernel(const vector<Point2<T>> &p) {
    static_assert(std::is_integral_v<T>);
    assert(p.size() >= 3 && p.size() <= INT_MAX);
    int n = int(p.size()); auto area = signedArea2(p); assert(area != 0);
    vector<Halfplane> h;
    for (int i = 0; i < n; ++i) {
        point a = p[i].template cast<lng>(), b = p[(i + 1) % n].template cast<lng>();
        if (a != b) { h.push_back(area > 0 ? Halfplane{a, b} : Halfplane{b, a}); }}
    return halfplaneIntersection(std::move(h));}

// T: O(n), M: O(n). Lee–Preparata march from the bounding box; simple polygon, else as polygonKernel (planes may be box sides).
template<typename T> HalfplaneResult polygonKernelLinear(const vector<Point2<T>> &p) {
    static_assert(std::is_integral_v<T>);
    assert(p.size() >= 3 && p.size() <= INT_MAX);
    auto area = signedArea2(p); assert(area != 0);
    vector<point> v;
    for (const auto &q : p) {
        point x = q.template cast<lng>(); linear_detail::check(x);
        if (v.empty() || v.back() != x) { v.push_back(x); }}
    while (v.size() > 1 && v.back() == v[0]) { v.pop_back(); }
    if (area < 0) { reverse(v.begin(), v.end()); }
    int n = int(v.size()), size = 4, k = 0; lng x0 = v[0].x, x1 = x0, y0 = v[0].y, y1 = y0;
    for (int i = 0; i < n; ++i) {
        point q = v[i], a = q - v[(i + n - 1) % n], b = v[(i + 1) % n] - q; assert(cross(a, b) != 0 || dot(a, b) > 0);
        x0 = min(x0, q.x); x1 = max(x1, q.x); y0 = min(y0, q.y); y1 = max(y1, q.y);}
    vector<Halfplane> line{{{x0, y0}, {x1, y0}}, {{x1, y0}, {x1, y1}}, {{x1, y1}, {x0, y1}}, {{x0, y1}, {x0, y0}}}, flat;
    vector<int> nxt{1, 2, 3, 0}, prv{3, 0, 1, 2}; vector<char> dead(4, 0);
    bool fixed = false; RationalPoint2 pt;
    // side(j, h): side of the vertex after node j; seen(j, w): w is not strictly inside node j.
    auto side = [&](int j, const Halfplane &h) { return halfplane_detail::side(line[j], line[nxt[j]], h); };
    auto seen = [&](int j, point w) { return orient(line[j].a, line[j].b, w) <= 0; };
    // The open kernel keeps a strictly convex cycle; once it empties, flat holds at most four lines of a point or segment.
    auto absorb = [&](vector<Halfplane> d) {
        sort(d.begin(), d.end(), halfplane_detail::less); auto r = halfplane_detail::solve(std::move(d));
        flat = r.planes; fixed = r.kind == HalfplaneKind::Point;
        if (fixed) { pt = r.vertices[0]; }
        return r.kind != HalfplaneKind::Empty;};
    auto cut = [&](const Halfplane &h, int j) {
        int lo = j, hi = j, cnt = 1;
        while (cnt < size && side(prv[lo], h) <= 0) { lo = prv[lo]; ++cnt; }
        if (cnt == size) {
            vector<Halfplane> d{h};
            for (int t = 0, s = j; t < size; ++t, s = nxt[s]) {
                if (side(s, h) == 0) { d.push_back(line[s]); d.push_back(line[nxt[s]]); }}
            return d.size() > 1 && absorb(d) ? -1 : -2;}
        while (side(nxt[hi], h) <= 0) { hi = nxt[hi]; }
        for (int t = nxt[lo]; t != nxt[hi]; t = nxt[t]) { dead[t] = 1; --size; }
        int z = int(line.size()), r = nxt[hi];
        line.push_back(h); dead.push_back(0); nxt.push_back(r); prv.push_back(lo); nxt[lo] = z; prv[r] = z; ++size;
        return z;};
    int t1 = 0;
    for (int j = 0; j < 4; ++j) {
        if (side(j, {v[n - 1], v[0]}) < 0) { t1 = cut({v[n - 1], v[0]}, j); break; }}
    if (t1 == -2) { return {}; }
    t1 = max(t1, 0);
    for (int s = 0; s < size && !(seen(t1, v[0]) && !seen(prv[t1], v[0])); ++s) { t1 = nxt[t1]; }
    int t2 = t1;
    for (int s = 0; s < size && seen(nxt[t2], v[0]) && nxt[t2] != t1; ++s) { t2 = nxt[t2]; }
    // Nodes t1..t2 are the edges seen from v[i]; a nonempty kernel keeps the lifted turning within 6 * pi.
    point dp = v[0] - v[n - 1];
    for (int i = 0; i + 1 < n; ++i) {
        point d = v[i + 1] - v[i], w = v[i + 1]; Halfplane h{v[i], w}; lll c = cross(dp, d);
        k += int(c > 0 && polarLess(d, dp)) - int(c < 0 && polarLess(dp, d)); dp = d;
        if (fixed && !h.contains(pt)) { return {}; }
        if (fixed) { continue; }
        if (!flat.empty()) {
            flat.push_back(h);
            if (!absorb(flat)) { return {}; }
            continue;}
        if (abs(k) >= 4) { return {}; }
        if (c != 0 && side(c > 0 ? t2 : prv[t1], h) < 0) {
            int z = cut(h, c > 0 ? t2 : prv[t1]);
            if (z == -2) { return {}; }
            if (z < 0) { continue; }
            if (c > 0 || dead[t2]) { t2 = z; }
            if (c < 0 || dead[t1]) { t1 = z; }}
        for (int s = 0; s < size && !seen(t1, w); ++s) { t1 = nxt[t1]; }
        if (!seen(t2, w)) { t2 = t1; }
        for (int s = 0; s < size && seen(nxt[t2], w) && nxt[t2] != t1; ++s) { t2 = nxt[t2]; }}
    if (!flat.empty()) {
        sort(flat.begin(), flat.end(), halfplane_detail::less);
        return halfplane_detail::solve(std::move(flat));}
    vector<Halfplane> res;
    for (int t = 0, s = t1; t < size; ++t, s = nxt[s]) { res.push_back(line[s]); }
    std::rotate(res.begin(), std::min_element(res.begin(), res.end(), halfplane_detail::less), res.end());
    return halfplane_detail::solve(std::move(res));}

// T: O(n * log(n) + n * B), M: O(n), B <= 256 bisection steps. False unless the intersection is a Polygon; rounded center and radius.
inline bool chebyshevCenterApprox(const vector<Halfplane> &h, dpoint &center, long double &radius) {
    auto res = halfplaneIntersection(h);
    if (res.kind != HalfplaneKind::Polygon) { return false; }
    int k = int(res.planes.size()); vector<point> u(k); vector<dpoint> a(k), n(k), v(k);
    long double lo = 0, hi = 0; center = {};
    for (int i = 0; i < k; ++i) {
        u[i] = halfplane_detail::dir(res.planes[i]); a[i] = res.planes[i].a.cast<long double>();
        n[i] = unitApprox(perp(u[i])); v[i] = res.vertices[i].approx(); center += v[i] / static_cast<long double>(k);}
    for (int i = 0; i < k; ++i) { hi += distanceApprox(v[i], v[(i + 1) % k]) / 4; }
    auto feasible = [&](long double r, dpoint &c) {
        auto at = [&](int i) { return a[i] + n[i] * r; };
        auto meet = [&](int i, int j) {
            dpoint p = at(i), s = u[i].cast<long double>(), t = u[j].cast<long double>();
            return p + s * (cross(at(j) - p, t) / cross(s, t));};
        auto bad = [&](int i, int j, int l) { return cross(u[l].cast<long double>(), meet(i, j) - at(l)) <= 0; };
        auto q = halfplane_detail::sweep(u, true, bad, [](int, int) { return false; });
        int m = int(q.size()); c = {};
        for (int t = 0; t < m; ++t) { c += meet(q[t], q[(t + 1) % m]) / static_cast<long double>(m); }
        return m > 0;};
    for (int it = 0; it < 256; ++it) {
        long double mid = std::midpoint(lo, hi); dpoint c;
        if (mid <= lo || mid >= hi) { break; }
        if (feasible(mid, c)) { lo = mid; center = c; }
        else { hi = mid; }}
    radius = lo;
    return true;}
