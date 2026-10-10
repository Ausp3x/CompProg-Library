#pragma once

#include "../01-Core/01-template.hpp"

// T is totally ordered by <; 0 <= n <= 2^29; value ranges are [lo, hi); kth is 0-based; maxLeq/minGeq return false when nothing qualifies.
// S: O(n * log(n)), U: NA, Q: O(log(n)) and O(log(n)^2) for kth, M: O(n * log(n))
template<typename T>
struct MergeSortTree {
    int n;
    vector<T> vals;
    vector<vector<int>> rk, lc;

    explicit MergeSortTree(const vector<T> &a = {}) : n(int(a.size())) {
        assert(a.size() <= (1u << 29));
        vector<int> ord(n), rank(n);
        std::iota(ord.begin(), ord.end(), 0);
        std::stable_sort(ord.begin(), ord.end(), [&](int i, int j) { return a[i] < a[j]; });
        for (int q = 0; q < n; ++q) { vals.push_back(a[ord[q]]); rank[ord[q]] = q; }
        int h = n ? int(std::bit_width(uint(n - 1))) + 1 : 0;
        rk.assign(h, vector<int>(n)); lc.assign(h, vector<int>(n));
        if (n) { build(0, 0, n, rank); }}
    // Level d keeps each node's ranks sorted in place; lc[d][a + p] counts left-child ranks among the node's first p.
    void build(int d, int a, int b, const vector<int> &rank) {
        if (b - a == 1) { rk[d][a] = rank[a]; return; }
        int m = std::midpoint(a, b);
        build(d + 1, a, m, rank); build(d + 1, m, b, rank);
        const vector<int> &x = rk[d + 1];
        for (int i = a, j = m, p = a; p < b; ++p) {
            lc[d][p] = i - a;
            rk[d][p] = j == b || (i < m && x[i] < x[j]) ? x[i++] : x[j++];}}

    // Calls g(d, a, b, p) on the canonical nodes of [l, r), where p counts the node's ranks below the root bound.
    template<typename G>
    void visit(int d, int a, int b, int l, int r, int p, const G &g) const {
        if (r <= a || b <= l) { return; }
        if (l <= a && b <= r) { g(d, a, b, p); return; }
        int m = std::midpoint(a, b), pl = p == b - a ? m - a : lc[d][a + p];
        visit(d + 1, a, m, l, r, pl, g); visit(d + 1, m, b, l, r, p - pl, g);}
    int countRank(int l, int r, int p) const { int res = 0; visit(0, 0, n, l, r, p, [&](int, int, int, int c) { res += c; }); return res; }
    int lowerRank(const T &x) const { return int(lower_bound(vals.begin(), vals.end(), x) - vals.begin()); }
    int upperRank(const T &x) const { return int(upper_bound(vals.begin(), vals.end(), x) - vals.begin()); }

    int countLess(int l, int r, const T &x) const { assert(0 <= l && l <= r && r <= n); return countRank(l, r, lowerRank(x)); }
    int countRange(int l, int r, const T &lo, const T &hi) const { return lo < hi ? countLess(l, r, hi) - countLess(l, r, lo) : 0; }
    int countEqual(int l, int r, const T &x) const { assert(0 <= l && l <= r && r <= n); return countRank(l, r, upperRank(x)) - countRank(l, r, lowerRank(x)); }
    const T &kth(int l, int r, int k) const {
        assert(0 <= l && l <= r && r <= n && 0 <= k && k < r - l);
        int lo = 0, hi = n;
        while (hi - lo > 1) {
            int mid = std::midpoint(lo, hi);
            (countRank(l, r, mid) <= k ? lo : hi) = mid;}
        return vals[lo];}
    bool maxLeq(int l, int r, const T &x, T &res) const {
        assert(0 <= l && l <= r && r <= n);
        int best = -1;
        visit(0, 0, n, l, r, upperRank(x), [&](int d, int a, int, int p) { if (p) { best = max(best, rk[d][a + p - 1]); }});
        if (best >= 0) { res = vals[best]; }
        return best >= 0;}
    bool minGeq(int l, int r, const T &x, T &res) const {
        assert(0 <= l && l <= r && r <= n);
        int best = n;
        visit(0, 0, n, l, r, lowerRank(x), [&](int d, int a, int b, int p) { if (p < b - a) { best = min(best, rk[d][a + p]); }});
        if (best < n) { res = vals[best]; }
        return best < n;}
};

// T is totally ordered by <; slots [0, n) hold multisets and set needs a slot with exactly one value; kth is 0-based; maxLeq/minGeq return false when nothing qualifies.
// S: O(n * log(n)^2), U: O(log(n) * log(m)), Q: O(log(n) * log(m)) and O(log(n) * log(m)^2) for kth, M: O(n + m * log(n)) for m stored values
template<typename T>
struct DynamicMergeSortTree {
    using Key = pair<T, int>;
    static constexpr int LO = std::numeric_limits<int>::min(), HI = std::numeric_limits<int>::max();
    int n, uid = 0;
    vector<indexed_set<Key>> d;
    indexed_set<Key> all;

    explicit DynamicMergeSortTree(int N = 0) : n(N), d(2 * size_t(max(N, 0))) { assert(N >= 0); }
    explicit DynamicMergeSortTree(const vector<T> &a) : DynamicMergeSortTree(int(a.size())) {
        for (int i = 0; i < n; ++i) { insert(i, a[i]); }}

    template<typename G>
    void visit(int l, int r, const G &g) const {
        assert(0 <= l && l <= r && r <= n);
        for (l += n, r += n; l < r; l >>= 1, r >>= 1) {
            if (l & 1) { g(d[l++]); }
            if (r & 1) { g(d[--r]); }}}
    int size(int l, int r) const { int res = 0; visit(l, r, [&](auto &s) { res += int(s.size()); }); return res; }

    void insert(int i, const T &x) {
        assert(0 <= i && i < n);
        Key key(x, uid++);
        for (int v = i + n; v; v >>= 1) { d[v].insert(key); }
        all.insert(key);}
    bool erase(int i, const T &x) {
        assert(0 <= i && i < n);
        auto it = d[i + n].lower_bound(Key(x, LO));
        if (it == d[i + n].end() || x < it->first) { return false; }
        Key key = *it;
        for (int v = i + n; v; v >>= 1) { d[v].erase(key); }
        all.erase(key);
        return true;}
    void set(int i, const T &x) {
        assert(0 <= i && i < n && d[i + n].size() == 1);
        if (!d[i + n].empty()) { T old = d[i + n].begin()->first; erase(i, old); }
        insert(i, x);}

    int countLess(int l, int r, const T &x) const { int res = 0; visit(l, r, [&](auto &s) { res += int(s.order_of_key(Key(x, LO))); }); return res; }
    int countRange(int l, int r, const T &lo, const T &hi) const { return lo < hi ? countLess(l, r, hi) - countLess(l, r, lo) : 0; }
    int countEqual(int l, int r, const T &x) const {
        int res = 0;
        visit(l, r, [&](auto &s) { res += int(s.order_of_key(Key(x, HI)) - s.order_of_key(Key(x, LO))); });
        return res;}
    T kth(int l, int r, int k) const {
        assert(0 <= k && k < size(l, r));
        int lo = 0, hi = int(all.size());
        while (hi - lo > 1) {
            int mid = std::midpoint(lo, hi), c = 0;
            Key key = *all.find_by_order(mid);
            visit(l, r, [&](auto &s) { c += int(s.order_of_key(key)); });
            (c <= k ? lo : hi) = mid;}
        return all.find_by_order(lo)->first;}
    bool maxLeq(int l, int r, const T &x, T &res) const {
        bool found = false;
        visit(l, r, [&](auto &s) {
            auto it = s.upper_bound(Key(x, HI));
            if (it != s.begin() && (!found || res < prev(it)->first)) { res = prev(it)->first; found = true; }});
        return found;}
    bool minGeq(int l, int r, const T &x, T &res) const {
        bool found = false;
        visit(l, r, [&](auto &s) {
            auto it = s.lower_bound(Key(x, LO));
            if (it != s.end() && (!found || it->first < res)) { res = it->first; found = true; }});
        return found;}
};

// T is totally ordered by <; 0 <= n <= 2^29; count returns the occurrences of x in [l, r).
// S: O(n * log(n)), U: O(log(n)), Q: O(log(n)), M: O(n)
template<typename T>
struct PointSetRangeFrequency {
    vector<T> a;
    map<T, indexed_set<int>> pos;

    explicit PointSetRangeFrequency(const vector<T> &v = {}) : a(v) {
        assert(a.size() <= (1u << 29));
        for (int i = 0; i < int(a.size()); ++i) { pos[a[i]].insert(i); }}

    const T &get(int i) const { assert(0 <= i && i < int(a.size())); return a[i]; }

    void set(int i, const T &x) {
        assert(0 <= i && i < int(a.size()));
        auto it = pos.find(a[i]);
        it->second.erase(i);
        if (it->second.empty()) { pos.erase(it); }
        pos[x].insert(i);
        a[i] = x;}

    int count(int l, int r, const T &x) const {
        assert(0 <= l && l <= r && r <= int(a.size()));
        auto it = pos.find(x);
        return it == pos.end() ? 0 : int(it->second.order_of_key(r) - it->second.order_of_key(l));}
};
