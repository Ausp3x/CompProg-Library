#pragma once
#include "01-template.hpp"
#ifdef __AVX2__
#include <immintrin.h>
#endif

// Odd modulus 1 <= mod < R=2^BITS, including the full unsigned word range.
// mod/inv/rsq are immutable after construction. inv=-mod^-1 mod R; rsq=R^2 mod mod.
// init accepts any ordinary word; get converts a canonical Montgomery residue.
// red(x) requires x < mod*R and returns x/R modulo mod in [0,mod).
// mul/powMont operate on canonical Montgomery residues; pow is ordinary in/out.
// redLazy requires mod<R/2 and x<mod*R, returning [0,2*mod). normalize accepts
// that interval. mulLazy accepts [0,2*mod) operands with mod<R/4, so chaining
// preserves the REDC domain. Canonical operations support every odd modulus.
// For even moduli use Barrett32/Barrett64 from 03-barrett.hpp instead.
// Bulk pointers permit exact output/input aliasing, but no partial overlap;
// n>=0, arrays have n words, and null pointers are permitted only for n=0.
// S: O(1), U: NA, Q: O(1), M: O(1); bulk O(n), powers O(log(e+1)).
template<typename T>
struct MontgomeryBackend {
    static_assert(std::is_same_v<T, uint> || std::is_same_v<T, ulng>);
    using Wide = std::conditional_t<std::is_same_v<T, uint>, ulng, ulll>;
    static constexpr int BITS = 8 * sizeof(T);
    T mod, inv, rsq;

    MontgomeryBackend(T n = 1) : mod(n), inv(n) {
        assert(n & 1);
        for (int i = 3; i < BITS; i *= 2) { inv *= 2 - mod * inv; }
        inv = -inv; rsq = -Wide(mod) % mod;}

    T red(Wide x) const {
        assert(T(x >> BITS) < mod);
        T q = T(x) * inv, h = T(x >> BITS), p = T((Wide(q) * mod) >> BITS);
        T r = h + p + (T(x) != 0);
        return r - ((r < h || r >= mod) ? mod : 0);}
    T init(T x) const { return red(Wide(x) * rsq); }
    T get(T x) const { assert(x < mod); return red(x); }
    T mul(T a, T b) const {
        assert(a < mod && b < mod);
        return red(Wide(a) * b);}

    T redLazy(Wide x) const {
        assert(mod < (T(1) << (BITS - 1)) && T(x >> BITS) < mod);
        T q = T(x) * inv;
        return T(x >> BITS) + T((Wide(q) * mod) >> BITS) + (T(x) != 0);}
    T normalize(T x) const {
        assert(mod < (T(1) << (BITS - 1)) && x < 2 * mod);
        return x - (x >= mod ? mod : 0);}
    T mulLazy(T a, T b) const {
        assert(mod < (T(1) << (BITS - 2)) && a < 2 * mod && b < 2 * mod);
        return redLazy(Wide(a) * b);}

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
    // Four independent wide products. The conceptual 65-bit sum is represented
    // by its 33-bit high part, so signed 64-bit comparisons are safe.
    __m256i red4(__m256i x, bool lazy) const {
        static_assert(BITS == 32);
        __m256i m = _mm256_set1_epi64x(mod), v = _mm256_set1_epi64x(inv);
        __m256i q = _mm256_mul_epu32(x, v), p = _mm256_mul_epu32(q, m);
        __m256i lo = _mm256_and_si256(x, _mm256_set1_epi64x(0xffffffffULL));
        __m256i c = _mm256_andnot_si256(_mm256_cmpeq_epi64(lo, _mm256_setzero_si256()),
                                      _mm256_set1_epi64x(1));
        __m256i r = _mm256_add_epi64(_mm256_add_epi64(_mm256_srli_epi64(x, 32),
                                                    _mm256_srli_epi64(p, 32)), c);
        if (!lazy) {
            __m256i ge = _mm256_cmpgt_epi64(r, _mm256_sub_epi64(m, _mm256_set1_epi64x(1)));
            r = _mm256_sub_epi64(r, _mm256_and_si256(ge, m));}
        return r;}
    __m256i mul8(__m256i a, __m256i b, bool lazy) const {
        static_assert(BITS == 32);
        __m256i even = red4(_mm256_mul_epu32(a, b), lazy);
        __m256i odd = red4(_mm256_mul_epu32(_mm256_srli_epi64(a, 32),
                                         _mm256_srli_epi64(b, 32)), lazy);
        return _mm256_or_si256(even, _mm256_slli_epi64(odd, 32));}
#endif

    void mul(const T *a, const T *b, T *out, int n) const {
        assert(n >= 0 && (!n || (a && b && out)));
        int i = 0;
#ifdef __AVX2__
        if constexpr (BITS == 32) {
            for (; n - i >= 8; i += 8) {
                for (int j = 0; j < 8; j++) { assert(a[i + j] < mod && b[i + j] < mod); }
                __m256i x = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(a + i));
                __m256i y = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(b + i));
                _mm256_storeu_si256(reinterpret_cast<__m256i *>(out + i), mul8(x, y, false)); }}
#endif
        for (; i < n; i++) { out[i] = mul(a[i], b[i]); }}
    void mulLazy(const T *a, const T *b, T *out, int n) const {
        assert(mod < (T(1) << (BITS - 2)) && n >= 0 && (!n || (a && b && out)));
        int i = 0;
#ifdef __AVX2__
        if constexpr (BITS == 32) {
            for (; n - i >= 8; i += 8) {
                for (int j = 0; j < 8; j++) { assert(a[i + j] < 2 * mod && b[i + j] < 2 * mod); }
                __m256i x = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(a + i));
                __m256i y = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(b + i));
                _mm256_storeu_si256(reinterpret_cast<__m256i *>(out + i), mul8(x, y, true)); }}
#endif
        for (; i < n; i++) { out[i] = mulLazy(a[i], b[i]); }}

    void init(const T *a, T *out, int n) const {
        assert(n >= 0 && (!n || (a && out)));
        int i = 0;
#ifdef __AVX2__
        if constexpr (BITS == 32) {
            for (; n - i >= 8; i += 8) {
                __m256i x = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(a + i));
                _mm256_storeu_si256(reinterpret_cast<__m256i *>(out + i), mul8(x, _mm256_set1_epi32(int(rsq)), false)); }}
#endif
        for (; i < n; i++) { out[i] = init(a[i]); }}
    void get(const T *a, T *out, int n) const {
        assert(n >= 0 && (!n || (a && out)));
        int i = 0;
#ifdef __AVX2__
        if constexpr (BITS == 32) {
            for (; n - i >= 8; i += 8) {
                for (int j = 0; j < 8; j++) { assert(a[i + j] < mod); }
                __m256i x = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(a + i));
                __m256i even = red4(_mm256_and_si256(x, _mm256_set1_epi64x(0xffffffffULL)), false);
                __m256i odd = red4(_mm256_srli_epi64(x, 32), false);
                _mm256_storeu_si256(reinterpret_cast<__m256i *>(out + i),
                                   _mm256_or_si256(even, _mm256_slli_epi64(odd, 32))); }}
#endif
        for (; i < n; i++) { out[i] = get(a[i]); }}
};

using Montgomery32 = MontgomeryBackend<uint>;
using Montgomery64 = MontgomeryBackend<ulng>;
using Montgomery = Montgomery64;
