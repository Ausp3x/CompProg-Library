#pragma once
#include "../01-Core/01-template.hpp"
#include "../06-Miscellaneous/002-customhash.hpp"
#include "001-point.hpp"

// T: O(1), M: O(1). Increasing original ids, lexicographically smallest on ties; fewer than two points: {-1, -1}, 0.
struct ClosestPair {
    pair<int, int> ids{-1, -1};
    lll distance2 = 0;
};

namespace closest_detail {
    template<typename W> struct Best {
        pair<int, int> ids{-1, -1};
        W d{};
    };
    inline void fits(std::initializer_list<ulll> spans) {
        [[maybe_unused]] ulll room = (ulll(1) << 127) - 1;
        for (ulll s : spans) {
            assert(s * s <= room);
            room -= s * s;}}
    template<typename T> void fits(const vector<Point2<T>> &p) {
        auto [x0, x1] = std::minmax_element(p.begin(), p.end(), [](auto a, auto b) { return a.x < b.x; });
        auto [y0, y1] = std::minmax_element(p.begin(), p.end(), [](auto a, auto b) { return a.y < b.y; });
        fits({ulll(lll(x1->x) - x0->x), ulll(lll(y1->y) - y0->y)});}
    template<typename T> void fits(const vector<Point3<T>> &p) {
        Point3<T> lo = p[0], hi = lo;
        for (auto q : p) {
            lo = {min(lo.x, q.x), min(lo.y, q.y), min(lo.z, q.z)};
            hi = {max(hi.x, q.x), max(hi.y, q.y), max(hi.z, q.z)};}
        fits({ulll(lll(hi.x) - lo.x), ulll(lll(hi.y) - lo.y), ulll(lll(hi.z) - lo.z)});}
    // floor(sqrt(d)) or one more for 0 <= d < 2^127: the 64-bit significand rounds the root by less than half a unit; any d stays bounded.
    inline lll cellSide(lll d) { return lll(std::sqrt(static_cast<long double>(ulll(d)))); }
    template<typename P, typename W> void update(const vector<P> &p, Best<W> &b, int i, int j) {
        pair<int, int> ij = std::minmax(i, j); W d = dist2(p[i], p[j]);
        if (b.ids.first < 0 || d < b.d || (d == b.d && ij < b.ids)) { b = {ij, d}; }}
    template<typename P> ClosestPair naive(const vector<P> &p) {
        Best<lll> b;
        for (int j = 1; j < int(p.size()); ++j) {
            for (int i = 0; i < j; ++i) { update(p, b, i, j); }}
        return {b.ids, b.d};}
    inline void updateTriangle(const vector<dpoint> &q, array<int, 3> &best, long double &per, int i, int j, int k) {
        array<int, 3> t{i, j, k}; sort(t.begin(), t.end());
        long double v = distanceApprox(q[t[0]], q[t[1]]) + distanceApprox(q[t[1]], q[t[2]]) + distanceApprox(q[t[0]], q[t[2]]);
        if (best[0] < 0 || v < per || (v == per && t < best)) { best = t; per = v; }}
    template<typename T> vector<dpoint> points(const vector<Point2<T>> &p) {
        vector<dpoint> q;
        for (auto r : p) { q.push_back(r.template cast<long double>()); }
        return q;}
    template<typename T> bool finite(const vector<Point2<T>> &p) {
        return std::all_of(p.begin(), p.end(), [](auto q) { return __builtin_isfinite(q.x) && __builtin_isfinite(q.y); });}
    // Equal points are adjacent in (point, id) order, so zero distances are settled before any strip.
    template<typename P, typename W> vector<int> sorted(const vector<P> &p, Best<W> &b) {
        vector<int> ids(p.size()); iota(ids.begin(), ids.end(), 0);
        sort(ids.begin(), ids.end(), [&](int i, int j) { return p[i] != p[j] ? p[i] < p[j] : i < j; });
        update(p, b, 0, 1);
        for (int i = 1; i < int(p.size()); ++i) {
            if (p[ids[i - 1]] == p[ids[i]]) { update(p, b, ids[i - 1], ids[i]); }}
        return ids;}
    template<typename T> Best<geometry_detail::Wide<T>> closest2(const vector<Point2<T>> &p) {
        using W = geometry_detail::Wide<T>;
        int n = int(p.size());
        Best<W> b;
        vector<int> ids = sorted(p, b);
        auto update = [&](int i, int j) { closest_detail::update(p, b, i, j); };
        // Integral only: rounded floating distances can tie a non-adjacent pair with a smaller id.
        bool line = std::is_integral_v<T> && (std::all_of(p.begin(), p.end(), [&](auto q) { return q.x == p[0].x; }) ||
                                              std::all_of(p.begin(), p.end(), [&](auto q) { return q.y == p[0].y; }));
        if (line) {
            for (int i = 1; i < n; ++i) { update(ids[i - 1], ids[i]); }}
        if (b.d == W(0) || line) { return b; }
        vector<int> tmp(n);
        auto by_y = [&](int i, int j) { return p[i].y != p[j].y ? p[i].y < p[j].y : p[i] < p[j]; };
        auto square = [](W x) { return x * x; };
        auto solve = [&](auto &&solve, int l, int r) -> void {
            if (r - l <= 3) {
                for (int i = l; i < r; ++i) { for (int j = i + 1; j < r; ++j) { update(ids[i], ids[j]); }}
                sort(ids.begin() + l, ids.begin() + r, by_y);
                return;}
            int m = std::midpoint(l, r); W x = W(p[ids[m]].x);
            solve(solve, l, m); solve(solve, m, r);
            std::merge(ids.begin() + l, ids.begin() + m, ids.begin() + m, ids.begin() + r, tmp.begin() + l, by_y);
            std::copy(tmp.begin() + l, tmp.begin() + r, ids.begin() + l);
            int k = l;
            for (int i = l; i < r; ++i) {
                int a = ids[i];
                if (square(W(p[a].x) - x) > b.d) { continue; }
                for (int j = k - 1; j >= l && square(W(p[a].y) - W(p[tmp[j]].y)) <= b.d; --j) { update(a, tmp[j]); }
                tmp[k++] = a;}};
        solve(solve, 0, n);
        return b;}
    // Random-incremental grid (Khuller–Matias): false on a side outside [2^-1020, 2^980) or below span / 2^40, where cells stop being exact.
    template<typename T> bool incremental(const vector<Point2<T>> &p, Best<long double> &b, ulng seed) {
        using C = std::conditional_t<(sizeof(T) <= sizeof(double)), double, long double>;
        struct Slot { lng x, y; int head; };
        int n = int(p.size());
        vector<int> ord(n), next(n);
        vector<lng> used;
        vector<Slot> slot(std::bit_ceil(2 * ulng(n)), {0, 0, -1});
        ulng mask = slot.size() - 1;
        iota(ord.begin(), ord.end(), 0);
        for (int k = 1; k < n; ++k) { swap(ord[k], ord[CustomHash::splitMix64(seed++) % ulng(k + 1)]); }
        auto [x0, x1] = std::minmax_element(p.begin(), p.end(), [](auto u, auto v) { return u.x < v.x; });
        auto [y0, y1] = std::minmax_element(p.begin(), p.end(), [](auto u, auto v) { return u.y < v.y; });
        C ox = C(x0->x), oy = C(y0->y), span = max(C(x1->x) - ox, C(y1->y) - oy);
        double inv = 0;
        auto find = [&](lng x, lng y) -> Slot & {
            ulng h = CustomHash::combine(CustomHash::splitMix64(ulng(x) + seed), ulng(y)) & mask;
            while (slot[h].head >= 0 && (slot[h].x != x || slot[h].y != y)) { h = (h + 1) & mask; }
            return slot[h];};
        // Fine cells of side s, offsets from the minimum truncated; a point's 2 x 2 block of coarse cells (side 2 s) holds every fine neighbour.
        auto fine = [&](int i) { return pair{lng(double(C(p[i].x) - ox) * inv), lng(double(C(p[i].y) - oy) * inv)}; };
        auto insert = [&](int i) {
            auto [u, v] = fine(i); Slot &c = find(u >> 1, v >> 1);
            if (c.head < 0) { c.x = u >> 1; c.y = v >> 1; used.push_back(&c - slot.data()); }
            next[i] = std::exchange(c.head, i);};
        // Side sqrt(d) (1 + 2^-8): a pair in non-adjacent fine cells is more than sqrt(d) apart despite rounding, so it cannot tie d.
        auto rebuild = [&](int m) {
            double s = double(std::sqrt(b.d) * (1 + 0x1p-8L));
            if (!(s >= 0x1p-1020 && s < 0x1p980 && span < 0x1p40 * s)) { return false; }
            inv = 1 / s;
            for (lng h : used) { slot[h].head = -1; }
            used.clear();
            for (int k = 0; k < m; ++k) { insert(ord[k]); }
            return true;};
        update(p, b, ord[0], ord[1]);
        if (!rebuild(2)) { return false; }
        for (int k = 2; k < n; ++k) {
            int i = ord[k]; auto [u, v] = fine(i); long double d = b.d;
            lng x = u >> 1, y = v >> 1, dx = u & 1 ? 1 : -1, dy = v & 1 ? 1 : -1;
            for (auto [a, c] : {pair{x, y}, {x + dx, y}, {x, y + dy}, {x + dx, y + dy}}) {
                for (int j = find(a, c).head; j >= 0; j = next[j]) { update(p, b, i, j); }}
            if (b.d >= d) { insert(i); }
            else if (!rebuild(k + 1)) { return false; }}
        return true;}
} // namespace closest_detail

// T: O(n * log(n)), M: O(n). Signed integral T of at most 64 bits, n <= INT_MAX; the squared bounding-box diagonal fits lll.
template<typename T> ClosestPair closestPair(const vector<Point2<T>> &p) {
    static_assert(std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) <= sizeof(lng));
    assert(p.size() <= INT_MAX);
    if (p.size() < 2) { return {}; }
    closest_detail::fits(p);
    auto b = closest_detail::closest2(p);
    return {b.ids, b.d};}

// T: O(n^2), M: O(1). closestPair's domain, result and tie rule, scanning every pair.
template<typename T> ClosestPair closestPairNaive(const vector<Point2<T>> &p) {
    static_assert(std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) <= sizeof(lng));
    assert(p.size() <= INT_MAX);
    if (p.size() < 2) { return {}; }
    closest_detail::fits(p);
    return closest_detail::naive(p);}

// T: O(n * log(n)), M: O(n). closestPair's domain, result and tie rule; y sweep over an x-ordered set (KACTL).
template<typename T> ClosestPair closestPairSimple(const vector<Point2<T>> &p) {
    static_assert(std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) <= sizeof(lng));
    assert(p.size() <= INT_MAX);
    int n = int(p.size());
    if (n < 2) { return {}; }
    closest_detail::fits(p);
    closest_detail::Best<lll> b;
    vector<int> ids = closest_detail::sorted(p, b);
    if (!b.d) { return {b.ids, 0}; }
    sort(ids.begin(), ids.end(), [&](int i, int j) { return p[i].y < p[j].y; });
    std::map<pair<lll, lll>, int> s;
    // d > sqrt(best): the window keeps every point within the best distance, ties included.
    for (int k = 0, j = 0; k < n; ++k) {
        auto q = p[ids[k]]; lll d = closest_detail::cellSide(b.d) + 1;
        for (; lll(p[ids[j]].y) <= q.y - d; ++j) { s.erase({p[ids[j]].x, p[ids[j]].y}); }
        for (auto it = s.lower_bound({q.x - d, q.y}); it != s.end() && it->first <= pair<lll, lll>{q.x + d, q.y}; ++it) {
            closest_detail::update(p, b, it->second, ids[k]);}
        s[{q.x, q.y}] = ids[k];}
    return {b.ids, b.d};}

// T: O(n) expected, M: O(n). closestPair's domain, result and tie rule; seeded Las Vegas (the seed affects only time).
template<typename T> ClosestPair closestPairRandomized(const vector<Point2<T>> &p, ulng seed = CustomHash::rnd) {
    static_assert(std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) <= sizeof(lng));
    assert(p.size() <= INT_MAX);
    int n = int(p.size());
    if (n < 2) { return {}; }
    closest_detail::fits(p);
    closest_detail::Best<lll> b;
    auto update = [&](int i, int j) { closest_detail::update(p, b, i, j); };
    for (int k = 0; k < n; ++k) {
        int i = int(CustomHash::splitMix64(seed++) % ulng(n)), j = int(CustomHash::splitMix64(seed++) % ulng(n - 1));
        update(i, j + (j >= i));}
    // Rabin: d is the least of n sampled distances, so cells of side s >= floor(sqrt(d)) (single points if d == 0) hold O(n) expected pairs.
    bool point_cells = !b.d;
    ulng s = ulng(point_cells ? 1 : closest_detail::cellSide(b.d));
    T x0 = std::min_element(p.begin(), p.end(), [](auto u, auto v) { return u.x < v.x; })->x;
    T y0 = std::min_element(p.begin(), p.end(), [](auto u, auto v) { return u.y < v.y; })->y;
    auto cell = [&](int i) { return pair<ulng, ulng>{ulng(lll(p[i].x) - x0) / s, ulng(lll(p[i].y) - y0) / s}; };
    ulng mask = std::bit_ceil(2 * ulng(n)) - 1;
    vector<int> slot(mask + 1, -1), of(n), start(n + 1), member(n);
    vector<pair<ulng, ulng>> key;
    auto find = [&](pair<ulng, ulng> c) {
        ulng h = CustomHash::combine(CustomHash::splitMix64(c.first + seed), c.second) & mask;
        while (slot[h] >= 0 && key[slot[h]] != c) { h = (h + 1) & mask; }
        return h;};
    for (int i = 0; i < n; ++i) {
        ulng h = find(cell(i));
        if (slot[h] < 0) { slot[h] = int(key.size()); key.push_back(cell(i)); }
        ++start[(of[i] = slot[h]) + 1];}
    int cells = int(key.size());
    for (int c = 0; c < cells; ++c) { start[c + 1] += start[c]; }
    vector<int> at(start.begin(), start.end() - 1);
    for (int i = 0; i < n; ++i) { member[at[of[i]]++] = i; }
    for (int c = 0; c < cells; ++c) {
        if (point_cells) {
            if (start[c + 1] - start[c] > 1) { update(member[start[c]], member[start[c] + 1]); }
            continue;}
        for (int i = start[c]; i < start[c + 1]; ++i) {
            for (int j = i + 1; j < start[c + 1]; ++j) { update(member[i], member[j]); }}
        auto [cx, cy] = key[c];
        for (auto [dx, dy] : {pair{1, -1}, {1, 0}, {1, 1}, {0, 1}}) {
            int e = slot[find({cx + ulng(dx), cy + ulng(lng(dy))})];
            if (e < 0) { continue; }
            for (int i = start[c]; i < start[c + 1]; ++i) {
                for (int j = start[e]; j < start[e + 1]; ++j) { update(member[i], member[j]); }}}}
    return {b.ids, b.d};}

// T: O(n) expected on distinct points whose best distances stay in [2^-1020, 2^980) and above span / 2^40, else O(n * log(n)) expected; O(n^3) worst; M: O(n).
// Floating T (wider ones as long double points), finite, n <= INT_MAX; closestPair's ids and tie rule on computed long double squared distances for every seed (Las Vegas); n < 2: {-1, -1}.
template<typename T> pair<int, int> closestPairApprox(const vector<Point2<T>> &p, ulng seed = CustomHash::rnd) {
    static_assert(std::is_floating_point_v<T>);
    if constexpr (!std::is_same_v<T, float> && !std::is_same_v<T, double> && !std::is_same_v<T, long double>) {
        return closestPairApprox(closest_detail::points(p), seed);}
    assert(p.size() <= INT_MAX);
    bool finite = closest_detail::finite(p);
    assert(finite);
    if (p.size() < 2 || !finite) { return {-1, -1}; }
    closest_detail::Best<long double> b;
    // Below 1024 points the divide and conquer is faster (i9-11900H, GCC 15).
    return p.size() >= 1024 && closest_detail::incremental(p, b, seed) ? b.ids : closest_detail::closest2(p).ids;}

// T: O(n * log(n)), M: O(n). closestPair's domain and tie rule for Point3; deterministic and exact.
template<typename T> ClosestPair closestPair3(const vector<Point3<T>> &p) {
    static_assert(std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) <= sizeof(lng));
    assert(p.size() <= INT_MAX);
    int n = int(p.size());
    if (n < 2) { return {}; }
    closest_detail::fits(p);
    closest_detail::Best<lll> ans;
    vector<int> ys = closest_detail::sorted(p, ans);
    if (!ans.d) { return {ans.ids, 0}; }
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
        lll d = ans.d, s = closest_detail::cellSide(d);
        // Truncated rows are at least s >= floor(sqrt(d)) tall: a pair within sqrt(d) shares a row or spans two adjacent rows.
        int rows = 0;
        for (int i = l; i < r; ++i) {
            int a = ys[i];
            if (square(p[a].x - x) > d) { continue; }
            lll q = p[a].y / s;
            if (!rows || key[rows - 1] != q) { key[rows] = q; pos[rows++] = 0; }
            row[a] = rows - 1; ++pos[rows - 1];}
        for (int t = 0, sum = 0; t < rows; ++t) { sum += std::exchange(pos[t], sum); }
        for (int i = l; i < r; ++i) {
            if (square(p[zs[i]].x - x) <= d) { slab[pos[row[zs[i]]]++] = zs[i]; }}
        for (int t = 0; t < rows; ++t) {
            int b = t ? pos[t - 1] : 0, j0 = t && key[t - 1] + 1 == key[t] ? (t > 1 ? pos[t - 2] : 0) : b;
            for (int i = b; i < pos[t]; ++i) {
                int a = slab[i];
                for (int j = i - 1; j >= b && square(lll(p[a].z) - p[slab[j]].z) <= ans.d; --j) { update(a, slab[j]); }
                while (j0 < b && p[slab[j0]].z < p[a].z - s) { ++j0; }
                for (int j = j0; j < b && p[slab[j]].z <= p[a].z + s; ++j) { update(a, slab[j]); }}}};
    solve(solve, 0, n);
    return {ans.ids, ans.d};}

// T: O(n^2), M: O(1). closestPair3's domain, result and tie rule, scanning every pair.
template<typename T> ClosestPair closestPair3Naive(const vector<Point3<T>> &p) {
    static_assert(std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) <= sizeof(lng));
    assert(p.size() <= INT_MAX);
    if (p.size() < 2) { return {}; }
    closest_detail::fits(p);
    return closest_detail::naive(p);}

// T: O(n * log(n)), M: O(n). Integral T of at most 64 bits or floating T, finite, differences zero or normal, n <= INT_MAX; increasing ids
// of a least computed perimeter d(i, j) + d(j, k) + d(i, k), smallest on ties; degenerate triangles count; fewer than three: {-1, -1, -1}.
template<typename T> array<int, 3> minimumPerimeterTriangleApprox(const vector<Point2<T>> &p) {
    static_assert(std::is_floating_point_v<T> || (std::is_integral_v<T> && sizeof(T) <= sizeof(lng)));
    assert(p.size() <= INT_MAX);
    int n = int(p.size());
    vector<dpoint> q = closest_detail::points(p);
    bool finite = closest_detail::finite(q);
    assert(finite);
    if (n < 3 || !finite) { return {-1, -1, -1}; }
    vector<int> ids(n), tmp(n); iota(ids.begin(), ids.end(), 0);
    sort(ids.begin(), ids.end(), [&](int i, int j) { return q[i] != q[j] ? q[i] < q[j] : i < j; });
    array<int, 3> best{-1, -1, -1};
    long double per = 0;
    auto update = [&](int i, int j, int k) { closest_detail::updateTriangle(q, best, per, i, j, k); };
    // Three equal points settle a zero perimeter; afterwards each point repeats at most twice, which the packing bound needs.
    for (int i = 2; i < n; ++i) {
        if (q[ids[i - 2]] == q[ids[i]]) { update(ids[i - 2], ids[i - 1], ids[i]); }}
    if (best[0] >= 0) { return best; }
    update(0, 1, 2);
    auto by_y = [&](int i, int j) { return q[i].y < q[j].y; };
    // The slack keeps every triple whose computed perimeter ties per although rounding moves it across a filter.
    auto half = [&] { return per / 2 * (1 + 0x1p-40L); };
    // A triangle of perimeter <= per spans at most per / 2 in x and y; one side holds O(1) points per such window.
    auto solve = [&](auto &&solve, int l, int r) -> void {
        if (r - l < 6) {
            for (int i = l; i < r; ++i) {
                for (int j = i + 1; j < r; ++j) {
                    for (int k = j + 1; k < r; ++k) { update(ids[i], ids[j], ids[k]); }}}
            sort(ids.begin() + l, ids.begin() + r, by_y);
            return;}
        int m = std::midpoint(l, r); long double x = q[ids[m]].x;
        solve(solve, l, m); solve(solve, m, r);
        std::merge(ids.begin() + l, ids.begin() + m, ids.begin() + m, ids.begin() + r, tmp.begin() + l, by_y);
        std::copy(tmp.begin() + l, tmp.begin() + r, ids.begin() + l);
        int e = l;
        for (int i = l; i < r; ++i) {
            if (abs(q[ids[i]].x - x) <= half()) { tmp[e++] = ids[i]; }}
        for (int i = l; i < e; ++i) {
            for (int j = i + 1; j < e && q[tmp[j]].y - q[tmp[i]].y <= half(); ++j) {
                for (int k = j + 1; k < e && q[tmp[k]].y - q[tmp[i]].y <= half(); ++k) { update(tmp[i], tmp[j], tmp[k]); }}}};
    solve(solve, 0, n);
    return best;}

// T: O(n^3), M: O(n). minimumPerimeterTriangleApprox's domain, result and tie rule, scanning every triple.
template<typename T> array<int, 3> minimumPerimeterTriangleApproxNaive(const vector<Point2<T>> &p) {
    static_assert(std::is_floating_point_v<T> || (std::is_integral_v<T> && sizeof(T) <= sizeof(lng)));
    assert(p.size() <= INT_MAX);
    vector<dpoint> q = closest_detail::points(p);
    array<int, 3> best{-1, -1, -1};
    long double per = 0;
    for (int k = 2; k < int(q.size()); ++k) {
        for (int j = 1; j < k; ++j) {
            for (int i = 0; i < j; ++i) { closest_detail::updateTriangle(q, best, per, i, j, k); }}}
    return best;}
