#pragma once
#include "../01-Core/01-template.hpp"
#include "../05-Mathematics/18-floorsum.hpp"
#include "01-point.hpp"
#include "02-line_segment.hpp"
#include "10-halfplaneintersection.hpp"

namespace lattice_detail {
    inline lll floorDiv(lll a, lll b) { return a / b - ((a % b != 0) && ((a < 0) != (b < 0))); }
    inline lll ceilDiv(lll a, lll b) { return -floorDiv(-a, b); }
    inline lll isqrt(lll v) {
        if (v <= 0) { return 0; }
        lll s = lll(std::sqrt(static_cast<long double>(v)));
        while (s * s > v) { --s; }
        while ((s + 1) * (s + 1) <= v) { ++s; }
        return s;}
    inline bool less(const pair<lll, lll> &x, const pair<lll, lll> &y) {
        lll a = floorDiv(x.first, x.second), b = floorDiv(y.first, y.second);
        return a != b ? a < b : (x.first - a * x.second) * y.second < (y.first - b * y.second) * x.second;}
    inline lll lineFloorSum(lll a, lll b, lll m, lll l, lll r) {
        if (l > r) { return 0; }
        if (m < 0) { a = -a; b = -b; m = -m; }
        assert(m <= std::numeric_limits<lng>::max() && r - l < std::numeric_limits<lng>::max());
        return floorSum(lng(r - l + 1), lng(m), a, b + a * l);}
    struct Edge { RationalPoint2 p, q; point dir; lll k; };
    inline int crossDelta(point u) { return u.x ? -geometry_detail::sign(u.x) : -geometry_detail::sign(u.y); }
    inline lll openCount(const pair<lll, lll> &lo, const pair<lll, lll> &hi, lll r, lll m) {
        lll a = floorDiv(lo.first, lo.second) + 1, b = ceilDiv(hi.first, hi.second) - 1;
        return a > b ? 0 : floorDiv(b - r, m) - floorDiv(a - 1 - r, m);}
    inline lll openLattice(const Edge &e) {
        lll g = gcd(abs(lll(e.dir.x)), abs(lll(e.dir.y)));
        if (e.k % g) { return 0; }
        lll dx = e.dir.x / g, dy = e.dir.y / g, k = e.k / g;
        if (!dx) {
            pair<lll, lll> a{e.p.y, e.p.d}, b{e.q.y, e.q.d};
            if (dy < 0) { swap(a, b); }
            return e.p.x % e.p.d ? 0 : openCount(a, b, 0, 1);}
        lll m = abs(dx), c = ((-dy) % m + m) % m, t = ((k % m) + m) % m, x0 = 0;
        if (m > 1) {
            lll r0 = m, r1 = c, s0 = 0, s1 = 1;
            while (r1) { lll z = r0 / r1; r0 -= z * r1; swap(r0, r1); s0 -= z * s1; swap(s0, s1); }
            x0 = ((s0 % m + m) % m) * t % m;}
        pair<lll, lll> a{e.p.x, e.p.d}, b{e.q.x, e.q.d};
        if (dx < 0) { swap(a, b); }
        return openCount(a, b, x0, m);}
    // s counts lattice P with P + (e', -e) inside, 0 < e' << e; boundary points where that shift points inward are then removed.
    inline lll count(vector<Edge> e, bool closed) {
        int n = int(e.size()); lll s = 0, inward = 0, boundary = 0;
        for (auto &x : e) {
            lll g = gcd(gcd(abs(lll(x.dir.x)), abs(lll(x.dir.y))), abs(x.k));
            x.dir /= lng(g); x.k /= g;}
        for (int i = 0; i < n; ++i) {
            const Edge &x = e[i]; lll on = openLattice(x); boundary += on;
            if (x.dir.x) {
                const RationalPoint2 &lo = x.dir.x > 0 ? x.p : x.q, &hi = x.dir.x > 0 ? x.q : x.p;
                lll sum = lineFloorSum(x.dir.y, x.k, x.dir.x, ceilDiv(lo.x, lo.d), ceilDiv(hi.x, hi.d) - 1);
                s += x.dir.x < 0 ? sum : -sum;
                if (x.dir.x < 0) { inward += on; }}
            else if (x.dir.y < 0) { inward += on; }
            if (x.p.d == 1) {
                point b = x.dir, c = -e[(i + n - 1) % n].dir; int bd = crossDelta(b), dc = -crossDelta(c);
                ++boundary; inward += cross(b, c) > 0 ? (bd > 0 && dc > 0) : (bd > 0 || dc > 0);}}
        lll interior = s - inward;
        return closed ? interior + boundary : interior;}
    inline vector<Edge> rationalEdges(const vector<RationalPoint2> &v) {
        int n = int(v.size()); vector<Edge> e(n);
        lll big = 0, den = 1;
        for (const auto &p : v) { den = max(den, p.d); big = max({big, ceilDiv(abs(p.x), p.d), ceilDiv(abs(p.y), p.d)}); }
        assert(den <= 1600000 && big * den * den * den <= 2000000000000000000LL);
        for (int i = 0; i < n; ++i) {
            const RationalPoint2 &p = v[i], &q = v[(i + 1) % n];
            lll ux = q.x * p.d - p.x * q.d, uy = q.y * p.d - p.y * q.d;
            assert(ux || uy);
            e[i] = {p, q, point{lng(ux * p.d), lng(uy * p.d)}, ux * p.y - uy * p.x};}
        int low = 0;
        for (int i = 1; i < n; ++i) {
            lll a = v[i].x * v[low].d, b = v[low].x * v[i].d;
            if (a < b || (a == b && v[i].y * v[low].d < v[low].y * v[i].d)) { low = i; }}
        lll turn = cross(e[(low + n - 1) % n].dir, e[low].dir);
        assert(turn != 0);
        if (turn < 0) {
            vector<RationalPoint2> w(v.rbegin(), v.rend());
            return rationalEdges(w);}
        return e;}
    // Sum of floor((b + sqrt(n - (q x - a)^2)) / q) over x in [l, r], slope in [-1, 0]: Stern–Brocot walk on the lattice hull.
    inline lll hullWalk(lll a, lll b, lll q, lll n, lll l, lll r) {
        if (l > r) { return 0; }
        auto under = [&](lll x, lll y) { lll u = q * x - a, v = q * y - b; return v < 0 || u * u + v * v <= n; };
        auto steep = [&](lll x, point f) { lll u = q * x - a; return u >= 0 && u * u * f.x * f.x >= (n - u * u) * f.y * f.y; };
        lll x = l, y = floorDiv(b + isqrt(n - (q * l - a) * (q * l - a)), q), res = 0;
        auto ok = [&](point v, lll k) { return x + k * v.x <= r && under(x + k * v.x, y - k * v.y); };
        vector<point> st{{1, 1}, {1, 0}};
        while (x < r) {
            point v = st.back(); lll k = 0, step = 1;
            while (ok(v, k + step)) { k += step; step *= 2; }
            for (; step; step /= 2) {
                if (ok(v, k + step)) { k += step; }}
            if (k) {
                res += v.x * (k * y - v.y * k * (k - 1) / 2) - k * ((lll(v.x) - 1) * (v.y - 1) / 2 + v.x - 1);
                x += k * v.x; y -= k * v.y;}
            if (x == r) { break; }
            point g = v; st.pop_back();
            while (!ok(st.back(), 1)) { g = st.back(); st.pop_back(); }
            point f = st.back();
            // f fits and g does not; mediants refine toward g until the curve beyond x + m.x is steeper than f.
            while (true) {
                point m = f + g;
                if (ok(m, 1)) { st.push_back(f = m); }
                else if (x + m.x > r || steep(x + m.x, f)) { break; }
                else { g = m; }}}
        return res + y;}
    inline lll cap(lll a, lll b, lll q, lll n, bool strict) {
        lll w = isqrt(n / 2), lo = ceilDiv(a - w, q), hi = floorDiv(a + w, q), mid = ceilDiv(a, q);
        if (lo > hi) { return 0; }
        lll top = hullWalk(a, b, q, n, max(lo, mid), hi) + hullWalk(-a, b, q, n, -min(hi, mid - 1), -lo);
        auto series = [](lll c, lll l, lll r) { return l > r ? lll(0) : (r - l + 1) * c + (l + r) * (r - l + 1) / 2; };
        lll base1 = strict ? floorDiv(b - a, q) + 1 : ceilDiv(b - a, q), base2 = strict ? floorDiv(a + b, q) + 1 : ceilDiv(a + b, q);
        lll low = series(base1, max(lo, mid), hi) + series(base2, -min(hi, mid - 1), -lo);
        if (!strict && a % q == 0 && b % q == 0 && lo <= a / q && a / q <= hi) { ++low; }
        return top - low + (hi - lo + 1);}
} // namespace lattice_detail

// T: O(log(C)), M: O(1), C the largest numerator or denominator. Rational p, q, p.x != q.x: sum of floor(l(x)) over integers x in the closed x-range,
// l the line through p and q (lattice points 0 < y <= l(x), negated where l(x) < 0). Domain: max |coordinate| * (max d)^3 <= 2 * 10^18, d <= 1.6 * 10^6.
inline lll latticeUnderLine(const RationalPoint2 &p, const RationalPoint2 &q) {
    lll ux = q.x * p.d - p.x * q.d, uy = q.y * p.d - p.y * q.d;
    lll big = max({lattice_detail::ceilDiv(abs(p.x), p.d), lattice_detail::ceilDiv(abs(p.y), p.d), lattice_detail::ceilDiv(abs(q.x), q.d), lattice_detail::ceilDiv(abs(q.y), q.d)});
    assert(ux != 0 && max(p.d, q.d) <= 1600000 && big * max(p.d, q.d) * max(p.d, q.d) * max(p.d, q.d) <= 2000000000000000000LL);
    const RationalPoint2 &lo = ux > 0 ? p : q, &hi = ux > 0 ? q : p;
    lll a = uy * p.d, b = ux * p.y - uy * p.x, m = ux * p.d, g = gcd(gcd(abs(a), abs(b)), abs(m));
    return lattice_detail::lineFloorSum(a / g, b / g, m / g, lattice_detail::ceilDiv(lo.x, lo.d), lattice_detail::floorDiv(hi.x, hi.d));}

// T: O(n * log(C)), M: O(n), C as latticeUnderLine. Simple polygon with rational vertices, either orientation, nonzero area, no repeated consecutive vertices, domain as latticeUnderLine;
// lattice points strictly inside, or inside and on the boundary when closed.
inline lll latticeInRationalPolygon(const vector<RationalPoint2> &v, bool closed) {
    assert(v.size() >= 3 && v.size() <= INT_MAX);
    return lattice_detail::count(lattice_detail::rationalEdges(v), closed);}

// T: O(m * log(m) + k * log(C)), M: O(m), k result edges, C = 10^18. Halfplanes as in halfplaneIntersection with a bounded result (Empty, Point, Segment or Polygon) inside |coordinate| <= 10^18;
// lattice points of the open interior, or of the closed intersection when closed.
inline lll latticeInHalfplanePolygon(const vector<Halfplane> &h, bool closed) {
    HalfplaneResult r = halfplaneIntersection(h);
    assert(r.kind == HalfplaneKind::Empty || r.kind == HalfplaneKind::Point || r.kind == HalfplaneKind::Segment || r.kind == HalfplaneKind::Polygon);
    for (const auto &p : r.vertices) { assert(abs(p.x) <= 1000000000000000000LL * p.d && abs(p.y) <= 1000000000000000000LL * p.d); }
    if (r.kind == HalfplaneKind::Empty || (!closed && r.kind != HalfplaneKind::Polygon)) { return 0; }
    if (r.kind == HalfplaneKind::Point) { return r.vertices[0].d == 1; }
    auto edge = [](const Halfplane &s, const RationalPoint2 &p, const RationalPoint2 &q) {
        point w = s.b - s.a;
        return lattice_detail::Edge{p, q, w, lll(w.x) * s.a.y - lll(w.y) * s.a.x};};
    if (r.kind == HalfplaneKind::Segment) {
        const Halfplane &s = r.planes[0]; point w = s.b - s.a;
        RationalPoint2 p = r.vertices[0], q = r.vertices[1];
        bool forward = w.x ? lattice_detail::less({p.x, p.d}, {q.x, q.d}) == (w.x > 0) : lattice_detail::less({p.y, p.d}, {q.y, q.d}) == (w.y > 0);
        if (!forward) { swap(p, q); }
        return lattice_detail::openLattice(edge(s, p, q)) + (p.d == 1) + (q.d == 1);}
    int k = int(r.planes.size()); vector<lattice_detail::Edge> e(k);
    for (int t = 0; t < k; ++t) { e[t] = edge(r.planes[t], r.vertices[t], r.vertices[(t + 1) % k]); }
    return lattice_detail::count(e, closed);}

// T: O(sqrt(n) / q + 1), M: O(1). Closed disk (q x - a)^2 + (q y - b)^2 <= n: centre (a / q, b / q), squared radius n / q^2; |a|, |b| <= 10^18, 1 <= q <= 10^18, 0 <= n <= 4 * 10^18.
inline lll latticeInDisk(lng a, lng b, lng q, lng n) {
    assert(abs(a) <= 1000000000000000000LL && abs(b) <= 1000000000000000000LL && 1 <= q && q <= 1000000000000000000LL && 0 <= n && n <= 4000000000000000000LL);
    lll s = lattice_detail::isqrt(n), res = 0;
    for (lll x = lattice_detail::ceilDiv(a - s, q), hi = lattice_detail::floorDiv(a + s, q); x <= hi; ++x) {
        lll u = q * x - a, t = lattice_detail::isqrt(n - u * u);
        res += lattice_detail::floorDiv(b + t, q) - lattice_detail::ceilDiv(b - t, q) + 1;}
    return res;}
// T: O(r^(2/3) * log(r) + 1), M: O(r^(2/3) * log(r)) direction stack, r = sqrt(n) / q. Same count by diagonal caps and Stern–Brocot hull walks.
inline lll latticeInDiskFast(lng a, lng b, lng q, lng n) {
    assert(abs(a) <= 1000000000000000000LL && abs(b) <= 1000000000000000000LL && 1 <= q && q <= 1000000000000000000LL && 0 <= n && n <= 4000000000000000000LL);
    return (a % q == 0 && b % q == 0) + lattice_detail::cap(a, b, q, n, false) + lattice_detail::cap(a, -lll(b), q, n, false)
         + lattice_detail::cap(b, a, q, n, true) + lattice_detail::cap(b, -lll(a), q, n, true);}
