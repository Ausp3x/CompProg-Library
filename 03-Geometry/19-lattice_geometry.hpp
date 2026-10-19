#pragma once
#include "../01-Core/01-template.hpp"
#include "../01-Core/07-infint.hpp"
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
        assert(m >= 1 && r - l < std::numeric_limits<lng>::max());
        lll c = b + a * l, lim = lll(1) << 126;
        if (m <= std::numeric_limits<lng>::max() && -lim <= a && a <= lim && -lim <= c && c <= lim) { return floorSum(lng(r - l + 1), lng(m), a, c); }
        return floorSumWide(lng(r - l + 1), m, a, c);}
    inline u256 mul(lll a, lll b) { return u256(a) * u256(b); }
    inline int crossSign(Point2<lll> u, Point2<lll> v) {
        u256 w = mul(u.x, v.y) - mul(u.y, v.x);
        return w.w[3] >> 63 ? -1 : w ? 1 : 0;}
    // floor(w / d) for 1 <= d < 2^63 when the quotient fits lll; exact reports d | w.
    inline lll floorDivWide(u256 w, lll d, bool &exact) {
        bool neg = w.w[3] >> 63;
        if (neg) { w = -w; }
        exact = !w.divWord(ulng(d));
        lll q = lll(ulll(w.w[1]) << 64 | w.w[0]);
        return neg ? -q - !exact : q;}
    inline lll mulMod(lll a, lll b, lll m) {
        ulll r = 0, x = ulll(a), mm = ulll(m);
        for (ulll y = ulll(b); y; y >>= 1) {
            if (y & 1) { r += x; r -= r >= mm ? mm : 0; }
            x += x; x -= x >= mm ? mm : 0;}
        return lll(r);}
    // Edge p -> q on the line dir.x * y - dir.y * x = c through t, dir primitive; c = fl when exact, else c lies in (fl, fl + 1).
    struct Edge { RationalPoint2 p, q; Point2<lll> dir; lll fl; bool exact; };
    inline Edge edge(const RationalPoint2 &p, const RationalPoint2 &q, const RationalPoint2 &t, Point2<lll> u) {
        lll g = gcd(abs(u.x), abs(u.y)); Point2<lll> d{u.x / g, u.y / g}; bool exact;
        lll fl = floorDivWide(mul(d.x, t.y) - mul(d.y, t.x), t.d, exact);
        return {p, q, d, fl, exact};}
    inline int crossDelta(Point2<lll> u) { return u.x ? -geometry_detail::sign(u.x) : -geometry_detail::sign(u.y); }
    inline lll openCount(const pair<lll, lll> &lo, const pair<lll, lll> &hi, lll r, lll m) {
        lll a = floorDiv(lo.first, lo.second) + 1, b = ceilDiv(hi.first, hi.second) - 1;
        return a > b ? 0 : floorDiv(b - r, m) - floorDiv(a - 1 - r, m);}
    inline lll openLattice(const Edge &e) {
        if (!e.exact) { return 0; }
        lll dx = e.dir.x, dy = e.dir.y;
        if (!dx) {
            pair<lll, lll> a{e.p.y, e.p.d}, b{e.q.y, e.q.d};
            if (dy < 0) { swap(a, b); }
            return openCount(a, b, 0, 1);}
        lll m = abs(dx), c = ((-dy) % m + m) % m, t = (e.fl % m + m) % m, x0 = 0;
        if (m > 1) {
            lll r0 = m, r1 = c, s0 = 0, s1 = 1;
            while (r1) { lll z = r0 / r1; r0 -= z * r1; swap(r0, r1); s0 -= z * s1; swap(s0, s1); }
            x0 = mulMod((s0 % m + m) % m, t, m);}
        pair<lll, lll> a{e.p.x, e.p.d}, b{e.q.x, e.q.d};
        if (dx < 0) { swap(a, b); }
        return openCount(a, b, x0, m);}
    // s counts lattice P with P + (e', -e) inside, 0 < e' << e; boundary points where that shift points inward are then removed.
    inline lll count(const vector<Edge> &e, bool closed) {
        int n = int(e.size()); ulll s = 0; lll inward = 0, boundary = 0;
        for (int i = 0; i < n; ++i) {
            const Edge &x = e[i]; lll on = openLattice(x); boundary += on;
            if (x.dir.x) {
                const RationalPoint2 &lo = x.dir.x > 0 ? x.p : x.q, &hi = x.dir.x > 0 ? x.q : x.p;
                lll l = ceilDiv(lo.x, lo.d), r = ceilDiv(hi.x, hi.d) - 1;
                // floor((c + dy * x) / dx) = floor((floor(c) + dy * x) / dx) for dx > 0; negate c, dy and dx otherwise.
                lll sum = x.dir.x > 0 ? lineFloorSum(x.dir.y, x.fl, x.dir.x, l, r) : lineFloorSum(-x.dir.y, -x.fl - !x.exact, -x.dir.x, l, r);
                s += x.dir.x < 0 ? ulll(sum) : ulll(0) - ulll(sum);
                if (x.dir.x < 0) { inward += on; }}
            else if (x.dir.y < 0) { inward += on; }
            if (x.p.d == 1) {
                Point2<lll> b = x.dir, c = -e[(i + n - 1) % n].dir; int bd = crossDelta(b), dc = -crossDelta(c);
                ++boundary; inward += crossSign(b, c) > 0 ? (bd > 0 && dc > 0) : (bd > 0 || dc > 0);}}
        lll interior = lll(s - ulll(inward));
        return closed ? interior + boundary : interior;}
    // Unbounded sets: boundary rays carry infinitely many lattice points (-1); an open interior has none only inside an open strip of lattice width 1.
    inline lll unbounded(const HalfplaneResult &r, bool closed) {
        if (closed) { return -1; }
        if (r.kind != HalfplaneKind::Unbounded) { return 0; }
        point d = r.recession[0];
        if (r.recession.size() > 2 || (r.recession.size() == 2 && cross(d, r.recession[1]) != 0)) { return -1; }
        lll lo = 0, hi = 0;
        for (const Halfplane &s : r.planes) {
            point w = s.b - s.a;
            if (cross(w, d) == 0) { (dot(w, d) > 0 ? lo : hi) = lll(d.x) * s.a.y - lll(d.y) * s.a.x; }}
        return hi - lo >= 2 ? -1 : 0;}
    inline void rationalDomain(const vector<RationalPoint2> &v) {
        lll big = 1, den = 1;
        for (const auto &p : v) { den = max(den, p.d); big = max({big, ceilDiv(abs(p.x), p.d), ceilDiv(abs(p.y), p.d)}); }
        assert(den <= 4000000000000000000LL && big <= 4000000000000000000LL / den);}
    inline Point2<lll> direction(const RationalPoint2 &p, const RationalPoint2 &q) { return {q.x * p.d - p.x * q.d, q.y * p.d - p.y * q.d}; }
    inline vector<Edge> rationalEdges(vector<RationalPoint2> v) {
        rationalDomain(v);
        int n = int(v.size()), low = 0; vector<Edge> e(n);
        for (int i = 1; i < n; ++i) {
            lll a = v[i].x * v[low].d, b = v[low].x * v[i].d;
            if (a < b || (a == b && v[i].y * v[low].d < v[low].y * v[i].d)) { low = i; }}
        int turn = crossSign(direction(v[(low + n - 1) % n], v[low]), direction(v[low], v[(low + 1) % n]));
        assert(turn != 0);
        if (turn < 0) { reverse(v.begin(), v.end()); }
        for (int i = 0; i < n; ++i) {
            const RationalPoint2 &p = v[i], &q = v[(i + 1) % n]; Point2<lll> u = direction(p, q);
            assert(u != Point2<lll>{});
            e[i] = edge(p, q, p, u);}
        return e;}
    // Sum of floor((b + sqrt(n - (q x - a)^2)) / q) over x in [l, r], slope in [-1, 0]: Stern–Brocot walk on the lattice hull.
    inline lll hullWalk(lll a, lll b, lll q, lll n, lll l, lll r) {
        if (l > r) { return 0; }
        auto under = [&](lll x, lll y) { lll u = q * x - a, v = q * y - b; return v < 0 || u * u + v * v <= n; };
        auto steep = [&](lll x, point f) { lll u = q * x - a; return u >= 0 && u * u * f.x * f.x >= (n - u * u) * f.y * f.y; };
        lll x = l, y = floorDiv(b + isqrt(n - (q * l - a) * (q * l - a)), q), res = 0;
        auto ok = [&](point v, lll k) { return x + k * v.x <= r && under(x + k * v.x, y - k * v.y); };
        // A run {f, g, k} holds the directions f + j g, j in [1, k]; the runs are the left moves of one Stern–Brocot path.
        struct Run { point f, g; lng k; };
        auto at = [](const Run &u, lng j) { return point{u.f.x + j * u.g.x, u.f.y + j * u.g.y}; };
        auto fits = [&](const Run &u, lng j) { return ok(at(u, j), 1); };
        vector<Run> st{{{1, 1}, {}, 1}, {{1, 0}, {}, 1}};
        while (x < r) {
            point v = at(st.back(), st.back().k); lll k = 0, step = 1;
            while (ok(v, k + step)) { k += step; step *= 2; }
            for (; step; step /= 2) {
                if (ok(v, k + step)) { k += step; }}
            if (k) {
                res += v.x * (k * y - v.y * k * (k - 1) / 2) - k * ((lll(v.x) - 1) * (v.y - 1) / 2 + v.x - 1);
                x += k * v.x; y -= k * v.y;}
            if (x == r) { break; }
            point g = v;
            if (!--st.back().k) { st.pop_back(); }
            // The directions of a run that still fit form a prefix (the region under the arc is convex and closed downward): gallop down from the top.
            while (true) {
                Run &u = st.back(); lng lo = 0, hi = u.k + 1;
                for (lng d = 1; hi - lo > 1; d *= 2) {
                    lng t = max(lo + 1, u.k + 1 - d);
                    if (fits(u, t)) { lo = t; break; }
                    hi = t;}
                while (hi - lo > 1) { lng t = std::midpoint(lo, hi); (fits(u, t) ? lo : hi) = t; }
                if (lo == u.k) { break; }
                g = at(u, hi);
                if (lo) { u.k = lo; break; }
                st.pop_back();}
            point f = at(st.back(), st.back().k);
            // f fits and g does not; mediants refine toward g until the curve beyond x + m.x is steeper than f.
            while (true) {
                Run u{f, g, 0}; lng d = 1;
                while (fits(u, u.k + d)) { u.k += d; d *= 2; }
                for (d /= 2; d; d /= 2) {
                    if (fits(u, u.k + d)) { u.k += d; }}
                if (u.k) { st.push_back(u); f = at(u, u.k); }
                point m = f + g;
                if (x + m.x > r || steep(x + m.x, f)) { break; }
                g = m;}}
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

// T: O(log(C)), M: O(1), C the largest numerator. Rational p, q, p.x != q.x, either order: sum of floor(l(x)) over integers x in
// the closed x-range, l the line through p and q (lattice points 0 < y <= l(x), negated where l(x) < 0). Domain: max(ceil(|x| / d), ceil(|y| / d), 1) * max d <= 4 * 10^18.
inline lll latticeUnderLine(const RationalPoint2 &p, const RationalPoint2 &q) {
    lattice_detail::rationalDomain({p, q});
    bool flip = lattice_detail::less({q.x, q.d}, {p.x, p.d});
    const RationalPoint2 &a = flip ? q : p, &b = flip ? p : q; Point2<lll> u = lattice_detail::direction(a, b);
    assert(u.x > 0);
    auto e = lattice_detail::edge(a, b, a, u);
    return lattice_detail::lineFloorSum(e.dir.y, e.fl, e.dir.x, lattice_detail::ceilDiv(a.x, a.d), lattice_detail::floorDiv(b.x, b.d));}

// T: O(n * log(C)), M: O(n), C as latticeUnderLine. Simple polygon with rational vertices, either orientation, nonzero area, no repeated consecutive vertices, domain as latticeUnderLine;
// lattice points strictly inside, or inside and on the boundary when closed.
inline lll latticeInRationalPolygon(const vector<RationalPoint2> &v, bool closed) {
    assert(v.size() >= 3 && v.size() <= INT_MAX);
    return lattice_detail::count(lattice_detail::rationalEdges(v), closed);}

// T: O(m * log(m) + k * log(C)), M: O(m), k result edges, C = 10^18. Halfplanes as in halfplaneIntersection, a bounded result inside |coordinate| <= 10^18;
// lattice points of the open interior, or of the closed intersection when closed; -1 when infinitely many (an unbounded result gives 0 or -1).
inline lll latticeInHalfplanePolygon(const vector<Halfplane> &h, bool closed) {
    HalfplaneResult r = halfplaneIntersection(h);
    if (r.kind == HalfplaneKind::Ray || r.kind == HalfplaneKind::Line || r.kind == HalfplaneKind::Unbounded) { return lattice_detail::unbounded(r, closed); }
    for (const auto &p : r.vertices) { assert(abs(p.x) <= 1000000000000000000LL * p.d && abs(p.y) <= 1000000000000000000LL * p.d); }
    if (r.kind == HalfplaneKind::Empty || (!closed && r.kind != HalfplaneKind::Polygon)) { return 0; }
    if (r.kind == HalfplaneKind::Point) { return r.vertices[0].d == 1; }
    auto edge = [](const Halfplane &s, const RationalPoint2 &p, const RationalPoint2 &q) {
        point w = s.b - s.a;
        return lattice_detail::edge(p, q, RationalPoint2(s.a), Point2<lll>{w.x, w.y});};
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
// T: O(r^(2/3) * log(r) + 1), M: O(log(r)), r = sqrt(n) / q. Same count by diagonal caps and Stern–Brocot hull walks.
inline lll latticeInDiskFast(lng a, lng b, lng q, lng n) {
    assert(abs(a) <= 1000000000000000000LL && abs(b) <= 1000000000000000000LL && 1 <= q && q <= 1000000000000000000LL && 0 <= n && n <= 4000000000000000000LL);
    return (a % q == 0 && b % q == 0) + lattice_detail::cap(a, b, q, n, false) + lattice_detail::cap(a, -lll(b), q, n, false)
         + lattice_detail::cap(b, a, q, n, true) + lattice_detail::cap(b, -lll(a), q, n, true);}
