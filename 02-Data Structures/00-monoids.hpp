#pragma once

#include "../01-Core/01-template.hpp"

// T: O(1) per call, M: O(1); T is a commutative ring, len counts points.
template<typename T>
struct RangeAddRangeSum {
    struct S { T sum, len; };
    using F = T;
    static S op(const S &a, const S &b) { return {a.sum + b.sum, a.len + b.len}; }
    static S e() { return {T(0), T(0)}; }
    static S mapping(const F &f, const S &x) { return {x.sum + f * x.len, x.len}; }
    static F composition(const F &f, const F &g) { return f + g; }
    static F id() { return T(0); }
    static S leaf(T x) { return {x, T(1)}; }
};

// T: O(1) per call, M: O(1); values stay below numeric max, which is the identity.
template<typename T>
struct RangeAddRangeMin {
    using S = T;
    using F = T;
    static S op(const S &a, const S &b) { return min(a, b); }
    static S e() { return std::numeric_limits<T>::max(); }
    static S mapping(const F &f, const S &x) { return x == e() ? x : x + f; }
    static F composition(const F &f, const F &g) { return f + g; }
    static F id() { return T(0); }
};

// T: O(1) per call, M: O(1); values stay above numeric lowest, which is the identity.
template<typename T>
struct RangeAddRangeMax {
    using S = T;
    using F = T;
    static S op(const S &a, const S &b) { return max(a, b); }
    static S e() { return std::numeric_limits<T>::lowest(); }
    static S mapping(const F &f, const S &x) { return x == e() ? x : x + f; }
    static F composition(const F &f, const F &g) { return f + g; }
    static F id() { return T(0); }
};

// T: O(1) per call, M: O(1); leftmost minimum, i = -1 marks the identity.
template<typename T>
struct RangeAddRangeArgmin {
    struct S { T x; lng i; };
    using F = T;
    static S op(const S &a, const S &b) { return b.i < 0 || (a.i >= 0 && !(b.x < a.x)) ? a : b; }
    static S e() { return {T(0), -1}; }
    static S mapping(const F &f, const S &a) { return a.i < 0 ? a : S{a.x + f, a.i}; }
    static F composition(const F &f, const F &g) { return f + g; }
    static F id() { return T(0); }
    static S leaf(lng i, T x) { return {x, i}; }
};

// T: O(1) per call, M: O(1); minimum and its multiplicity, cnt = 0 marks the identity.
template<typename T>
struct RangeAddRangeMinCount {
    struct S { T x; lng cnt; };
    using F = T;
    static S op(const S &a, const S &b) {
        if (!a.cnt || (b.cnt && b.x < a.x)) { return b; }
        if (!b.cnt || a.x < b.x) { return a; }
        return {a.x, a.cnt + b.cnt};}
    static S e() { return {T(0), 0}; }
    static S mapping(const F &f, const S &a) { return a.cnt ? S{a.x + f, a.cnt} : a; }
    static F composition(const F &f, const F &g) { return f + g; }
    static F id() { return T(0); }
    static S leaf(T x) { return {x, 1}; }
};

// T: O(1) per call, M: O(1); F {a, b} maps x to a * x + b.
template<typename T>
struct RangeAffineRangeSum {
    struct S { T sum, len; };
    struct F { T a, b; };
    static S op(const S &a, const S &b) { return {a.sum + b.sum, a.len + b.len}; }
    static S e() { return {T(0), T(0)}; }
    static S mapping(const F &f, const S &x) { return {f.a * x.sum + f.b * x.len, x.len}; }
    static F composition(const F &f, const F &g) { return {f.a * g.a, f.a * g.b + f.b}; }
    static F id() { return {T(1), T(0)}; }
    static S leaf(T x) { return {x, T(1)}; }
};

// T: O(1) per call, M: O(1); F {set, x} assigns x when set.
template<typename T>
struct RangeAssignRangeSum {
    struct S { T sum, len; };
    struct F { bool set; T x; };
    static S op(const S &a, const S &b) { return {a.sum + b.sum, a.len + b.len}; }
    static S e() { return {T(0), T(0)}; }
    static S mapping(const F &f, const S &x) { return f.set ? S{f.x * x.len, x.len} : x; }
    static F composition(const F &f, const F &g) { return f.set ? f : g; }
    static F id() { return {false, T(0)}; }
    static S leaf(T x) { return {x, T(1)}; }
};

// T: O(log(len)) for mapping and O(1) otherwise, M: O(1); S {a, b} is the composite x -> a * x + b, leftmost applied first.
template<typename T>
struct RangeSetRangeComposite {
    struct S { T a, b; lng len; };
    struct F { bool set; T a, b; };
    static S op(const S &p, const S &q) { return {q.a * p.a, q.a * p.b + q.b, p.len + q.len}; }
    static S e() { return {T(1), T(0), 0}; }
    static S mapping(const F &f, const S &x) {
        if (!f.set) { return x; }
        S res = e(), p{f.a, f.b, 1};
        for (lng k = x.len; k; k >>= 1, p = op(p, p)) {
            if (k & 1) { res = op(res, p); }}
        return res;}
    static F composition(const F &f, const F &g) { return f.set ? f : g; }
    static F id() { return {false, T(1), T(0)}; }
    static S leaf(T a, T b) { return {a, b, 1}; }
};

// T: O(1) per call, M: O(1); F {b, d} adds b + d * i at index i, idx sums indices.
template<typename T>
struct RangeArithmeticAddRangeSum {
    struct S { T sum, len, idx; };
    struct F { T b, d; };
    static S op(const S &a, const S &b) { return {a.sum + b.sum, a.len + b.len, a.idx + b.idx}; }
    static S e() { return {T(0), T(0), T(0)}; }
    static S mapping(const F &f, const S &x) { return {x.sum + f.b * x.len + f.d * x.idx, x.len, x.idx}; }
    static F composition(const F &f, const F &g) { return {f.b + g.b, f.d + g.d}; }
    static F id() { return {T(0), T(0)}; }
    static S leaf(lng i, T x) { return {x, T(1), T(i)}; }
    static F progression(lng l, T a, T d) { return {a - d * T(l), d}; }
};

// T: O(1) per call, M: O(1); affine maps with any sign, leftmost argmin/argmax, len = 0 marks the identity.
template<typename T>
struct RangeAffineRangeMinMaxArg {
    struct S { T sum, mx, mn; lng len, lo, mxi, mni; };
    struct F { T a, b; };
    static S op(const S &a, const S &b) {
        if (!a.len) { return b; }
        if (!b.len) { return a; }
        S res{a.sum + b.sum, a.mx, a.mn, a.len + b.len, a.lo, a.mxi, a.mni};
        if (a.mx < b.mx) { res.mx = b.mx; res.mxi = b.mxi; }
        if (b.mn < a.mn) { res.mn = b.mn; res.mni = b.mni; }
        return res;}
    static S e() { return {T(0), T(0), T(0), 0, -1, -1, -1}; }
    static S mapping(const F &f, const S &x) {
        if (!x.len || (f.a == T(1) && f.b == T(0))) { return x; }
        S res = x;
        res.sum = f.a * x.sum + f.b * T(x.len);
        if (f.a == T(0)) { res.mx = res.mn = f.b; res.mxi = res.mni = x.lo; }
        else if (f.a < T(0)) { res.mx = f.a * x.mn + f.b; res.mn = f.a * x.mx + f.b; res.mxi = x.mni; res.mni = x.mxi; }
        else { res.mx = f.a * x.mx + f.b; res.mn = f.a * x.mn + f.b; }
        return res;}
    static F composition(const F &f, const F &g) { return {f.a * g.a, f.a * g.b + f.b}; }
    static F id() { return {T(1), T(0)}; }
    static S leaf(lng i, T x) { return {x, x, x, 1, i, i, i}; }
};

// T: O(1) per call, M: O(1); F {a, b} maps x to (x & a) ^ b, len = 0 marks the identity.
template<typename T = ulng>
struct RangeBitwiseRangeAndOrXor {
    struct S { T band, bor, bxor; lng len; };
    struct F { T a, b; };
    static S op(const S &a, const S &b) {
        if (!a.len) { return b; }
        if (!b.len) { return a; }
        return {T(a.band & b.band), T(a.bor | b.bor), T(a.bxor ^ b.bxor), a.len + b.len};}
    static S e() { return {T(~T(0)), T(0), T(0), 0}; }
    static S mapping(const F &f, const S &x) {
        if (!x.len) { return x; }
        T keep = f.a & ~f.b, flip = f.a & f.b;
        return {T((x.band & keep) | (~x.bor & flip) | (f.b & ~f.a)), T((x.bor & keep) | (~x.band & flip) | (f.b & ~f.a)),
                T((x.bxor & f.a) ^ (x.len & 1 ? f.b : T(0))), x.len};}
    static F composition(const F &f, const F &g) { return {T(g.a & f.a), T((g.b & f.a) ^ f.b)}; }
    static F id() { return {T(~T(0)), T(0)}; }
    static S leaf(T x) { return {x, x, x, 1}; }
    static F andWith(T x) { return {x, T(0)}; }
    static F orWith(T x) { return {T(~x), x}; }
    static F xorWith(T x) { return {T(~T(0)), x}; }
    static F assign(T x) { return {T(0), x}; }
};

// T: O(log(max)) per call, M: O(1); values >= 0, F {a, b} maps x to lcm(gcd(x, a), b), l tracks lcm only when LCM.
template<typename T, bool LCM = false>
struct RangeGcdLcm {
    struct S { T g, l; lng len; };
    struct F { T a, b; };
    static S op(const S &a, const S &b) {
        if (!a.len) { return b; }
        if (!b.len) { return a; }
        return {gcd(a.g, b.g), LCM ? lcm(a.l, b.l) : T(0), a.len + b.len};}
    static S e() { return {T(0), T(0), 0}; }
    static S mapping(const F &f, const S &x) {
        if (!x.len) { return x; }
        return {lcm(gcd(x.g, f.a), f.b), LCM ? lcm(gcd(x.l, f.a), f.b) : T(0), x.len};}
    static F composition(const F &f, const F &g) { return {gcd(g.a, f.a), lcm(gcd(g.b, f.a), f.b)}; }
    static F id() { return {T(0), T(1)}; }
    static S leaf(T x) { return {x, LCM ? x : T(0), 1}; }
    static F gcdWith(T x) { return {x, T(1)}; }
    static F lcmWith(T x) { return {T(0), x}; }
    static F assign(T x) { return {x, x}; }
};

// T: O(1) per call, M: O(1); best nonempty subarray, prefix and suffix sums, len = 0 marks the identity.
template<typename T>
struct RangeAssignMaxSubarray {
    struct S { T sum, pre, suf, best; lng len; };
    struct F { bool set; T x; };
    static S op(const S &a, const S &b) {
        if (!a.len) { return b; }
        if (!b.len) { return a; }
        return {a.sum + b.sum, max(a.pre, a.sum + b.pre), max(b.suf, b.sum + a.suf), max({a.best, b.best, a.suf + b.pre}), a.len + b.len};}
    static S e() { return {T(0), T(0), T(0), T(0), 0}; }
    static S mapping(const F &f, const S &x) {
        if (!f.set || !x.len) { return x; }
        T sum = f.x * T(x.len), top = f.x > T(0) ? sum : f.x;
        return {sum, top, top, top, x.len};}
    static F composition(const F &f, const F &g) { return f.set ? f : g; }
    static F id() { return {false, T(0)}; }
    static S leaf(T x) { return {x, x, x, x, 1}; }
};
