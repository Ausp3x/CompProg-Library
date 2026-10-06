#pragma once
#include "01-template.hpp"
#ifdef __AVX2__
#include <immintrin.h>
#endif

// Nonzero moduli: [1, 2^32) / [1, 2^64); the default is one. Do not modify
// reducer fields: mod, mu (reciprocal; zero on special paths) and the mask flag
// (power-of-two modulus). divMod/div/reduce accept every 64/128-bit dividend
// and return the exact quotient (same width as the dividend) and remainder;
// mul/pow accept arbitrary words. Results are canonical [0, mod); mod=1 returns
// zero even for exponent=0. For all other moduli a^0=1, including a=0.
// Generic reduction uses mu=floor(2^(2w)/mod), equal to
// floor((2^(2w)-1)/mod) for non-powers of two. Special paths leave mu=0.
// q=high(x*mu) is the true quotient or one less, so the
// unreduced remainder is <2mod and one subtraction suffices. Keep w+1 bits.
// Pointer mul: n>=0; buffers contain n words, are pairwise disjoint or have
// exactly equal starts (in-place is supported); no alignment requirement.
// Null pointers are allowed only for n=0. Stored fields are immutable state;
// copies/moves are independent. No allocation, caches or global state.
// S: O(1), Q: O(1), M: O(1); pow O(1 + log(e + 1)) for exponent e, pointer mul O(n).
struct Barrett32 {
    uint mod;
    ulng mu;
    bool mask;

    explicit Barrett32(uint m = 1) : mod(m), mu(0), mask((m & (m - 1)) == 0) {
        assert(m);
        if (!mask) { mu = ~ulng(0) / m; }}

    pair<ulng, uint> divMod(ulng x) const {
        if (mask) { return {x >> std::countr_zero(mod), uint(x) & (mod - 1)}; }
        ulng q = ulng(ulll(x) * mu >> 64), r = x - q * mod;
        bool over = r >= mod;
        return {q + over, uint(over ? r - mod : r)};}
    ulng div(ulng x) const { return divMod(x).first; }
    uint reduce(ulng x) const { return divMod(x).second; }
    uint mul(uint a, uint b) const { return reduce(ulng(a) * b); }
    uint pow(uint a, ulng e) const {
        uint r = uint(mod != 1); a = reduce(a);
        for (; e > 1; e >>= 1, a = mul(a, a)) { if (e & 1) { r = mul(r, a); } }
        return e ? mul(r, a) : r;}

#ifdef __AVX2__
    static __m256i load(const uint *p) { return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p)); }
    static void store(uint *p, __m256i x) { _mm256_storeu_si256(reinterpret_cast<__m256i *>(p), x); }
#endif
    void mul(const uint *a, const uint *b, uint *out, int n) const {
        assert(n >= 0 && (!n || (a && b && out)));
        int i = 0;
#ifdef __AVX2__
        if (mask) {
            __m256i m = _mm256_set1_epi32(int(mod - 1));
            for (; n - i >= 8; i += 8) { store(out + i, _mm256_and_si256(_mm256_mullo_epi32(load(a + i), load(b + i)), m)); }}
#endif
        for (; i < n; ++i) { out[i] = mul(a[i], b[i]); }}

    // Fixed ordinary multiplier (Shoup): value=b%mod, mu=floor(value*2^32/mod).
    // a*mu/2^32 underestimates a*value/mod by <1: the floored quotient is
    // at most one short and the residual is <2mod.
    // Same input, output, state and pointer contracts as the parent reducer.
    // S: O(1), Q: O(1), M: O(1); pointer mul O(n).
    struct Multiplier {
        uint mod, value, mu;
        bool mask;

        Multiplier(const Barrett32 &r, uint b) : mod(r.mod), value(r.reduce(b)),
            mu(r.mask ? 0 : uint((ulng(value) << 32) / mod)), mask(r.mask) {}

        uint mul(uint a) const {
            if (mask) { return (a * value) & (mod - 1); }
            ulng q = ulng(a) * mu >> 32, r = ulng(a) * value - q * mod;
            return uint(r >= mod ? r - mod : r);}

        void mul(const uint *a, uint *out, int n) const {
            assert(n >= 0 && (!n || (a && out)));
            int i = 0;
#ifdef __AVX2__
            if (mask) {
                __m256i b = _mm256_set1_epi32(int(value)), m = _mm256_set1_epi32(int(mod - 1));
                for (; n - i >= 8; i += 8) { store(out + i, _mm256_and_si256(_mm256_mullo_epi32(load(a + i), b), m)); }}
            else if (n >= 8) {
                __m256i b = _mm256_set1_epi64x(value), c = _mm256_set1_epi64x(mu);
                __m256i m = _mm256_set1_epi64x(mod), mm = _mm256_set1_epi64x(ulng(mod) - 1);
                for (; n - i >= 4; i += 4) {
                    __m256i x = _mm256_cvtepu32_epi64(_mm_loadu_si128(reinterpret_cast<const __m128i *>(a + i)));
                    __m256i q = _mm256_srli_epi64(_mm256_mul_epu32(x, c), 32);
                    __m256i r = _mm256_sub_epi64(_mm256_mul_epu32(x, b), _mm256_mul_epu32(q, m));
                    r = _mm256_sub_epi64(r, _mm256_and_si256(m, _mm256_cmpgt_epi64(r, mm)));
                    r = _mm256_permutevar8x32_epi32(r, _mm256_setr_epi32(0, 2, 4, 6, 0, 0, 0, 0));
                    _mm_storeu_si128(reinterpret_cast<__m128i *>(out + i), _mm256_castsi256_si128(r));}}
#endif
            for (; i < n; ++i) { out[i] = mul(a[i]); }}
    };
    Multiplier multiplier(uint b) const { return Multiplier(*this, b); }
};

// S: O(1), Q: O(1), M: O(1); pow O(1 + log(e + 1)) for exponent e, pointer mul O(n).
struct Barrett64 {
    static constexpr ulng MERSENNE61 = (ulng(1) << 61) - 1;
    ulng mod;
    ulll mu;
    bool mask;

    explicit Barrett64(ulng m = 1) : mod(m), mu(0), mask((m & (m - 1)) == 0) {
        assert(m);
        if (!mask && m != MERSENNE61) { mu = ~ulll(0) / m; }}

    // Exact high half of a 128 x 128 product: middle needs at most 66 bits.
    static ulll highProduct(ulll x, ulll y) {
        ulll a = ulll(ulng(x)) * ulng(y), b = ulll(ulng(x)) * ulng(y >> 64);
        ulll c = ulll(ulng(x >> 64)) * ulng(y), d = (x >> 64) * (y >> 64);
        ulll t = (a >> 64) + ulng(b) + ulll(ulng(c));
        return d + (b >> 64) + (c >> 64) + (t >> 64);}
    pair<ulll, ulng> divMod(ulll x) const {
        if (mask) { return {x >> std::countr_zero(mod), ulng(x) & (mod - 1)}; }
        if (mod == MERSENNE61) { // 2^61 = mod + 1: two folds give z <= mod + 64.
            ulll y = (x & mod) + (x >> 61), z = (y & mod) + (y >> 61);
            bool over = z >= mod;
            return {(x >> 61) + (y >> 61) + over, ulng(over ? z - mod : z)};}
        ulll q = highProduct(x, mu), r = x - q * mod;
        bool over = r >= mod;
        return {q + over, ulng(over ? r - mod : r)};}
    ulll div(ulll x) const { return divMod(x).first; }
    ulng reduce(ulll x) const { return divMod(x).second; }
    ulng mul(ulng a, ulng b) const { return reduce(ulll(a) * b); }
    ulng pow(ulng a, ulng e) const {
        ulng r = mod != 1; a = reduce(a);
        for (; e > 1; e >>= 1, a = mul(a, a)) { if (e & 1) { r = mul(r, a); } }
        return e ? mul(r, a) : r;}

    void mul(const ulng *a, const ulng *b, ulng *out, int n) const {
        assert(n >= 0 && (!n || (a && b && out)));
        for (int i = 0; i < n; ++i) { out[i] = mul(a[i], b[i]); }}

    // Fixed ordinary multiplier, with 128-bit intermediates retaining the
    // 65th residual bit for full-width moduli. Independent of reducer lifetime.
    // S: O(1), Q: O(1), M: O(1); pointer mul O(n).
    struct Multiplier {
        ulng mod, value, mu;
        bool mask;

        Multiplier(const Barrett64 &r, ulng b) : mod(r.mod), value(r.reduce(b)),
            mu(r.mask ? 0 : ulng((ulll(value) << 64) / mod)), mask(r.mask) {}

        ulng mul(ulng a) const {
            if (mask) { return (a * value) & (mod - 1); }
            ulng q = ulng(ulll(a) * mu >> 64);
            ulll r = ulll(a) * value - ulll(q) * mod;
            return ulng(r >= mod ? r - mod : r);}

        void mul(const ulng *a, ulng *out, int n) const {
            assert(n >= 0 && (!n || (a && out)));
            for (int i = 0; i < n; ++i) { out[i] = mul(a[i]); }}
    };
    Multiplier multiplier(ulng b) const { return Multiplier(*this, b); }
};

using Barrett = Barrett64;
