#pragma once
#include "01-template.hpp"
#ifdef __AVX2__
#include <immintrin.h>
#endif

// S: O(1), U: NA, Q: O(1), M: O(1); bulk O(n), powers O(log(e + 1)) for exponent e.
// Odd modulus in [1, 2^BITS), default 1; canonical ops in [0, mod); lazy ops need mod < R/4 (redLazy R/2), return [0, 2 * mod).
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

    // The carry is set iff low(x) != 0; r < h detects the wrap past R.
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
    static __m256i load(const T *p) { return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p)); }
    static void store(T *p, __m256i x) { _mm256_storeu_si256(reinterpret_cast<__m256i *>(p), x); }
    // The 65-bit sum is kept as its 33-bit high part, so signed 64-bit comparisons are safe.
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
    __m256i red8(__m256i even, __m256i odd, bool lazy) const {
        return _mm256_or_si256(red4(even, lazy), _mm256_slli_epi64(red4(odd, lazy), 32));}
    __m256i mul8(__m256i a, __m256i b, bool lazy) const {
        return red8(_mm256_mul_epu32(a, b), _mm256_mul_epu32(_mm256_srli_epi64(a, 32), _mm256_srli_epi64(b, 32)), lazy);}
#endif

    // T: O(1) per note and ok, M: O(1).
    // Deferred bulk operand check: ok is true iff every noted word or 32-bit lane was below top >= 1.
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
