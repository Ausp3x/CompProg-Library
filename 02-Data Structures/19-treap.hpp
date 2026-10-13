#pragma once

#include "../01-Core/01-template.hpp"
#include "../06-Miscellaneous/02-customhash.hpp"

// T is totally ordered by < and default-constructible; trees are root ids in one pool (0 is empty) holding multisets; split(v, x) gives keys < x and >= x; find/lowerBound/upperBound return 0 when absent.
// S: O(1), U: O(log(n)) expected, Q: O(log(n)) expected, M: O(m) for m created nodes
template<typename T>
struct Treap {
    struct Node { T key; uint pri; int sz, l, r; };
    vector<Node> t;
    ulng seed;

    explicit Treap(ulng seed = CustomHash::rnd) : t(1, Node{T(), 0, 0, 0, 0}), seed(seed) {}
    int node(const T &x) { t.push_back({x, uint(CustomHash::splitMix64(seed++) >> 32), 1, 0, 0}); return int(t.size()) - 1; }
    // T: O(n), M: O(n).
    int build(const vector<T> &a) {
        assert(std::is_sorted(a.begin(), a.end()));
        vector<int> st;
        for (const T &x : a) {
            int u = node(x), last = 0;
            while (!st.empty() && t[st.back()].pri < t[u].pri) { last = st.back(); st.pop_back(); }
            t[u].l = last;
            if (!st.empty()) { t[st.back()].r = u; }
            st.push_back(u);}
        auto fix = [&](auto &&f, int v) -> void { if (v) { f(f, t[v].l); f(f, t[v].r); pull(v); }};
        if (st.empty()) { return 0; }
        fix(fix, st[0]);
        return st[0];}

    int size(int v) const { return t[v].sz; }
    // T: O(n), M: O(n).
    vector<T> values(int v) const {
        vector<T> res;
        vector<int> st;
        while (v || !st.empty()) {
            for (; v; v = t[v].l) { st.push_back(v); }
            v = st.back(); st.pop_back();
            res.push_back(t[v].key);
            v = t[v].r;}
        return res;}

    void pull(int v) { t[v].sz = t[t[v].l].sz + 1 + t[t[v].r].sz; }
    pair<int, int> split(int v, const T &x) {
        if (!v) { return {0, 0}; }
        if (t[v].key < x) { auto [a, b] = split(t[v].r, x); t[v].r = a; pull(v); return {v, b}; }
        auto [a, b] = split(t[v].l, x); t[v].l = b; pull(v); return {a, v};}
    int join(int a, int b) {
        if (!a || !b) { return a ? a : b; }
        if (t[a].pri > t[b].pri) { t[a].r = join(t[a].r, b); pull(a); return a; }
        t[b].l = join(a, t[b].l); pull(b); return b;}
    int merge(int a, int b) {
        int x = a, y = b;
        for (; x && t[x].r; x = t[x].r) {}
        for (; y && t[y].l; y = t[y].l) {}
        assert(!x || !y || !(t[y].key < t[x].key));
        return join(a, b);}
    // T: O(m * log(n / m + 1)) expected for sizes m <= n.
    int unite(int a, int b) {
        if (!a || !b) { return a ? a : b; }
        if (t[a].pri < t[b].pri) { swap(a, b); }
        auto [l, r] = split(b, t[a].key);
        t[a].l = unite(t[a].l, l); t[a].r = unite(t[a].r, r);
        pull(a);
        return a;}
    void insert(int &v, const T &x) { auto [a, b] = split(v, x); v = join(join(a, node(x)), b); }
    bool erase(int &v, const T &x) {
        if (!v) { return false; }
        if (!(x < t[v].key) && !(t[v].key < x)) { v = join(t[v].l, t[v].r); return true; }
        bool res = erase(x < t[v].key ? t[v].l : t[v].r, x);
        if (res) { pull(v); }
        return res;}

    int find(int v, const T &x) const {
        while (v && (x < t[v].key || t[v].key < x)) { v = x < t[v].key ? t[v].l : t[v].r; }
        return v;}
    const T &kth(int v, int k) const {
        assert(0 <= k && k < size(v));
        while (v && k != t[t[v].l].sz) {
            if (k < t[t[v].l].sz) { v = t[v].l; } else { k -= t[t[v].l].sz + 1; v = t[v].r; }}
        return t[v].key;}
    int rank(int v, const T &x) const {
        int res = 0;
        while (v) {
            if (t[v].key < x) { res += t[t[v].l].sz + 1; v = t[v].r; } else { v = t[v].l; }}
        return res;}
    int upperRank(int v, const T &x) const {
        int res = 0;
        while (v) {
            if (x < t[v].key) { v = t[v].l; } else { res += t[t[v].l].sz + 1; v = t[v].r; }}
        return res;}
    int lowerBound(int v, const T &x) const {
        int res = 0;
        while (v) {
            if (t[v].key < x) { v = t[v].r; } else { res = v; v = t[v].l; }}
        return res;}
    int upperBound(int v, const T &x) const {
        int res = 0;
        while (v) {
            if (x < t[v].key) { res = v; v = t[v].l; } else { v = t[v].r; }}
        return res;}
};

// A is an acted monoid (S, F, op, e, mapping, composition = new after old, id); reverse needs a commutative op or A::reverse(s), the product read backwards; trees are root ids in one pool (0 is empty); search predicates hold on e and stay false as the range grows.
// S: O(1), U: O(log(n)) expected, Q: O(log(n)) expected, M: O(m) for m created nodes
template<typename A>
struct ImplicitTreap {
    using S = typename A::S;
    using F = typename A::F;
    struct Node { S val, sum; F lz; uint pri; int sz, l, r; bool rev, act; };
    vector<Node> t;
    ulng seed;

    explicit ImplicitTreap(ulng seed = CustomHash::rnd) : t(1, Node{A::e(), A::e(), A::id(), 0, 0, 0, 0, false, false}), seed(seed) {}
    int node(const S &x) { t.push_back({x, x, A::id(), uint(CustomHash::splitMix64(seed++) >> 32), 1, 0, 0, false, false}); return int(t.size()) - 1; }
    // T: O(n), M: O(n).
    int build(const vector<S> &a) {
        vector<int> st;
        for (const S &x : a) {
            int u = node(x), last = 0;
            while (!st.empty() && t[st.back()].pri < t[u].pri) { last = st.back(); st.pop_back(); }
            t[u].l = last;
            if (!st.empty()) { t[st.back()].r = u; }
            st.push_back(u);}
        auto fix = [&](auto &&f, int v) -> void { if (v) { f(f, t[v].l); f(f, t[v].r); pull(v); }};
        if (st.empty()) { return 0; }
        fix(fix, st[0]);
        return st[0];}

    int size(int v) const { return t[v].sz; }
    // T: O(n), M: O(n).
    vector<S> values(int v) {
        vector<S> res;
        auto walk = [&](auto &&f, int u) -> void { if (u) { push(u); f(f, t[u].l); res.push_back(t[u].val); f(f, t[u].r); }};
        walk(walk, v);
        return res;}
    S at(int v, int i) {
        assert(0 <= i && i < size(v));
        for (push(v); v && i != t[t[v].l].sz; push(v)) {
            if (i < t[t[v].l].sz) { v = t[v].l; } else { i -= t[t[v].l].sz + 1; v = t[v].r; }}
        return t[v].val;}

    void pull(int v) { Node &x = t[v]; x.sz = t[x.l].sz + 1 + t[x.r].sz; x.sum = A::op(A::op(t[x.l].sum, x.val), t[x.r].sum); }
    void all(int v, const F &f) {
        if (!v) { return; }
        Node &x = t[v];
        x.val = A::mapping(f, x.val); x.sum = A::mapping(f, x.sum);
        x.lz = x.act ? A::composition(f, x.lz) : f; x.act = true;}
    // A flipped node already has its children swapped; rev flips their subtrees on push.
    void flip(int v) {
        if (!v) { return; }
        Node &x = t[v];
        swap(x.l, x.r); x.rev = !x.rev;
        if constexpr (requires (const S &s) { A::reverse(s); }) { x.sum = A::reverse(x.sum); }}
    void push(int v) {
        Node &x = t[v];
        if (x.rev) { flip(x.l); flip(x.r); x.rev = false; }
        if (x.act) { all(x.l, x.lz); all(x.r, x.lz); x.act = false; }}
    pair<int, int> cut(int v, int k) {
        if (!v) { return {0, 0}; }
        push(v);
        if (t[t[v].l].sz < k) { auto [a, b] = cut(t[v].r, k - t[t[v].l].sz - 1); t[v].r = a; pull(v); return {v, b}; }
        auto [a, b] = cut(t[v].l, k); t[v].l = b; pull(v); return {a, v};}
    pair<int, int> split(int v, int k) { assert(0 <= k && k <= size(v)); return cut(v, k); }
    int merge(int a, int b) {
        if (!a || !b) { return a ? a : b; }
        if (t[a].pri > t[b].pri) { push(a); t[a].r = merge(t[a].r, b); pull(a); return a; }
        push(b); t[b].l = merge(a, t[b].l); pull(b); return b;}
    void insert(int &v, int i, const S &x) { assert(0 <= i && i <= size(v)); auto [a, b] = cut(v, i); v = merge(merge(a, node(x)), b); }
    void erase(int &v, int i) { assert(0 <= i && i < size(v)); auto [a, b] = cut(v, i); v = merge(a, cut(b, 1).second); }
    // Calls g on the tree holding [l, r), then reassembles.
    template<typename G>
    void range(int &v, int l, int r, G g) {
        assert(0 <= l && l <= r && r <= size(v));
        auto [a, b] = cut(v, l);
        auto [m, c] = cut(b, r - l);
        g(m);
        v = merge(merge(a, m), c);}
    void set(int &v, int i, const S &x) { assert(0 <= i && i < size(v)); range(v, i, i + 1, [&](int m) { t[m].val = t[m].sum = x; }); }
    void reverse(int &v, int l, int r) { range(v, l, r, [&](int m) { flip(m); }); }
    void apply(int &v, int l, int r, const F &f) { range(v, l, r, [&](int m) { all(m, f); }); }

    S prod(int &v, int l, int r) { S res = A::e(); range(v, l, r, [&](int m) { res = t[m].sum; }); return res; }
    template<typename G>
    int maxRight(int &v, int l, G pred) {
        assert(0 <= l && l <= size(v) && pred(A::e()));
        auto [a, b] = cut(v, l);
        S acc = A::e();
        int res = l;
        for (int u = b; u;) {
            push(u);
            S s = A::op(acc, t[t[u].l].sum);
            if (!pred(s)) { u = t[u].l; continue; }
            acc = s; res += t[t[u].l].sz;
            s = A::op(acc, t[u].val);
            if (!pred(s)) { break; }
            acc = s; ++res; u = t[u].r;}
        v = merge(a, b);
        return res;}
    template<typename G>
    int minLeft(int &v, int r, G pred) {
        assert(0 <= r && r <= size(v) && pred(A::e()));
        auto [a, b] = cut(v, r);
        S acc = A::e();
        int res = r;
        for (int u = a; u;) {
            push(u);
            S s = A::op(t[t[u].r].sum, acc);
            if (!pred(s)) { u = t[u].r; continue; }
            acc = s; res -= t[t[u].r].sz;
            s = A::op(t[u].val, acc);
            if (!pred(s)) { break; }
            acc = s; --res; u = t[u].l;}
        v = merge(a, b);
        return res;}
};
