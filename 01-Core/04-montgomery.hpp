#pragma once
#include "01-template.hpp"
#ifdef __AVX2__
#include <immintrin.h>
#endif

// Odd modulus 1 <= mod < R=2^BITS, including the full unsigned word range.
// mod/inv/rsq are immutable after construction. inv=-mod^-1 mod R; rsq=R^2 mod mod.
// init accepts any ordinary word; get converts a canonical Montgomery residue.
// red(x) requires x < mod*R and returns x/R modulo mod in [0,mod).
// mul/add/sub/powMont operate on canonical Montgomery residues (add/sub are the
// same formulas for ordinary residues); pow is ordinary in/out.
// redLazy requires mod<R/2 and x<mod*R, returning [0,2*mod). normalize accepts
// that interval. mulLazy/addLazy/subLazy accept [0,2*mod) operands with mod<R/4
// and return [0,2*mod), so chains stay in the REDC domain. Canonical operations
// support every odd modulus.
// For even moduli use Barrett32/Barrett64 from 03-barrett.hpp instead.
// Bulk pointers permit exact output/input aliasing, but no partial overlap;
// n>=0, arrays have n words, and null pointers are permitted only for n=0.
// Scalar bulk loops run the unchecked core redc that every scalar operation
// shares; the AVX2 loops run red4/red8. A Guard accumulates operand ranges
// branch-free and is asserted once after each loop, so checked builds keep the
// kernels' cost and NDEBUG drops it entirely.
// S: O(1), U: NA, Q: O(1), M: O(1); bulk O(n), powers O(log(e+1)) for exponent e.
template<typename T>
struct MontgomeryBackend {
    static_assert(std::is_same_v<T, uint> || std::is_same_v<T, ulng>);
    using Wide = std::conditional_t<std::is_same_v<T, uint>, ulng, ulll>;
    static constexpr int BITS = 8 * sizeof(T);
    T mod, inv, rsq;

    MontgomeryBackend(T m = 1) : mod(m), inv(m) {
        assert(m & 1);
        for (int i = 3; i < BITS; i *= 2) { inv *= 2 - mod * inv; }
        inv = -inv; rsq = T(-Wide(mod) % mod);}

    // Unchecked REDC of x < mod*R: (x + q*mod)/R = h + p + carry with the carry
    // set exactly when low(x) != 0. The sum wraps past R only when it is >= R,
    // which r < h detects; canonical output then subtracts mod, as for r >= mod.
    T redc(Wide x, bool lazy) const {
        T q = T(x) * inv, h = T(x >> BITS), p = T((Wide(q) * mod) >> BITS);
        T r = h + p + (T(x) != 0);
        return lazy ? r : r - ((r < h || r >= mod) ? mod : 0);}
    T red(Wide x) const { assert(T(x >> BITS) < mod); return redc(x, false); }
    T init(T x) const { return redc(Wide(x) * rsq, false); }
    T get(T x) const { assert(x < mod); return redc(x, false); }
    T mul(T a, T b) const { assert(a < mod && b < mod); return redc(Wide(a) * b, false); }
    // The sum is < 2*mod; it wraps past R only when >= R, detected by s < a.
    T add(T a, T b) const {
        assert(a < mod && b < mod);
        T s = a + b;
        return s - ((s < a || s >= mod) ? mod : 0);}
    T sub(T a, T b) const { assert(a < mod && b < mod); return a - b + (a < b ? mod : 0); }

    T redLazy(Wide x) const {
        assert(mod < (T(1) << (BITS - 1)) && T(x >> BITS) < mod);
        return redc(x, true);}
    T normalize(T x) const {
        assert(mod < (T(1) << (BITS - 1)) && x < 2 * mod);
        return x - (x >= mod ? mod : 0);}
    T mulLazy(T a, T b) const {
        assert(mod < (T(1) << (BITS - 2)) && a < 2 * mod && b < 2 * mod);
        return redc(Wide(a) * b, true);}
    T addLazy(T a, T b) const {
        assert(mod < (T(1) << (BITS - 2)) && a < 2 * mod && b < 2 * mod);
        T s = a + b;
        return s - (s >= 2 * mod ? 2 * mod : 0);}
    T subLazy(T a, T b) const {
        assert(mod < (T(1) << (BITS - 2)) && a < 2 * mod && b < 2 * mod);
        T d = a - b + 2 * mod;
        return d - (d >= 2 * mod ? 2 * mod : 0);}

    T powMont(T a, ulng e) const {
        assert(a < mod);
        T r = init(1);
        while (e) {
            if (e & 1) { r = mul(r, a); }
            e >>= 1;
            if (e) { a = mul(a, a); }}
        return r;}
    T pow(T a, ulng e) const { return get(powMont(init(a), e)); }

#ifdef __AVX2__
    // Internal ISA kernels, used through the checked bulk APIs below.
    static __m256i load(const T *p) { return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p)); }
    static void store(T *p, __m256i x) { _mm256_storeu_si256(reinterpret_cast<__m256i *>(p), x); }
    // Four independent wide products. The conceptual 65-bit sum is represented
    // by its 33-bit high part, so signed 64-bit comparisons are safe.
    __m256i red4(__m256i x, bool lazy) const {
        static_assert(BITS == 32);
        __m256i m = _mm256_set1_epi64x(mod), v = _mm256_set1_epi64x(inv);
        __m256i q = _mm256_mul_epu32(x, v), p = _mm256_mul_epu32(q, m);
        __m256i lo = _mm256_and_si256(x, _mm256_set1_epi64x(0xffffffffLL));
        __m256i c = _mm256_andnot_si256(_mm256_cmpeq_epi64(lo, _mm256_setzero_si256()), _mm256_set1_epi64x(1));
        __m256i r = _mm256_add_epi64(_mm256_add_epi64(_mm256_srli_epi64(x, 32), _mm256_srli_epi64(p, 32)), c);
        if (!lazy) {
            __m256i ge = _mm256_cmpgt_epi64(r, _mm256_sub_epi64(m, _mm256_set1_epi64x(1)));
            r = _mm256_sub_epi64(r, _mm256_and_si256(ge, m));}
        return r;}
    // Reduces the even- and odd-lane wide products and packs eight 32-bit results.
    __m256i red8(__m256i even, __m256i odd, bool lazy) const {
        return _mm256_or_si256(red4(even, lazy), _mm256_slli_epi64(red4(odd, lazy), 32));}
    __m256i mul8(__m256i a, __m256i b, bool lazy) const {
        return red8(_mm256_mul_epu32(a, b), _mm256_mul_epu32(_mm256_srli_epi64(a, 32), _mm256_srli_epi64(b, 32)), lazy);}
#endif

    // Deferred operand check for bulk loops: note keeps the running maximum of
    // words or 32-bit lanes; ok, read by one assertion after the loop, is true
    // when every noted value was below top >= 1.
    // T: O(1) per note and ok, M: O(1).
    struct Guard {
        T top, mx = 0;
        void note(T x) { mx = max(mx, x); }
#ifdef __AVX2__
        __m256i acc = _mm256_setzero_si256();
        void note(__m256i x) { static_assert(BITS == 32); acc = _mm256_max_epu32(acc, x); }
#endif
        bool ok() const {
            bool fine = mx < top;
#ifdef __AVX2__
            if constexpr (BITS == 32) { // max(lane, top-1) == top-1 for every lane iff all lanes < top
                __m256i t = _mm256_set1_epi32(int(top - 1)), over = _mm256_xor_si256(_mm256_max_epu32(acc, t), t);
                fine = fine && _mm256_testz_si256(over, over);}
#endif
            return fine;}
    };

    void mul(const T *a, const T *b, T *out, int n) const {
        assert(n >= 0 && (!n || (a && b && out)));
        Guard g{mod}; int i = 0;
#ifdef __AVX2__
        if constexpr (BITS == 32) {
            for (; n - i >= 8; i += 8) {
                __m256i x = load(a + i), y = load(b + i);
                g.note(x); g.note(y); store(out + i, mul8(x, y, false));}}
#endif
        for (; i < n; i++) { g.note(a[i]); g.note(b[i]); out[i] = redc(Wide(a[i]) * b[i], false); }
        assert(g.ok());}
    void mulLazy(const T *a, const T *b, T *out, int n) const {
        assert(mod < (T(1) << (BITS - 2)) && n >= 0 && (!n || (a && b && out)));
        Guard g{T(2 * mod)}; int i = 0;
#ifdef __AVX2__
        if constexpr (BITS == 32) {
            for (; n - i >= 8; i += 8) {
                __m256i x = load(a + i), y = load(b + i);
                g.note(x); g.note(y); store(out + i, mul8(x, y, true));}}
#endif
        for (; i < n; i++) { g.note(a[i]); g.note(b[i]); out[i] = redc(Wide(a[i]) * b[i], true); }
        assert(g.ok());}

    void init(const T *a, T *out, int n) const {
        assert(n >= 0 && (!n || (a && out)));
        int i = 0;
#ifdef __AVX2__
        if constexpr (BITS == 32) {
            __m256i r = _mm256_set1_epi32(int(rsq));
            for (; n - i >= 8; i += 8) { store(out + i, mul8(load(a + i), r, false)); }}
#endif
        for (; i < n; i++) { out[i] = redc(Wide(a[i]) * rsq, false); }}
    void get(const T *a, T *out, int n) const {
        assert(n >= 0 && (!n || (a && out)));
        Guard g{mod}; int i = 0;
#ifdef __AVX2__
        if constexpr (BITS == 32) {
            __m256i low = _mm256_set1_epi64x(0xffffffffLL);
            for (; n - i >= 8; i += 8) {
                __m256i x = load(a + i);
                g.note(x); store(out + i, red8(_mm256_and_si256(x, low), _mm256_srli_epi64(x, 32), false));}}
#endif
        for (; i < n; i++) { g.note(a[i]); out[i] = redc(a[i], false); }
        assert(g.ok());}
};

using Montgomery32 = MontgomeryBackend<uint>;
using Montgomery64 = MontgomeryBackend<ulng>;
using Montgomery = Montgomery64;
