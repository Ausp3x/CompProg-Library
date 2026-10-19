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
    // Rightmost row maxima of a totally monotone matrix f(r, c) into res; a column at or after its row is invalid and loses to every column on its
    // left. Each level pushes its reduced columns, odd rows and their answers on ints (cached values on vals) and pops them on return.
    template<typename V, typename F> void smawk(const int *rows, int n, const int *cols, int m, int *res, vector<int> &ints, vector<V> &vals, F f) {
        if (!n) { return; }
        size_t ib = ints.size(), vb = vals.size();
        // vals caches f(rows[k], c[k]), the value the k-th surviving column is compared with while it is the top.
        for (int i = 0; i < m; ++i) {
            int x = cols[i], k = int(ints.size() - ib);
            while (k && x < rows[k - 1] && !(f(rows[k - 1], x) < vals.back())) { ints.pop_back(); vals.pop_back(); --k; }
            if (k < n) { ints.push_back(x); vals.push_back(x < rows[k] ? f(rows[k], x) : V{}); }}
        int k = int(ints.size() - ib), h = n / 2; size_t ob = ints.size();
        for (int i = 1; i < n; i += 2) { ints.push_back(rows[i]); }
        ints.resize(ob + 2 * size_t(h));
        smawk<V>(ints.data() + ob, h, ints.data() + ib, k, ints.data() + ob + h, ints, vals, f);
        const int *c = ints.data() + ib, *sub = ints.data() + ob + h;
        for (int i = 0; i < h; ++i) { res[2 * i + 1] = sub[i]; }
        for (int i = 0, j = 0; i < n; i += 2) {
            res[i] = c[j];
            int end = i + 1 < n ? res[i + 1] : c[k - 1]; V bv = f(rows[i], c[j]);
            while (c[j] != end) {
                ++j;
                if (c[j] >= rows[i]) { continue; }
                V x = f(rows[i], c[j]);
                if (!(x < bv)) { res[i] = c[j]; bv = x; }}}
        ints.resize(ib); vals.resize(vb);}
    // One layer: fc[j - l] = max over i < j of fp[i - pl] + w(i, j), arg the rightmost maximizer; ints and vals are SMAWK's reusable workspace.
    template<typename V, bool SMAWK, typename W> void layer(int pl, int ph, const vector<V> &fp, int l, int h, vector<V> &fc, vector<int> &arg, W w, vector<int> &ints, vector<V> &vals) {
        auto val = [&](int i, int j) { return fp[i - pl] + w(i, j); };
        fc.assign(h - l + 1, V{}); arg.assign(h - l + 1, 0);
        if constexpr (SMAWK) {
            // Rows, columns and five rows' worth of levels: the reserve keeps every pointer into ints valid down the recursion.
            int n = h - l + 1, m = ph - pl + 1;
            ints.clear(); vals.clear(); ints.reserve(size_t(m) + 6 * size_t(n) + 64); vals.reserve(2 * size_t(n) + 64);
            for (int x = l; x <= h; ++x) { ints.push_back(x); }
            for (int x = pl; x <= ph; ++x) { ints.push_back(x); }
            smawk<V>(ints.data(), n, ints.data() + n, m, arg.data(), ints, vals, [&](int r, int c) { return val(c, r); });
            for (int x = 0; x < n; ++x) { fc[x] = val(arg[x], l + x); }}
        else {
            auto solve = [&](auto &&self, int jl, int jr, int il, int ir) -> void {
                if (jl > jr) { return; }
                int j = std::midpoint(jl, jr), best = il; V bv = val(il, j);
                for (int i = il + 1; i <= min(ir, j - 1); ++i) {
                    V c = val(i, j);
                    if (!(c < bv)) { best = i; bv = c; }}
                fc[j - l] = bv; arg[j - l] = best;
                self(self, jl, j - 1, il, best); self(self, j + 1, jr, best, ir);};
            solve(solve, l, h, pl, ph);}}
    // Best path lo[0] = v[0] < ... < v[m - 1] < e, lo[t] <= v[t] <= hi[t], closed by w(v[m - 1], e). A table above cap entries and 2 (e - lo[0] + m)
    // is not stored: the forward pass carries each state's vertex at layer m / 2, its buffers are freed, and both halves are solved again (Hirschberg).
    template<typename V, bool SMAWK, typename W> pair<V, vector<int>> path(const vector<int> &lo, const vector<int> &hi, int e, W w, lng cap) {
        int m = int(lo.size()), mid = m / 2, best = lo[m - 1], x = 0; lng cells = 0; V bv{}; vector<vector<int>> args;
        for (int t = 0; t < m; ++t) { cells += hi[t] - lo[t] + 1; }
        bool direct = m <= 2 || cells <= max(2 * (lng(e) - lo[0] + m), cap);
        {
            vector<V> fp{V{}}, fc, vals; vector<int> arg, mv{lo[0]}, mc, ints;
            if (direct) { args.reserve(m - 1); }
            for (int t = 1; t < m; ++t) {
                vector<int> &ar = direct ? args.emplace_back() : arg;
                layer<V, SMAWK>(lo[t - 1], hi[t - 1], fp, lo[t], hi[t], fc, ar, w, ints, vals);
                if (!direct && t >= mid) {
                    mc.resize(ar.size());
                    for (int y = 0; y < int(ar.size()); ++y) { mc[y] = t == mid ? lo[t] + y : mv[ar[y] - lo[t - 1]]; }
                    mv.swap(mc);}
                fp.swap(fc);}
            for (int j = lo[m - 1]; j <= hi[m - 1]; ++j) {
                V c = fp[j - lo[m - 1]] + w(j, e);
                if (j == lo[m - 1] || bv < c) { best = j; bv = c; }}
            if (!direct) { x = mv[best - lo[m - 1]]; }}
        vector<int> v(m); v[m - 1] = best;
        if (direct) {
            for (int t = m - 1; t > 0; --t) { v[t - 1] = args[t - 1][v[t] - lo[t]]; }
            return {bv, v};}
        vector<int> l1(lo.begin(), lo.begin() + mid), h1(hi.begin(), hi.begin() + mid), l2(lo.begin() + mid, lo.end()), h2(hi.begin() + mid, hi.end());
        for (int t = 0; t < mid; ++t) { h1[t] = min(h1[t], x - (mid - t)); }
        for (int t = 0; t < m - mid; ++t) { l2[t] = max(l2[t], x + t); h2[t] = t ? h2[t] : x; }
        auto a = path<V, SMAWK>(l1, h1, x, w, cap).second, b = path<V, SMAWK>(l2, h2, e, w, cap).second;
        std::copy(a.begin(), a.end(), v.begin()); std::copy(b.begin(), b.end(), v.begin() + mid);
        return {bv, v};}
    // Interleaving divide and conquer over the roots in [1, a[1]] of an optimal path a rooted at 0 (Boyce et al.); tables up to cap entries are stored.
    template<typename V, bool SMAWK, typename W> pair<V, vector<int>> inscribed(int n, int k, V whole, W w, lng cap = lng(1) << 23) {
        vector<int> lo(min(k, n));
        iota(lo.begin(), lo.end(), 0);
        if (k >= n) { return {whole, lo}; }
        vector<int> hi(k);
        for (int t = 0; t < k; ++t) { hi[t] = t ? n - k + t : 0; }
        auto res = path<V, SMAWK>(lo, hi, n, w, cap);
        vector<int> up(k);
        for (int t = 0; t < k; ++t) { up[t] = t + 1 < k ? res.second[t + 1] : n; }
        auto rec = [&](auto &&self, int sl, int sr, const vector<int> &l, const vector<int> &u) -> void {
            if (sl > sr) { return; }
            int s = std::midpoint(sl, sr); vector<int> a(k), b(k);
            a[0] = b[0] = s;
            for (int t = 1; t < k; ++t) { a[t] = max(l[t], a[t - 1] + 1); b[t] = min(u[t], s + n - k + t); }
            auto cur = path<V, SMAWK>(a, b, s + n, w, cap);
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

// T: O(1), M: O(1). Twice an area, exactly whole + num / den with 0 <= num < den and gcd(num, den) = 1.
struct ExactArea2 {
    lll whole = 0, num = 0, den = 1;
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
        [[maybe_unused]] int wraps = 0;
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
    // T: O(q * log(2 + n / q)), M: O(q). Queries sorted by polarLess(q - p[0]) (asserted); results in query order.
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
            // Gallop to the last fan vertex not counterclockwise of q; orient(o, p[i], q) >= 0 holds from the previous query.
            int step = 1;
            while (i + step <= n - 2 && orient(o, p[i + step], q) >= 0) { i += step; step *= 2; }
            for (step /= 2; step > 0; step /= 2) {
                if (i + step <= n - 2 && orient(o, p[i + step], q) >= 0) { i += step; }}
            int s = orient(p[i], p[i + 1], q);
            if (s < 0) { continue; }
            bool edge0 = i == 1 && orient(o, p[1], q) == 0, edge1 = i == n - 2 && orient(o, p[n - 1], q) == 0;
            res[t] = s == 0 || edge0 || edge1 ? PolygonLocation::Boundary : PolygonLocation::Inside;}
        return res;}
    int extreme(point d) const {
        [[maybe_unused]] const lng B = 1000000000000000000LL; assert(d != point{} && -B <= d.x && d.x <= B && -B <= d.y && d.y <= B);
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
    pair<lll, lll> pointDistance2(point q) const {
        if (contains(q) != PolygonLocation::Outside) { return {0, 1}; }
        auto [s, e] = tangents(q); int len = (e - s + n) % n;
        int c = convex_query_detail::firstFalse(0, len, [&](int t) { return dot(edge(s + t), q - at(s + t)) > 0; });
        // The nearest point lies on edge c - 1, past its start: its projection or its end.
        point a = at(s + c - (c > 0)), b = at(s + c);
        if (c == 0 || dot(a - b, q - b) <= 0) { return {norm2(q - b), 1}; }
        lll h = cross(b - a, q - a), num = h * h, den = norm2(b - a), g = gcd(num, den);
        return {num / g, den / g};}
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
    ExactArea2 areaLeftOfLine2(point a, point b) const {
        int j1, j2, kind = leftChain(a, b, j1, j2);
        if (kind != 1) { return {kind == 2 ? pre[n] : 0, 0, 1}; }
        // Origin a: both crossings lie on a + t (b - a), so each adds (its edge parameter) * cross(edge ends) and cross(y, x) = 0.
        point w = b - a, u1 = at(j1 - 1 + n) - a, v1 = at(j1) - a, u2 = at(j2 - 1) - a, v2 = at(j2) - a;
        lll t1 = cross(w, v1), d1 = t1 - cross(w, u1), s2 = cross(w, u2), d2 = s2 - cross(w, v2);
        lll n1 = t1 * cross(u1, v1), n2 = s2 * cross(u2, v2), q1 = n1 / d1 - (n1 % d1 < 0), q2 = n2 / d2 - (n2 % d2 < 0);
        lll whole = prefixArea2(j1, (j2 - 1) % n) - cross(at(j2 - 1), at(j1)) - cross(a, at(j2 - 1) - at(j1)) + q1 + q2;
        lll den = d1 * d2, f = (n1 - q1 * d1) * d2 + (n2 - q2 * d2) * d1;
        if (f >= den) { f -= den; ++whole; }
        lll g = gcd(f, den);
        return {whole, f / g, den / g};}
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

// T: O(k * n * log(n) + n * log(n)^2), M: O(n + k * log(n)). Strictly convex CCW ring, n >= 3, 3 <= k (k >= n takes every vertex); vertices among the polygon's.
// Area exact; perimeter rounded, so near ties may pick a near-optimal k-gon.
template<typename T> InscribedKgon<lll> maxAreaInscribedKgon(const vector<Point2<T>> &poly, int k) {
    ConvexPolygonQuery c(poly); assert(k >= 3);
    auto r = convex_query_detail::inscribed<lll, false>(c.n, max(k, 3), c.pre[c.n], [&](int i, int j) { return cross(c.at(i), c.at(j)); });
    return {r.first, r.second};}
template<typename T> InscribedKgon<long double> maxPerimeterInscribedKgonApprox(const vector<Point2<T>> &poly, int k) {
    ConvexPolygonQuery c(poly); assert(k >= 3);
    auto r = convex_query_detail::inscribed<long double, false>(c.n, max(k, 3), polygonPerimeter(c.p), [&](int i, int j) { return distanceApprox(c.at(i), c.at(j)); });
    return {r.first, r.second};}

// T: O(k * n + n * log(n)), M: O(n + k * log(n)). As maxAreaInscribedKgon and maxPerimeterInscribedKgonApprox, with SMAWK row maxima in every layer.
template<typename T> InscribedKgon<lll> maxAreaInscribedKgonSmawk(const vector<Point2<T>> &poly, int k) {
    ConvexPolygonQuery c(poly); assert(k >= 3);
    auto r = convex_query_detail::inscribed<lll, true>(c.n, max(k, 3), c.pre[c.n], [&](int i, int j) { return cross(c.at(i), c.at(j)); });
    return {r.first, r.second};}
template<typename T> InscribedKgon<long double> maxPerimeterInscribedKgonSmawkApprox(const vector<Point2<T>> &poly, int k) {
    ConvexPolygonQuery c(poly); assert(k >= 3);
    auto r = convex_query_detail::inscribed<long double, true>(c.n, max(k, 3), polygonPerimeter(c.p), [&](int i, int j) { return distanceApprox(c.at(i), c.at(j)); });
    return {r.first, r.second};}

// T: O(n), M: O(n). As maxAreaInscribedKgon with k = 3: Jin–Zhu–Luo's Rotate-and-Kill over the 3-stable triangles.
template<typename T> InscribedKgon<lll> maxAreaInscribedTriangle(const vector<Point2<T>> &poly) {
    ConvexPolygonQuery c(poly); int n = c.n;
    if (n == 3) { return {c.pre[n], {0, 1, 2}}; }
    // The paper lists vertices clockwise: index x is c.p[n - 1 - x mod n], and A(i, j, k) is twice the area of the clockwise triangle.
    auto at = [&](int x) { return c.p[n - 1 - (x % n + n) % n]; };
    auto area = [&](int i, int j, int k) { return cross(at(k) - at(i), at(j) - at(i)); };
    auto same = [&](int x, int y) { return (x - y) % n == 0; };
    int r = 0, s = 1, t = 2;
    for (int j = 1, k = 2; j <= n - 2; ++j) {
        k += k == j;
        while (k + 1 < n && area(r, j, k + 1) > area(r, j, k)) { ++k; }
        if (area(r, j, k) > area(r, s, t)) { s = j; t = k; }}
    auto fr = [&] { return same(r + 1, s) || area(r + 1, s, t) <= area(r, s, t); };
    auto fs = [&] { return same(s + 1, t) || area(r, s + 1, t) <= area(r, s, t); };
    auto ft = [&] { return same(t + 1, r) || area(r, s, t + 1) <= area(r, s, t); };
    auto br = [&] { return same(r - 1, t) || area(r - 1, s, t) <= area(r, s, t); };
    auto bs = [&] { return same(s - 1, r) || area(r, s - 1, t) <= area(r, s, t); };
    auto bt = [&] { return same(t - 1, s) || area(r, s, t - 1) <= area(r, s, t); };
    // Rounds of one-way moves, each raising the area, until all three corners are stable: at most n moves (see the evidence).
    if (!fr()) {
        for (bool moved = true; moved;) {
            moved = false;
            while (!fr()) { ++r; moved = true; }
            while (!fs()) { ++s; moved = true; }
            while (!ft()) { ++t; moved = true; }}}
    else {
        for (bool moved = true; moved;) {
            moved = false;
            while (!br()) { --r; moved = true; }
            while (!bs()) { --s; moved = true; }
            while (!bt()) { --t; moved = true; }}}
    assert(fr() && fs() && ft() && br() && bs() && bt());
    r = (r % n + n) % n; s = (s % n + n) % n; t = (t % n + n) % n;
    if (s < r) { s += n; }
    if (t < s) { t += n; }
    int a = r, b = s, d = t, best[3] = {r, s, t}; lll bv = area(r, s, t);
    for (int it = 0; it <= 2 * n && !(b == t && d == r + n); ++it) {
        while (area(a + 1, b, d) > area(a, b, d)) { ++a; }
        if (area(a, b, d) > bv) { bv = area(a, b, d); best[0] = a; best[1] = b; best[2] = d; }
        // Kill: advance b unless a lies farther from line b d than I, where the rays b + x e_d and d - y e_b (x, y >= 0) meet.
        point w = at(d) - at(b), eb = at(b + 1) - at(b), ed = at(d + 1) - at(d);
        lll den = cross(ed, eb), x = cross(w, eb), y = cross(ed, w), h = area(a, b, d);
        bool meet = den != 0 && (x == 0 || (x > 0) == (den > 0)) && (y == 0 || (y > 0) == (den > 0));
        if (meet && (den > 0 ? h * den > x * y : h * den < x * y)) { ++d; } else { ++b; }}
    assert(b == t && d == r + n);
    vector<int> v;
    for (int x : best) { v.push_back(n - 1 - x % n); }
    sort(v.begin(), v.end());
    return {bv, v};}
// T: O(n), M: O(n). As maxAreaInscribedKgon with k = 4: Rote's antipodal sweep.
template<typename T> InscribedKgon<lll> maxAreaInscribedQuadrilateral(const vector<Point2<T>> &poly) {
    ConvexPolygonQuery c(poly); int n = c.n;
    if (n <= 4) {
        vector<int> v(n);
        iota(v.begin(), v.end(), 0);
        return {c.pre[n], v};}
    auto area = [&](int i, int j, int k) { return cross(c.at(j) - c.at(i), c.at(k) - c.at(i)); };
    int a0 = 0, c0 = 0;
    for (int i = 1; i < n; ++i) {
        if (pair(c.p[i].y, c.p[i].x) < pair(c.p[a0].y, c.p[a0].x)) { a0 = i; }
        if (pair(c.p[i].y, c.p[i].x) > pair(c.p[c0].y, c.p[c0].x)) { c0 = i; }}
    int a = a0, b = a0, x = c0, d = c0; array<int, 4> best{}; lll bv = -1;
    // a advances (c0 - a0) mod n times and x advances (a0 - c0) mod n times: exactly n steps on valid input.
    for (int it = 0; it < n; ++it) {
        while (area(x, a, b + 1) > area(x, a, b)) { b = (b + 1) % n; }
        while (area(a, x, d + 1) > area(a, x, d)) { d = (d + 1) % n; }
        lll cur = cross(c.at(x) - c.at(a), c.at(d) - c.at(b));
        if (cur > bv) { bv = cur; best = {a, b, x, d}; }
        if (cross(c.at(a + 1) - c.at(a), c.at(x + 1) - c.at(x)) <= 0) { a = (a + 1) % n; } else { x = (x + 1) % n; }}
    assert(a == c0 && x == a0);
    sort(best.begin(), best.end());
    return {bv, vector<int>(best.begin(), best.end())};}
