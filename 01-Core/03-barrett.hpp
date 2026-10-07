#pragma once
#include "01-template.hpp"
#ifdef __AVX2__
#include <immintrin.h>
#endif

// S: O(1), Q: O(1), M: O(1); pow O(1 + log(e + 1)) for exponent e, pointer mul O(n).
// Modulus in [1, 2^32), default 1; any dividend or operand word; results canonical in [0, mod).
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

    // S: O(1), Q: O(1), M: O(1); pointer mul O(n).
    // Shoup factor: value = b % mod, mu = floor(value * 2^32 / mod); the quotient is at most one short.
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
// Modulus in [1, 2^64), default 1; any dividend or operand word; results canonical in [0, mod).
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

    // S: O(1), Q: O(1), M: O(1); pointer mul O(n).
    // 128-bit intermediates keep the 65th residual bit for full-width moduli.
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
