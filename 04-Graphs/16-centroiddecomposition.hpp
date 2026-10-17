#pragma once
#include "../01-Core/01-template.hpp"
#include "01-graph.hpp"
#include "../05-Mathematics/13-convolution.hpp"

// Undirected forest, n <= 2^24; distances count edges, or add W(w) when weighted; comp(c) is order[tin[c], tin[c] + size[c]).
// S: O(n * log(n)), Q: O(1), M: O(n * log(n)); levels stay below L = bit_width(n).
template<class W = int>
struct CentroidDecomposition {
    int n, L;
    vector<int> parent, level, size, tin, order;
    vector<W> dist;

    template<class G>
    explicit CentroidDecomposition(const G &g, bool weighted = false) : n(g.n), L(int(std::bit_width(uint(n)))),
        parent(n, -1), level(n, -1), size(n, 1), tin(n), order(n), dist(size_t(n) * L) {
        assert(!g.directed && n <= 1 << 24);
        vector<int> up(n, -1), sub(n), mark(n, -1), que, todo;
        auto bfs = [&](int s, int t, int k) {
            que.assign(1, s); mark[s] = t;
            for (int i = 0; i < int(que.size()); ++i) {
                int u = que[i]; sub[u] = 1;
                for (int a : g[u]) {
                    int v = g.arcs[a].to;
                    if (level[v] != -1 || mark[v] == t) { continue; }
                    mark[v] = t; up[v] = u; que.push_back(v);
                    if (k >= 0) { dist[v * L + k] = dist[u * L + k] + (weighted ? W(g.arcs[a].w) : W(1)); }}}
            for (int i = int(que.size()) - 1; i > 0; --i) { sub[up[que[i]]] += sub[que[i]]; }};
        for (int s = 0; s < n; ++s) {
            if (mark[s] == -1) { bfs(s, n + s, -1); todo.push_back(s); }}
        bool forest = !g.directed && g.arcs.size() == 2 * (size_t(n) - todo.size());
        assert(forest);
        if (!forest) { todo.clear(); }
        int timer = 0;
        // Stack order makes creation order a preorder of the centroid forest; up[r] is the centroid that split r off.
        while (!todo.empty()) {
            int r = todo.back(), c = r; todo.pop_back();
            for (int x = -1; x != c;) {
                x = c;
                for (int a : g[x]) {
                    int v = g.arcs[a].to;
                    if (level[v] == -1 && up[v] == x && 2 * sub[v] > sub[r]) { c = v; break; }}}
            parent[c] = up[r]; level[c] = up[r] == -1 ? 0 : level[up[r]] + 1; size[c] = sub[r];
            order[tin[c] = timer++] = c;
            bfs(c, c, level[c]);
            for (int a : g[c]) {
                if (level[g.arcs[a].to] == -1) { todo.push_back(g.arcs[a].to); }}}
        for (int v = 0; v < n && !forest; ++v) { level[v] = 0; tin[v] = order[v] = v; }}

    W distance(int v, int k) const {
        assert(0 <= v && v < n && 0 <= k && k <= level[v]); return dist[v * L + k];}
};

// Nonnegative weights; marking is idempotent and unmark is lazy; nearest is -1 when no marked vertex shares v's tree.
// S: O(n * log(n)), U: O(log(n)^2) mark, O(1) unmark, Q: O(log(n)^2) amortized, M: O(n * log(n) + marks * log(n)).
template<class W = int>
struct NearestMarked {
    CentroidDecomposition<W> cd;
    vector<std::priority_queue<pair<W, int>, vector<pair<W, int>>, std::greater<>>> heap;
    vector<char> marked;

    template<class G>
    explicit NearestMarked(const G &g, bool weighted = false) : cd(g, weighted), heap(cd.n), marked(cd.n) {
        assert(!weighted || std::ranges::all_of(g.edges, [](const GraphEdge &e) { return e.w >= 0; }));}

    void mark(int v) {
        assert(0 <= v && v < cd.n);
        if (marked[v]) { return; }
        marked[v] = 1;
        for (int c = v, k = cd.level[v]; c != -1; c = cd.parent[c], --k) { heap[c].push({cd.distance(v, k), v}); }}
    void unmark(int v) {
        assert(0 <= v && v < cd.n); marked[v] = 0;}

    W nearest(int v) {
        assert(0 <= v && v < cd.n);
        W res = -1;
        for (int c = v, k = cd.level[v]; c != -1; c = cd.parent[c], --k) {
            auto &h = heap[c];
            while (!h.empty() && !marked[h.top().se]) { h.pop(); }
            if (!h.empty() && (res == -1 || h.top().fi + cd.distance(v, k) < res)) { res = h.top().fi + cd.distance(v, k); }}
        return res;}
};

// T a commutative ring holding W; sum(v) = sum of x * dist(v, u) over add(u, x) with u in v's tree.
// S: O(n * log(n)), U: O(log(n)), Q: O(log(n)), M: O(n * log(n)).
template<class T, class W = int>
struct DistanceSum {
    CentroidDecomposition<W> cd;
    vector<T> cnt, own, rel;

    template<class G>
    explicit DistanceSum(const G &g, bool weighted = false) : cd(g, weighted), cnt(cd.n), own(cd.n), rel(cd.n) {}

    void add(int v, T x) {
        assert(0 <= v && v < cd.n);
        for (int c = v, k = cd.level[v]; c != -1; c = cd.parent[c], --k) {
            cnt[c] += x; own[c] += x * T(cd.distance(v, k));
            if (k) { rel[c] += x * T(cd.distance(v, k - 1)); }}}

    T sum(int v) const {
        assert(0 <= v && v < cd.n);
        T res = T();
        for (int c = v, k = cd.level[v], prev = -1; c != -1; prev = c, c = cd.parent[c], --k) {
            T d = T(cd.distance(v, k));
            res += own[c] + cnt[c] * d;
            if (prev != -1) { res -= rel[prev] + cnt[prev] * d; }}
        return res;}
};

namespace centroid_detail {
    // S: O(n * log(n)), U: O(log(n)), Q: O(log(n)), M: O(n * log(n)); per centroid c, Fenwick banks of size[c] + 1 slots:
    // distances to c, then distances of comp(c) to parent[c].
    template<class T>
    struct Contour {
        CentroidDecomposition<int> cd;
        vector<int> off;
        vector<T> bit;

        template<class G>
        explicit Contour(const G &g) : cd(g), off(cd.n + 1) {
            for (int c = 0; c < cd.n; ++c) { off[c + 1] = off[c] + 2 * (cd.size[c] + 1); }
            bit.assign(off[cd.n], T());}

        int base(int c, bool rel) const { return off[c] + (rel ? cd.size[c] + 1 : 0); }
        void add(int c, bool rel, int i, T x) {
            int b = base(c, rel) - 1, m = cd.size[c] + 1;
            for (i = min(i, m) + 1; i <= m; i += i & -i) { bit[b + i] += x; }}
        T prefix(int c, bool rel, int i) const {
            int b = base(c, rel) - 1; T res = T();
            for (i = min(i, cd.size[c] + 1); i > 0; i -= i & -i) { res += bit[b + i]; }
            return res;}
    };

    // T: O(n), M: O(n); real vertices [0, n) of a forest, child lists chained through virtual vertices by 0-weight edges; degrees <= 3.
    template<class G>
    Graph binarize(const G &g) {
        assert(!g.directed);
        int n = g.n, m = n, comps = 0;
        vector<char> seen(n);
        vector<int> que, kids;
        vector<GraphEdge> es;
        for (int s = 0; s < n; ++s) {
            if (seen[s]) { continue; }
            seen[s] = 1; que.assign(1, s); ++comps;
            for (int i = 0; i < int(que.size()); ++i) {
                int cur = que[i];
                kids.clear();
                for (int a : g[cur]) {
                    int v = g.arcs[a].to;
                    if (!seen[v]) { seen[v] = 1; que.push_back(v); kids.push_back(v); }}
                for (int j = 0; j < int(kids.size()); ++j) {
                    es.push_back({cur, kids[j], 1});
                    if (int(kids.size()) - j > 2) { es.push_back({cur, m, 0}); cur = m++; }}}}
        assert(g.arcs.size() == 2 * size_t(n - comps)); (void)comps;
        Graph res(m);
        for (auto [u, v, w] : es) { res.addEdge(u, v, w); }
        return res;}

    // S: O(n * log(n)), Q: O(log(n)), M: O(n * log(n)); binarized centroids; child comp c lists its real vertices at
    // [off[c], off[c + 1]) by distance dv to parent[c].
    struct Layout {
        int n;
        CentroidDecomposition<int> cd;
        vector<int> off, dv, at;

        template<class G>
        explicit Layout(const G &g) : n(g.n), cd(binarize(g), true), off(cd.n + 1), at(size_t(n) * cd.L) {
            vector<int> cnt;
            for (int c = 0; c < cd.n; ++c) {
                off[c + 1] = off[c];
                for (int i = cd.tin[c]; i < cd.tin[c] + cd.size[c] && cd.parent[c] != -1; ++i) { off[c + 1] += cd.order[i] < n; }}
            dv.resize(off[cd.n]);
            for (int c = 0; c < cd.n; ++c) {
                if (cd.parent[c] == -1) { continue; }
                int k = cd.level[c] - 1;
                cnt.assign(cd.size[c] + 2, 0);
                for (int i = cd.tin[c]; i < cd.tin[c] + cd.size[c]; ++i) {
                    if (cd.order[i] < n) { ++cnt[cd.distance(cd.order[i], k) + 1]; }}
                for (int d = 0; d <= cd.size[c]; ++d) { cnt[d + 1] += cnt[d]; }
                for (int i = cd.tin[c]; i < cd.tin[c] + cd.size[c]; ++i) {
                    int v = cd.order[i];
                    if (v < n) { int p = cnt[cd.distance(v, k)]++; at[v * cd.L + k] = p; dv[off[c] + p] = cd.distance(v, k); }}}}

        int len(int c) const { return off[c + 1] - off[c]; }
        // Local index range of comp c entries with distance in [l, r).
        pair<int, int> range(int c, int l, int r) const {
            auto b = dv.begin() + off[c], e = dv.begin() + off[c + 1];
            return {int(lower_bound(b, e, l) - b), int(lower_bound(b, e, max(l, r)) - b)};}
    };
} // namespace centroid_detail

// Edge-count distances, T a commutative group; contours are [l, r) with 0 <= l <= r.
// S: O(n * log(n)), U: O(log(n)^2), Q: O(log(n)^2), M: O(n * log(n)).
template<class T>
struct ContourSum : centroid_detail::Contour<T> {
    using B = centroid_detail::Contour<T>;
    using B::cd, B::bit, B::base, B::add, B::prefix;

    template<class G>
    ContourSum(const G &g, const vector<T> &a) : B(g) {
        assert(int(a.size()) == cd.n);
        for (int v = 0; v < cd.n; ++v) {
            for (int c = v, k = cd.level[v]; c != -1; c = cd.parent[c], --k) {
                bit[base(c, false) + cd.distance(v, k)] += a[v];
                if (k) { bit[base(c, true) + cd.distance(v, k - 1)] += a[v]; }}}
        for (int c = 0, m; c < cd.n; ++c) {
            m = cd.size[c] + 1;
            for (int b : {base(c, false) - 1, base(c, true) - 1}) {
                for (int i = 1; i <= m; ++i) {
                    if (i + (i & -i) <= m) { bit[b + i + (i & -i)] += bit[b + i]; }}}}}

    void pointAdd(int v, T x) {
        assert(0 <= v && v < cd.n);
        for (int c = v, k = cd.level[v]; c != -1; c = cd.parent[c], --k) {
            add(c, false, cd.distance(v, k), x);
            if (k) { add(c, true, cd.distance(v, k - 1), x); }}}

    T rangeContourSum(int v, int l, int r) const {
        assert(0 <= v && v < cd.n && 0 <= l && l <= r);
        T res = T();
        for (int c = v, k = cd.level[v], prev = -1; c != -1; prev = c, c = cd.parent[c], --k) {
            int d = cd.distance(v, k);
            res += prefix(c, false, r - d) - prefix(c, false, l - d);
            if (prev != -1) { res -= prefix(prev, true, r - d) - prefix(prev, true, l - d); }}
        return res;}
};

// Edge-count distances, T a commutative group; pointGet is a[v] plus every rangeContourAdd covering v.
// S: O(n * log(n)), U: O(log(n)^2), Q: O(log(n)^2), M: O(n * log(n)).
template<class T>
struct ContourAdd : centroid_detail::Contour<T> {
    using B = centroid_detail::Contour<T>;
    using B::cd, B::add, B::prefix;
    vector<T> a;

    template<class G>
    ContourAdd(const G &g, vector<T> init) : B(g), a(std::move(init)) {
        assert(int(a.size()) == cd.n);}

    void rangeContourAdd(int v, int l, int r, T x) {
        assert(0 <= v && v < cd.n && 0 <= l && l <= r);
        for (int c = v, k = cd.level[v], prev = -1; c != -1; prev = c, c = cd.parent[c], --k) {
            int d = cd.distance(v, k), lo = max(l - d, 0), hi = r - d;
            if (lo >= hi) { continue; }
            add(c, false, lo, x); add(c, false, hi, -x);
            if (prev != -1) { add(prev, true, lo, x); add(prev, true, hi, -x); }}}

    T pointGet(int v) const {
        assert(0 <= v && v < cd.n);
        T res = a[v];
        for (int c = v, k = cd.level[v], prev = -1; c != -1; prev = c, c = cd.parent[c], --k) {
            int d = cd.distance(v, k);
            res += prefix(c, false, d + 1);
            if (prev != -1) { res -= prefix(prev, true, d + 1); }}
        return res;}
};

// Edge-count distances, M a commutative monoid (S, op, e); contours are [l, r) with 0 <= l <= r.
// S: O(n * log(n)), U: O(log(n)^2), Q: O(log(n)^2), M: O(n * log(n)).
template<class M>
struct ContourFold : centroid_detail::Layout {
    using S = typename M::S;
    [[no_unique_address]] M m;
    vector<S> val, seg;

    template<class G>
    ContourFold(const G &g, const vector<S> &a, M mon = {}) : Layout(g), m(std::move(mon)), val(a), seg(2 * size_t(off.back()), m.e()) {
        assert(int(a.size()) == n);
        for (int v = 0; v < n; ++v) {
            for (int c = v; cd.parent[c] != -1; c = cd.parent[c]) { seg[2 * off[c] + len(c) + at[v * cd.L + cd.level[c] - 1]] = a[v]; }}
        for (int c = 0; c < cd.n; ++c) {
            for (int i = len(c) - 1, b = 2 * off[c]; i > 0; --i) { seg[b + i] = m.op(seg[b + 2 * i], seg[b + 2 * i + 1]); }}}

    void set(int v, S x) {
        assert(0 <= v && v < n);
        val[v] = x;
        for (int c = v; cd.parent[c] != -1; c = cd.parent[c]) {
            int b = 2 * off[c], i = len(c) + at[v * cd.L + cd.level[c] - 1];
            for (seg[b + i] = x; i > 1; i >>= 1) { seg[b + i / 2] = m.op(seg[b + (i & ~1)], seg[b + (i | 1)]); }}}

    S rangeContourFold(int v, int l, int r) const {
        assert(0 <= v && v < n && 0 <= l && l <= r);
        S res = m.e();
        for (int c = v, k = cd.level[v], prev = -1; c != -1; prev = c, c = cd.parent[c], --k) {
            int d = cd.distance(v, k);
            if (c < n && l <= d && d < r) { res = m.op(res, val[c]); }
            for (int p = cd.tin[c] + 1; p < cd.tin[c] + cd.size[c]; p += cd.size[cd.order[p]]) {
                int ch = cd.order[p], b = 2 * off[ch];
                if (ch == prev) { continue; }
                auto [lo, hi] = range(ch, l - d, r - d);
                for (lo += len(ch), hi += len(ch); lo < hi; lo >>= 1, hi >>= 1) {
                    if (lo & 1) { res = m.op(res, seg[b + lo++]); }
                    if (hi & 1) { res = m.op(res, seg[b + --hi]); }}}}
        return res;}
};

// Edge-count distances, A commutative actions (F, composition, id); pointGet composes every action covering v.
// S: O(n * log(n)), U: O(log(n)^2), Q: O(log(n)^2), M: O(n * log(n)).
template<class A>
struct ContourApply : centroid_detail::Layout {
    using F = typename A::F;
    [[no_unique_address]] A m;
    vector<F> self, tag;

    template<class G>
    explicit ContourApply(const G &g, A mon = {}) : Layout(g), m(std::move(mon)), self(n, m.id()), tag(2 * size_t(off.back()), m.id()) {}

    void rangeContourApply(int v, int l, int r, F f) {
        assert(0 <= v && v < n && 0 <= l && l <= r);
        for (int c = v, k = cd.level[v], prev = -1; c != -1; prev = c, c = cd.parent[c], --k) {
            int d = cd.distance(v, k);
            if (c < n && l <= d && d < r) { self[c] = m.composition(f, self[c]); }
            for (int p = cd.tin[c] + 1; p < cd.tin[c] + cd.size[c]; p += cd.size[cd.order[p]]) {
                int ch = cd.order[p], b = 2 * off[ch];
                if (ch == prev) { continue; }
                auto [lo, hi] = range(ch, l - d, r - d);
                for (lo += len(ch), hi += len(ch); lo < hi; lo >>= 1, hi >>= 1) {
                    if (lo & 1) { tag[b + lo] = m.composition(f, tag[b + lo]); ++lo; }
                    if (hi & 1) { --hi; tag[b + hi] = m.composition(f, tag[b + hi]); }}}}}

    F pointGet(int v) const {
        assert(0 <= v && v < n);
        F res = self[v];
        for (int c = v; cd.parent[c] != -1; c = cd.parent[c]) {
            int b = 2 * off[c];
            for (int i = len(c) + at[v * cd.L + cd.level[c] - 1]; i > 0; i >>= 1) { res = m.composition(tag[b + i], res); }}
        return res;}
};

// T: O(n * log(n)^2), M: O(n * log(n)); n < 2^24; res[d] = unordered vertex pairs d edges apart, d in [0, n), res[0] = 0.
template<class G>
vector<lng> treeDistanceFrequency(const G &g) {
    assert(g.n < 1 << 24);
    CentroidDecomposition<int> cd(g);
    vector<lng> res(cd.n), h, sq;
    // Distances of comp(c) to its level-k ancestor, squared: pairs through that ancestor, plus same-child pairs to cancel.
    auto square = [&](int c, int k, lng sign) {
        int b = cd.tin[c], e = b + cd.size[c], top = 0;
        for (int i = b; i < e; ++i) { top = max(top, cd.distance(cd.order[i], k)); }
        h.assign(top + 1, 0);
        for (int i = b; i < e; ++i) { ++h[cd.distance(cd.order[i], k)]; }
        if (top < 800) {
            sq.assign(2 * top + 1, 0);
            for (int i = 0; i <= top; ++i) {
                sq[2 * i] += h[i] * h[i];
                for (int j = i + 1; j <= top; ++j) { sq[i + j] += 2 * h[i] * h[j]; }}}
        else { sq = convolutionLong(h, h); }
        for (int d = 1; d < min(int(sq.size()), cd.n); ++d) { res[d] += sign * sq[d]; }};
    for (int c = 0; c < cd.n; ++c) {
        square(c, cd.level[c], 1);
        if (cd.parent[c] != -1) { square(c, cd.level[c] - 1, -1); }}
    for (auto &x : res) { x /= 2; }
    return res;}

// T: O(n * log(n)^2), M: O(n * log(n)); weighted, path sums in [-2^62, 2^62); unordered pairs u != v with dist <= k.
template<class G>
lng countPathsWithin(const G &g, lng k) {
    CentroidDecomposition<lng> cd(g, true);
    vector<lng> d;
    auto count = [&](int c, int lev) {
        d.clear();
        for (int i = cd.tin[c]; i < cd.tin[c] + cd.size[c]; ++i) { d.push_back(cd.distance(cd.order[i], lev)); }
        sort(d.begin(), d.end());
        lng res = 0;
        for (int i = 0, j = int(d.size()) - 1; i < j;) {
            if (d[i] + d[j] <= k) { res += j - i++; } else { --j; }}
        return res;};
    lng res = 0;
    for (int c = 0; c < cd.n; ++c) {
        res += count(c, cd.level[c]);
        if (cd.parent[c] != -1) { res -= count(c, cd.level[c] - 1); }}
    return res;}
