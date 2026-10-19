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
    // Covered lengths: at most 8 * 10^18 for integral coordinates.
    template<typename T> using Len = std::conditional_t<std::is_integral_v<T>, ulng, long double>;
    inline constexpr int TREE_K = 16;
    template<typename T> bool valid(const array<T, 4> &r) {
        static_assert((std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) <= 8) || std::is_floating_point_v<T>);
        assert(std::all_of(r.begin(), r.end(), [](T v) {
            if constexpr (std::is_integral_v<T>) { return -4000000000000000000LL <= v && v <= 4000000000000000000LL; }
            else { return bool(std::isfinite(v)); }}));
        return r[0] < r[2] && r[1] < r[3];}
    template<typename T> vector<T> coordinates(const vector<array<T, 4>> &a, int axis) {
        vector<T> res;
        for (const auto &r : a) {
            if (valid(r)) { res.push_back(r[axis]); res.push_back(r[axis + 2]); }}
        sort(res.begin(), res.end()); res.erase(unique(res.begin(), res.end()), res.end());
        return res;}
    template<typename T> int rank(const vector<T> &v, T z) { return int(lower_bound(v.begin(), v.end(), z) - v.begin()); }
    // Orthogonal segments: NaN would break the sort's ordering, so non-finite segments are skipped after the assertion.
    template<typename T> bool finite(const array<T, 3> &s) {
        if constexpr (std::is_floating_point_v<T>) { return std::isfinite(s[0]) && std::isfinite(s[1]) && std::isfinite(s[2]); }
        else { return true; }}
    // Node [l, r) with r - l >= 2 is stored at its split point (l + r) / 2, which no other node shares; leaves are computed from their counts.
    // len[mid * K + j - 1]: measure covered at least j times inside the node, counting cover counts at the node and below.
    template<typename T> struct DepthTree {
        int K, m; vector<T> ys; vector<int> leaf, node; vector<Len<T>> len;
        DepthTree(vector<T> y, int K) : K(K), m(max(1, int(y.size()) - 1)), ys(std::move(y)), leaf(m), node(m), len(size_t(m) * K) {}
        Len<T> width(int l, int r) const { return Len<T>(ys[r]) - Len<T>(ys[l]); }
        int count(int l, int r) const { return r - l == 1 ? leaf[l] : node[(l + r) / 2]; }
        Len<T> at(int l, int r, int j) const {
            if (r - l == 1) { return j <= leaf[l] ? width(l, r) : Len<T>(0); }
            return len[size_t((l + r) / 2) * K + j - 1];}
        void update(int a, int b, int d, int l, int r) {
            if (b <= l || r <= a) { return; }
            int mid = (l + r) / 2;
            if (a <= l && r <= b) { (r - l == 1 ? leaf[l] : node[mid]) += d; }
            else { update(a, b, d, l, mid); update(a, b, d, mid, r); }
            if (r - l == 1) { return; }
            Len<T> *p = &len[size_t(mid) * K]; int c = node[mid];
            for (int j = 1; j <= K; ++j) { p[j - 1] = j <= c ? width(l, r) : at(l, mid, j - c) + at(mid, r, j - c); }}
        int index(T y) const { return rank(ys, y); }
        void add(T lo, T hi, int d) { update(index(lo), index(hi), d, 0, m); }
        Len<T> covered(int j = 1) const { return at(0, m, j); }
        // Calls f(lo, hi) for each maximal uncovered run inside [ys[a], ys[b]).
        template<typename F> void uncovered(int a, int b, int l, int r, F &f) const {
            if (b <= l || r <= a || count(l, r)) { return; }
            if (r - l == 1 || at(l, r, 1) == 0) { f(ys[max(a, l)], ys[min(b, r)]); return; }
            uncovered(a, b, l, (l + r) / 2, f); uncovered(a, b, (l + r) / 2, r, f);}
    };
    // Depths of elementary intervals in blocks of b: base depths, a lazy add per block, intervals sorted by base, distinct bases with suffix weights and the first
    // reaching k (a +-1 lazy change moves it once); at0, at1 cache each block's weight at depth >= k and > k; floating totals are recounted every nb changes.
    template<typename T> struct DepthBlocks {
        int m, b, nb, k, ops = 0; vector<Len<T>> w, suf, at0, at1; Len<T> tot[2]{}; vector<int> base, ord, val, cnt, lz, ptr, in, out;
        DepthBlocks(const vector<T> &ys, int k) : m(max(1, int(ys.size()) - 1)), b(max(1, int(std::sqrt(m / 32.)))), nb((m + b - 1) / b), k(k),
                w(m), suf(size_t(nb) * (b + 1)), at0(nb), at1(nb), base(m), ord(m), val(m), cnt(nb), lz(nb), ptr(nb), in(b), out(b) {
            for (int i = 0; i + 1 < int(ys.size()); ++i) { w[i] = Len<T>(ys[i + 1]) - Len<T>(ys[i]); }
            iota(ord.begin(), ord.end(), 0);
            for (int i = 0; i < nb; ++i) { rebuild(i); settle(i); }}
        void recount() {
            ops = 0; tot[0] = tot[1] = 0;
            for (int i = 0; i < nb; ++i) { tot[0] += at0[i]; tot[1] += at1[i]; }}
        void settle(int i) {
            int s = i * b, p = ptr[i], q = p + (p < cnt[i] && val[s + p] + lz[i] == k); size_t o = size_t(i) * (b + 1);
            tot[0] += suf[o + p] - at0[i]; tot[1] += suf[o + q] - at1[i]; at0[i] = suf[o + p]; at1[i] = suf[o + q];
            if (std::is_floating_point_v<Len<T>> && ++ops >= nb) { recount(); }}
        void rebuild(int i) {
            int s = i * b, e = min(m, s + b), c = 0; size_t o = size_t(i) * (b + 1); Len<T> acc = 0;
            for (int j = s; j < e; ++j) {
                if (j == s || base[ord[j]] != base[ord[j - 1]]) { val[s + c++] = base[ord[j]]; }}
            cnt[i] = c; suf[o + c] = 0;
            for (int j = e - 1, d = c; j >= s; --j) {
                acc += w[ord[j]];
                if (j == s || base[ord[j]] != base[ord[j - 1]]) { suf[o + --d] = acc; }}
            ptr[i] = int(lower_bound(val.begin() + s, val.begin() + s + c, k - lz[i]) - val.begin()) - s;}
        void shift(int i, int d) {
            lz[i] += d; int &p = ptr[i], need = k - lz[i], s = i * b;
            if (d > 0 && p > 0 && val[s + p - 1] >= need) { --p; }
            if (d < 0 && p < cnt[i] && val[s + p] < need) { ++p; }
            settle(i);}
        void partial(int i, int l, int r, int d) {
            int s = i * b, e = min(m, s + b), x = 0, y = 0;
            if (l == s && r == e) { shift(i, d); return; }
            for (int j = l; j < r; ++j) { base[j] += d; }
            // The changed intervals and the others each stay sorted by base; merge them.
            for (int j = s; j < e; ++j) {
                int v = ord[j];
                if (l <= v && v < r) { in[x++] = v; } else { out[y++] = v; }}
            std::merge(in.begin(), in.begin() + x, out.begin(), out.begin() + y, ord.begin() + s, [&](int u, int v) { return base[u] < base[v]; });
            rebuild(i); settle(i);}
        void add(int l, int r, int d) {
            if (l >= r) { return; }
            int bl = l / b, br = (r - 1) / b;
            if (bl == br) { partial(bl, l, r, d); return; }
            partial(bl, l, (bl + 1) * b, d); partial(br, br * b, r, d);
            for (int i = bl + 1; i < br; ++i) { shift(i, d); }}
        Len<T> atLeast(int t) const { return tot[t]; }
    };
    // Calls f(x, next x, tree) after each group of events at x; adds precede removals at equal x, and g(tree) runs between them.
    template<typename T, typename F, typename G> void sweep(const vector<array<T, 4>> &a, int K, F f, G g) {
        assert(a.size() <= INT_MAX / 2);
        DepthTree<T> t(coordinates(a, 1), K); vector<tuple<T, int, T, T>> e;
        for (const auto &r : a) {
            if (valid(r)) { e.push_back({r[0], 0, r[1], r[3]}); e.push_back({r[2], 1, r[1], r[3]}); }}
        sort(e.begin(), e.end());
        for (int i = 0, j = 0; i < int(e.size()); i = j) {
            T x = std::get<0>(e[i]);
            for (; j < int(e.size()) && std::get<0>(e[j]) == x; ++j) {
                auto [p, out, lo, hi] = e[j];
                if (out && (j == i || !std::get<1>(e[j - 1]))) { g(t); }
                t.add(lo, hi, out ? -1 : 1);}
            f(x, j < int(e.size()) ? std::get<0>(e[j]) : x, t);}}
    template<typename T> auto vertical(const vector<array<T, 4>> &a) {
        using W = geometry_detail::Wide<T>; W res = 0, before = 0, mid = -1;
        sweep(a, 1, [&](T, T, const DepthTree<T> &t) {
            W now = W(t.covered());
            res += mid < 0 ? now - before : (mid - before) + (mid - now);
            before = now; mid = -1;}, [&](const DepthTree<T> &t) { mid = W(t.covered()); });
        return res;}
    template<typename T> vector<array<T, 4>> transpose(vector<array<T, 4>> a) {
        for (auto &r : a) { r = {r[1], r[0], r[3], r[2]}; }
        return a;}
    template<typename T> void boundary(const vector<array<T, 4>> &a, bool flip, vector<array<Point2<T>, 2>> &res) {
        assert(a.size() <= INT_MAX / 2);
        DepthTree<T> t(coordinates(a, 1), 1); vector<tuple<T, int, T, T>> e;
        for (const auto &r : a) {
            if (valid(r)) { e.push_back({r[0], 1, r[1], r[3]}); e.push_back({r[2], -1, r[1], r[3]}); }}
        sort(e.begin(), e.end());
        for (int i = 0, j = 0; i < int(e.size()); i = j) {
            T x = std::get<0>(e[i]); vector<pair<T, T>> span, runs;
            for (; j < int(e.size()) && std::get<0>(e[j]) == x; ++j) { span.push_back({std::get<2>(e[j]), std::get<3>(e[j])}); }
            sort(span.begin(), span.end());
            int k = 0;
            for (auto [lo, hi] : span) {
                if (k && lo <= span[k - 1].second) { span[k - 1].second = max(span[k - 1].second, hi); }
                else { span[k++] = {lo, hi}; }}
            span.resize(k);
            auto g = [&](T l, T r) {
                if (!runs.empty() && runs.back().second == l) { runs.back().second = r; }
                else { runs.push_back({l, r}); }};
            // Every y of a span is covered just before x or just after x, so each uncovered run is a boundary piece.
            for (int pass = 0; pass < 2; ++pass) {
                runs.clear();
                for (auto [lo, hi] : span) { t.uncovered(t.index(lo), t.index(hi), 0, t.m, g); }
                for (auto [l, r] : runs) {
                    Point2<T> p{x, pass ? l : r}, q{x, pass ? r : l};
                    res.push_back(flip ? array<Point2<T>, 2>{Point2<T>{q.y, q.x}, Point2<T>{p.y, p.x}} : array<Point2<T>, 2>{p, q});}
                if (!pass) {
                    for (int s = i; s < j; ++s) { t.add(std::get<2>(e[s]), std::get<3>(e[s]), std::get<1>(e[s])); }}}}}
    // Gueting's divide and conquer over vertical edges in rank space: each y-stripe keeps a tree of the true x transitions of the union, shared by refined stripes.
    struct Contour {
        struct Edge { int x, side, y1, y2, id; };
        struct Node { int key, hi, l, r; int8_t side, hiSide; };
        struct Part { vector<int> P, tree, L, R; };
        int m; vector<Edge> e; vector<Node> t; vector<int> y1, y2, pos; vector<char> mark;
        int leaf(int x, int side) { t.push_back({x, x, -1, -1, int8_t(side), int8_t(side)}); return int(t.size()) - 1; }
        int join(int a, int b) {
            if (a < 0 || b < 0) { return a < 0 ? b : a; }
            t.push_back({t[a].hi, t[b].hi, a, b, 0, t[b].hiSide});
            return int(t.size()) - 1;}
        void index(const vector<int> &P) {
            for (int i = 0; i < int(P.size()); ++i) { pos[P[i] + 1] = i; }}
        // Stripes of P whose y-range lies inside an interval of the rectangles ids.
        vector<char> inside(const vector<int> &P, const vector<int> &ids) {
            vector<int> d(P.size()); vector<char> res(P.size() - 1);
            for (int id : ids) { ++d[pos[y1[id] + 1]]; --d[pos[y2[id] + 1]]; }
            for (int i = 0, c = 0; i + 1 < int(P.size()); ++i) { c += d[i]; res[i] = c > 0; }
            return res;}
        Part build(int l, int r) {
            if (r - l == 1) {
                const Edge &x = e[l]; Part res{{-1, x.y1, x.y2, m}, {-1, leaf(x.x, x.side), -1}, {}, {}};
                (x.side > 0 ? res.L : res.R).push_back(x.id);
                return res;}
            Part a = build(l, (l + r) / 2), b = build((l + r) / 2, r), res; vector<int> right, left;
            // Rectangles closing in b with their left edge in a are complete; the others span a whole half.
            for (int id : b.R) { mark[id] = 1; }
            for (int id : a.L) {
                if (!mark[id]) { right.push_back(id); }}
            for (int id : a.L) { mark[id] = 0; }
            for (int id : b.R) {
                if (mark[id]) { left.push_back(id); }
                mark[id] = 0;}
            res.L = right; res.L.insert(res.L.end(), b.L.begin(), b.L.end());
            res.R = a.R; res.R.insert(res.R.end(), left.begin(), left.end());
            std::set_union(a.P.begin(), a.P.end(), b.P.begin(), b.P.end(), std::back_inserter(res.P));
            index(res.P);
            auto c1 = inside(res.P, left), c2 = inside(res.P, right);
            res.tree.resize(res.P.size() - 1);
            for (int i = 0, j = 0, k = 0; i + 1 < int(res.P.size()); ++i) {
                while (a.P[j + 1] <= res.P[i]) { ++j; }
                while (b.P[k + 1] <= res.P[i]) { ++k; }
                res.tree[i] = join(c1[i] ? -1 : a.tree[j], c2[i] ? -1 : b.tree[k]);}
            return res;}
        // Covered just left of x: the side of the last leaf before x, uncovered when there is none.
        bool coveredBefore(int v, int x) const {
            int side = -1;
            while (v >= 0) {
                if (t[v].l < 0) {
                    if (t[v].key < x) { side = t[v].side; }
                    break;}
                if (t[v].key < x) { side = t[t[v].l].hiSide; v = t[v].r; }
                else { v = t[v].l; }}
            return side > 0;}
        template<typename F> void each(int v, int lo, int hi, F &f) const {
            if (v < 0) { return; }
            if (t[v].l < 0) {
                if (lo <= t[v].key && t[v].key < hi) { f(t[v].key, t[v].side); }
                return;}
            if (lo <= t[v].key) { each(t[v].l, lo, hi, f); }
            if (t[v].key < hi) { each(t[v].r, lo, hi, f); }}
        // Calls emit(a, b) for every maximal uncovered run [a, b) of [lo, hi) in the stripe tree v.
        template<typename F> void runs(int v, int lo, int hi, F emit) const {
            bool cov = coveredBefore(v, lo); int start = lo;
            auto f = [&](int x, int side) {
                if (side > 0 && !cov && start < x) { emit(start, x); }
                cov = side > 0; start = x;};
            each(v, lo, hi, f);
            if (!cov && start < hi) { emit(start, hi); }}
    };
    // Horizontal pieces {x1, y1, x2, y2} in ranks of xs and ys, the x and y coordinates of a (of the transpose when flip, mapped back).
    template<typename T> void gutingHorizontal(const vector<array<T, 4>> &a, const vector<T> &xs, const vector<T> &ys, bool flip, vector<array<int, 4>> &res) {
        assert(a.size() <= INT_MAX / 2);
        Contour c; c.m = int(ys.size());
        c.y1.assign(a.size(), 0); c.y2.assign(a.size(), 0); c.mark.assign(a.size(), 0); c.pos.assign(ys.size() + 2, 0);
        vector<array<int, 4>> sides; vector<tuple<T, int, int>> ev; vector<int> xr(2 * a.size());
        for (int i = 0; i < int(a.size()); ++i) {
            if (!valid(a[i])) { continue; }
            c.y1[i] = rank(ys, a[i][1]); c.y2[i] = rank(ys, a[i][3]);
            ev.push_back({a[i][0], 0, i}); ev.push_back({a[i][2], 1, i});}
        if (ev.empty()) { return; }
        // Edges by x with left edges first; the x-ranks follow from one scan of xs.
        sort(ev.begin(), ev.end());
        for (int j = 0, r = 0; j < int(ev.size()); ++j) {
            auto [x, right, i] = ev[j];
            while (xs[r] < x) { ++r; }
            c.e.push_back({r, right ? -1 : 1, c.y1[i], c.y2[i], i}); xr[2 * i + right] = r;}
        for (int i = 0; i < int(a.size()); ++i) {
            if (valid(a[i])) { sides.push_back({c.y1[i], 0, xr[2 * i], xr[2 * i + 1]}); sides.push_back({c.y2[i], 1, xr[2 * i], xr[2 * i + 1]}); }}
        auto root = c.build(0, int(c.e.size()));
        c.index(root.P);
        sort(sides.begin(), sides.end());
        // Bottom edges look at the stripe below their y, top edges at the stripe above; touching edges at one y are merged first.
        for (int i = 0, j; i < int(sides.size()); i = j) {
            int y = sides[i][0], top = sides[i][1], lo = sides[i][2], hi = sides[i][3];
            for (j = i + 1; j < int(sides.size()) && sides[j][0] == y && sides[j][1] == top && sides[j][2] <= hi; ++j) { hi = max(hi, sides[j][3]); }
            c.runs(root.tree[c.pos[y + 1] - !top], lo, hi, [&](int l, int r) {
                array<int, 4> p{top ? r : l, y, top ? l : r, y};
                res.push_back(flip ? array<int, 4>{p[3], p[2], p[1], p[0]} : p);});}}
    template<typename T> vector<array<int, 4>> gutingPieces(const vector<array<T, 4>> &a, const vector<T> &xs, const vector<T> &ys) {
        vector<array<int, 4>> res;
        gutingHorizontal(a, xs, ys, false, res); gutingHorizontal(transpose(a), ys, xs, true, res);
        return res;}
    // Rings of rank-space pieces: sorted by vertex, leaving and entering pieces coincide position by position, and where two of each meet
    // an entering piece continues with the leaving piece on its right; a ring starts at its first piece in vertex order, its smallest vertex.
    inline vector<vector<array<int, 2>>> rings(const vector<array<int, 4>> &p, int nx, int ny) {
        int n = int(p.size());
        auto order = [&](int o) {
            vector<int> res(n), tmp(n), c(size_t(max(nx, ny)) + 1);
            iota(res.begin(), res.end(), 0);
            for (int pass = 1; pass >= 0; --pass) {
                fill(c.begin(), c.end(), 0);
                for (int i : res) { ++c[p[i][o + pass] + 1]; }
                for (int i = 1; i < int(c.size()); ++i) { c[i] += c[i - 1]; }
                for (int i : res) { tmp[c[p[i][o + pass]]++] = i; }
                swap(res, tmp);}
            return res;};
        vector<int> s = order(0), e = order(2), succ(n); vector<char> seen(n); vector<vector<array<int, 2>>> res;
        for (int i = 0, j = 0; i < n; i = j) {
            while (j < n && p[s[j]][0] == p[s[i]][0] && p[s[j]][1] == p[s[i]][1]) { ++j; }
            for (int u = i; u < j; ++u) {
                const auto &q = p[e[u]]; succ[e[u]] = s[i];
                for (int v = i; v < j; ++v) {
                    const auto &r = p[s[v]];
                    if (lng(q[2] - q[0]) * (r[3] - r[1]) - lng(q[3] - q[1]) * (r[2] - r[0]) < 0) { succ[e[u]] = s[v]; }}}}
        for (int i : s) {
            if (seen[i]) { continue; }
            auto &r = res.emplace_back();
            for (int x = i; !seen[x]; x = succ[x]) { seen[x] = 1; r.push_back({p[x][0], p[x][1]}); }}
        return res;}
    template<typename T> auto coverageTree(const vector<array<T, 4>> &a, int k) {
        using W = geometry_detail::Wide<T>; W exact = 0, least = 0;
        if (k > int(a.size())) { return pair<W, W>{0, 0}; }
        sweep(a, k + 1, [&](T x, T next, const DepthTree<T> &t) {
            W dx = W(next) - x;
            least += W(t.covered(k)) * dx; exact += (W(t.covered(k)) - W(t.covered(k + 1))) * dx;}, [](const DepthTree<T> &) {});
        return pair<W, W>{exact, least};}
    template<typename T> auto coverageBlocks(const vector<array<T, 4>> &a, int k) {
        using W = geometry_detail::Wide<T>; W exact = 0, least = 0;
        if (k > int(a.size())) { return pair<W, W>{0, 0}; }
        vector<T> ys = coordinates(a, 1); DepthBlocks<T> s(ys, k); vector<tuple<T, int, int, int>> e;
        for (const auto &r : a) {
            if (valid(r)) { e.push_back({r[0], 1, rank(ys, r[1]), rank(ys, r[3])}); e.push_back({r[2], -1, rank(ys, r[1]), rank(ys, r[3])}); }}
        sort(e.begin(), e.end());
        for (int i = 0, j = 0; i < int(e.size()); i = j) {
            for (; j < int(e.size()) && std::get<0>(e[j]) == std::get<0>(e[i]); ++j) { s.add(std::get<2>(e[j]), std::get<3>(e[j]), std::get<1>(e[j])); }
            if (j < int(e.size())) {
                W dx = W(std::get<0>(e[j])) - std::get<0>(e[i]), atk = W(s.atLeast(0));
                least += atk * dx; exact += (atk - W(s.atLeast(1))) * dx;}}
        return pair<W, W>{exact, least};}
} // namespace rectangle_detail

// T: O(n * log(n)), M: O(n). Half-open [x1, x2) x [y1, y2) given as {x1, y1, x2, y2}; signed integral (|coordinate| <= 4 * 10^18, exact lll) or finite
// floating (rounded long double, no epsilon) coordinates; empty rectangles are ignored. Points and segments below use the coordinate type.
template<typename T = lng> auto rectangleUnionArea(const vector<array<T, 4>> &a) {
    geometry_detail::Wide<T> res = 0;
    rectangle_detail::sweep(a, 1, [&](T x, T next, const auto &t) { res += geometry_detail::Wide<T>(t.covered()) * (geometry_detail::Wide<T>(next) - x); }, [](const auto &) {});
    return res;}
template<typename T = lng> auto rectangleUnionPerimeter(const vector<array<T, 4>> &a) {
    return rectangle_detail::vertical(a) + rectangle_detail::vertical(rectangle_detail::transpose(a));}

// T: O(n * k * log(n)) for k <= 16, else O(n * sqrt(n)), M: O(n). As rectangleUnionArea, k >= 1: {area covered by exactly k rectangles, area covered by at least k}.
template<typename T = lng> auto rectangleCoverageArea(const vector<array<T, 4>> &a, int k) {
    assert(k >= 1); k = max(k, 1);
    return k <= rectangle_detail::TREE_K ? rectangle_detail::coverageTree(a, k) : rectangle_detail::coverageBlocks(a, k);}

// T: O(n * log(n)), M: O(n). As rectangleUnionArea: {maximum number of rectangles sharing a point, a witness corner}; {0, (0, 0)} when all are empty.
template<typename T = lng> pair<int, Point2<T>> maxRectangleOverlap(const vector<array<T, 4>> &a) {
    assert(a.size() <= INT_MAX / 2);
    vector<T> ys = rectangle_detail::coordinates(a, 1); int m = max(1, int(ys.size()) - 1);
    vector<int> mx(4 * m), lz(4 * m); vector<tuple<T, int, int, int>> e;
    auto update = [&](auto &&self, int l0, int r0, int d, int x, int l, int r) -> void {
        if (r0 <= l || r <= l0) { return; }
        if (l0 <= l && r <= r0) { lz[x] += d; }
        else {
            int mid = (l + r) / 2;
            self(self, l0, r0, d, 2 * x, l, mid); self(self, l0, r0, d, 2 * x + 1, mid, r);}
        mx[x] = lz[x] + (r - l == 1 ? 0 : max(mx[2 * x], mx[2 * x + 1]));};
    for (const auto &r : a) {
        if (rectangle_detail::valid(r)) {
            int lo = rectangle_detail::rank(ys, r[1]), hi = rectangle_detail::rank(ys, r[3]);
            e.push_back({r[0], 1, lo, hi}); e.push_back({r[2], 0, lo, hi});}}
    sort(e.begin(), e.end()); pair<int, Point2<T>> res{0, {}};
    for (int i = 0, j = 0; i < int(e.size()); i = j) {
        for (; j < int(e.size()) && std::get<0>(e[j]) == std::get<0>(e[i]); ++j) { update(update, std::get<2>(e[j]), std::get<3>(e[j]), std::get<1>(e[j]) ? 1 : -1, 1, 0, m); }
        if (mx[1] > res.first) {
            int x = 1, l = 0, r = m;
            while (r - l > 1) {
                int mid = (l + r) / 2;
                if (mx[2 * x] >= mx[2 * x + 1]) { x = 2 * x; r = mid; } else { x = 2 * x + 1; l = mid; }}
            res = {mx[1], {std::get<0>(e[i]), ys[l]}};}}
    return res;}

// T: O((n + out) * log(n)), M: O(n + out). As rectangleUnionArea: maximal axis-parallel boundary segments {a, b} of the union, directed with the union on the left.
template<typename T = lng> vector<array<Point2<T>, 2>> rectangleUnionBoundary(const vector<array<T, 4>> &a) {
    vector<array<Point2<T>, 2>> res;
    rectangle_detail::boundary(a, false, res); rectangle_detail::boundary(rectangle_detail::transpose(a), true, res);
    return res;}
// T: O(n * log(n) + out), M: O(n * log(n) + out). The same segments by Gueting's divide and conquer over the vertical edges.
template<typename T = lng> vector<array<Point2<T>, 2>> rectangleUnionBoundaryGuting(const vector<array<T, 4>> &a) {
    vector<T> xs = rectangle_detail::coordinates(a, 0), ys = rectangle_detail::coordinates(a, 1); vector<array<Point2<T>, 2>> res;
    for (auto [x1, y1, x2, y2] : rectangle_detail::gutingPieces(a, xs, ys)) { res.push_back({Point2<T>{xs[x1], ys[y1]}, Point2<T>{xs[x2], ys[y2]}}); }
    return res;}
// T: O(n * log(n) + out), M: O(n * log(n) + out). Rings of corners, union on the left (outer CCW, holes CW), each from its smallest vertex, in that order;
// where four pieces meet a ring turns right, so rings bound the closed union: corner-touching squares form one ring through the corner twice.
template<typename T = lng> vector<vector<Point2<T>>> rectangleUnionContours(const vector<array<T, 4>> &a) {
    vector<T> xs = rectangle_detail::coordinates(a, 0), ys = rectangle_detail::coordinates(a, 1); vector<vector<Point2<T>>> res;
    for (const auto &r : rectangle_detail::rings(rectangle_detail::gutingPieces(a, xs, ys), int(xs.size()), int(ys.size()))) {
        auto &q = res.emplace_back();
        for (auto [x, y] : r) { q.push_back({xs[x], ys[y]}); }}
    return res;}

// T: O((n + m) * log(n + m)), M: O(n + m) (orthogonalIntersections O(n + m + out)). Closed horizontal {y, x1, x2} and vertical {x, y1, y2} segments, endpoints
// in either order, signed integral or finite floating; pairs (horizontal, vertical) sharing a point, grouped by vertical in x order, then by y.
template<typename T = lng> lng orthogonalIntersectionCount(const vector<array<T, 3>> &h, const vector<array<T, 3>> &v) {
    static_assert((std::is_integral_v<T> && std::is_signed_v<T>) || std::is_floating_point_v<T>);
    assert(h.size() <= INT_MAX / 4 && v.size() <= INT_MAX / 4);
    vector<T> ys; vector<tuple<T, int, T, T>> e;
    for (auto s : h) {
        assert(rectangle_detail::finite(s));
        if (s[2] < s[1]) { swap(s[1], s[2]); }
        if (rectangle_detail::finite(s)) { ys.push_back(s[0]); e.push_back({s[1], 0, s[0], s[0]}); e.push_back({s[2], 2, s[0], s[0]}); }}
    for (auto s : v) {
        assert(rectangle_detail::finite(s));
        if (s[2] < s[1]) { swap(s[1], s[2]); }
        if (rectangle_detail::finite(s)) { e.push_back({s[0], 1, s[1], s[2]}); }}
    sort(ys.begin(), ys.end()); ys.erase(unique(ys.begin(), ys.end()), ys.end()); sort(e.begin(), e.end());
    Fenwick<int> f(int(ys.size())); lng res = 0;
    for (const auto &[x, k, l, r] : e) {
        if (k == 1) { res += f.sum(rectangle_detail::rank(ys, l), int(upper_bound(ys.begin(), ys.end(), r) - ys.begin())); }
        else { f.add(rectangle_detail::rank(ys, l), k ? -1 : 1); }}
    return res;}
template<typename T = lng> vector<pair<int, int>> orthogonalIntersections(const vector<array<T, 3>> &h, const vector<array<T, 3>> &v) {
    static_assert((std::is_integral_v<T> && std::is_signed_v<T>) || std::is_floating_point_v<T>);
    assert(h.size() <= INT_MAX / 4 && v.size() <= INT_MAX / 4);
    vector<tuple<T, int, T, T, int>> e; set<pair<T, int>> on; vector<pair<int, int>> res;
    for (int i = 0; i < int(h.size()); ++i) {
        auto [y, l, r] = h[i]; assert(rectangle_detail::finite(h[i]));
        if (rectangle_detail::finite(h[i])) { e.push_back({min(l, r), 0, y, y, i}); e.push_back({max(l, r), 2, y, y, i}); }}
    for (int i = 0; i < int(v.size()); ++i) {
        auto [x, l, r] = v[i]; assert(rectangle_detail::finite(v[i]));
        if (rectangle_detail::finite(v[i])) { e.push_back({x, 1, min(l, r), max(l, r), i}); }}
    sort(e.begin(), e.end());
    for (const auto &[x, k, l, r, id] : e) {
        if (k == 0) { on.insert({l, id}); }
        else if (k == 2) { on.erase({l, id}); }
        else {
            for (auto it = on.lower_bound({l, INT_MIN}); it != on.end() && it->first <= r; ++it) { res.push_back({it->second, id}); }}}
    return res;}
