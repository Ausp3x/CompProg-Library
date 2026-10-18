#pragma once
#include "../01-Core/01-template.hpp"
#include "../02-Data Structures/61-hashmap.hpp"
#include "01-point.hpp"
#include "02-line_segment.hpp"

namespace fixed_radius_detail {
    inline lng floorDiv(lng x, lng r) { return x >= 0 ? x / r : -((r - 1 - x) / r); }
    inline ulng key(lng x, lng y) { return ulng(uint32_t(int32_t(x))) << 32 | uint32_t(int32_t(y)); }
    inline int compareProducts(lll a, lll b, lll c, lll d) {
        auto mul = [](ulll x, ulll y) {
            ulll x0 = ulng(x), x1 = x >> 64, y0 = ulng(y), y1 = y >> 64;
            ulll p00 = x0 * y0, p01 = x0 * y1, p10 = x1 * y0, mid = (p00 >> 64) + ulng(p01) + ulng(p10);
            return pair<ulll, ulll>{x1 * y1 + (p01 >> 64) + (p10 >> 64) + (mid >> 64), (mid << 64) | ulng(p00)};};
        auto x = mul(ulll(a), ulll(b)), y = mul(ulll(c), ulll(d));
        return x < y ? -1 : x > y ? 1 : 0;}
    inline int signSum(lll a, lll x, lll b, lll y) {
        int s = geometry_detail::sign(a) * (x > 0), t = geometry_detail::sign(b) * (y > 0);
        if (!s || !t || s == t) { return s ? s : t; }
        lll l = a * a * x, r = b * b * y;
        return l > r ? s : l < r ? t : 0;}
    struct Event { point d; lll D, X; int e; };
    inline bool lower(const Event &a) {
        int y = signSum(a.d.y, a.D, a.e * a.d.x, a.X);
        return y < 0 || (y == 0 && signSum(a.d.x, a.D, -a.e * a.d.y, a.X) < 0);}
    // Sign of cross(u, v) = sqrt(D1) * P + sqrt(X1) * Q; when they disagree, D1 * P^2 - X1 * Q^2 = 4 * r2 * (P^2 - D1 * D2 * X1).
    inline int crossSign(const Event &u, const Event &v) {
        lll c = cross(u.d, v.d), t = dot(u.d, v.d);
        int sp = signSum(c, v.D, v.e * t, v.X), sq = u.X ? u.e * signSum(v.e * c, v.X, -t, v.D) : 0;
        if (!sp || !sq || sp == sq) { return sp ? sp : sq; }
        lll k = c * c * v.D + t * t * v.X - u.D * v.D * u.X, l = 2 * v.e * c * t, m = v.D * v.X;
        int sk = geometry_detail::sign(k), sl = geometry_detail::sign(l) * (m > 0), s;
        if (!sk || !sl || sk == sl) { s = sk ? sk : sl; }
        else { int z = compareProducts(abs(k), abs(k), l * l, m); s = z > 0 ? sk : z < 0 ? sl : 0; }
        return s > 0 ? sp : s < 0 ? sq : 0;}
    inline bool before(const Event &a, const Event &b) {
        bool x = lower(a), y = lower(b);
        if (x != y) { return y; }
        int c = crossSign(a, b);
        return c ? c > 0 : a.e < b.e;}
} // namespace fixed_radius_detail

// S: O(n) expected, Q: O(1 + points in the 3 x 3 cells around p[i]) expected, M: O(n). Integral |coordinate| <= 10^9, cell size r with 1 <= r <= 4 * 10^9; closed distance <= r.
// neighbours(i): j != i; pairsWithin: i < j; diskCoverCounts[i]: points within r of p[i], itself included; the batch queries are output-sensitive.
struct GridHash {
    lng r;
    vector<point> p;
    vector<int> order, start;
    HashMap<ulng, int> cells;

    template<typename T> GridHash(const vector<Point2<T>> &pts, lng r) : r(r), p(pts.size()) {
        static_assert(std::is_integral_v<T>);
        assert(1 <= r && r <= 4000000000LL && pts.size() <= (1 << 29));
        int n = int(pts.size()); vector<int> id(n); cells.reserve(n);
        for (int i = 0; i < n; ++i) {
            p[i] = pts[i].template cast<lng>(); linear_detail::check(p[i]);
            int &c = cells[cellKey(p[i])];
            if (!c) { c = int(cells.size()); }
            id[i] = c - 1;}
        start.assign(cells.size() + 1, 0); order.resize(n);
        for (int i = 0; i < n; ++i) { ++start[id[i] + 1]; }
        for (int c = 0; c < int(cells.size()); ++c) { start[c + 1] += start[c]; }
        vector<int> fill(start.begin(), start.end() - 1);
        for (int i = 0; i < n; ++i) { order[fill[id[i]]++] = i; }}

    ulng cellKey(point q, lng dx = 0, lng dy = 0) const { return fixed_radius_detail::key(fixed_radius_detail::floorDiv(q.x, r) + dx, fixed_radius_detail::floorDiv(q.y, r) + dy); }
    template<typename F> void forEachNear(int i, F f) const {
        assert(0 <= i && i < int(p.size()));
        for (lng dx = -1; dx <= 1; ++dx) {
            for (lng dy = -1; dy <= 1; ++dy) {
                int c = cells.get(cellKey(p[i], dx, dy), 0) - 1;
                if (c < 0) { continue; }
                for (int t = start[c]; t < start[c + 1]; ++t) {
                    if (dist2(p[i], p[order[t]]) <= lll(r) * r) { f(order[t]); }}}}}
    vector<int> neighbours(int i) const {
        vector<int> res;
        forEachNear(i, [&](int j) { if (j != i) { res.push_back(j); } });
        return res;}
    // T: O(n + out) expected, M: O(out).
    vector<pair<int, int>> pairsWithin() const {
        vector<pair<int, int>> res;
        for (int i = 0; i < int(p.size()); ++i) { forEachNear(i, [&](int j) { if (i < j) { res.push_back({i, j}); } }); }
        return res;}
    // T: O(n + P) expected, P pairs within r, M: O(n).
    vector<int> diskCoverCounts() const {
        vector<int> res(p.size());
        for (int i = 0; i < int(p.size()); ++i) { forEachNear(i, [&](int) { ++res[i]; }); }
        return res;}
};

// T: O(n + sum of m * log(m)) expected, m points within 2 * sqrt(r2) of a pivot (O(n^2 * log(n)) worst), M: O(n). Integral |coordinate| <= 10^9, 0 <= r2 <= 4 * 10^8.
// Closed disk of radius sqrt(r2): {maximum number of points inside, a rounded centre attaining it}; exact count by an exact angular sweep; {0, (0, 0)} when empty.
template<typename T> pair<int, dpoint> maxPointsInDisk(const vector<Point2<T>> &pts, lng r2) {
    using fixed_radius_detail::Event;
    assert(0 <= r2 && r2 <= 400000000); r2 = min(max(r2, lng(0)), lng(400000000));
    int n = int(pts.size()); pair<int, dpoint> res{0, {}};
    if (!n) { return res; }
    lng s = max(lng(1), lng(std::sqrt(static_cast<long double>(4 * r2))));
    while (s * s < 4 * r2) { ++s; }
    GridHash g(pts, s); long double radius = std::sqrt(static_cast<long double>(r2));
    for (int i = 0; i < n; ++i) {
        point o = g.p[i]; int cur = 0; vector<Event> e;
        g.forEachNear(i, [&](int j) {
            point d = g.p[j] - o; lll dd = norm2(d);
            if (dd == 0) { ++cur; }
            else if (dd <= 4 * r2) { e.push_back({d, dd, 4 * r2 - dd, -1}); e.push_back({d, dd, 4 * r2 - dd, 1}); }});
        // An arc whose exit sorts before its entry wraps through the direction +x where the sweep starts.
        for (int t = 0; t < int(e.size()); t += 2) { cur += fixed_radius_detail::before(e[t + 1], e[t]); }
        sort(e.begin(), e.end(), fixed_radius_detail::before);
        auto centre = [&](const Event &x) {
            dpoint d = x.d.cast<long double>(); long double t = std::sqrt(static_cast<long double>(x.X) / static_cast<long double>(x.D));
            return o.cast<long double>() + (d + perp(d) * (x.e * t)) / 2.L;};
        if (cur > res.first) { res = {cur, o.cast<long double>() + dpoint{radius, 0}}; }
        for (const Event &x : e) {
            if (x.e < 0 && ++cur > res.first) { res = {cur, centre(x)}; }
            if (x.e > 0) { --cur; }}}
    return res;}
