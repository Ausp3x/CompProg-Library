#pragma once

#include "../01-Core/01-template.hpp"

namespace beats_detail {
    // Visits node k = [a, b) for the range [l, r); g(k) finishes a covered node or asks to descend, and a leaf always finishes.
    template<typename T, typename G>
    void walk(T &t, int k, int a, int b, int l, int r, const G &g) {
        if (r <= a || b <= l || (l <= a && b <= r && (g(k) || b - a == 1))) { return; }
        t.push(k);
        int m = std::midpoint(a, b);
        walk(t, 2 * k, a, m, l, r, g); walk(t, 2 * k + 1, m, b, l, r, g);
        t.pull(k);}
} // namespace beats_detail

// 0 <= n <= 2^29 and n * |value| < 2^62 at all times; sqrt and mod need values >= 0; empty max/min return lng min/max.
// S: O(n), U: O(log(n)^2) amortized (divide, sqrt and mod bounds in the evidence), Q: O(log(n)), M: O(n)
struct SegTreeBeats {
    static constexpr int MAX_SIZE = 1 << 29;
    static constexpr lng LOW = std::numeric_limits<lng>::min(), HIGH = std::numeric_limits<lng>::max();
    struct Side { lng v, v2; int c; };
    struct Node { lng sum, add; Side hi, lo; int len; };
    int n;
    vector<Node> d;

    static size_t checkedSize(int N) { assert(0 <= N && N <= MAX_SIZE); return size_t(max(N, 0)); }
    explicit SegTreeBeats(int N = 0) : SegTreeBeats(vector<lng>(checkedSize(N))) {}
    explicit SegTreeBeats(const vector<lng> &a) : n(int(a.size())), d(2 * std::bit_ceil(max<size_t>(a.size(), 1))) {
        assert(a.size() <= MAX_SIZE);
        if (n) { build(1, 0, n, a); }}
    void build(int k, int a, int b, const vector<lng> &v) {
        d[k].len = b - a;
        if (b - a == 1) { assign(k, v[a]); return; }
        int m = std::midpoint(a, b);
        build(2 * k, a, m, v); build(2 * k + 1, m, b, v);
        pull(k);}

    static void merge(Side &p, const Side &x, const Side &y) {
        if (x.v == y.v) { p = {x.v, max(x.v2, y.v2), x.c + y.c}; }
        else if (x.v > y.v) { p = {x.v, max(x.v2, y.v), x.c}; }
        else { p = {y.v, max(x.v, y.v2), y.c}; }}
    void pull(int k) {
        Node &p = d[k], &x = d[2 * k], &y = d[2 * k + 1];
        p.sum = x.sum + y.sum;
        merge(p.hi, x.hi, y.hi); merge(p.lo, x.lo, y.lo);}
    void assign(int k, lng x) {
        Node &p = d[k];
        p.sum = x * p.len; p.add = 0;
        p.hi = {x, LOW, p.len}; p.lo = {-x, LOW, p.len};}
    void shift(int k, lng x) {
        Node &p = d[k];
        p.sum += x * p.len; p.add += x;
        p.hi.v += x; p.lo.v -= x;
        if (p.hi.v2 != LOW) { p.hi.v2 += x; }
        if (p.lo.v2 != LOW) { p.lo.v2 -= x; }}
    // lo holds negated values, so chmax is a cap on lo; sg is +1 for hi and -1 for lo; requires t > s.v2.
    static void cap(Node &p, Side &s, Side &o, lng t, lng sg) {
        if (t >= s.v) { return; }
        p.sum -= sg * (s.v - t) * s.c;
        if (o.v == -s.v) { o.v = -t; }
        else if (o.v2 == -s.v) { o.v2 = -t; }
        s.v = t;}
    void push(int k) {
        Node &p = d[k];
        for (int c : {2 * k, 2 * k + 1}) {
            if (p.hi.v == -p.lo.v) { assign(c, p.hi.v); continue; }
            shift(c, p.add);
            cap(d[c], d[c].hi, d[c].lo, p.hi.v, 1); cap(d[c], d[c].lo, d[c].hi, p.lo.v, -1);}
        p.add = 0;}
    // f maps the node's max and min to fmx and fmn, and f(x) - x is nonincreasing; equal shifts at the ends mean one shift for all.
    bool settle(int k, lng fmx, lng fmn, bool monotone) {
        lng mx = d[k].hi.v, mn = -d[k].lo.v;
        if (monotone && fmx == fmn) { assign(k, fmx); return true; }
        if (fmx - mx != fmn - mn) { return false; }
        shift(k, fmx - mx); return true;}
    template<typename G>
    void walk(int l, int r, const G &g) { assert(0 <= l && l <= r && r <= n); beats_detail::walk(*this, 1, 0, n, l, r, g); }
    static lng floorDiv(lng x, lng q) { return x / q - (x % q < 0); }
    static lng isqrt(lng x) {
        if (x <= 0) { return 0; }
        lng s = lng(std::sqrt(double(x)));
        while (s * s > x) { --s; }
        while ((s + 1) * (s + 1) <= x) { ++s; }
        return s;}

    void addUpdate(int l, int r, lng x) { walk(l, r, [&](int k) { shift(k, x); return true; }); }
    void setUpdate(int l, int r, lng x) { walk(l, r, [&](int k) { assign(k, x); return true; }); }
    void chminUpdate(int l, int r, lng x) {
        assert(x > LOW);
        walk(l, r, [&](int k) {
            if (x <= d[k].hi.v2) { return false; }
            cap(d[k], d[k].hi, d[k].lo, x, 1); return true;});}
    void chmaxUpdate(int l, int r, lng x) {
        assert(x > LOW);
        walk(l, r, [&](int k) {
            if (-x <= d[k].lo.v2) { return false; }
            cap(d[k], d[k].lo, d[k].hi, -x, -1); return true;});}
    void divideUpdate(int l, int r, lng q) {
        assert(q >= 1);
        walk(l, r, [&](int k) { return settle(k, floorDiv(d[k].hi.v, q), floorDiv(-d[k].lo.v, q), true); });}
    void sqrtUpdate(int l, int r) {
        assert(minQuery(l, r) >= 0);
        walk(l, r, [&](int k) { return settle(k, isqrt(d[k].hi.v), isqrt(-d[k].lo.v), true); });}
    void modUpdate(int l, int r, lng m) {
        assert(m >= 1 && minQuery(l, r) >= 0);
        walk(l, r, [&](int k) { return settle(k, d[k].hi.v % m, -d[k].lo.v % m, false); });}

    lng sumQuery(int l, int r) { lng res = 0; walk(l, r, [&](int k) { res += d[k].sum; return true; }); return res; }
    lng maxQuery(int l, int r) { lng res = LOW; walk(l, r, [&](int k) { res = max(res, d[k].hi.v); return true; }); return res; }
    lng minQuery(int l, int r) { lng res = HIGH; walk(l, r, [&](int k) { res = min(res, -d[k].lo.v); return true; }); return res; }
};

// 0 <= n <= 2^29; values, their history and time * sum fit lng; tick() adds every current value to its historic sum.
// S: O(n), U: O(log(n)), Q: O(log(n)), M: O(n)
struct HistoricSegTree {
    static constexpr int MAX_SIZE = 1 << 29;
    struct Node { lng sum, csum, mx, mn, hmx, hmn, add, up, down, cadd; int len; };
    int n;
    lng time = 0;
    vector<Node> d;

    explicit HistoricSegTree(int N = 0) : HistoricSegTree(vector<lng>(SegTreeBeats::checkedSize(N))) {}
    explicit HistoricSegTree(const vector<lng> &a) : n(int(a.size())), d(2 * std::bit_ceil(max<size_t>(a.size(), 1))) {
        assert(a.size() <= MAX_SIZE);
        if (n) { build(1, 0, n, a); }}
    void build(int k, int a, int b, const vector<lng> &v) {
        d[k].len = b - a;
        if (b - a == 1) { d[k].sum = d[k].mx = d[k].mn = d[k].hmx = d[k].hmn = v[a]; return; }
        int m = std::midpoint(a, b);
        build(2 * k, a, m, v); build(2 * k + 1, m, b, v);
        pull(k);}

    void pull(int k) {
        Node &p = d[k], &x = d[2 * k], &y = d[2 * k + 1];
        p.sum = x.sum + y.sum; p.csum = x.csum + y.csum;
        p.mx = max(x.mx, y.mx); p.mn = min(x.mn, y.mn);
        p.hmx = max(x.hmx, y.hmx); p.hmn = min(x.hmn, y.hmn);}
    // Adds a after pending adds whose running total peaked at up and bottomed at down; csum stores history minus time * value.
    void apply(int k, lng a, lng up, lng down, lng c) {
        Node &p = d[k];
        p.hmx = max(p.hmx, p.mx + up); p.hmn = min(p.hmn, p.mn + down);
        p.up = max(p.up, p.add + up); p.down = min(p.down, p.add + down);
        p.mx += a; p.mn += a; p.add += a;
        p.sum += a * p.len; p.csum += c * p.len; p.cadd += c;}
    void push(int k) {
        Node &p = d[k];
        for (int c : {2 * k, 2 * k + 1}) { apply(c, p.add, p.up, p.down, p.cadd); }
        p.add = p.up = p.down = p.cadd = 0;}
    template<typename G>
    void walk(int l, int r, const G &g) { assert(0 <= l && l <= r && r <= n); beats_detail::walk(*this, 1, 0, n, l, r, g); }

    void addUpdate(int l, int r, lng x) { walk(l, r, [&](int k) { apply(k, x, max<lng>(x, 0), min<lng>(x, 0), -time * x); return true; }); }
    void tick() { ++time; }

    lng sumQuery(int l, int r) { lng res = 0; walk(l, r, [&](int k) { res += d[k].sum; return true; }); return res; }
    lng maxQuery(int l, int r) { lng res = SegTreeBeats::LOW; walk(l, r, [&](int k) { res = max(res, d[k].mx); return true; }); return res; }
    lng minQuery(int l, int r) { lng res = SegTreeBeats::HIGH; walk(l, r, [&](int k) { res = min(res, d[k].mn); return true; }); return res; }
    lng historicMaxQuery(int l, int r) { lng res = SegTreeBeats::LOW; walk(l, r, [&](int k) { res = max(res, d[k].hmx); return true; }); return res; }
    lng historicMinQuery(int l, int r) { lng res = SegTreeBeats::HIGH; walk(l, r, [&](int k) { res = min(res, d[k].hmn); return true; }); return res; }
    lng historicSumQuery(int l, int r) { lng res = 0; walk(l, r, [&](int k) { res += d[k].csum + time * d[k].sum; return true; }); return res; }
};

// 0 <= n <= 2^29; A is an acted monoid plus fail(s), which mapping may set on a node of two or more elements, never on one.
// S: O(n + 1), U: O(log(n + 1)) plus O(1) per failed node (amortized by the preset), Q: O(log(n + 1)), M: O(n + 1)
template<typename A>
struct LazySegmentTreeBeats {
    using S = typename A::S;
    using F = typename A::F;
    static constexpr int MAX_SIZE = 1 << 29;
    int n, lg, base;
    vector<S> d;
    vector<F> lz;

    static int checkedSize(size_t n) { assert(n <= MAX_SIZE); return int(n); }
    explicit LazySegmentTreeBeats(int N = 0) : n(N) { assert(0 <= n && n <= MAX_SIZE); init(); }
    explicit LazySegmentTreeBeats(const vector<S> &a) : n(checkedSize(a.size())) {
        init();
        std::copy(a.begin(), a.end(), d.begin() + base);
        for (int k = base - 1; k; --k) { pull(k); }}
    void init() {
        for (lg = 0; (1 << lg) < n; ++lg) {}
        base = 1 << lg;
        d.assign(2 * base, A::e());
        lz.assign(base, A::id());}

    void pull(int k) { d[k] = A::op(d[2 * k], d[2 * k + 1]); }
    void put(int k, const F &f) {
        d[k] = A::mapping(f, d[k]);
        if (k < base) {
            lz[k] = A::composition(f, lz[k]);
            if (A::fail(d[k])) { push(k); pull(k); }}}
    void push(int k) { put(2 * k, lz[k]); put(2 * k + 1, lz[k]); lz[k] = A::id(); }
    void pushPath(int p) { for (int i = lg; i >= 1; --i) { push(p >> i); }}
    void pullPath(int p) { for (int i = 1; i <= lg; ++i) { pull(p >> i); }}
    void pushBounds(int l, int r) {
        for (int i = lg; i >= 1; --i) {
            if (((l >> i) << i) != l) { push(l >> i); }
            if (((r >> i) << i) != r) { push((r - 1) >> i); }}}

    S get(int p) { assert(0 <= p && p < n); p += base; pushPath(p); return d[p]; }
    const S &allProd() const { return d[1]; }

    void set(int p, const S &x) { assert(0 <= p && p < n); p += base; pushPath(p); d[p] = x; pullPath(p); }
    void apply(int l, int r, const F &f) {
        assert(0 <= l && l <= r && r <= n);
        if (l == r) { return; }
        l += base; r += base;
        pushBounds(l, r);
        for (int a = l, b = r; a < b; a >>= 1, b >>= 1) {
            if (a & 1) { put(a++, f); }
            if (b & 1) { put(--b, f); }}
        for (int i = 1; i <= lg; ++i) {
            if (((l >> i) << i) != l) { pull(l >> i); }
            if (((r >> i) << i) != r) { pull((r - 1) >> i); }}}

    S prod(int l, int r) {
        assert(0 <= l && l <= r && r <= n);
        if (l == r) { return A::e(); }
        l += base; r += base;
        pushBounds(l, r);
        S left = A::e(), right = A::e();
        for (; l < r; l >>= 1, r >>= 1) {
            if (l & 1) { left = A::op(left, d[l++]); }
            if (r & 1) { right = A::op(d[--r], right); }}
        return A::op(left, right);}
};

// T: O(1) per call, M: O(1); T unsigned (sum modulo 2^w) or signed with nonnegative values and or-operands; F {a, b} maps x to (x & a) | b.
template<typename T = ulng>
struct RangeAndOrRangeSumMax {
    struct S { T sum, mx, band, bor; lng len; bool bad; };
    struct F { T a, b; };
    static S op(const S &x, const S &y) {
        if (!x.len) { return y; }
        if (!y.len) { return x; }
        return {T(x.sum + y.sum), max(x.mx, y.mx), T(x.band & y.band), T(x.bor | y.bor), x.len + y.len, false};}
    static S e() { return {T(0), T(0), T(~T(0)), T(0), 0, false}; }
    // Every element shares the touched bits, so all elements move by the same amount.
    static S mapping(const F &f, const S &x) {
        if (!x.len) { return x; }
        T touched = T(~f.a | f.b);
        if (touched & ~T(x.band | ~x.bor)) { S res = x; res.bad = true; return res; }
        T before = T(x.band & touched), after = T(((x.band & f.a) | f.b) & touched);
        return {T(x.sum + T(after - before) * T(x.len)), T((x.mx & f.a) | f.b), T((x.band & f.a) | f.b), T((x.bor & f.a) | f.b), x.len, false};}
    static F composition(const F &f, const F &g) { return {T(g.a & f.a), T((g.b & f.a) | f.b)}; }
    static F id() { return {T(~T(0)), T(0)}; }
    static bool fail(const S &x) { return x.bad; }
    static S leaf(T x) { return {x, x, x, x, 1, false}; }
    static F andWith(T x) { return {x, T(0)}; }
    static F orWith(T x) { return {T(~T(0)), x}; }
    static F assign(T x) { return {T(0), x}; }
};
