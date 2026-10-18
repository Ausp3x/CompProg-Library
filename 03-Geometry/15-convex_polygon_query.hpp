#pragma once
#include "../01-Core/01-template.hpp"
#include "01-point.hpp"
#include "02-line_segment.hpp"
#include "03-polygon.hpp"
#include "10-halfplaneintersection.hpp"

namespace convex_query_detail {
    inline bool half(point a) { return a.y < 0 || (a.y == 0 && a.x < 0); }
    inline bool angleLess(point a, point b) { return half(a) != half(b) ? half(a) < half(b) : cross(a, b) > 0; }
    template<typename F> int firstFalse(int l, int r, F f) {
        while (l < r) {
            int m = std::midpoint(l, r);
            if (f(m)) { l = m + 1; } else { r = m; }}
        return l;}
    struct Frac {
        lll num, den;
        friend bool operator<(const Frac &x, const Frac &y) { return x.num * y.den < y.num * x.den; }
        friend bool operator==(const Frac &x, const Frac &y) { return x.num * y.den == y.num * x.den; }
    };
    // Rightmost row maxima of a totally monotone matrix: better(r, u, v) for u < v holds for every row after r once it holds at r.
    template<typename B> vector<int> smawk(const vector<int> &rows, const vector<int> &cols, B better) {
        int n = int(rows.size());
        if (!n) { return {}; }
        vector<int> c, odd;
        for (int x : cols) {
            while (!c.empty() && better(rows[c.size() - 1], c.back(), x)) { c.pop_back(); }
            if (int(c.size()) < n) { c.push_back(x); }}
        for (int i = 1; i < n; i += 2) { odd.push_back(rows[i]); }
        vector<int> sub = smawk(odd, c, better), res(n);
        for (int i = 0; i < int(sub.size()); ++i) { res[2 * i + 1] = sub[i]; }
        for (int i = 0, j = 0; i < n; i += 2) {
            res[i] = c[j];
            int end = i + 1 < n ? res[i + 1] : c.back();
            while (c[j] != end) {
                ++j;
                if (better(rows[i], res[i], c[j])) { res[i] = c[j]; }}}
        return res;}
    // Best path s = v[0] < ... < v[k - 1] < s + n, lo[t] <= v[t] <= hi[t], closed by w(v[k - 1], s + n); rightmost optimal predecessors move right.
    template<typename V, bool SMAWK, typename W> pair<V, vector<int>> rooted(int n, int k, const vector<int> &lo, const vector<int> &hi, W w) {
        int s = lo[0]; vector<vector<V>> f(k); vector<vector<int>> arg(k); f[0] = {V{}};
        for (int t = 1; t < k; ++t) {
            f[t].resize(hi[t] - lo[t] + 1); arg[t].resize(hi[t] - lo[t] + 1);
            auto val = [&](int i, int j) { return f[t - 1][i - lo[t - 1]] + w(i, j); };
            if constexpr (SMAWK) {
                vector<int> rows(hi[t] - lo[t] + 1), cols(hi[t - 1] - lo[t - 1] + 1);
                iota(rows.begin(), rows.end(), lo[t]); iota(cols.begin(), cols.end(), lo[t - 1]);
                // A column at or after its row is invalid and loses to every column on its left.
                auto best = smawk(rows, cols, [&](int r, int u, int v) { return v < r && !(val(v, r) < val(u, r)); });
                for (int x = 0; x < int(rows.size()); ++x) { arg[t][x] = best[x]; f[t][x] = val(best[x], rows[x]); }}
            else {
                auto solve = [&](auto &&self, int jl, int jr, int il, int ir) -> void {
                    if (jl > jr) { return; }
                    int j = std::midpoint(jl, jr), best = il; V bv = val(il, j);
                    for (int i = il + 1; i <= min(ir, j - 1); ++i) {
                        V c = val(i, j);
                        if (!(c < bv)) { best = i; bv = c; }}
                    f[t][j - lo[t]] = bv; arg[t][j - lo[t]] = best;
                    self(self, jl, j - 1, il, best); self(self, j + 1, jr, best, ir);};
                solve(solve, lo[t], hi[t], lo[t - 1], hi[t - 1]);}}
        int best = lo[k - 1]; V bv{};
        for (int j = lo[k - 1]; j <= hi[k - 1]; ++j) {
            V c = f[k - 1][j - lo[k - 1]] + w(j, s + n);
            if (j == lo[k - 1] || bv < c) { best = j; bv = c; }}
        vector<int> v(k); v[k - 1] = best;
        for (int t = k - 1; t > 0; --t) { v[t - 1] = arg[t][v[t] - lo[t]]; }
        return {bv, v};}
    // Interleaving divide and conquer over the roots in [1, a[1]] of an optimal path a rooted at 0 (Boyce et al.).
    template<typename V, bool SMAWK, typename W> pair<V, vector<int>> inscribed(int n, int k, V whole, W w) {
        vector<int> lo(min(k, n));
        iota(lo.begin(), lo.end(), 0);
        if (k >= n) { return {whole, lo}; }
        vector<int> hi(k);
        for (int t = 0; t < k; ++t) { hi[t] = t ? n - k + t : 0; }
        auto res = rooted<V, SMAWK>(n, k, lo, hi, w);
        vector<int> up(k);
        for (int t = 0; t < k; ++t) { up[t] = t + 1 < k ? res.second[t + 1] : n; }
        auto rec = [&](auto &&self, int sl, int sr, const vector<int> &l, const vector<int> &u) -> void {
            if (sl > sr) { return; }
            int s = std::midpoint(sl, sr); vector<int> a(k), b(k);
            a[0] = b[0] = s;
            for (int t = 1; t < k; ++t) { a[t] = max(l[t], a[t - 1] + 1); b[t] = min(u[t], s + n - k + t); }
            auto cur = rooted<V, SMAWK>(n, k, a, b, w);
            if (res.first < cur.first) { res = cur; }
            self(self, sl, s - 1, l, cur.second); self(self, s + 1, sr, cur.second, u);};
        vector<int> a0 = res.second;
        rec(rec, 1, a0[1], a0, up);
        for (int &x : res.second) { x %= n; }
        sort(res.second.begin(), res.second.end());
        return res;}
} // namespace convex_query_detail

// T: O(1), M: O(1). count 0, 1 (a point) or 2 (a segment from points[0] to points[1] along the query direction); only [0, count) is meaningful.
struct ConvexLineIntersection {
    int count = 0;
    array<RationalPoint2, 2> points{};
};

// S: O(n), Q: O(log(n)), M: O(n). Strictly convex CCW ring, n >= 3, any start (convexHull(..., false)); integral |coordinate| <= 10^9, indices into the input.
// tangents needs q outside; contains and cut are closed; prefixArea2 is twice the area of p[i..j], cyclic; rounded results end in Approx.
struct ConvexPolygonQuery {
    int n, s0 = 0;
    vector<point> p;
    vector<lll> pre;

    template<typename T> explicit ConvexPolygonQuery(const vector<Point2<T>> &poly) : n(int(poly.size())), p(poly.size()), pre(poly.size() + 1) {
        static_assert(std::is_integral_v<T>);
        assert(poly.size() >= 3 && poly.size() <= INT_MAX / 2);
        for (int i = 0; i < n; ++i) { p[i] = poly[i].template cast<lng>(); linear_detail::check(p[i]); }
        int wraps = 0;
        for (int i = 0; i < n; ++i) {
            pre[i + 1] = pre[i] + cross(p[i], at(i + 1));
            wraps += convex_query_detail::angleLess(edge(i + 1), edge(i));
            if (convex_query_detail::angleLess(edge(i), edge(s0))) { s0 = i; }}
        assert(polygonConvex(p, true) && signedArea2(p) > 0 && wraps == 1);}

    point at(int i) const { return p[i % n]; }
    point edge(int i) const { return at(i + 1) - at(i); }
    lll prefixArea2(int i, int j) const {
        assert(0 <= i && i < n && 0 <= j && j < n);
        return (i <= j ? pre[j] - pre[i] : pre[n] - pre[i] + pre[j]) + cross(p[j], p[i]);}

    PolygonLocation contains(point q) const {
        linear_detail::check(q);
        point o = p[0];
        if (orient(o, p[1], q) < 0 || orient(o, p[n - 1], q) > 0) { return PolygonLocation::Outside; }
        int i = convex_query_detail::firstFalse(2, n - 1, [&](int k) { return orient(o, p[k], q) >= 0; }) - 1, s = orient(p[i], p[i + 1], q);
        if (s < 0) { return PolygonLocation::Outside; }
        bool edge0 = i == 1 && orient(o, p[1], q) == 0, edge1 = i == n - 2 && orient(o, p[n - 1], q) == 0;
        return s == 0 || edge0 || edge1 ? PolygonLocation::Boundary : PolygonLocation::Inside;}
    // T: O(n + q), M: O(q). Queries sorted by polarLess(q - p[0]) (asserted); results in query order.
    vector<PolygonLocation> batchedLocate(const vector<point> &qs) const {
        assert(qs.size() <= INT_MAX);
        int m = int(qs.size()), z = 0; point o = p[0], u = p[1] - o; vector<PolygonLocation> res(m, PolygonLocation::Outside);
        for (int t = 0; t < m; ++t) {
            linear_detail::check(qs[t]);
            assert(t == 0 || !polarLess(qs[t] - o, qs[t - 1] - o));}
        for (; z < m && qs[z] == o; ++z) { res[z] = PolygonLocation::Boundary; }
        int r = convex_query_detail::firstFalse(z, m, [&](int t) { return convex_query_detail::angleLess(qs[t] - o, u); }), i = 1;
        for (int c = 0; c < m - z; ++c) {
            int t = r + c < m ? r + c : r + c - (m - z);
            point q = qs[t];
            if (orient(o, p[1], q) < 0 || orient(o, p[n - 1], q) > 0) { continue; }
            while (i < n - 2 && orient(o, p[i + 1], q) >= 0) { ++i; }
            int s = orient(p[i], p[i + 1], q);
            if (s < 0) { continue; }
            bool edge0 = i == 1 && orient(o, p[1], q) == 0, edge1 = i == n - 2 && orient(o, p[n - 1], q) == 0;
            res[t] = s == 0 || edge0 || edge1 ? PolygonLocation::Boundary : PolygonLocation::Inside;}
        return res;}
    int extreme(point d) const {
        const lng B = 1000000000000000000LL; assert(d != point{} && -B <= d.x && d.x <= B && -B <= d.y && d.y <= B);
        point g = perp(d);
        return (s0 + convex_query_detail::firstFalse(0, n, [&](int t) { return convex_query_detail::angleLess(edge(s0 + t), g); })) % n;}
    pair<int, int> tangents(point q) const {
        assert(contains(q) == PolygonLocation::Outside);
        auto visible = [&](int k) { return orient(at(k), at(k + 1), q) < 0; };
        auto fan = [&](point u) { return convex_query_detail::firstFalse(2, n - 1, [&](int k) { return cross(p[k] - p[0], u) >= 0; }) - 1; };
        int v = visible(0) ? 0 : visible(n - 1) ? n - 1 : fan(q - p[0]);
        int w = !visible(0) ? 0 : !visible(n - 1) ? n - 1 : fan(p[0] - q);
        int a = convex_query_detail::firstFalse(1, (w - v + n) % n, [&](int t) { return visible(v + t); });
        int b = convex_query_detail::firstFalse(1, (v - w + n) % n, [&](int t) { return !visible(w + t); });
        return {(w + b) % n, (v + a) % n};}
    long double pointDistanceApprox(point q) const {
        if (contains(q) != PolygonLocation::Outside) { return 0; }
        auto [s, e] = tangents(q); int len = (e - s + n) % n;
        int c = convex_query_detail::firstFalse(0, len, [&](int t) { return dot(edge(s + t), q - at(s + t)) > 0; });
        long double res = std::numeric_limits<long double>::infinity();
        if (c > 0) { res = pointSegmentDistanceApprox(q, at(s + c - 1), at(s + c)); }
        if (c < len) { res = min(res, pointSegmentDistanceApprox(q, at(s + c), at(s + c + 1))); }
        return res;}
    ConvexLineIntersection lineIntersection(Linear2 s) const {
        using convex_query_detail::Frac;
        linear_detail::check(s);
        if (s.a == s.b) { return contains(s.a) == PolygonLocation::Outside ? ConvexLineIntersection{} : ConvexLineIntersection{1, {RationalPoint2(s.a), {}}}; }
        point w = s.b - s.a; lll ww = norm2(w);
        auto side = [&](int k) { return cross(w, at(k) - s.a); };
        auto vertex = [&](int k) { return Frac{dot(at(k) - s.a, w), ww}; };
        auto crossing = [&](int k) {
            point u = at(k), v = edge(k); lll d = cross(w, v), m = cross(u - s.a, v);
            return d < 0 ? Frac{-m, -d} : Frac{m, d};};
        int hi = extreme(perp(w)), lo = extreme(-perp(w)); lll smax = side(hi), smin = side(lo);
        if (smax < 0 || smin > 0) { return {}; }
        Frac x, y;
        if (smax == 0 || smin == 0) {
            int k = smax == 0 ? hi : lo;
            x = y = vertex(k);
            if (side(k + 1) == 0) { y = vertex(k + 1); }
            if (y < x) { swap(x, y); }}
        else {
            int c = (lo + convex_query_detail::firstFalse(0, (hi - lo + n) % n + 1, [&](int t) { return side(lo + t) < 0; })) % n;
            int d = (hi + convex_query_detail::firstFalse(0, (lo - hi + n) % n + 1, [&](int t) { return side(hi + t) > 0; })) % n;
            y = side(c) == 0 ? vertex(c) : crossing(c + n - 1);
            x = side(d) == 0 ? vertex(d) : crossing(d + n - 1);}
        if (s.kind != LinearKind::Line && x < Frac{0, 1}) { x = {0, 1}; }
        if (s.kind == LinearKind::Segment && Frac{1, 1} < y) { y = {1, 1}; }
        if (y < x) { return {}; }
        auto point_at = [&](Frac t) { return RationalPoint2(s.a.x * t.den + w.x * t.num, s.a.y * t.den + w.y * t.num, t.den); };
        if (x == y) { return {1, {point_at(x), {}}}; }
        return {2, {point_at(x), point_at(y)}};}
    int leftChain(point a, point b, int &j1, int &j2) const {
        linear_detail::check(a); linear_detail::check(b); assert(a != b);
        point w = b - a;
        auto side = [&](int k) { return cross(w, at(k) - a); };
        int hi = extreme(perp(w)), lo = extreme(-perp(w)); lll smax = side(hi), smin = side(lo);
        j1 = hi; j2 = hi + 1 + (side(hi + 1) == 0);
        if (smin >= 0 || smax <= 0) { return smin >= 0 ? 2 : smax < 0 ? -1 : 0; }
        j1 = (lo + convex_query_detail::firstFalse(0, (hi - lo + n) % n + 1, [&](int t) { return side(lo + t) <= 0; })) % n;
        j2 = (hi + convex_query_detail::firstFalse(0, (lo - hi + n) % n + 1, [&](int t) { return side(hi + t) > 0; })) % n;
        if (j2 <= j1) { j2 += n; }
        return 1;}
    RationalPoint2 crossing(point a, point b, int k) const {
        point u = at(k), v = at(k + 1); lll su = cross(b - a, u - a), sv = cross(b - a, v - a);
        return {v.x * su - u.x * sv, v.y * su - u.y * sv, su - sv};}
    // T: O(log(n) + out), M: O(out). Closed left side of a -> b, a != b: CCW ring from its first crossing, a point or two points when touching, empty when disjoint.
    vector<RationalPoint2> cut(point a, point b) const {
        int j1, j2, kind = leftChain(a, b, j1, j2); vector<RationalPoint2> res;
        if (kind == 2) { res.assign(p.begin(), p.end()); }
        if (kind == 0) {
            for (int k = j1; k < j2; ++k) { res.push_back(at(k)); }}
        if (kind == 1) {
            res.push_back(crossing(a, b, j1 - 1 + n));
            for (int k = j1; k < j2; ++k) { res.push_back(at(k)); }
            res.push_back(crossing(a, b, j2 - 1));}
        return res;}
    long double areaLeftOfLineApprox(point a, point b) const {
        int j1, j2, kind = leftChain(a, b, j1, j2);
        if (kind != 1) { return kind == 2 ? static_cast<long double>(pre[n]) / 2 : 0; }
        dpoint x = crossing(a, b, j1 - 1 + n).approx(), y = crossing(a, b, j2 - 1).approx();
        dpoint u = at(j1).cast<long double>() - x, v = at(j2 - 1).cast<long double>() - x;
        return (static_cast<long double>(prefixArea2(j1, (j2 - 1) % n)) + cross(u, v) + cross(v, y - x)) / 2;}
};

// T: O(n + m), M: O(n + m). Two convex CCW polygons (collinear vertices allowed, positive area), integral |coordinate| <= 10^9; exact, as clipConvex.
template<typename T> HalfplaneResult convexIntersection(const vector<Point2<T>> &p, const vector<Point2<T>> &q) {
    static_assert(std::is_integral_v<T>);
    for (const auto *r : {&p, &q}) { assert(r->size() >= 3 && r->size() <= INT_MAX / 2 && polygonConvex(*r, false) && signedArea2(*r) > 0); }
    auto e = halfplane_detail::edges(p), f = halfplane_detail::edges(q);
    vector<Halfplane> all(e.size() + f.size());
    std::merge(e.begin(), e.end(), f.begin(), f.end(), all.begin(), halfplane_detail::less);
    return halfplane_detail::solve(std::move(all));}

// T: O(1), M: O(k). Increasing vertex indices of the k-gon and its value (twice the area, or the rounded perimeter).
template<typename V> struct InscribedKgon {
    V value{};
    vector<int> vertices;
};

// T: O(k * n * log(n) + n * log(n)^2), M: O(k * n). Strictly convex CCW ring, n >= 3, 3 <= k (k >= n takes every vertex); vertices among the polygon's.
// Area exact; perimeter rounded, so near ties may pick a near-optimal k-gon.
template<typename T> InscribedKgon<lll> maxAreaInscribedKgon(const vector<Point2<T>> &poly, int k) {
    ConvexPolygonQuery c(poly); assert(k >= 3);
    auto r = convex_query_detail::inscribed<lll, false>(c.n, max(k, 3), c.pre[c.n], [&](int i, int j) { return cross(c.at(i), c.at(j)); });
    return {r.first, r.second};}
template<typename T> InscribedKgon<long double> maxPerimeterInscribedKgonApprox(const vector<Point2<T>> &poly, int k) {
    ConvexPolygonQuery c(poly); assert(k >= 3);
    auto r = convex_query_detail::inscribed<long double, false>(c.n, max(k, 3), polygonPerimeter(c.p), [&](int i, int j) { return distanceApprox(c.at(i), c.at(j)); });
    return {r.first, r.second};}

// T: O(k * n + n * log(n)), M: O(k * n). As maxAreaInscribedKgon and maxPerimeterInscribedKgonApprox, with SMAWK row maxima in every layer.
template<typename T> InscribedKgon<lll> maxAreaInscribedKgonSmawk(const vector<Point2<T>> &poly, int k) {
    ConvexPolygonQuery c(poly); assert(k >= 3);
    auto r = convex_query_detail::inscribed<lll, true>(c.n, max(k, 3), c.pre[c.n], [&](int i, int j) { return cross(c.at(i), c.at(j)); });
    return {r.first, r.second};}
template<typename T> InscribedKgon<long double> maxPerimeterInscribedKgonSmawkApprox(const vector<Point2<T>> &poly, int k) {
    ConvexPolygonQuery c(poly); assert(k >= 3);
    auto r = convex_query_detail::inscribed<long double, true>(c.n, max(k, 3), polygonPerimeter(c.p), [&](int i, int j) { return distanceApprox(c.at(i), c.at(j)); });
    return {r.first, r.second};}
