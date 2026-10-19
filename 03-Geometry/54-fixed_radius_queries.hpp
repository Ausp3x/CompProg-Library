#pragma once
#include "../01-Core/01-template.hpp"
#include "../02-Data Structures/61-hashmap.hpp"
#include "01-point.hpp"
#include "02-line_segment.hpp"

namespace fixed_radius_detail {
    inline lng floorDiv(lng x, lng r) { return x >= 0 ? x / r : -((r - 1 - x) / r); }
    inline ulng key(lng x, lng y) { return ulng(uint32_t(int32_t(x))) << 32 | uint32_t(int32_t(y)); }
    inline int compareProducts(ulll a, ulll b, ulll c, ulll d) {
        auto mul = [](ulll x, ulll y) {
            ulll x0 = ulng(x), x1 = x >> 64, y0 = ulng(y), y1 = y >> 64;
            ulll p00 = x0 * y0, p01 = x0 * y1, p10 = x1 * y0, mid = (p00 >> 64) + ulng(p01) + ulng(p10);
            return pair<ulll, ulll>{x1 * y1 + (p01 >> 64) + (p10 >> 64) + (mid >> 64), (mid << 64) | ulng(p00)};};
        auto x = mul(a, b), y = mul(c, d);
        return x < y ? -1 : x > y ? 1 : 0;}
    // 384-bit magnitudes, least significant limb first, for r2 above 2^30.
    using Mag = array<ulng, 6>;
    inline Mag mag(ulll a, ulng x = 1) { ulll lo = ulll(ulng(a)) * x, hi = (a >> 64) * x + (lo >> 64); return {ulng(lo), ulng(hi), ulng(hi >> 64)}; }
    inline bool less(const Mag &a, const Mag &b) { return std::lexicographical_compare(a.rbegin(), a.rend(), b.rbegin(), b.rend()); }
    inline Mag add(Mag a, const Mag &b, bool minus = false) {
        ulng c = 0;
        for (int i = 0; i < 6; ++i) { ulll x = minus ? ulll(a[i]) - b[i] - c : ulll(a[i]) + b[i] + c; a[i] = ulng(x); c = ulng(x >> 64) & (minus ? 1 : ~ulng(0)); }
        return a;}
    inline Mag mul(const Mag &a, const Mag &b) {
        Mag r{};
        for (int i = 0; i < 6; ++i) {
            ulng c = 0;
            for (int j = 0; a[i] && i + j < 6; ++j) { ulll x = ulll(a[i]) * b[j] + r[i + j] + c; r[i + j] = ulng(x); c = ulng(x >> 64); }}
        return r;}
    template<typename T> int sgn(const T &x) { return (T(0) < x) - (x < T(0)); }
    // Sign of a * sqrt(x) + b * sqrt(y), x, y >= 0; Wide compares the squares (below 2^190) as magnitudes.
    template<bool Wide> int signSum(lll a, lll x, lll b, lll y) {
        int s = sgn(a) * (x > 0), t = sgn(b) * (y > 0);
        if (!s || !t || s == t) { return s ? s : t; }
        if constexpr (Wide) {
            Mag l = mag(ulll(a * a), ulng(x)), r = mag(ulll(b * b), ulng(y));
            return less(r, l) ? s : less(l, r) ? t : 0;}
        else {
            lll l = a * a * x, r = b * b * y;
            return r < l ? s : l < r ? t : 0;}}
    struct Event { point d; lll D, X; int e; };
    inline bool lower(const Event &a) {
        int y = signSum<false>(a.d.y, a.D, a.e * a.d.x, a.X);
        return y < 0 || (y == 0 && signSum<false>(a.d.x, a.D, -a.e * a.d.y, a.X) < 0);}
    // Sign of cross(u, v) = sqrt(D1) * P + sqrt(X1) * Q; when they disagree, D1 * P^2 - X1 * Q^2 = 4 * r2 * (k + l * sqrt(m)).
    template<bool Wide> int crossSign(const Event &u, const Event &v) {
        lll c = cross(u.d, v.d), t = dot(u.d, v.d); int e = v.e;
        int sp = signSum<Wide>(c, v.D, e * t, v.X), sq = u.X ? u.e * signSum<Wide>(e * c, v.X, -t, v.D) : 0;
        if (!sp || !sq || sp == sq) { return sp ? sp : sq; }
        lll l = 2 * e * c * t, m = v.D * v.X; int sl = sgn(l) * (m > 0), sk = 0, z = 0;
        if constexpr (Wide) {
            Mag a = add(mag(ulll(c * c), ulng(v.D)), mag(ulll(t * t), ulng(v.X))), b = mag(ulll(u.D * v.D), ulng(u.X));
            sk = less(a, b) ? -1 : less(b, a);
            if (sk && sl && sk != sl) {
                Mag k = sk > 0 ? add(a, b, true) : add(b, a, true), kk = mul(k, k), ll = mul(mul(mag(ulll(abs(l))), mag(ulll(abs(l)))), mag(ulll(m)));
                z = less(kk, ll) ? -1 : less(ll, kk);}}
        else {
            lll k = c * c * v.D + t * t * v.X - u.D * v.D * u.X; sk = sgn(k);
            if (sk && sl && sk != sl) { z = compareProducts(ulll(abs(k)), ulll(abs(k)), ulll(abs(l)) * ulll(abs(l)), ulll(m)); }}
        int s = !sk || !sl || sk == sl ? (sk ? sk : sl) : z > 0 ? sk : z < 0 ? sl : 0;
        return s > 0 ? sp : s < 0 ? sq : 0;}
    template<bool Wide> bool before(const Event &a, const Event &b) {
        bool x = lower(a), y = lower(b);
        if (x != y) { return y; }
        int c = crossSign<Wide>(a, b);
        return c ? c > 0 : a.e < b.e;}
    template<typename G> vector<int> neighbours(const G &g, int i) {
        vector<int> res;
        g.forEachNear(i, [&](int j) { if (j != i) { res.push_back(j); } });
        return res;}
    template<typename G> vector<pair<int, int>> pairsWithin(const G &g) {
        vector<pair<int, int>> res;
        for (int i = 0; i < int(g.p.size()); ++i) { g.forEachNear(i, [&](int j) { if (i < j) { res.push_back({i, j}); } }); }
        return res;}
    template<typename G> vector<int> diskCoverCounts(const G &g) {
        vector<int> res(g.p.size());
        for (int i = 0; i < int(g.p.size()); ++i) { g.forEachNear(i, [&](int) { ++res[i]; }); }
        return res;}
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
    vector<int> neighbours(int i) const { return fixed_radius_detail::neighbours(*this, i); }
    // T: O(n + out) expected, M: O(out).
    vector<pair<int, int>> pairsWithin() const { return fixed_radius_detail::pairsWithin(*this); }
    // T: O(n + P) expected, P pairs within r, M: O(n).
    vector<int> diskCoverCounts() const { return fixed_radius_detail::diskCoverCounts(*this); }
};

// S: O(n), Q: O(1 + points in the 3 x 3 cells around p[i]), M: O(n). Deterministic GridHash: its domain, results and result order,
// from an LSD radix sort of the cell keys and a table of the 9 neighbouring cells instead of hashing.
struct SortedGrid {
    lng r;
    vector<point> p;
    vector<int> order, start, cell;
    vector<array<int, 9>> near;

    template<typename T> SortedGrid(const vector<Point2<T>> &pts, lng r) : r(r), p(pts.size()) {
        static_assert(std::is_integral_v<T>);
        assert(1 <= r && r <= 4000000000LL && pts.size() <= (1 << 29));
        int n = int(pts.size()); vector<ulng> k(n); vector<int> tmp(n); order.resize(n); cell.resize(n);
        for (int i = 0; i < n; ++i) { p[i] = pts[i].template cast<lng>(); linear_detail::check(p[i]); k[i] = cellKey(p[i]); }
        iota(order.begin(), order.end(), 0);
        for (int b = 0; b < 64; b += 8) {
            array<int, 257> cnt{};
            for (int i : order) { ++cnt[(k[i] >> b & 255) + 1]; }
            if (*std::ranges::max_element(cnt) == n) { continue; }
            for (int d = 0; d < 256; ++d) { cnt[d + 1] += cnt[d]; }
            for (int i : order) { tmp[cnt[k[i] >> b & 255]++] = i; }
            swap(order, tmp);}
        vector<ulng> keys;
        for (int t = 0; t < n; ++t) {
            if (!t || k[order[t]] != keys.back()) { keys.push_back(k[order[t]]); start.push_back(t); }
            cell[order[t]] = int(keys.size()) - 1;}
        start.push_back(n); near.resize(keys.size());
        for (int d = 0; d < 9; ++d) {
            ulng shift = ulng(lng(d / 3 - 1)) << 32, j = 0;
            shift += ulng(lng(d % 3 - 1));
            for (int c = 0; c < int(keys.size()); ++c) {
                ulng want = keys[c] + shift;
                while (j < keys.size() && keys[j] < want) { ++j; }
                near[c][d] = j < keys.size() && keys[j] == want ? int(j) : -1;}}}

    ulng cellKey(point q) const { return ulng(fixed_radius_detail::floorDiv(q.x, r) + (lng(1) << 31)) << 32 | ulng(fixed_radius_detail::floorDiv(q.y, r) + (lng(1) << 31)); }
    template<typename F> void forEachNear(int i, F f) const {
        assert(0 <= i && i < int(p.size()));
        for (int c : near[cell[i]]) {
            if (c < 0) { continue; }
            for (int t = start[c]; t < start[c + 1]; ++t) {
                if (dist2(p[i], p[order[t]]) <= lll(r) * r) { f(order[t]); }}}}
    vector<int> neighbours(int i) const { return fixed_radius_detail::neighbours(*this, i); }
    // T: O(n + out), M: O(out).
    vector<pair<int, int>> pairsWithin() const { return fixed_radius_detail::pairsWithin(*this); }
    // T: O(n + P), P pairs within r, M: O(n).
    vector<int> diskCoverCounts() const { return fixed_radius_detail::diskCoverCounts(*this); }
};

// T: O(n * K * log(K + 1)), K the answer (O(n^2 * log(n)) worst), M: O(n). Integral |coordinate| <= 10^9, 0 <= r2 <= 4 * 10^18.
// Closed disk of radius sqrt(r2): {maximum number of points inside, a rounded centre attaining it}; exact count by an exact angular sweep; {0, (0, 0)} when empty.
template<typename T> pair<int, dpoint> maxPointsInDisk(const vector<Point2<T>> &pts, lng r2) {
    using fixed_radius_detail::Event;
    const lng R2 = 4000000000000000000LL;
    assert(0 <= r2 && r2 <= R2); r2 = min(max(r2, lng(0)), R2);
    int n = int(pts.size()); pair<int, dpoint> res{0, {}};
    if (!n) { return res; }
    lll q = 4 * lll(r2); lng s = max(lng(1), lng(std::sqrt(static_cast<long double>(q))));
    while (lll(s) * s < q) { ++s; }
    SortedGrid g(pts, s); long double radius = std::sqrt(static_cast<long double>(r2));
    // Up to 2^30, 4 * r2 <= 2^32 keeps the comparator in lll with one 256-bit product; larger radii use 384-bit magnitudes.
    auto order = [wide = r2 > (lng(1) << 30)](const Event &a, const Event &b) {
        return wide ? fixed_radius_detail::before<true>(a, b) : fixed_radius_detail::before<false>(a, b);};
    for (int i = 0; i < n; ++i) {
        point o = g.p[i]; int cur = 0; vector<Event> e;
        g.forEachNear(i, [&](int j) {
            point d = g.p[j] - o; lll dd = norm2(d);
            if (dd == 0) { ++cur; }
            else if (dd <= q) { e.push_back({d, dd, q - dd, -1}); e.push_back({d, dd, q - dd, 1}); }});
        // An arc whose exit sorts before its entry wraps through the direction +x where the sweep starts.
        for (int t = 0; t < int(e.size()); t += 2) { cur += order(e[t + 1], e[t]); }
        sort(e.begin(), e.end(), order);
        auto centre = [&](const Event &x) {
            dpoint d = x.d.cast<long double>(); long double t = std::sqrt(static_cast<long double>(x.X) / static_cast<long double>(x.D));
            return o.cast<long double>() + (d + perp(d) * (x.e * t)) / 2.L;};
        if (cur > res.first) { res = {cur, o.cast<long double>() + dpoint{radius, 0}}; }
        for (const Event &x : e) {
            if (x.e < 0 && ++cur > res.first) { res = {cur, centre(x)}; }
            if (x.e > 0) { --cur; }}}
    return res;}
