#pragma once
#include "../01-Core/01-template.hpp"
#include "../02-Data Structures/02-fenwick.hpp"
#include "01-point.hpp"

namespace segment_union_detail {
    template<typename T> vector<pair<T, int>> events(const vector<pair<T, T>> &s) {
        static_assert((std::is_integral_v<T> && std::is_signed_v<T>) || std::is_floating_point_v<T>);
        assert(s.size() <= INT_MAX / 2); vector<pair<T, int>> e;
        for (auto [l, r] : s) {
            if (r < l) { swap(l, r); }
            if (l < r) { e.push_back({l, 1}); e.push_back({r, -1}); }}
        sort(e.begin(), e.end());
        return e;}
} // namespace segment_union_detail

// T: O(n * log(n)), M: O(n). Endpoints in either order, signed integral (exact lll) or floating (rounded long double); empty segments are ignored.
// coverageCount[k] is the length covered by exactly k segments, k in [1, n]; [0] is the gap length between the extreme endpoints.
template<typename T> auto segmentUnionLength(const vector<pair<T, T>> &s) {
    auto e = segment_union_detail::events(s); geometry_detail::Wide<T> res = 0; int c = 0;
    for (int i = 0; i + 1 < int(e.size()); ++i) {
        c += e[i].second;
        if (c > 0) { res += geometry_detail::Wide<T>(e[i + 1].first) - e[i].first; }}
    return res;}
template<typename T> auto coverageCount(const vector<pair<T, T>> &s) {
    auto e = segment_union_detail::events(s); vector<geometry_detail::Wide<T>> res(s.size() + 1); int c = 0;
    for (int i = 0; i + 1 < int(e.size()); ++i) {
        c += e[i].second;
        res[c] += geometry_detail::Wide<T>(e[i + 1].first) - e[i].first;}
    return res;}

namespace rectangle_detail {
    // len[x * (K + 1) + j]: measure covered at least j times inside node x, counting cover counts at x and below.
    struct DepthTree {
        int K, m; vector<lng> ys; vector<int> cnt; vector<lll> len;
        DepthTree(vector<lng> y, int K) : K(K), m(max(1, int(y.size()) - 1)), ys(std::move(y)), cnt(4 * m), len(4 * size_t(m) * (K + 1)) {}
        int index(lng y) const { return int(lower_bound(ys.begin(), ys.end(), y) - ys.begin()); }
        void update(int a, int b, int d, int x, int l, int r) {
            if (b <= l || r <= a) { return; }
            if (a <= l && r <= b) { cnt[x] += d; }
            else {
                int mid = (l + r) / 2;
                update(a, b, d, 2 * x, l, mid); update(a, b, d, 2 * x + 1, mid, r);}
            lll *p = &len[size_t(x) * (K + 1)];
            for (int j = 0; j <= K; ++j) {
                if (j <= cnt[x]) { p[j] = lll(ys[r]) - ys[l]; }
                else { p[j] = r - l == 1 ? 0 : len[size_t(2 * x) * (K + 1) + j - cnt[x]] + len[size_t(2 * x + 1) * (K + 1) + j - cnt[x]]; }}}
        void add(lng lo, lng hi, int d) { update(index(lo), index(hi), d, 1, 0, m); }
        lll covered(int j = 1) const { return len[K + 1 + j]; }
        // Calls f(lo, hi) for each maximal uncovered run inside [ys[a], ys[b]).
        template<typename F> void uncovered(int a, int b, int x, int l, int r, F &f) const {
            if (b <= l || r <= a || cnt[x]) { return; }
            if (len[size_t(x) * (K + 1) + 1] == 0 || r - l == 1) { f(ys[max(a, l)], ys[min(b, r)]); return; }
            int mid = (l + r) / 2;
            uncovered(a, b, 2 * x, l, mid, f); uncovered(a, b, 2 * x + 1, mid, r, f);}
    };
    inline bool valid(const array<lng, 4> &r) {
        for (lng v : r) { assert(-4000000000000000000LL <= v && v <= 4000000000000000000LL); }
        return r[0] < r[2] && r[1] < r[3];}
    inline vector<lng> coordinates(const vector<array<lng, 4>> &a) {
        vector<lng> ys;
        for (const auto &r : a) {
            if (valid(r)) { ys.push_back(r[1]); ys.push_back(r[3]); }}
        sort(ys.begin(), ys.end()); ys.erase(unique(ys.begin(), ys.end()), ys.end());
        return ys;}
    // Calls f(x, next x, tree) after each group of events at x; adds precede removals at equal x, and g(tree) runs between them.
    template<typename F, typename G> void sweep(const vector<array<lng, 4>> &a, int K, F f, G g) {
        assert(a.size() <= INT_MAX / 2);
        DepthTree t(coordinates(a), K); vector<array<lng, 4>> e;
        for (const auto &r : a) {
            if (valid(r)) { e.push_back({r[0], 0, r[1], r[3]}); e.push_back({r[2], 1, r[1], r[3]}); }}
        sort(e.begin(), e.end());
        for (int i = 0, j = 0; i < int(e.size()); i = j) {
            for (; j < int(e.size()) && e[j][0] == e[i][0]; ++j) {
                if (e[j][1] && (j == i || !e[j - 1][1])) { g(t); }
                t.add(e[j][2], e[j][3], e[j][1] ? -1 : 1);}
            f(e[i][0], j < int(e.size()) ? e[j][0] : e[i][0], t);}}
    inline lll vertical(const vector<array<lng, 4>> &a) {
        lll res = 0, before = 0, mid = -1;
        sweep(a, 1, [&](lng, lng, const DepthTree &t) {
            lll now = t.covered();
            res += mid < 0 ? now - before : (mid - before) + (mid - now);
            before = now; mid = -1;}, [&](const DepthTree &t) { mid = t.covered(); });
        return res;}
    inline vector<array<lng, 4>> transpose(vector<array<lng, 4>> a) {
        for (auto &r : a) { r = {r[1], r[0], r[3], r[2]}; }
        return a;}
    inline void boundary(const vector<array<lng, 4>> &a, bool flip, vector<array<point, 2>> &res) {
        assert(a.size() <= INT_MAX / 2);
        DepthTree t(coordinates(a), 1); vector<array<lng, 4>> e;
        for (const auto &r : a) {
            if (valid(r)) { e.push_back({r[0], 1, r[1], r[3]}); e.push_back({r[2], -1, r[1], r[3]}); }}
        sort(e.begin(), e.end());
        for (int i = 0, j = 0; i < int(e.size()); i = j) {
            lng x = e[i][0]; vector<pair<lng, lng>> span, runs;
            for (; j < int(e.size()) && e[j][0] == x; ++j) { span.push_back({e[j][2], e[j][3]}); }
            sort(span.begin(), span.end());
            int k = 0;
            for (auto [lo, hi] : span) {
                if (k && lo <= span[k - 1].second) { span[k - 1].second = max(span[k - 1].second, hi); }
                else { span[k++] = {lo, hi}; }}
            span.resize(k);
            auto g = [&](lng l, lng r) {
                if (!runs.empty() && runs.back().second == l) { runs.back().second = r; }
                else { runs.push_back({l, r}); }};
            // Every y of a span is covered just before x or just after x, so each uncovered run is a boundary piece.
            for (int pass = 0; pass < 2; ++pass) {
                runs.clear();
                for (auto [lo, hi] : span) { t.uncovered(t.index(lo), t.index(hi), 1, 0, t.m, g); }
                for (auto [l, r] : runs) {
                    point p{x, pass ? l : r}, q{x, pass ? r : l};
                    res.push_back(flip ? array<point, 2>{point{q.y, q.x}, point{p.y, p.x}} : array<point, 2>{p, q});}
                if (!pass) {
                    for (int s = i; s < j; ++s) { t.add(e[s][2], e[s][3], int(e[s][1])); }}}}}
} // namespace rectangle_detail

// T: O(n * log(n)), M: O(n). Half-open [x1, x2) x [y1, y2) given as {x1, y1, x2, y2}, |coordinate| <= 4 * 10^18; empty rectangles are ignored; exact.
inline lll rectangleUnionArea(const vector<array<lng, 4>> &a) {
    lll res = 0;
    rectangle_detail::sweep(a, 1, [&](lng x, lng next, const rectangle_detail::DepthTree &t) { res += t.covered() * (lll(next) - x); }, [](const rectangle_detail::DepthTree &) {});
    return res;}
inline lll rectangleUnionPerimeter(const vector<array<lng, 4>> &a) {
    return rectangle_detail::vertical(a) + rectangle_detail::vertical(rectangle_detail::transpose(a));}

// T: O(n * k * log(n)), M: O(n * k). As rectangleUnionArea, k >= 1: {area covered by exactly k rectangles, area covered by at least k}.
inline pair<lll, lll> rectangleCoverageArea(const vector<array<lng, 4>> &a, int k) {
    assert(k >= 1); k = max(k, 1); lll exact = 0, least = 0;
    if (k > int(a.size())) { return {0, 0}; }
    rectangle_detail::sweep(a, k + 1, [&](lng x, lng next, const rectangle_detail::DepthTree &t) {
        least += t.covered(k) * (lll(next) - x); exact += (t.covered(k) - t.covered(k + 1)) * (lll(next) - x);}, [](const rectangle_detail::DepthTree &) {});
    return {exact, least};}

// T: O(n * log(n)), M: O(n). As rectangleUnionArea: {maximum number of rectangles sharing a point, a lattice witness}; {0, (0, 0)} when all are empty.
inline pair<int, point> maxRectangleOverlap(const vector<array<lng, 4>> &a) {
    assert(a.size() <= INT_MAX / 2);
    vector<lng> ys = rectangle_detail::coordinates(a); int m = max(1, int(ys.size()) - 1);
    vector<int> mx(4 * m), lz(4 * m); vector<array<lng, 4>> e;
    auto update = [&](auto &&self, int l0, int r0, int d, int x, int l, int r) -> void {
        if (r0 <= l || r <= l0) { return; }
        if (l0 <= l && r <= r0) { lz[x] += d; }
        else {
            int mid = (l + r) / 2;
            self(self, l0, r0, d, 2 * x, l, mid); self(self, l0, r0, d, 2 * x + 1, mid, r);}
        mx[x] = lz[x] + (r - l == 1 ? 0 : max(mx[2 * x], mx[2 * x + 1]));};
    for (const auto &r : a) {
        if (rectangle_detail::valid(r)) { e.push_back({r[0], 1, r[1], r[3]}); e.push_back({r[2], 0, r[1], r[3]}); }}
    sort(e.begin(), e.end()); pair<int, point> res{0, {}};
    auto at = [&](lng y) { return int(lower_bound(ys.begin(), ys.end(), y) - ys.begin()); };
    for (int i = 0, j = 0; i < int(e.size()); i = j) {
        for (; j < int(e.size()) && e[j][0] == e[i][0]; ++j) { update(update, at(e[j][2]), at(e[j][3]), e[j][1] ? 1 : -1, 1, 0, m); }
        if (mx[1] > res.first) {
            int x = 1, l = 0, r = m;
            while (r - l > 1) {
                int mid = (l + r) / 2;
                if (mx[2 * x] >= mx[2 * x + 1]) { x = 2 * x; r = mid; } else { x = 2 * x + 1; l = mid; }}
            res = {mx[1], {e[i][0], ys[l]}};}}
    return res;}

// T: O((n + out) * log(n)), M: O(n + out). As rectangleUnionArea: maximal axis-parallel boundary segments {a, b} of the union, directed with the union on the left.
inline vector<array<point, 2>> rectangleUnionBoundary(const vector<array<lng, 4>> &a) {
    vector<array<point, 2>> res;
    rectangle_detail::boundary(a, false, res); rectangle_detail::boundary(rectangle_detail::transpose(a), true, res);
    return res;}

// T: O((n + m) * log(n + m)), M: O(n + m) (orthogonalIntersections O(n + m + out)). Closed horizontal {y, x1, x2} and vertical {x, y1, y2} segments,
// endpoints in either order; pairs (horizontal, vertical) sharing a point, grouped by vertical in x order, then by y.
inline lng orthogonalIntersectionCount(const vector<array<lng, 3>> &h, const vector<array<lng, 3>> &v) {
    assert(h.size() <= INT_MAX / 4 && v.size() <= INT_MAX / 4);
    vector<lng> ys; vector<array<lng, 4>> e;
    for (auto s : h) {
        if (s[2] < s[1]) { swap(s[1], s[2]); }
        ys.push_back(s[0]); e.push_back({s[1], 0, s[0], 0}); e.push_back({s[2], 2, s[0], 0});}
    for (auto s : v) {
        if (s[2] < s[1]) { swap(s[1], s[2]); }
        e.push_back({s[0], 1, s[1], s[2]});}
    sort(ys.begin(), ys.end()); ys.erase(unique(ys.begin(), ys.end()), ys.end()); sort(e.begin(), e.end());
    Fenwick<int> f(int(ys.size())); lng res = 0;
    auto at = [&](lng y) { return int(lower_bound(ys.begin(), ys.end(), y) - ys.begin()); };
    for (const auto &x : e) {
        if (x[1] == 1) { res += f.sum(at(x[2]), int(upper_bound(ys.begin(), ys.end(), x[3]) - ys.begin())); }
        else { f.add(at(x[2]), x[1] ? -1 : 1); }}
    return res;}
inline vector<pair<int, int>> orthogonalIntersections(const vector<array<lng, 3>> &h, const vector<array<lng, 3>> &v) {
    assert(h.size() <= INT_MAX / 4 && v.size() <= INT_MAX / 4);
    vector<array<lng, 5>> e; set<pair<lng, int>> on; vector<pair<int, int>> res;
    for (int i = 0; i < int(h.size()); ++i) {
        auto [y, l, r] = h[i];
        e.push_back({min(l, r), 0, y, 0, i}); e.push_back({max(l, r), 2, y, 0, i});}
    for (int i = 0; i < int(v.size()); ++i) {
        auto [x, l, r] = v[i];
        e.push_back({x, 1, min(l, r), max(l, r), i});}
    sort(e.begin(), e.end());
    for (const auto &x : e) {
        int id = int(x[4]);
        if (x[1] == 0) { on.insert({x[2], id}); }
        else if (x[1] == 2) { on.erase({x[2], id}); }
        else {
            for (auto it = on.lower_bound({x[2], INT_MIN}); it != on.end() && it->first <= x[3]; ++it) { res.push_back({it->second, id}); }}}
    return res;}
