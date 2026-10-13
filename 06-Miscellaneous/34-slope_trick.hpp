#pragma once
#include "../01-Core/01-template.hpp"
#include "01-random.hpp"

// S: O(1), U: O(log(n)), Q: O(1), M: O(n); n unit breakpoints; addLinear O(|a| * log(n)), evaluate O(n), merge O(min(n, m) * log(n + m)).
// T signed integral or floating, unit slope changes; argminRange marks an unbounded end with -INF or INF.
template<class T>
struct SlopeTrick {
    static constexpr T INF = std::numeric_limits<T>::max();
    T mn = 0, addl = 0, addr = 0;
    vector<T> left, right;

    T topL() const { return left[0] + addl; }
    T topR() const { return right[0] + addr; }
    void pushL(T a) { left.push_back(a - addl); std::push_heap(left.begin(), left.end()); }
    void pushR(T a) { right.push_back(a - addr); std::push_heap(right.begin(), right.end(), std::greater<T>()); }
    T popL() { std::pop_heap(left.begin(), left.end()); T a = left.back() + addl; left.pop_back(); return a; }
    T popR() { std::pop_heap(right.begin(), right.end(), std::greater<T>()); T a = right.back() + addr; right.pop_back(); return a; }
    int size() const { return int(left.size() + right.size()); }

    void addConst(T c) { mn += c; }
    void addLinear(T a) {
        assert(-T(right.size()) <= a && a <= T(left.size()));
        for (; a > 0 && !left.empty(); --a) { T l = popL(); mn += l; pushR(l); }
        for (; a < 0 && !right.empty(); ++a) { T r = popR(); mn -= r; pushL(r); }}
    void addXMinusA(T a) {
        if (!left.empty() && topL() > a) { mn += topL() - a; pushL(a); a = popL(); }
        pushR(a);}
    void addAMinusX(T a) {
        if (!right.empty() && topR() < a) { mn += a - topR(); pushR(a); a = popR(); }
        pushL(a);}
    void addAbs(T a) { addXMinusA(a); addAMinusX(a); }
    void prefixMin() { right.clear(); }
    void suffixMin() { left.clear(); }
    void shift(T d) { addl += d; addr += d; }
    void slide(T a, T b) { assert(a <= b); addl += a; addr += b; }
    void merge(SlopeTrick o) {
        if (size() < o.size()) { std::swap(*this, o); }
        for (T l : o.left) { addAMinusX(l + o.addl); }
        for (T r : o.right) { addXMinusA(r + o.addr); }
        mn += o.mn;}

    T min() const { return mn; }
    pair<T, T> argminRange() const { return {left.empty() ? -INF : topL(), right.empty() ? INF : topR()}; }
    T evaluate(T x) const {
        T res = mn;
        for (T l : left) { res += max(T(0), l + addl - x); }
        for (T r : right) { res += max(T(0), x - r - addr); }
        return res;}
};

// S: O(1), U: expected O(log(n)), Q: expected O(log(n)), M: O(n); n nodes; merge expected O(m * log(n / m + 1)), minPlusConvolution expected O(m * log(n)), m <= n.
// Signed integral T, weights w >= 0, sums in lll; +INF outside domain(); argminRange marks an unbounded end with -INF or INF.
template<class T>
struct WeightedSlopeTrick {
    static_assert(std::is_integral_v<T> && std::is_signed_v<T>);
    static constexpr T INF = std::numeric_limits<T>::max();
    struct Node { T x, w, lz, sw; lll sx; uint pri; int sz, l, r; };
    vector<Node> t{Node{}};
    vector<int> freed;
    int root = 0;
    // f(x) = k - wl * x + sum of w * max(0, x - p) on [lo, hi], every p in [lo, hi]; wl >= 0 if lo is finite, wl <= weight() if hi is.
    lll k = 0;
    T wl = 0, lo = -INF, hi = INF;

    WeightedSlopeTrick() = default;
    WeightedSlopeTrick(const WeightedSlopeTrick &) = default;
    WeightedSlopeTrick(WeightedSlopeTrick &&o) noexcept { swap(o); }
    WeightedSlopeTrick &operator=(WeightedSlopeTrick o) noexcept { swap(o); return *this; }
    void swap(WeightedSlopeTrick &o) noexcept {
        std::swap(t, o.t); std::swap(freed, o.freed); std::swap(root, o.root);
        std::swap(k, o.k); std::swap(wl, o.wl); std::swap(lo, o.lo); std::swap(hi, o.hi);}

    int alloc() {
        if (freed.empty()) { t.emplace_back(); return int(t.size()) - 1; }
        int v = freed.back(); freed.pop_back();
        for (int c : {t[v].l, t[v].r}) { if (c) { freed.push_back(c); } }
        return v;}
    int node(T x, T w) { int v = alloc(); t[v] = {x, w, 0, w, lll(w) * x, uint(rng() >> 32), 1, 0, 0}; return v; }
    void discard(int v) { if (v) { freed.push_back(v); } }
    void apply(int v, T d) { if (v) { t[v].x += d; t[v].lz += d; t[v].sx += lll(t[v].sw) * d; } }
    void push(int v) { if (t[v].lz) { apply(t[v].l, t[v].lz); apply(t[v].r, t[v].lz); t[v].lz = 0; } }
    int pull(int v) {
        Node &n = t[v], &a = t[n.l], &b = t[n.r];
        n.sw = a.sw + n.w + b.sw; n.sx = a.sx + lll(n.w) * n.x + b.sx; n.sz = a.sz + 1 + b.sz;
        return v;}
    int join(int a, int b) {
        if (!a || !b) { return a | b; }
        if (t[a].pri > t[b].pri) { push(a); t[a].r = join(t[a].r, b); return pull(a); }
        push(b); t[b].l = join(a, t[b].l); return pull(b);}
    pair<int, int> splitAt(int v, T x) {
        if (!v) { return {0, 0}; }
        push(v);
        if (t[v].x < x) { auto [a, b] = splitAt(t[v].r, x); t[v].r = a; return {pull(v), b}; }
        auto [a, b] = splitAt(t[v].l, x); t[v].l = b; return {a, pull(v)};}
    pair<int, int> splitUnits(int v, T c) {
        if (!v) { return {0, 0}; }
        push(v);
        T lw = t[t[v].l].sw;
        if (c <= lw) { auto [a, b] = splitUnits(t[v].l, c); t[v].l = b; return {a, pull(v)}; }
        if (c >= lw + t[v].w) { auto [a, b] = splitUnits(t[v].r, c - lw - t[v].w); t[v].r = a; return {pull(v), b}; }
        int u = node(t[v].x, lw + t[v].w - c), r = t[v].r;
        t[v].w = c - lw; t[v].r = 0;
        return {pull(v), join(u, r)};}
    // Duplicate keys keep a fixed order (new or first-operand ones first), so the shape never depends on priorities.
    int insert(int v, int u) {
        if (!v) { return u; }
        if (t[u].pri > t[v].pri) { auto [a, b] = splitAt(v, t[u].x); t[u].l = a; t[u].r = b; return pull(u); }
        push(v);
        if (t[u].x <= t[v].x) { t[v].l = insert(t[v].l, u); } else { t[v].r = insert(t[v].r, u); }
        return pull(v);}
    int unite(int a, int b) {
        if (!a || !b) { return a | b; }
        if (t[a].pri < t[b].pri) {
            push(b);
            auto [l, r] = splitAt(a, t[b].x + 1);
            int x = unite(l, t[b].l), y = unite(r, t[b].r);
            t[b].l = x; t[b].r = y; return pull(b);}
        push(a);
        auto [l, r] = splitAt(b, t[a].x);
        int x = unite(t[a].l, l), y = unite(t[a].r, r);
        t[a].l = x; t[a].r = y; return pull(a);}
    int build(const vector<pair<T, T>> &s) {
        vector<int> st;
        for (auto [x, w] : s) {
            int u = node(x, w), last = 0;
            while (!st.empty() && t[st.back()].pri < t[u].pri) { last = pull(st.back()); st.pop_back(); }
            t[u].l = last;
            if (!st.empty()) { t[st.back()].r = u; }
            st.push_back(u);}
        int res = 0;
        while (!st.empty()) { res = pull(st.back()); st.pop_back(); }
        return res;}
    void collect(int v, vector<pair<T, T>> &res) {
        if (!v) { return; }
        push(v); collect(t[v].l, res); res.push_back({t[v].x, t[v].w}); collect(t[v].r, res);}
    pair<lll, T> prefix(T c) const {
        lll s = 0; T d = 0, x = 0;
        for (int v = root; v;) {
            const Node &n = t[v], &a = t[n.l];
            T e = d + n.lz;
            if (c <= a.sw) { v = n.l; }
            else {
                s += a.sx + lll(a.sw) * e; c -= a.sw; x = n.x + d;
                if (c <= n.w) { return {s + lll(c) * x, x}; }
                s += lll(n.w) * x; c -= n.w; v = n.r;}
            d = e;}
        return {s, x};}
    int nodes() const { return t[root].sz; }
    T weight() const { return t[root].sw; }
    lll minimum() const { return k - (wl > 0 ? prefix(wl).fi : 0); }
    pair<T, T> domain() const { return {lo, hi}; }
    void put(T a, T w) {
        if (!w || a > hi) { return; }
        if (a < lo) { k -= lll(w) * a; wl -= w; return; }
        int u = node(a, w); root = insert(root, u);}
    void norm() {
        if (wl < 0 && lo > -INF) { T w = -wl; k += lll(w) * lo; wl = 0; put(lo, w); }
        if (wl > weight() && hi < INF) { put(hi, wl - weight()); }}
    void clip() {
        if (lo > -INF) { auto [a, b] = splitAt(root, lo); k -= t[a].sx; wl -= t[a].sw; discard(a); root = b; }
        if (hi < INF) { auto [a, b] = splitAt(root, hi + 1); discard(b); root = a; }
        norm();}

    void addConst(T c) { k += c; }
    void addLinear(T a) { wl -= a; norm(); }
    void addXMinusA(T a, T w = 1) { assert(w >= 0); put(a, w); norm(); }
    void addAMinusX(T a, T w = 1) { assert(w >= 0); k += lll(w) * a; wl += w; put(a, w); norm(); }
    void addAbs(T a, T w = 1) { assert(w >= 0); k += lll(w) * a; wl += w; put(a, 2 * w); norm(); }
    void restrictDomain(T l, T r) { lo = max(lo, l); hi = std::min(hi, r); assert(lo <= hi); clip(); }
    void prefixMin() {
        assert(wl >= 0);
        if (wl < weight()) { auto [a, b] = splitUnits(root, wl); discard(b); root = a; }
        hi = INF;}
    void suffixMin() {
        assert(wl <= weight());
        if (wl > 0) { k = minimum(); auto [a, b] = splitUnits(root, wl); discard(a); root = b; wl = 0; }
        lo = -INF;}
    void shift(T d) {
        apply(root, d); k += lll(wl) * d;
        if (lo > -INF) { lo += d; }
        if (hi < INF) { hi += d; }}
    void slide(T a, T b) {
        assert(a <= b);
        auto [x, y] = splitUnits(root, std::clamp(wl, T(0), weight()));
        apply(x, a); apply(y, b); root = join(x, y); k += lll(wl) * (wl < 0 ? b : a);
        if (lo > -INF) { lo += a; }
        if (hi < INF) { hi += b; }}
    void clampSlope(T s1, T s2) {
        assert(s1 <= 0 && 0 <= s2 && bounded());
        lll m = minimum();
        T kl = std::min(wl, -s1), kr = std::min(weight() - wl, s2);
        auto [a, b] = splitUnits(root, wl);
        auto [a1, a2] = splitUnits(a, wl - kl);
        auto [b1, b2] = splitUnits(b, kr);
        discard(a1); discard(b2);
        if (lo > -INF && kl < -s1) { a2 = join(node(lo, -s1 - kl), a2); kl = -s1; }
        if (hi < INF && kr < s2) { b1 = join(b1, node(hi, s2 - kr)); }
        k = m + t[a2].sx; wl = kl; root = join(a2, b1); lo = -INF; hi = INF;}
    void merge(WeightedSlopeTrick o) {
        if (nodes() < o.nodes()) { swap(o); }
        vector<pair<T, T>> s;
        o.collect(o.root, s);
        root = unite(root, build(s)); k += o.k; wl += o.wl;
        lo = max(lo, o.lo); hi = std::min(hi, o.hi); assert(lo <= hi); clip();}
    void minPlusConvolution(WeightedSlopeTrick o) {
        if (nodes() < o.nodes()) { swap(o); }
        assert(bounded() && o.bounded());
        lll m = minimum() + o.minimum();
        T rw = weight() - wl;
        auto [a, b] = splitUnits(root, wl);
        auto [oa, ob] = o.splitUnits(o.root, o.wl);
        vector<pair<T, T>> ls, rs;
        o.collect(oa, ls); o.collect(ob, rs);
        int done = 0, ext = 0; T rest = wl;
        for (int i = int(ls.size()) - 1; i >= 0; --i) {
            auto [p, w] = ls[i];
            T take = std::min(w, rest);
            auto [x, y] = splitUnits(a, rest - take);
            apply(y, p); done = join(y, done); a = x; rest -= take;
            if (w > take) {
                if (lo == -INF) { break; }
                ext = join(node(p + lo, w - take), ext);}}
        if (o.lo > -INF) { apply(a, o.lo); } else { discard(a); a = 0; }
        a = join(join(ext, a), done);
        done = ext = 0; rest = rw;
        for (auto [p, w] : rs) {
            T take = std::min(w, rest);
            auto [x, y] = splitUnits(b, take);
            apply(x, p); done = join(done, x); b = y; rest -= take;
            if (w > take) {
                if (hi == INF) { break; }
                ext = join(ext, node(p + hi, w - take));}}
        if (o.hi < INF) { apply(b, o.hi); } else { discard(b); b = 0; }
        b = join(done, join(b, ext));
        wl = t[a].sw; k = m + t[a].sx; root = join(a, b);
        lo = lo > -INF && o.lo > -INF ? lo + o.lo : -INF;
        hi = hi < INF && o.hi < INF ? hi + o.hi : INF;}

    bool bounded() const { return 0 <= wl && wl <= weight(); }
    T min() const { assert(bounded()); return T(minimum()); }
    pair<T, T> argminRange() const {
        assert(bounded());
        return {wl > 0 ? prefix(wl).se : lo, wl < weight() ? prefix(wl + 1).se : hi};}
    T evaluate(T x) const {
        if (x < lo || x > hi) { return INF; }
        lll res = k - lll(wl) * x; T d = 0;
        for (int v = root; v;) {
            const Node &n = t[v], &a = t[n.l];
            T e = d + n.lz;
            if (n.x + d < x) { res += (lll(a.sw) + n.w) * x - a.sx - lll(a.sw) * e - lll(n.w) * (n.x + d); v = n.r; }
            else { v = n.l; }
            d = e;}
        return T(res);}
};

// S: O(1), U: O(min(|d|, n) * log(n)) shift, O(min(|a|, n) * log(n)) addXMinusA and addAMinusX, O((min(|a|, n) + b - a) * log(n)) slide, O(1) otherwise, Q: O(1), M: O(n); n domain width.
// Signed integral T, f finite exactly on domain() (initially {0}), weights w >= 0; atZero returns INF when 0 is outside.
template<class T>
struct DualSlopeTrick {
    static_assert(std::is_integral_v<T> && std::is_signed_v<T>);
    static constexpr T INF = std::numeric_limits<T>::max();
    // g is the convex conjugate of f(x + c), so domain() has |g.left| points left of c; c = clamp(0, domain()).
    SlopeTrick<T> g;
    T c = 0;

    pair<T, T> domain() const { return {c - T(g.left.size()), c + T(g.right.size())}; }

    void addConst(T b) { g.addConst(-b); }
    void addLinear(T a) { g.shift(a); g.addConst(-a * c); }
    void shift(T d) {
        auto [l, r] = domain();
        T nc = std::clamp(T(0), l + d, r + d);
        g.addLinear(c + d - nc); c = nc;}
    void addXMinusA(T a, T w = 1) {
        assert(w >= 0);
        auto [l, r] = domain();
        if (a <= l) { addLinear(w); addConst(-w * a); }
        else if (a < r) { g.addLinear(c - a); g.slide(0, w); g.addLinear(a - c); }}
    void addAMinusX(T a, T w = 1) { assert(w >= 0); addLinear(-w); addConst(w * a); addXMinusA(a, w); }
    void slide(T a, T b) {
        assert(a <= b);
        shift(a);
        for (T i = a; i < b; ++i) { g.addXMinusA(0); }
        shift(0);}

    T atZero() const { return c ? INF : -g.mn; }
};
