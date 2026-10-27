#pragma once

#include "../01-Core/01-template.hpp"

namespace persistentsegmenttree_detail {
    struct NoAction {};
    template<typename A> struct Action { using type = NoAction; };
    template<typename A> requires requires { typename A::F; } struct Action<A> { using type = typename A::F; };
} // namespace persistentsegmenttree_detail

// Positions in [0, n) for any n >= 0; A is a monoid or an acted monoid; versions are dense ids from 0 and untouched positions hold e(); COMMUTATIVE keeps tags in place on apply and needs commuting actions.
// S: O(1) empty, O(log(n)^2) filled, O(n) from a vector; U: O(log(n)), merge O(k) for k positions of tree nodes present in both versions; Q: O(log(n)); M: O(n + q * log(n) + K) for q updates and merges of total cost K
template<typename A, bool COMMUTATIVE = false>
struct PersistentSegmentTree {
    using S = typename A::S;
    using F = typename persistentsegmenttree_detail::Action<A>::type;
    static constexpr bool LAZY = !std::is_empty_v<F>;
    struct Node { S x; [[no_unique_address]] F f; int l, r; };
    [[no_unique_address]] A m;
    lng n;
    vector<Node> t;
    vector<int> roots;

    explicit PersistentSegmentTree(lng N = 0, A mon = {}) : m(std::move(mon)), n(max(N, lng(0))), t{{m.e(), id(), 0, 0}}, roots{0} { assert(N >= 0); }
    // Subtrees of equal length are equal, so each of the O(log(n)) lengths is built once.
    PersistentSegmentTree(lng N, const S &x, A mon = {}) : PersistentSegmentTree(N, std::move(mon)) {
        vector<pair<lng, int>> memo;
        auto build = [&](auto &&build, lng len) -> int {
            for (auto [sz, k] : memo) { if (sz == len) { return k; }}
            int k = len == 1 ? make(x, id(), 0, 0) : node(id(), build(build, len / 2), build(build, len - len / 2));
            memo.push_back({len, k});
            return k;};
        if (n) { roots[0] = build(build, n); }}
    explicit PersistentSegmentTree(const vector<S> &v, A mon = {}) : PersistentSegmentTree(lng(v.size()), std::move(mon)) {
        t.reserve(2 * v.size());
        auto build = [&](auto &&build, lng a, lng b) -> int {
            if (b - a == 1) { return make(v[a], id(), 0, 0); }
            lng mid = std::midpoint(a, b);
            int lc = build(build, a, mid);
            return node(id(), lc, build(build, mid, b));};
        if (n) { roots[0] = build(build, 0, n); }}
    void reserve(int nodes) { t.reserve(size_t(nodes) + 1); }
    S map(const F &f, const S &x) const { if constexpr (LAZY) { return m.mapping(f, x); } return x; }
    F comp(const F &f, const F &g) const { if constexpr (LAZY) { return m.composition(f, g); } return f; }
    F id() const { if constexpr (LAZY) { return m.id(); } return {}; }
    int make(const S &x, const F &f, int l, int r) { t.push_back({x, f, l, r}); return int(t.size()) - 1; }
    int node(const F &f, int l, int r) { return l || r ? make(map(f, m.op(t[l].x, t[r].x)), f, l, r) : 0; }
    // Copy of k with the pending action f applied; nodes are immutable once made.
    int tagged(int k, const F &f) {
        if constexpr (LAZY) {
            if constexpr (std::equality_comparable<F>) { if (f == m.id()) { return k; }}
            if (k) { return make(m.mapping(f, t[k].x), m.composition(f, t[k].f), t[k].l, t[k].r); }}
        return k;}
    int commit(int root) { roots.push_back(root); return int(roots.size()) - 1; }

    int nodeCount() const { return int(t.size()) - 1; }
    int versionCount() const { return int(roots.size()); }
    S get(int v, lng i) const {
        assert(0 <= v && v < versionCount() && 0 <= i && i < n);
        F g = id();
        int k = roots[v];
        for (lng a = 0, b = n; k && b - a > 1;) {
            g = comp(g, t[k].f);
            lng mid = std::midpoint(a, b);
            if (i < mid) { k = t[k].l; b = mid; }
            else { k = t[k].r; a = mid; }}
        return map(g, t[k].x);}
    S allProd(int v) const { assert(0 <= v && v < versionCount()); return t[roots[v]].x; }
    // T: O(n), M: O(n)
    vector<S> toVector(int v) const {
        assert(0 <= v && v < versionCount());
        vector<S> res(n, m.e());
        auto dfs = [&](auto &&dfs, int k, const F &g, lng a, lng b) -> void {
            if (!k) { return; }
            if (b - a == 1) { res[a] = map(g, t[k].x); return; }
            F h = comp(g, t[k].f);
            lng mid = std::midpoint(a, b);
            dfs(dfs, t[k].l, h, a, mid); dfs(dfs, t[k].r, h, mid, b);};
        dfs(dfs, roots[v], id(), 0, n);
        return res;}

    int set(int v, lng i, const S &x) { assert(0 <= v && v < versionCount() && 0 <= i && i < n); return commit(point(roots[v], id(), 0, n, i, x)); }
    int point(int k, const F &g, lng a, lng b, lng i, const S &x) {
        if (b - a <= 1) { return make(x, id(), 0, 0); }
        F h = comp(g, t[k].f);
        lng mid = std::midpoint(a, b);
        int lc = t[k].l, rc = t[k].r;
        if (i < mid) { lc = point(lc, h, a, mid, i, x); rc = tagged(rc, h); }
        else { lc = tagged(lc, h); rc = point(rc, h, mid, b, i, x); }
        return node(id(), lc, rc);}
    int apply(int v, lng l, lng r, const F &f) {
        static_assert(LAZY);
        assert(0 <= v && v < versionCount() && 0 <= l && l <= r && r <= n);
        return commit(l < r ? act(roots[v], id(), 0, n, l, r, f) : roots[v]);}
    int act(int k, const F &g, lng a, lng b, lng l, lng r, const F &f) {
        if (r <= a || b <= l) { return COMMUTATIVE ? k : tagged(k, g); }
        if (l <= a && b <= r) { return tagged(k, comp(f, g)); }
        F h = COMMUTATIVE ? g : comp(g, t[k].f);
        lng mid = std::midpoint(a, b);
        int lc = act(t[k].l, h, a, mid, l, r, f), rc = act(t[k].r, h, mid, b, l, r, f);
        return node(COMMUTATIVE ? t[k].f : id(), lc, rc);}
    int copyRange(int v, int w, lng l, lng r) {
        assert(0 <= v && v < versionCount() && 0 <= w && w < versionCount() && 0 <= l && l <= r && r <= n);
        return commit(l < r ? copy(roots[v], id(), roots[w], id(), 0, n, l, r) : roots[v]);}
    int copy(int k, const F &g, int j, const F &h, lng a, lng b, lng l, lng r) {
        if (r <= a || b <= l) { return tagged(k, g); }
        if (l <= a && b <= r) { return tagged(j, h); }
        F g2 = comp(g, t[k].f), h2 = comp(h, t[j].f);
        lng mid = std::midpoint(a, b);
        int lc = copy(t[k].l, g2, t[j].l, h2, a, mid, l, r), rc = copy(t[k].r, g2, t[j].r, h2, mid, b, l, r);
        return node(id(), lc, rc);}
    pair<int, int> split(int v, lng i) {
        assert(0 <= v && v < versionCount() && 0 <= i && i <= n);
        int lo = copy(roots[v], id(), 0, id(), 0, n, i, n), hi = copy(roots[v], id(), 0, id(), 0, n, 0, i);
        return {commit(lo), commit(hi)};}
    int merge(int v, int w) { assert(0 <= v && v < versionCount() && 0 <= w && w < versionCount()); return commit(meld(roots[v], id(), roots[w], id(), n)); }
    int meld(int k, const F &g, int j, const F &h, lng len) {
        if (!k || !j) { return k ? tagged(k, g) : tagged(j, h); }
        if (len <= 1) { return make(m.op(map(g, t[k].x), map(h, t[j].x)), id(), 0, 0); }
        F g2 = comp(g, t[k].f), h2 = comp(h, t[j].f);
        int lc = meld(t[k].l, g2, t[j].l, h2, len / 2), rc = meld(t[k].r, g2, t[j].r, h2, len - len / 2);
        return node(id(), lc, rc);}

    S prod(int v, lng l, lng r) const { assert(0 <= v && v < versionCount() && 0 <= l && l <= r && r <= n); return fold(roots[v], 0, n, l, r); }
    S fold(int k, lng a, lng b, lng l, lng r) const {
        if (!k || r <= a || b <= l) { return m.e(); }
        if (l <= a && b <= r) { return t[k].x; }
        lng mid = std::midpoint(a, b);
        return map(t[k].f, m.op(fold(t[k].l, a, mid, l, r), fold(t[k].r, mid, b, l, r)));}
    template<typename G>
    lng maxRight(int v, lng l, G pred) const {
        assert(0 <= v && v < versionCount() && 0 <= l && l <= n && pred(m.e()));
        S acc = m.e();
        lng res = n;
        descendRight(roots[v], id(), 0, n, l, pred, acc, res);
        return res;}
    template<typename G>
    bool descendRight(int k, const F &g, lng a, lng b, lng l, G &pred, S &acc, lng &res) const {
        if (!k || b <= l) { return false; }
        if (l <= a) {
            S next = m.op(acc, map(g, t[k].x));
            if (pred(next)) { acc = std::move(next); return false; }
            if (b - a == 1) { res = a; return true; }}
        F h = comp(g, t[k].f);
        lng mid = std::midpoint(a, b);
        return descendRight(t[k].l, h, a, mid, l, pred, acc, res) || descendRight(t[k].r, h, mid, b, l, pred, acc, res);}
    template<typename G>
    lng minLeft(int v, lng r, G pred) const {
        assert(0 <= v && v < versionCount() && 0 <= r && r <= n && pred(m.e()));
        S acc = m.e();
        lng res = 0;
        descendLeft(roots[v], id(), 0, n, r, pred, acc, res);
        return res;}
    template<typename G>
    bool descendLeft(int k, const F &g, lng a, lng b, lng r, G &pred, S &acc, lng &res) const {
        if (!k || r <= a) { return false; }
        if (b <= r) {
            S next = m.op(map(g, t[k].x), acc);
            if (pred(next)) { acc = std::move(next); return false; }
            if (b - a == 1) { res = b; return true; }}
        F h = comp(g, t[k].f);
        lng mid = std::midpoint(a, b);
        return descendLeft(t[k].r, h, mid, b, r, pred, acc, res) || descendLeft(t[k].l, h, a, mid, r, pred, acc, res);}
};

// Positions in [0, n) for any n >= 0; A supplies F, composition (new after old) and id; get returns the composed action at i; COMMUTATIVE keeps tags in place and needs commuting actions.
// S: O(1), U: O(log(n)), Q: O(log(n)), M: O(q * log(n)) for q applications
template<typename A, bool COMMUTATIVE = false>
struct PersistentDualSegmentTree {
    using F = typename A::F;
    struct Node { F f; int l, r; };
    [[no_unique_address]] A m;
    lng n;
    vector<Node> t;
    vector<int> roots;

    explicit PersistentDualSegmentTree(lng N = 0, A mon = {}) : m(std::move(mon)), n(max(N, lng(0))), t{{m.id(), 0, 0}}, roots{0} { assert(N >= 0); }
    void reserve(int nodes) { t.reserve(size_t(nodes) + 1); }
    int make(const F &f, int l, int r) { t.push_back({f, l, r}); return int(t.size()) - 1; }
    int tagged(int k, const F &f) {
        if constexpr (std::equality_comparable<F>) { if (f == m.id()) { return k; }}
        return make(m.composition(f, t[k].f), t[k].l, t[k].r);}

    int nodeCount() const { return int(t.size()) - 1; }
    int versionCount() const { return int(roots.size()); }
    F get(int v, lng i) const {
        assert(0 <= v && v < versionCount() && 0 <= i && i < n);
        F res = m.id();
        int k = roots[v];
        for (lng a = 0, b = n; k;) {
            res = m.composition(res, t[k].f);
            lng mid = std::midpoint(a, b);
            if (i < mid) { k = t[k].l; b = mid; }
            else { k = t[k].r; a = mid; }}
        return res;}

    int apply(int v, lng l, lng r, const F &f) {
        assert(0 <= v && v < versionCount() && 0 <= l && l <= r && r <= n);
        roots.push_back(l < r ? act(roots[v], m.id(), 0, n, l, r, f) : roots[v]);
        return versionCount() - 1;}
    int act(int k, const F &g, lng a, lng b, lng l, lng r, const F &f) {
        if (r <= a || b <= l) { return COMMUTATIVE ? k : tagged(k, g); }
        if (l <= a && b <= r) { return tagged(k, m.composition(f, g)); }
        F h = COMMUTATIVE ? g : m.composition(g, t[k].f);
        lng mid = std::midpoint(a, b);
        int lc = act(t[k].l, h, a, mid, l, r, f), rc = act(t[k].r, h, mid, b, l, r, f);
        return make(COMMUTATIVE ? t[k].f : m.id(), lc, rc);}
};

// 0 <= n < 2^31; T has operator< (a strict weak order); W is an additive group with W() as zero; k-th counts from 0 in (value, index) order; maxLeq and minGeq return false when no value qualifies.
// S: O(n * log(n)), U: NA, Q: O(log(n)), M: O(n * log(n))
template<typename T, typename W = lng>
struct PersistentValueTree {
    struct Node { int c, l, r; };
    int n;
    vector<T> vals;
    vector<Node> t;
    vector<W> s;
    vector<int> roots;

    // Leaf p is the p-th element in (value, index) order; version i holds a[0, i).
    explicit PersistentValueTree(const vector<T> &a = {}, const vector<W> &w = {}) : n(int(a.size())), t{{0, 0, 0}}, s{W()}, roots{0} {
        assert(a.size() <= size_t(INT_MAX) && (w.empty() || w.size() == a.size()));
        vector<int> ord(n), pos(n);
        iota(ord.begin(), ord.end(), 0);
        std::stable_sort(ord.begin(), ord.end(), [&](int i, int j) { return a[i] < a[j]; });
        vals.reserve(n);
        for (int p = 0; p < n; ++p) { vals.push_back(a[ord[p]]); pos[ord[p]] = p; }
        for (int i = 0; i < n; ++i) { roots.push_back(insert(roots[i], 0, n, pos[i], w.empty() ? nullptr : &w[i])); }}
    int insert(int k, int a, int b, int p, const W *w) {
        int l = t[k].l, r = t[k].r, mid = std::midpoint(a, b);
        if (b - a > 1 && p < mid) { l = insert(l, a, mid, p, w); }
        else if (b - a > 1) { r = insert(r, mid, b, p, w); }
        t.push_back({t[k].c + 1, l, r});
        if (w) { s.push_back(s[k] + *w); }
        return int(t.size()) - 1;}
    int lowerRank(const T &x) const { return int(lower_bound(vals.begin(), vals.end(), x) - vals.begin()); }
    int upperRank(const T &x) const { return int(upper_bound(vals.begin(), vals.end(), x) - vals.begin()); }
    // Elements of a[l, r) whose leaf is below p.
    int below(int l, int r, int p) const {
        int res = 0;
        for (int kl = roots[l], kr = roots[r], a = 0, b = n; kr && a < p;) {
            if (b <= p) { return res + t[kr].c - t[kl].c; }
            int mid = std::midpoint(a, b);
            if (p <= mid) { kl = t[kl].l; kr = t[kr].l; b = mid; }
            else { res += t[t[kr].l].c - t[t[kl].l].c; kl = t[kl].r; kr = t[kr].r; a = mid; }}
        return res;}
    // Leaf of the k-th element of a[l, r); adds the weights before it to *sum.
    int leaf(int l, int r, int k, W *sum = nullptr) const {
        int kl = roots[l], kr = roots[r], a = 0, b = n;
        while (b - a > 1) {
            int mid = std::midpoint(a, b), c = t[t[kr].l].c - t[t[kl].l].c;
            if (k < c) { kl = t[kl].l; kr = t[kr].l; b = mid; continue; }
            if (sum) { *sum += s[t[kr].l] - s[t[kl].l]; }
            k -= c; kl = t[kl].r; kr = t[kr].r; a = mid;}
        return a;}

    int countEqual(int l, int r, const T &x) const { assert(0 <= l && l <= r && r <= n); return below(l, r, upperRank(x)) - below(l, r, lowerRank(x)); }
    int countRange(int l, int r, const T &lo, const T &hi) const { assert(0 <= l && l <= r && r <= n); return lo < hi ? below(l, r, lowerRank(hi)) - below(l, r, lowerRank(lo)) : 0; }
    T rangeKth(int l, int r, int k) const { assert(0 <= l && l <= r && r <= n && 0 <= k && k < r - l); return vals[leaf(l, r, k)]; }
    W kthSum(int l, int r, int k) const {
        assert(0 <= l && l <= r && r <= n && 0 <= k && k <= r - l && s.size() == t.size());
        if (k == r - l) { return s[roots[r]] - s[roots[l]]; }
        W res = W();
        leaf(l, r, k, &res);
        return res;}
    bool maxLeq(int l, int r, const T &x, T &res) const {
        assert(0 <= l && l <= r && r <= n);
        int c = below(l, r, upperRank(x));
        if (!c) { return false; }
        res = vals[leaf(l, r, c - 1)];
        return true;}
    bool minGeq(int l, int r, const T &x, T &res) const {
        assert(0 <= l && l <= r && r <= n);
        int c = below(l, r, lowerRank(x));
        if (c == r - l) { return false; }
        res = vals[leaf(l, r, c)];
        return true;}
};

// 0 <= n < 2^31; T has operator< (a strict weak order whose equivalence is equality).
// S: O(n * log(n)), U: NA, Q: O(log(n)), M: O(n * log(n))
template<typename T>
struct PersistentDistinctCount {
    PersistentValueTree<int, int> tree;

    explicit PersistentDistinctCount(const vector<T> &a = {}) : tree(previous(a)) {}
    static vector<int> previous(const vector<T> &a) {
        assert(a.size() <= size_t(INT_MAX));
        vector<int> ord(a.size()), res(a.size(), -1);
        iota(ord.begin(), ord.end(), 0);
        std::stable_sort(ord.begin(), ord.end(), [&](int i, int j) { return a[i] < a[j]; });
        for (int p = 1; p < int(ord.size()); ++p) {
            if (!(a[ord[p - 1]] < a[ord[p]])) { res[ord[p]] = ord[p - 1]; }}
        return res;}

    // Positions j in [l, r) with no equal value in [l, j).
    int countDistinct(int l, int r) const { assert(0 <= l && l <= r && r <= tree.n); return tree.countRange(l, r, -1, l); }
};
