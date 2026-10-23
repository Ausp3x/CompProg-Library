#pragma once

#include "../01-Core/01-template.hpp"
#include "01-dsu.hpp"
#include "61-hashmap.hpp"
#include "../06-Miscellaneous/02-customhash.hpp"

// DSU whose components are also circular lists; members(u) starts at u and follows the list.
// S: O(n), U: O(alpha(n)) amortized (unionEnumerate O(a * b) more for component sizes a, b), Q: O(alpha(n)) amortized (members O(k) for k members), M: O(n)
struct LinkedDSU : DSU {
    vector<int> nxt;

    explicit LinkedDSU(int N = 0) : DSU(N), nxt(par) {}
    int makeSet() { nxt.push_back(n); return DSU::makeSet(); }

    bool uniteSets(int u, int v) {
        if (!DSU::uniteSets(u, v)) { return false; }
        swap(nxt[u], nxt[v]);
        return true;}
    template<typename F>
    bool unionEnumerate(int u, int v, F f) {
        if (isSameSet(u, v)) { return false; }
        vector<int> a = members(u), b = members(v);
        for (int x : a) {
            for (int y : b) { f(x, y); }}
        return uniteSets(u, v);}

    vector<int> members(int u) const {
        assert(0 <= u && u < n);
        vector<int> res{u};
        for (int x = nxt[u]; x != u; x = nxt[x]) { res.push_back(x); }
        return res;}
};

// M is a commutative monoid (S, op); componentValue(u) folds the initial values of u's component and every addValue on it.
// S: O(n), U: O(alpha(n)) amortized, Q: O(alpha(n)) amortized, M: O(n)
template<typename M>
struct ComponentSumDSU : DSU {
    using S = typename M::S;
    [[no_unique_address]] M m;
    vector<S> val;

    explicit ComponentSumDSU(const vector<S> &a, M mon = {}) : DSU(int(a.size())), m(std::move(mon)), val(a) { assert(a.size() <= size_t(INT_MAX)); }
    int makeSet(const S &x) { val.push_back(x); return DSU::makeSet(); }

    bool uniteSets(int u, int v) {
        u = findSet(u); v = findSet(v);
        if (!DSU::uniteSets(u, v)) { return false; }
        if (par[u] != u) { swap(u, v); }
        val[u] = m.op(val[u], val[v]);
        return true;}
    void addValue(int u, const S &x) { u = findSet(u); val[u] = m.op(val[u], x); }

    typename vector<S>::const_reference componentValue(int u) { return val[findSet(u)]; }
};

// Values in a group G (S, op, e, inv), not necessarily commutative; apply(u, x) replaces each value y in u's component by op(y, x).
// S: O(n), U: O(alpha(n)) amortized, Q: O(alpha(n)) amortized, M: O(n)
template<typename G>
struct DualDSU {
    using S = typename G::S;
    int n;
    [[no_unique_address]] G m;
    vector<int> par, siz;
    vector<S> tag;

    explicit DualDSU(const vector<S> &a, G mon = {}) : n(int(a.size())), m(std::move(mon)), tag(a) {
        assert(a.size() <= size_t(INT_MAX));
        par.resize(n); iota(par.begin(), par.end(), 0);
        siz.assign(n, 1);}

    bool uniteSets(int u, int v) {
        u = findSet(u); v = findSet(v);
        if (u == v) { return false; }
        if (siz[u] < siz[v]) { swap(u, v); }
        // A value is the product of the tags from its vertex up to the root, so v cancels u's tag.
        par[v] = u; siz[u] += siz[v]; tag[v] = m.op(tag[v], m.inv(tag[u]));
        return true;}
    void apply(int u, const S &x) { u = findSet(u); tag[u] = m.op(tag[u], x); }

    int findSet(int u) { assert(0 <= u && u < n); return root(u); }
    int root(int u) {
        int p = par[u];
        if (p == u) { return u; }
        int r = root(p);
        if (p != r) { tag[u] = m.op(tag[u], tag[p]); par[u] = r; }
        return r;}
    bool isSameSet(int u, int v) { return findSet(u) == findSet(v); }
    S get(int u) {
        int r = findSet(u);
        return r == u ? tag[u] : m.op(tag[u], tag[r]);}
};

// Keys of a hashable K (H as in HashMap); a key never passed to uniteSets is a singleton and allocates nothing.
// S: O(1), U: O(alpha(k)) amortized plus O(1) expected hashing, Q: as U, M: O(k) for k distinct united keys
template<typename K, typename H = CustomHash>
struct SparseDSU {
    DSU d;
    HashMap<K, int, H> id;
    vector<K> key;

    int index(const K &k) {
        int i = id.get(k, -1);
        if (i < 0) { i = d.makeSet(); id.insert(k, i); key.push_back(k); }
        return i;}

    bool uniteSets(const K &a, const K &b) { return d.uniteSets(index(a), index(b)); }

    K findSet(const K &k) {
        int i = id.get(k, -1);
        return i < 0 ? k : key[d.findSet(i)];}
    bool isSameSet(const K &a, const K &b) { return findSet(a) == findSet(b); }
    int getSize(const K &k) {
        int i = id.get(k, -1);
        return i < 0 ? 1 : d.getSize(i);}
};

// Leaves [0, n), n <= 2^30; the k-th merging union, union i of the sequence, makes node n + k with time i + 1; -1 means never.
// S: O(n + q * alpha(n)), Q: O(log(n)) by heavy paths (child[x][0] has at least as many leaves), M: O(n)
struct MergeHistoryForest {
    int n;
    vector<int> parent, tim, cnt, depth, head, pos, order, start, leaves;
    vector<array<int, 2>> child;

    MergeHistoryForest(int N, const vector<pair<int, int>> &unions) : n(N) {
        assert(0 <= n && n <= (1 << 30));
        parent.assign(n, -1); tim.assign(n, 0); cnt.assign(n, 1); child.assign(n, {-1, -1});
        DSU d(n);
        vector<int> top(n);
        iota(top.begin(), top.end(), 0);
        for (int i = 0; i < int(unions.size()); ++i) {
            int u = d.findSet(unions[i].first), v = d.findSet(unions[i].second);
            if (u == v) { continue; }
            int x = int(parent.size()), a = top[u], b = top[v];
            if (cnt[a] < cnt[b]) { swap(a, b); }
            parent[a] = parent[b] = x;
            parent.push_back(-1); tim.push_back(i + 1); cnt.push_back(cnt[a] + cnt[b]); child.push_back({a, b});
            d.uniteSets(u, v); top[d.findSet(u)] = x;}
        int m = int(parent.size());
        vector<int> len(m, 1);
        for (int x = n; x < m; ++x) { len[x] += len[child[x][0]]; }
        depth.resize(m); head.resize(m); pos.resize(m); order.resize(m); start.resize(m); leaves.resize(n);
        for (int x = m - 1, nxt = 0, slot = 0; x >= 0; --x) {
            int p = parent[x];
            bool heavy = p >= 0 && child[p][0] == x;
            depth[x] = p < 0 ? 0 : depth[p] + 1;
            head[x] = heavy ? head[p] : x;
            pos[x] = heavy ? pos[p] + 1 : std::exchange(slot, slot + len[x]);
            order[pos[x]] = x;
            if (p < 0) { start[x] = nxt; nxt += cnt[x]; }
            else { start[x] = start[p] + (heavy ? 0 : cnt[child[p][0]]); }
            if (x < n) { leaves[start[x]] = x; }}}

    int nodeTime(int x) const { assert(0 <= x && x < int(tim.size())); return tim[x]; }
    int mergeTime(int x) const { assert(0 <= x && x < int(tim.size())); return parent[x] < 0 ? -1 : tim[parent[x]]; }
    int earliestConnectTime(int u, int v) const {
        assert(0 <= u && u < n && 0 <= v && v < n);
        while (head[u] != head[v]) {
            if (depth[head[u]] < depth[head[v]]) { swap(u, v); }
            u = parent[head[u]];
            if (u < 0) { return -1; }}
        return tim[depth[u] < depth[v] ? u : v];}
    int componentAt(int u, int t) const {
        assert(0 <= u && u < n && t >= 0);
        for (int h = head[u]; tim[h] <= t; h = head[u]) {
            if (parent[h] < 0 || tim[parent[h]] > t) { return h; }
            u = parent[h];}
        // Times fall down a heavy path: the first position from its top with time <= t.
        int lo = pos[head[u]], hi = pos[u];
        while (hi - lo > 1) {
            int mid = std::midpoint(lo, hi);
            if (tim[order[mid]] <= t) { hi = mid; }
            else { lo = mid; }}
        return order[hi];}
    pair<int, int> leafRange(int x) const { assert(0 <= x && x < int(tim.size())); return {start[x], start[x] + cnt[x]}; }
};
