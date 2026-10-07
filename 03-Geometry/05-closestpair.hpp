#pragma once
#include "../01-Core/01-template.hpp"
#include "01-point.hpp"

// T: O(1), M: O(1). Increasing original ids, lexicographically smallest on ties; fewer than two points: {-1, -1}, 0.
struct ClosestPair {
    pair<int, int> ids{-1, -1};
    lll distance2 = 0;
};

namespace closest_detail {
    inline void fits(std::initializer_list<ulll> spans) {
        ulll room = (ulll(1) << 127) - 1;
        for (ulll s : spans) {
            assert(s * s <= room);
            room -= s * s;}}
    template<typename P> void update(const vector<P> &p, ClosestPair &ans, int i, int j) {
        pair<int, int> ij = std::minmax(i, j); lll d = dist2(p[i], p[j]);
        if (d < ans.distance2 || (d == ans.distance2 && ij < ans.ids)) { ans = {ij, d}; }}
    // Equal points are adjacent in (point, id) order, so zero distances are settled before any strip.
    template<typename P> vector<int> sorted(const vector<P> &p, ClosestPair &ans) {
        vector<int> ids(p.size()); iota(ids.begin(), ids.end(), 0);
        sort(ids.begin(), ids.end(), [&](int i, int j) { return p[i] != p[j] ? p[i] < p[j] : i < j; });
        ans = {{0, 1}, dist2(p[0], p[1])};
        for (int i = 1; i < int(p.size()); ++i) {
            if (p[ids[i - 1]] == p[ids[i]]) { update(p, ans, ids[i - 1], ids[i]); }}
        return ids;}
} // namespace closest_detail

// T: O(n * log(n)), M: O(n). Signed integral T of at most 64 bits, n <= INT_MAX; the squared bounding-box diagonal fits lll.
template<typename T> ClosestPair closestPair(const vector<Point2<T>> &p) {
    static_assert(std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) <= sizeof(lng));
    assert(p.size() <= INT_MAX);
    int n = int(p.size());
    if (n < 2) { return {}; }
    T x0 = p[0].x, x1 = x0, y0 = p[0].y, y1 = y0;
    for (auto q : p) { x0 = min(x0, q.x); x1 = max(x1, q.x); y0 = min(y0, q.y); y1 = max(y1, q.y); }
    closest_detail::fits({ulll(lll(x1) - x0), ulll(lll(y1) - y0)});
    ClosestPair ans;
    vector<int> ids = closest_detail::sorted(p, ans);
    auto update = [&](int i, int j) { closest_detail::update(p, ans, i, j); };
    bool line = x0 == x1 || y0 == y1;
    if (line) {
        for (int i = 1; i < n; ++i) { update(ids[i - 1], ids[i]); }}
    if (!ans.distance2 || line) { return ans; }
    vector<int> tmp(n);
    auto by_y = [&](int i, int j) { return p[i].y != p[j].y ? p[i].y < p[j].y : p[i] < p[j]; };
    auto square = [](lll x) { return x * x; };
    auto solve = [&](auto &&solve, int l, int r) -> void {
        if (r - l <= 3) {
            for (int i = l; i < r; ++i) { for (int j = i + 1; j < r; ++j) { update(ids[i], ids[j]); }}
            sort(ids.begin() + l, ids.begin() + r, by_y);
            return;}
        int m = std::midpoint(l, r); T x = p[ids[m]].x;
        solve(solve, l, m); solve(solve, m, r);
        std::merge(ids.begin() + l, ids.begin() + m, ids.begin() + m, ids.begin() + r, tmp.begin() + l, by_y);
        std::copy(tmp.begin() + l, tmp.begin() + r, ids.begin() + l);
        int k = l;
        for (int i = l; i < r; ++i) {
            int a = ids[i];
            if (square(lll(p[a].x) - x) > ans.distance2) { continue; }
            for (int j = k - 1; j >= l && square(lll(p[a].y) - p[tmp[j]].y) <= ans.distance2; --j) { update(a, tmp[j]); }
            tmp[k++] = a;}};
    solve(solve, 0, n);
    return ans;}

// T: O(n * log(n)), M: O(n). closestPair's domain and tie rule for Point3; deterministic and exact.
template<typename T> ClosestPair closestPair3(const vector<Point3<T>> &p) {
    static_assert(std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) <= sizeof(lng));
    assert(p.size() <= INT_MAX);
    int n = int(p.size());
    if (n < 2) { return {}; }
    Point3<T> lo = p[0], hi = lo;
    for (auto q : p) {
        lo = {min(lo.x, q.x), min(lo.y, q.y), min(lo.z, q.z)};
        hi = {max(hi.x, q.x), max(hi.y, q.y), max(hi.z, q.z)};}
    closest_detail::fits({ulll(lll(hi.x) - lo.x), ulll(lll(hi.y) - lo.y), ulll(lll(hi.z) - lo.z)});
    ClosestPair ans;
    vector<int> ys = closest_detail::sorted(p, ans);
    if (!ans.distance2) { return ans; }
    vector<int> zs = ys, tmp(n), row(n), pos(n), slab(n);
    vector<lll> key(n);
    auto update = [&](int i, int j) { closest_detail::update(p, ans, i, j); };
    auto by_y = [&](int i, int j) { return p[i].y < p[j].y; };
    auto by_z = [&](int i, int j) { return p[i].z < p[j].z; };
    auto square = [](lll x) { return x * x; };
    auto solve = [&](auto &&solve, int l, int r) -> void {
        if (r - l <= 3) {
            for (int i = l; i < r; ++i) { for (int j = i + 1; j < r; ++j) { update(ys[i], ys[j]); }}
            sort(ys.begin() + l, ys.begin() + r, by_y); sort(zs.begin() + l, zs.begin() + r, by_z);
            return;}
        int m = std::midpoint(l, r); lll x = p[ys[m]].x;
        solve(solve, l, m); solve(solve, m, r);
        auto merge = [&](vector<int> &v, auto less) {
            std::merge(v.begin() + l, v.begin() + m, v.begin() + m, v.begin() + r, tmp.begin() + l, less);
            std::copy(tmp.begin() + l, tmp.begin() + r, v.begin() + l);};
        merge(ys, by_y); merge(zs, by_z);
        lll d = ans.distance2, s = lll(std::sqrt(static_cast<long double>(d)));
        while (ulll(s) * ulll(s) < ulll(d)) { ++s; }
        // Rows of height s >= sqrt(d): a pair within sqrt(d) shares a row or spans two adjacent rows.
        int rows = 0;
        for (int i = l; i < r; ++i) {
            int a = ys[i];
            if (square(p[a].x - x) > d) { continue; }
            lll q = p[a].y / s;
            if (q * s > p[a].y) { --q; }
            if (!rows || key[rows - 1] != q) { key[rows] = q; pos[rows++] = 0; }
            row[a] = rows - 1; ++pos[rows - 1];}
        for (int t = 0, sum = 0; t < rows; ++t) { sum += std::exchange(pos[t], sum); }
        for (int i = l; i < r; ++i) {
            if (square(p[zs[i]].x - x) <= d) { slab[pos[row[zs[i]]]++] = zs[i]; }}
        for (int t = 0; t < rows; ++t) {
            int b = t ? pos[t - 1] : 0, j0 = t && key[t - 1] + 1 == key[t] ? (t > 1 ? pos[t - 2] : 0) : b;
            for (int i = b; i < pos[t]; ++i) {
                int a = slab[i];
                for (int j = i - 1; j >= b && square(lll(p[a].z) - p[slab[j]].z) <= ans.distance2; --j) { update(a, slab[j]); }
                while (j0 < b && p[slab[j0]].z < p[a].z - s) { ++j0; }
                for (int j = j0; j < b && p[slab[j]].z <= p[a].z + s; ++j) { update(a, slab[j]); }}}};
    solve(solve, 0, n);
    return ans;}
