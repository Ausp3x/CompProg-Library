#pragma once

#include "../01-Core/01-template.hpp"

namespace monoid_detail {
    // T: O(1) per call, M: O(1); sum and length of a range, shared by the sum-folding acted presets.
    template<typename T>
    struct SumLen {
        struct S { T sum, len; };
        static S op(const S &a, const S &b) { return {a.sum + b.sum, a.len + b.len}; }
        static S e() { return {T(0), T(0)}; }
        static S leaf(T x) { return {x, T(1)}; }
    };

    // T: O(log(k + 1)) calls of op, M: O(1); x to the power k >= 0, squaring only while bits of k remain so no intermediate exceeds k factors.
    template<typename M, typename S>
    S power(const M &m, S x, lng k) {
        S res = m.e();
        for (; k; k >>= 1) {
            if (k & 1) { res = m.op(res, x); }
            if (k > 1) { x = m.op(x, x); }}
        return res;}
} // namespace monoid_detail

// T: O(1) per call, M: O(1); T(0) is the identity of +.
template<typename T>
struct Sum {
    using S = T;
    static S op(const S &a, const S &b) { return T(a + b); }
    static S e() { return T(0); }
};

// T: O(1) per call, M: O(1); T(1) is the identity of *.
template<typename T>
struct Mul {
    using S = T;
    static S op(const S &a, const S &b) { return T(a * b); }
    static S e() { return T(1); }
};

// T: O(1) per call, M: O(1); T has numeric limits, numeric max is the identity.
template<typename T>
struct Min {
    using S = T;
    static S op(const S &a, const S &b) { return min(a, b); }
    static S e() { return std::numeric_limits<T>::max(); }
};

// T: O(1) per call, M: O(1); T has numeric limits, numeric lowest is the identity.
template<typename T>
struct Max {
    using S = T;
    static S op(const S &a, const S &b) { return max(a, b); }
    static S e() { return std::numeric_limits<T>::lowest(); }
};

// T: O(1) per call, M: O(1); S {mn, mx}, the identity has mn > mx.
template<typename T>
struct MinMax {
    struct S { T mn, mx; };
    static S op(const S &a, const S &b) { return {min(a.mn, b.mn), max(a.mx, b.mx)}; }
    static S e() { return {std::numeric_limits<T>::max(), std::numeric_limits<T>::lowest()}; }
    static S leaf(T x) { return {x, x}; }
};

// T: O(1) per call, M: O(1); integral T.
template<typename T>
struct Xor {
    using S = T;
    static S op(const S &a, const S &b) { return T(a ^ b); }
    static S e() { return T(0); }
};

// T: O(1) per call, M: O(1); integral T, all bits set is the identity.
template<typename T>
struct And {
    using S = T;
    static S op(const S &a, const S &b) { return T(a & b); }
    static S e() { return T(~T(0)); }
};

// T: O(1) per call, M: O(1); integral T.
template<typename T>
struct Or {
    using S = T;
    static S op(const S &a, const S &b) { return T(a | b); }
    static S e() { return T(0); }
};

// T: O(log(max)) per call, M: O(1); integral T, values >= 0, 0 is the identity.
template<typename T>
struct Gcd {
    using S = T;
    static S op(const S &a, const S &b) { return T(gcd(a, b)); }
    static S e() { return T(0); }
};

// T: O(log(max)) per call, M: O(1); integral T, values >= 1 and every lcm fits T, 1 is the identity.
template<typename T>
struct Lcm {
    using S = T;
    static S op(const S &a, const S &b) { return T(lcm(a, b)); }
    static S e() { return T(1); }
};

// T: O(1) per call, M: O(1); S {a, b} maps x to a * x + b, op(f, g) applies f first.
template<typename T>
struct AffineComposite {
    struct S { T a, b; };
    static S op(const S &f, const S &g) { return {g.a * f.a, g.a * f.b + g.b}; }
    static S e() { return {T(1), T(0)}; }
    static T eval(const S &f, const T &x) { return f.a * x + f.b; }
};

// T: O(1) per call, M: O(1); S {a, lo, hi} maps x to max(min(x + a, hi), lo), op(f, g) applies f first; values and shifted bounds stay strictly inside (lowest, max).
template<typename T>
struct AddChminChmax {
    struct S { T a, lo, hi; };
    static constexpr T LOW = std::numeric_limits<T>::lowest(), HIGH = std::numeric_limits<T>::max();
    static S op(const S &f, const S &g) {
        T lo = f.lo == LOW ? g.lo : max(min(T(f.lo + g.a), g.hi), g.lo), hi = f.hi == HIGH ? g.hi : min(T(f.hi + g.a), g.hi);
        return {T(f.a + g.a), lo, hi};}
    static S e() { return {T(0), LOW, HIGH}; }
    static T eval(const S &f, const T &x) { return max(min(T(x + f.a), f.hi), f.lo); }
    static S add(T x) { return {x, LOW, HIGH}; }
    static S chmin(T x) { return {T(0), LOW, x}; }
    static S chmax(T x) { return {T(0), x, HIGH}; }
    static S assign(T x) { return {T(0), x, x}; }
};

// T: O(1) per call, M: O(1); S {a, b} maps x to (x & a) ^ b, op(f, g) applies f first.
template<typename T = ulng>
struct BitwiseComposite {
    struct S { T a, b; };
    static S op(const S &f, const S &g) { return {T(f.a & g.a), T((f.b & g.a) ^ g.b)}; }
    static S e() { return {T(~T(0)), T(0)}; }
    static T eval(const S &f, const T &x) { return T((x & f.a) ^ f.b); }
    static S andWith(T x) { return {x, T(0)}; }
    static S orWith(T x) { return {T(~x), x}; }
    static S xorWith(T x) { return {T(~T(0)), x}; }
    static S assign(T x) { return {T(0), x}; }
};

// T: O(1) per call, M: O(1); leftmost minimum under Less (MaxIndex: maximum), i = -1 marks the identity; Opposite gives the rightmost.
template<typename T, typename Less = std::less<T>>
struct MinIndex {
    struct S { T x; lng i; };
    static S op(const S &a, const S &b) { return b.i < 0 || (a.i >= 0 && !Less()(b.x, a.x)) ? a : b; }
    static S e() { return {T(), -1}; }
    static S leaf(lng i, T x) { return {x, i}; }
};
template<typename T> using MaxIndex = MinIndex<T, std::greater<T>>;

// T: O(1) per call, M: O(1); minimum under Less (MaxCount: maximum) and its multiplicity, cnt = 0 marks the identity.
template<typename T, typename Less = std::less<T>>
struct MinCount {
    struct S { T x; lng cnt; };
    static S op(const S &a, const S &b) {
        if (!a.cnt || (b.cnt && Less()(b.x, a.x))) { return b; }
        if (!b.cnt || Less()(a.x, b.x)) { return a; }
        return {a.x, a.cnt + b.cnt};}
    static S e() { return {T(), 0}; }
    static S leaf(T x) { return {x, 1}; }
};
template<typename T> using MaxCount = MinCount<T, std::greater<T>>;

// T: O(1) per call, M: O(1); S {x1, c1, x2, c2} holds the two largest distinct values with multiplicities, count 0 marks an absent value.
template<typename T>
struct Max2 {
    struct S { T x1; lng c1; T x2; lng c2; };
    static S op(S a, const S &b) {
        for (auto [x, c] : {pair(b.x1, b.c1), pair(b.x2, b.c2)}) {
            if (!c) { continue; }
            if (a.c1 && x == a.x1) { a.c1 += c; }
            else if (!a.c1 || a.x1 < x) { a.x2 = a.x1; a.c2 = a.c1; a.x1 = x; a.c1 = c; }
            else if (a.c2 && x == a.x2) { a.c2 += c; }
            else if (!a.c2 || a.x2 < x) { a.x2 = x; a.c2 = c; }}
        return a;}
    static S e() { return {T(), 0, T(), 0}; }
    static S leaf(T x) { return {x, 1, T(), 0}; }
};

// T: O(1) per call, M: O(1); best nonempty subarray, prefix and suffix sums, len = 0 marks the identity.
template<typename T>
struct MaxSubarray {
    struct S { T sum, pre, suf, best; lng len; };
    static S op(const S &a, const S &b) {
        if (!a.len) { return b; }
        if (!b.len) { return a; }
        return {a.sum + b.sum, max(a.pre, a.sum + b.pre), max(b.suf, b.sum + a.suf), max({a.best, b.best, a.suf + b.pre}), a.len + b.len};}
    static S e() { return {T(0), T(0), T(0), T(0), 0}; }
    static S leaf(T x) { return {x, x, x, x, 1}; }
};

// T: O(1) per call, M: O(1); best a[j] - a[i] over i <= j (0 for one element) with the minimum and maximum, the identity has mn > mx; differences fit T.
template<typename T>
struct MaxGain {
    struct S { T mn, mx, best; };
    static S op(const S &a, const S &b) {
        if (a.mx < a.mn) { return b; }
        if (b.mx < b.mn) { return a; }
        return {min(a.mn, b.mn), max(a.mx, b.mx), max({a.best, b.best, T(b.mx - a.mn)})};}
    static S e() { return {std::numeric_limits<T>::max(), std::numeric_limits<T>::lowest(), T(0)}; }
    static S leaf(T x) { return {x, x, T(0)}; }
};

// T: O(1) per call, M: O(1); S {first, last, runs} counts maximal blocks of equal values, runs = 0 marks the identity.
template<typename T>
struct RunCount {
    struct S { T first, last; lng runs; };
    static S op(const S &a, const S &b) {
        if (!a.runs) { return b; }
        if (!b.runs) { return a; }
        return {a.first, b.last, a.runs + b.runs - (a.last == b.first)};}
    static S e() { return {T(), T(), 0}; }
    static S leaf(T x) { return {x, x, 1}; }
};

// T: O(1) per call, M: O(1); 0/1 sequences: length, ones, and the longest prefix, suffix and overall runs of ones.
struct LongestRun {
    struct S { lng len, ones, pre, suf, best; };
    static S op(const S &a, const S &b) {
        return {a.len + b.len, a.ones + b.ones, a.pre == a.len ? a.len + b.pre : a.pre, b.suf == b.len ? b.len + a.suf : b.suf, max({a.best, b.best, a.suf + b.pre})};}
    static S e() { return {0, 0, 0, 0, 0}; }
    static S leaf(bool bit) { return {1, bit, bit, bit, bit}; }
};

// T: O(1) per call, M: O(1); S {open, close} counts unmatched '(' and ')': balanced iff both are 0, and len - open - close characters match.
struct BracketBalance {
    struct S { lng open, close; };
    static S op(const S &a, const S &b) {
        lng k = min(a.open, b.close);
        return {a.open + b.open - k, a.close + b.close - k};}
    static S e() { return {0, 0}; }
    static S leaf(bool opening) { return opening ? S{1, 0} : S{0, 1}; }
};

// T: O(K^3) per op, M: O(K^2); T is a ring with T(0) and T(1), op(a, b) = a * b.
template<typename T, int K>
struct MatrixProduct {
    using S = array<array<T, K>, K>;
    static S op(const S &a, const S &b) {
        S res{};
        for (int i = 0; i < K; ++i) {
            for (int k = 0; k < K; ++k) {
                for (int j = 0; j < K; ++j) { res[i][j] += a[i][k] * b[k][j]; }}}
        return res;}
    static S e() {
        S res{};
        for (int i = 0; i < K; ++i) { res[i][i] = T(1); }
        return res;}
};

// T: O(K^3) per op, M: O(K^2); op(a, b)[i][j] = max over k of a[i][k] + b[k][j], numeric lowest means no entry, finite sums fit T.
template<typename T, int K>
struct MaxPlusMatrix {
    using S = array<array<T, K>, K>;
    static constexpr T NONE = std::numeric_limits<T>::lowest();
    static S op(const S &a, const S &b) {
        S res;
        for (auto &row : res) { row.fill(NONE); }
        for (int i = 0; i < K; ++i) {
            for (int k = 0; k < K; ++k) {
                if (a[i][k] == NONE) { continue; }
                for (int j = 0; j < K; ++j) {
                    if (b[k][j] != NONE) { res[i][j] = max(res[i][j], T(a[i][k] + b[k][j])); }}}}
        return res;}
    static S e() {
        S res;
        for (auto &row : res) { row.fill(NONE); }
        for (int i = 0; i < K; ++i) { res[i][i] = T(0); }
        return res;}
};

// T: O(1) per call, M: O(1); op is not associative: every bracketing yields the strict majority when one exists, else any value, so verify its count.
template<typename T>
struct MajorityVote {
    struct S { T x; lng cnt; };
    static S op(const S &a, const S &b) {
        if (a.x == b.x) { return {a.x, a.cnt + b.cnt}; }
        return a.cnt >= b.cnt ? S{a.x, a.cnt - b.cnt} : S{b.x, b.cnt - a.cnt};}
    static S e() { return {T(), 0}; }
    static S leaf(T x) { return {x, 1}; }
};

// T: O(1) plus f, M: O(1); op(a, b) = f(a, b) and e() = unit, f associative with two-sided identity unit.
template<typename T, typename Op>
struct Monoid {
    using S = T;
    T unit;
    [[no_unique_address]] Op f;
    S op(const S &a, const S &b) const { return f(a, b); }
    S e() const { return unit; }
};

// T: O(1) plus the functions, M: O(1); monoid M with act(f, x) as mapping, compose(f, g) (f after g) as composition and noop as id.
template<typename M, typename U, typename Map, typename Comp>
struct ActedMonoid : M {
    using S = typename M::S;
    using F = U;
    U noop;
    [[no_unique_address]] Map act;
    [[no_unique_address]] Comp compose;
    S mapping(const F &f, const S &x) const { return act(f, x); }
    F composition(const F &f, const F &g) const { return compose(f, g); }
    F id() const { return noop; }
};

// T: O(1) plus A and B, M: O(1); pairs of A and B values combined componentwise.
template<typename A, typename B>
struct PairMonoid {
    using S = pair<typename A::S, typename B::S>;
    [[no_unique_address]] A a;
    [[no_unique_address]] B b;
    S op(const S &x, const S &y) const { return {a.op(x.first, y.first), b.op(x.second, y.second)}; }
    S e() const { return {a.e(), b.e()}; }
};

// T: O(K) plus K calls of M, M: O(1); arrays of K default-constructible M values combined componentwise.
template<typename M, int K>
struct ArrayMonoid {
    using S = array<typename M::S, K>;
    [[no_unique_address]] M m;
    S op(const S &x, const S &y) const {
        S res = x;
        for (int i = 0; i < K; ++i) { res[i] = m.op(x[i], y[i]); }
        return res;}
    S e() const { S res; res.fill(m.e()); return res; }
};

// T: O(1) plus M, M: O(1); M with operands swapped, so products read right to left; an acted M keeps its action.
template<typename M>
struct Opposite : M {
    using S = typename M::S;
    S op(const S &a, const S &b) const { return M::op(b, a); }
};

// T: O(1) plus 2 calls of M, M: O(1); S {x, y} holds the product and the reversed product, reverse swaps them; an acted M acts on both.
template<typename M>
struct Reversible : M {
    struct S { typename M::S x, y; };
    S op(const S &a, const S &b) const { return {M::op(a.x, b.x), M::op(b.y, a.y)}; }
    S e() const { return {M::e(), M::e()}; }
    static S reverse(const S &a) { return {a.y, a.x}; }
    static S leaf(const typename M::S &v) { return {v, v}; }
    template<typename G>
    S mapping(const G &f, const S &a) const { return {M::mapping(f, a.x), M::mapping(f, a.y)}; }
};

// T: O(1) plus M, M: O(1); M's values as actions of a dual tree: F = S, composition(f, g) = op(g, f) applies g first, id = e.
template<typename M>
struct MonoidAction {
    using F = typename M::S;
    [[no_unique_address]] M m;
    F composition(const F &f, const F &g) const { return m.op(g, f); }
    F id() const { return m.e(); }
};

// T: O(1) per call, M: O(1); T is a commutative ring, len counts points.
template<typename T>
struct RangeAddRangeSum : monoid_detail::SumLen<T> {
    using S = typename monoid_detail::SumLen<T>::S;
    using F = T;
    static S mapping(const F &f, const S &x) { return {x.sum + f * x.len, x.len}; }
    static F composition(const F &f, const F &g) { return f + g; }
    static F id() { return T(0); }
    static F add(T x) { return x; }
};

// T: O(1) per call, M: O(1); values stay below numeric max, which is the identity.
template<typename T>
struct RangeAddRangeMin : Min<T> {
    using S = T;
    using F = T;
    static S mapping(const F &f, const S &x) { return x == Min<T>::e() ? x : x + f; }
    static F composition(const F &f, const F &g) { return f + g; }
    static F id() { return T(0); }
    static F add(T x) { return x; }
};

// T: O(1) per call, M: O(1); values stay above numeric lowest, which is the identity.
template<typename T>
struct RangeAddRangeMax : Max<T> {
    using S = T;
    using F = T;
    static S mapping(const F &f, const S &x) { return x == Max<T>::e() ? x : x + f; }
    static F composition(const F &f, const F &g) { return f + g; }
    static F id() { return T(0); }
    static F add(T x) { return x; }
};

// T: O(1) per call, M: O(1); leftmost minimum under Less (RangeAddRangeMaxIndex: maximum) as in MinIndex.
template<typename T, typename Less = std::less<T>>
struct RangeAddRangeMinIndex : MinIndex<T, Less> {
    using S = typename MinIndex<T, Less>::S;
    using F = T;
    static S mapping(const F &f, const S &a) { return a.i < 0 ? a : S{a.x + f, a.i}; }
    static F composition(const F &f, const F &g) { return f + g; }
    static F id() { return T(0); }
    static F add(T x) { return x; }
};
template<typename T> using RangeAddRangeMaxIndex = RangeAddRangeMinIndex<T, std::greater<T>>;

// T: O(1) per call, M: O(1); minimum under Less (RangeAddRangeMaxCount: maximum) and its multiplicity as in MinCount.
template<typename T, typename Less = std::less<T>>
struct RangeAddRangeMinCount : MinCount<T, Less> {
    using S = typename MinCount<T, Less>::S;
    using F = T;
    static S mapping(const F &f, const S &a) { return a.cnt ? S{a.x + f, a.cnt} : a; }
    static F composition(const F &f, const F &g) { return f + g; }
    static F id() { return T(0); }
    static F add(T x) { return x; }
};
template<typename T> using RangeAddRangeMaxCount = RangeAddRangeMinCount<T, std::greater<T>>;

// T: O(1) per call, M: O(1); T is a commutative ring, F {a, b} maps x to a * x + b.
template<typename T>
struct RangeAffineRangeSum : monoid_detail::SumLen<T> {
    using S = typename monoid_detail::SumLen<T>::S;
    using F = typename AffineComposite<T>::S;
    static S mapping(const F &f, const S &x) { return {f.a * x.sum + f.b * x.len, x.len}; }
    static F composition(const F &f, const F &g) { return AffineComposite<T>::op(g, f); }
    static F id() { return AffineComposite<T>::e(); }
    static F affine(T a, T b) { return {a, b}; }
};

// T: O(1) per call, M: O(1); T is a commutative ring, S {sum, sq, len} with sq the sum of squares, F {a, b} maps x to a * x + b.
template<typename T>
struct RangeAffineRangeSumSquares {
    struct S { T sum, sq, len; };
    using F = typename AffineComposite<T>::S;
    static S op(const S &x, const S &y) { return {x.sum + y.sum, x.sq + y.sq, x.len + y.len}; }
    static S e() { return {T(0), T(0), T(0)}; }
    static S mapping(const F &f, const S &x) { return {f.a * x.sum + f.b * x.len, f.a * f.a * x.sq + T(2) * f.a * f.b * x.sum + f.b * f.b * x.len, x.len}; }
    static F composition(const F &f, const F &g) { return AffineComposite<T>::op(g, f); }
    static F id() { return AffineComposite<T>::e(); }
    static S leaf(T x) { return {x, x * x, T(1)}; }
    static F affine(T a, T b) { return {a, b}; }
};

// T: O(1) per call, M: O(1); affine maps with any sign, leftmost argmin/argmax, len = 0 marks the identity.
template<typename T>
struct RangeAffineRangeSumMinMaxIndex {
    struct S { T sum, mx, mn; lng len, lo, mxi, mni; };
    using F = typename AffineComposite<T>::S;
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
    static F composition(const F &f, const F &g) { return AffineComposite<T>::op(g, f); }
    static F id() { return AffineComposite<T>::e(); }
    static S leaf(lng i, T x) { return {x, x, x, 1, i, i, i}; }
    static F affine(T a, T b) { return {a, b}; }
};

// T: O(1) per call, M: O(1); F {assign, x} assigns x when assign.
template<typename T>
struct RangeAssignRangeSum : monoid_detail::SumLen<T> {
    using S = typename monoid_detail::SumLen<T>::S;
    struct F { bool assign; T x; };
    static S mapping(const F &f, const S &x) { return f.assign ? S{f.x * x.len, x.len} : x; }
    static F composition(const F &f, const F &g) { return f.assign ? f : g; }
    static F id() { return {false, T(0)}; }
    static F assign(T x) { return {true, x}; }
};

// T: O(log(len)) for mapping and O(1) otherwise, M: O(1); S {a, b} is the composite x -> a * x + b, leftmost applied first.
template<typename T>
struct RangeAssignRangeComposite {
    struct S { T a, b; lng len; };
    struct F { bool assign; T a, b; };
    static S op(const S &p, const S &q) { return {q.a * p.a, q.a * p.b + q.b, p.len + q.len}; }
    static S e() { return {T(1), T(0), 0}; }
    static S mapping(const F &f, const S &x) { return f.assign ? monoid_detail::power(RangeAssignRangeComposite(), S{f.a, f.b, 1}, x.len) : x; }
    static F composition(const F &f, const F &g) { return f.assign ? f : g; }
    static F id() { return {false, T(1), T(0)}; }
    static S leaf(T a, T b) { return {a, b, 1}; }
    static F assign(T a, T b) { return {true, a, b}; }
};

// T: O(1) per call, M: O(1); MaxSubarray under assignment, F {assign, x}.
template<typename T>
struct RangeAssignRangeMaxSubarray : MaxSubarray<T> {
    using S = typename MaxSubarray<T>::S;
    struct F { bool assign; T x; };
    static S mapping(const F &f, const S &x) {
        if (!f.assign || !x.len) { return x; }
        T sum = f.x * T(x.len), top = f.x > T(0) ? sum : f.x;
        return {sum, top, top, top, x.len};}
    static F composition(const F &f, const F &g) { return f.assign ? f : g; }
    static F id() { return {false, T(0)}; }
    static F assign(T x) { return {true, x}; }
};

// T: O(1) per call, M: O(1); RunCount under assignment, F {assign, x}.
template<typename T>
struct RangeAssignRangeRunCount : RunCount<T> {
    using S = typename RunCount<T>::S;
    struct F { bool assign; T x; };
    static S mapping(const F &f, const S &x) { return f.assign && x.runs ? S{f.x, f.x, 1} : x; }
    static F composition(const F &f, const F &g) { return f.assign ? f : g; }
    static F id() { return {false, T()}; }
    static F assign(T x) { return {true, x}; }
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

// T: O(1) per call, M: O(1); F as BitwiseComposite maps x to (x & a) ^ b, len = 0 marks the identity.
template<typename T = ulng>
struct RangeAndOrXorRangeAndOrXor {
    struct S { T band, bor, bxor; lng len; };
    using F = typename BitwiseComposite<T>::S;
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
    static F composition(const F &f, const F &g) { return BitwiseComposite<T>::op(g, f); }
    static F id() { return BitwiseComposite<T>::e(); }
    static S leaf(T x) { return {x, x, x, 1}; }
    static F andWith(T x) { return BitwiseComposite<T>::andWith(x); }
    static F orWith(T x) { return BitwiseComposite<T>::orWith(x); }
    static F xorWith(T x) { return BitwiseComposite<T>::xorWith(x); }
    static F assign(T x) { return BitwiseComposite<T>::assign(x); }
};

// T: O(B) per call, M: O(B); values in [0, 2^B), S counts set bits per position, sum(s) is the range sum (modulo 2^w for unsigned T); F as BitwiseComposite.
template<typename T = ulng, int B = std::numeric_limits<std::make_unsigned_t<T>>::digits>
struct RangeAndOrXorRangeSum {
    struct S { array<lng, B> cnt; lng len; };
    using F = typename BitwiseComposite<T>::S;
    static S op(const S &x, const S &y) {
        S res{{}, x.len + y.len};
        for (int k = 0; k < B; ++k) { res.cnt[k] = x.cnt[k] + y.cnt[k]; }
        return res;}
    static S e() { return {{}, 0}; }
    static S mapping(const F &f, const S &x) {
        S res = x;
        for (int k = 0; k < B; ++k) {
            if (!(f.a >> k & 1)) { res.cnt[k] = f.b >> k & 1 ? x.len : 0; }
            else if (f.b >> k & 1) { res.cnt[k] = x.len - x.cnt[k]; }}
        return res;}
    static F composition(const F &f, const F &g) { return BitwiseComposite<T>::op(g, f); }
    static F id() { return BitwiseComposite<T>::e(); }
    static S leaf(T x) {
        S res{{}, 1};
        for (int k = 0; k < B; ++k) { res.cnt[k] = x >> k & 1; }
        return res;}
    static T sum(const S &x) {
        T res = T(0);
        for (int k = 0; k < B; ++k) { res = T(res + T(T(x.cnt[k]) << k)); }
        return res;}
    static F andWith(T x) { return BitwiseComposite<T>::andWith(x); }
    static F orWith(T x) { return BitwiseComposite<T>::orWith(x); }
    static F xorWith(T x) { return BitwiseComposite<T>::xorWith(x); }
    static F assign(T x) { return BitwiseComposite<T>::assign(x); }
};

// T: O(log(max)) per call, M: O(1); values >= 0, F {a, b} maps x to lcm(gcd(x, a), b), l tracks lcm only when LCM.
template<typename T, bool LCM = false>
struct RangeGcdLcmRangeGcdLcm {
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

// T: O(1) per call, M: O(1); MinMax under the AddChminChmax clamp maps (monotone, so exact on min and max).
template<typename T>
struct RangeAddChminChmaxRangeMinMax : MinMax<T> {
    using S = typename MinMax<T>::S;
    using F = typename AddChminChmax<T>::S;
    static S mapping(const F &f, const S &x) { return x.mx < x.mn ? x : S{AddChminChmax<T>::eval(f, x.mn), AddChminChmax<T>::eval(f, x.mx)}; }
    static F composition(const F &f, const F &g) { return AddChminChmax<T>::op(g, f); }
    static F id() { return AddChminChmax<T>::e(); }
    static F add(T x) { return AddChminChmax<T>::add(x); }
    static F chmin(T x) { return AddChminChmax<T>::chmin(x); }
    static F chmax(T x) { return AddChminChmax<T>::chmax(x); }
    static F assign(T x) { return AddChminChmax<T>::assign(x); }
};

// T: O(1) per call, M: O(1); 0/1 sequences, S counts zeros, ones and inversions (a one before a zero), F = true flips every bit.
struct RangeFlipRangeInversion {
    struct S { lng zero, one, inv; };
    using F = bool;
    static S op(const S &a, const S &b) { return {a.zero + b.zero, a.one + b.one, a.inv + b.inv + a.one * b.zero}; }
    static S e() { return {0, 0, 0}; }
    static S mapping(const F &f, const S &x) { return f ? S{x.one, x.zero, x.zero * x.one - x.inv} : x; }
    static F composition(const F &f, const F &g) { return f != g; }
    static F id() { return false; }
    static S leaf(bool bit) { return bit ? S{0, 1, 0} : S{1, 0, 0}; }
    static F flip() { return true; }
};

// T: O(1) plus M, O(log(len)) calls of M when assigning, M: O(1); M folds 0/1 sequences with static leaf(bit); S keeps the fold of the bits and of their complement; F {assign, flip} sets every bit to assign (unless -1), then flips.
template<typename M>
struct RangeAssignFlip {
    struct S { typename M::S x, y; lng len; };
    struct F { int assign; bool flip; };
    [[no_unique_address]] M m;
    S op(const S &a, const S &b) const { return {m.op(a.x, b.x), m.op(a.y, b.y), a.len + b.len}; }
    S e() const { return {m.e(), m.e(), 0}; }
    S mapping(const F &f, const S &a) const {
        if (f.assign < 0) { return f.flip ? S{a.y, a.x, a.len} : a; }
        bool bit = bool(f.assign) != f.flip;
        return {monoid_detail::power(m, M::leaf(bit), a.len), monoid_detail::power(m, M::leaf(!bit), a.len), a.len};}
    static F composition(const F &f, const F &g) { return f.assign >= 0 ? f : F{g.assign, g.flip != f.flip}; }
    static F id() { return {-1, false}; }
    static S leaf(bool bit) { return {M::leaf(bit), M::leaf(!bit), 1}; }
    static F assign(bool bit) { return {int(bit), false}; }
    static F flip() { return {-1, true}; }
};

// T: O(K^2) mapping, O(K^3) composition, O(K) otherwise, M: O(K^2); T is a commutative ring, S sums K-vectors, F is a K x K matrix acting as F * x.
template<typename T, int K>
struct RangeLinearRangeSum {
    using S = array<T, K>;
    using F = typename MatrixProduct<T, K>::S;
    static S op(const S &x, const S &y) {
        S res = x;
        for (int i = 0; i < K; ++i) { res[i] += y[i]; }
        return res;}
    static S e() { return S{}; }
    static S mapping(const F &f, const S &x) {
        S res{};
        for (int i = 0; i < K; ++i) {
            for (int j = 0; j < K; ++j) { res[i] += f[i][j] * x[j]; }}
        return res;}
    static F composition(const F &f, const F &g) { return MatrixProduct<T, K>::op(f, g); }
    static F id() { return MatrixProduct<T, K>::e(); }
};
