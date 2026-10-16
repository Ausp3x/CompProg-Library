#pragma once
#include "01-template.hpp"
#ifdef __AVX2__
#include <immintrin.h>
#endif

// Word kernels on little-endian base 2^64 words; n, m count words, r never aliases an input unless stated.
namespace infint_detail {
    using W = ulng;
    using WW = ulll;
    static_assert(std::endian::native == std::endian::little);
    constexpr int KARATSUBA = 28, KARATSUBA_SQR = 40, TOOM3 = 128, TOOM3_SQR = 128, TRUNC = 32;
#ifdef __AVX2__
    constexpr int FFT = 1024, FFT_SQR = 1024, MIDDLE = 128, NEWTON_DIV = 512, NEWTON_LONG = 32, BINARY_GCD = 96;
#else
    constexpr int FFT = 2048, FFT_SQR = 2048, MIDDLE = 256, NEWTON_DIV = 768, NEWTON_LONG = 1 << 29, BINARY_GCD = 48;
#endif
    constexpr int BURNIKEL = 48, HENSEL = 48, HALFGCD = 4096, RADIX = 64, MONTGOMERY_POW = 2;

    inline void mul(W *r, const W *a, int n, const W *b, int m);
    inline void square(W *r, const W *a, int n);
    inline W divWord(W *q, const W *a, int n, W d);

    inline int norm(const W *a, int n) { while (n > 0 && a[n - 1] == 0) { --n; } return n; }
    inline int cmp(const W *a, int n, const W *b, int m) {
        if (n != m) { return n < m ? -1 : 1; }
        for (int i = n - 1; i >= 0; --i) { if (a[i] != b[i]) { return a[i] < b[i] ? -1 : 1; } }
        return 0;}
    // r = a + b for n >= m; r may alias a or b; returns the carry.
    inline W add(W *r, const W *a, int n, const W *b, int m) {
        W c = 0;
        for (int i = 0; i < m; ++i) { WW s = WW(a[i]) + b[i] + c; r[i] = W(s); c = W(s >> 64); }
        for (int i = m; i < n; ++i) { WW s = WW(a[i]) + c; r[i] = W(s); c = W(s >> 64); }
        return c;}
    inline W addWord(W *a, int n, W w) {
        for (int i = 0; i < n && w; ++i) { W s = a[i] + w; w = s < w; a[i] = s; }
        return w;}
    // r = a - b for n >= m; r may alias a; returns the borrow.
    inline W sub(W *r, const W *a, int n, const W *b, int m) {
        W c = 0;
        for (int i = 0; i < m; ++i) { WW s = WW(a[i]) - b[i] - c; r[i] = W(s); c = W(s >> 64) & 1; }
        for (int i = m; i < n; ++i) { WW s = WW(a[i]) - c; r[i] = W(s); c = W(s >> 64) & 1; }
        return c;}
    inline W subWord(W *a, int n, W w) {
        for (int i = 0; i < n && w; ++i) { W s = a[i] - w; w = a[i] < w; a[i] = s; }
        return w;}
    // r = a * w; r may alias a; returns the carry word.
    inline W mulWord(W *r, const W *a, int n, W w) {
        W c = 0;
        for (int i = 0; i < n; ++i) { WW t = WW(a[i]) * w + c; r[i] = W(t); c = W(t >> 64); }
        return c;}
    // r += a * w; returns the carry word.
    inline W addMul(W *r, const W *a, int n, W w) {
        W c = 0;
        for (int i = 0; i < n; ++i) { WW t = WW(a[i]) * w + r[i] + c; r[i] = W(t); c = W(t >> 64); }
        return c;}
    // r -= a * w; returns the borrow word.
    inline W subMul(W *r, const W *a, int n, W w) {
        W c = 0;
        for (int i = 0; i < n; ++i) {
            WW t = WW(a[i]) * w + c; W lo = W(t), x = r[i];
            r[i] = x - lo; c = W(t >> 64) + (x < lo);}
        return c;}
    // r = a << s for 0 < s < 64 (r may alias a); returns the bits shifted out.
    inline W shl(W *r, const W *a, int n, int s) {
        W out = a[n - 1] >> (64 - s);
        for (int i = n - 1; i > 0; --i) { r[i] = (a[i] << s) | (a[i - 1] >> (64 - s)); }
        r[0] = a[0] << s;
        return out;}
    inline W shr(W *r, const W *a, int n, int s) {
        W out = a[0] << (64 - s);
        for (int i = 0; i + 1 < n; ++i) { r[i] = (a[i] >> s) | (a[i + 1] << (64 - s)); }
        r[n - 1] = a[n - 1] >> s;
        return out;}
    inline void addInto(W *r, int n, const W *b, int m) { [[maybe_unused]] W c = add(r, r, n, b, m); assert(c == 0); }
    inline void zero(W *r, int n) { if (n > 0) { std::fill(r, r + n, 0); } }
    inline void subFrom(W *r, int n, const W *b, int m) { [[maybe_unused]] W c = sub(r, r, n, b, m); assert(c == 0); }
    // r = a * b for any lengths, r zero-filled to n + m words.
    inline void mulAny(W *r, const W *a, int n, const W *b, int m) {
        int la = norm(a, n), lb = norm(b, m);
        zero(r, n + m);
        if (la == 0 || lb == 0) { return; }
        if (la >= lb) { mul(r, a, la, b, lb); }
        else { mul(r, b, lb, a, la); }}

    // T: O(n * m) word products, M: O(1); n >= m >= 1, r has n + m words.
    inline void mulBasecase(W *r, const W *a, int n, const W *b, int m) {
        r[n] = mulWord(r, a, n, b[0]);
        for (int j = 1; j < m; ++j) { r[n + j] = addMul(r + j, a, n, b[j]); }}
    // T: O(n^2 / 2) word products, M: O(1); r has 2n words.
    inline void squareBasecase(W *r, const W *a, int n) {
        std::fill(r, r + 2 * n, 0);
        for (int i = 0; i + 1 < n; ++i) { r[n + i] = addMul(r + 2 * i + 1, a + i + 1, n - i - 1, a[i]); }
        W c = 0;
        for (int i = 0; i < 2 * n; ++i) { W x = r[i]; r[i] = (x << 1) | c; c = x >> 63; }
        for (int i = 0; i < n; ++i) {
            WW sq = WW(a[i]) * a[i];
            WW t = WW(r[2 * i]) + W(sq) + c; r[2 * i] = W(t); c = W(t >> 64);
            t = WW(r[2 * i + 1]) + W(sq >> 64) + c; r[2 * i + 1] = W(t); c = W(t >> 64);}}
    // T: O(n^1.585) word products, M: O(n); n >= m > n / 2, r has n + m words.
    inline void karatsuba(W *r, const W *a, int n, const W *b, int m) {
        int h = n / 2, t = n - h, u = m - h;
        mulAny(r, a, h, b, h); mulAny(r + 2 * h, a + h, t, b + h, u);
        vector<W> s(4 * t + 4);
        W *sa = s.data(), *sb = sa + t + 1, *z = sb + t + 1;
        sa[t] = add(sa, a + h, t, a, h);
        std::fill(sb, sb + t + 1, 0);
        if (u >= h) { sb[u] = add(sb, b + h, u, b, h); }
        else { sb[h] = add(sb, b, h, b + h, u); }
        mulAny(z, sa, t + 1, sb, t + 1);
        subFrom(z, 2 * t + 2, r, 2 * h); subFrom(z, 2 * t + 2, r + 2 * h, t + u);
        addInto(r + h, n + m - h, z, norm(z, 2 * t + 2));}
    inline void karatsubaSquare(W *r, const W *a, int n) {
        int h = n / 2, t = n - h;
        mulAny(r, a, h, a, h); mulAny(r + 2 * h, a + h, t, a + h, t);
        vector<W> s(3 * t + 3);
        W *sa = s.data(), *z = sa + t + 1;
        sa[t] = add(sa, a + h, t, a, h);
        mulAny(z, sa, t + 1, sa, t + 1);
        subFrom(z, 2 * t + 2, r, 2 * h); subFrom(z, 2 * t + 2, r + 2 * h, 2 * t);
        addInto(r + h, 2 * n - h, z, norm(z, 2 * t + 2));}
    // Evaluations at 1, -1 (sign in sm) and 2 of the k, k, k2-word thirds of x; each k + 1 words.
    inline void toomEval(const W *x, int k, int k2, W *p1, W *pm, W *p2, int &sm) {
        W *s = pm;
        std::copy(x, x + k, s); s[k] = add(s, s, k, x + 2 * k, k2);
        p1[k] = s[k] + add(p1, s, k, x + k, k);
        std::copy(x + 2 * k, x + 2 * k + k2, p2); std::fill(p2 + k2, p2 + k, 0);
        p2[k] = shl(p2, p2, k, 1); p2[k] += add(p2, p2, k, x + k, k);
        shl(p2, p2, k + 1, 1); p2[k] += add(p2, p2, k, x, k);
        if (s[k] || cmp(s, k, x + k, k) >= 0) { s[k] -= sub(s, s, k, x + k, k); sm = 1; }
        else { sub(s, x + k, k, s, k); sm = -1; }}
    // Bodrato interpolation of r1 = r(1), rm = |r(-1)| with sign sm, r2 = r(2); r holds r(0) in [0, 2k) and r(inf) in [4k, 2n).
    inline void toomInterpolate(W *r, int n, int k, W *r1, W *rm, W *r2, W *t2, int sm) {
        int len = 2 * k + 2, k2 = n - 2 * k;
        std::copy(r1, r1 + len, t2); subFrom(t2, len, r, 2 * k);
        if (sm > 0) { subFrom(r2, len, rm, len); subFrom(r1, len, rm, len); }
        else { addInto(r2, len, rm, len); addInto(r1, len, rm, len); }
        divWord(r2, r2, len, 3); shr(r1, r1, len, 1);
        subFrom(r2, len, t2, len); shr(r2, r2, len, 1);
        vector<W> d(2 * k2 + 1); d[2 * k2] = shl(d.data(), r + 4 * k, 2 * k2, 1);
        subFrom(r2, len, d.data(), 2 * k2 + 1);
        subFrom(t2, len, r1, len); subFrom(t2, len, r + 4 * k, 2 * k2);
        subFrom(r1, len, r2, len);
        std::fill(r + 2 * k, r + 4 * k, 0);
        addInto(r + k, 2 * n - k, r1, norm(r1, len));
        addInto(r + 2 * k, 2 * n - 2 * k, t2, norm(t2, len));
        addInto(r + 3 * k, 2 * n - 3 * k, r2, norm(r2, len));}
    // T: O(n^1.465) word products, M: O(n); equal lengths n >= 6, r has 2n words.
    inline void toom3(W *r, const W *a, const W *b, int n) {
        int k = (n + 2) / 3, k2 = n - 2 * k, len = 2 * k + 2;
        vector<W> buf(6 * (k + 1) + 4 * len);
        W *p1 = buf.data(), *pm = p1 + k + 1, *p2 = pm + k + 1, *q1 = p2 + k + 1, *qm = q1 + k + 1, *q2 = qm + k + 1;
        W *r1 = q2 + k + 1, *rm = r1 + len, *r2 = rm + len, *t2 = r2 + len;
        int sa, sb;
        toomEval(a, k, k2, p1, pm, p2, sa); toomEval(b, k, k2, q1, qm, q2, sb);
        mulAny(r1, p1, k + 1, q1, k + 1); mulAny(rm, pm, k + 1, qm, k + 1); mulAny(r2, p2, k + 1, q2, k + 1);
        mulAny(r, a, k, b, k); mulAny(r + 4 * k, a + 2 * k, k2, b + 2 * k, k2);
        toomInterpolate(r, n, k, r1, rm, r2, t2, sa * sb);}
    inline void toom3Square(W *r, const W *a, int n) {
        int k = (n + 2) / 3, k2 = n - 2 * k, len = 2 * k + 2;
        vector<W> buf(3 * (k + 1) + 4 * len);
        W *p1 = buf.data(), *pm = p1 + k + 1, *p2 = pm + k + 1, *r1 = p2 + k + 1, *rm = r1 + len, *r2 = rm + len, *t2 = r2 + len;
        int sa;
        toomEval(a, k, k2, p1, pm, p2, sa);
        mulAny(r1, p1, k + 1, p1, k + 1); mulAny(rm, pm, k + 1, pm, k + 1); mulAny(r2, p2, k + 1, p2, k + 1);
        mulAny(r, a, k, a, k); mulAny(r + 4 * k, a + 2 * k, k2, a + 2 * k, k2);
        toomInterpolate(r, n, k, r1, rm, r2, t2, 1);}
    // T: O((n / m) * M(m)), M: O(m); n >= 2m: balanced m-word chunks of a times b.
    inline void unbalancedMul(W *r, const W *a, int n, const W *b, int m) {
        std::fill(r, r + n + m, 0);
        vector<W> t(2 * m);
        for (int i = 0; i < n; i += m) {
            int len = min(m, n - i);
            mulAny(t.data(), b, m, a + i, len);
            addInto(r + i, n + m - i, t.data(), norm(t.data(), m + len));}}
    // T: O(n^2 / 2) word products below TRUNC, else M(n); r = a * b mod B^n for n-word a, b.
    inline void truncatedMul(W *r, const W *a, const W *b, int n) {
        if (n < TRUNC) {
            zero(r, n);
            for (int j = 0; j < n; ++j) { addMul(r + j, a, n - j, b[j]); }
            return;}
        vector<W> t(2 * n); mulAny(t.data(), a, n, b, n);
        std::copy(t.data(), t.data() + n, r);}
    inline void nttCyclic(vector<W> &lo, vector<W> &hi, const W *a, int n, const W *b, int m, int len);
    // T: O(na * nb) word products below MIDDLE, M(na + nb) below FFT, else one transform of max(lo + cnt, na + nb - 1 - lo, na, nb) words;
    // r (cnt words) = words [lo, lo + cnt) of a * b over the terms with i + j >= lo, so the exact words exceed r by less than min(na, nb) * B.
    inline void middleProduct(W *r, const W *a, int na, const W *b, int nb, int lo, int cnt) {
        zero(r, cnt);
        if (min(na, nb) < MIDDLE) {
            for (int j = 0; j < nb; ++j) {
                int i0 = max(lo - j, 0), i1 = min(lo + cnt - j, na);
                if (i0 >= i1) { continue; }
                W c = addMul(r + (i0 + j - lo), a + i0, i1 - i0, b[j]);
                if (i1 + j - lo < cnt) { addWord(r + (i1 + j - lo), cnt - (i1 + j - lo), c); }}
            return;}
        if (min(na, nb) < FFT) {
            vector<W> full(size_t(na + nb) + 1, 0);
            mulAny(full.data(), a, na, b, nb);
            std::copy(full.begin() + min(lo, na + nb), full.begin() + min(lo + cnt, na + nb), r);
            return;}
        vector<W> low, high;
        nttCyclic(low, high, a, na, b, nb, max({lo + cnt, na + nb - 1 - lo, na, nb}));
        WW carry = 0;
        for (int i = 0; i < cnt; ++i) {
            int l = 2 * (lo + i);
            WW c0 = low[size_t(l)] + (WW(high[size_t(l)]) << 64) + carry;
            W lw = uint(c0); carry = c0 >> 32;
            WW c1 = low[size_t(l) + 1] + (WW(high[size_t(l) + 1]) << 64) + carry;
            r[i] = lw | (W(uint(c1)) << 32); carry = c1 >> 32;}}
    constexpr uint NTT_PRIMES[3] = {2013265921, 1811939329, 469762049}, NTT_ROOTS[3] = {31, 13, 3};
    constexpr int NTT_MAX_LOG = 26;
    constexpr uint modPow32(uint a, uint e, uint p) {
        ulng r = 1, b = a % p;
        for (; e; e >>= 1, b = b * b % p) { if (e & 1) { r = r * b % p; } }
        return uint(r);}

    // S: O(len) roots per prime, Q: O(len * log(len)), M: O(len); Montgomery NTT modulo P with 2^k | P - 1.
    template<uint P, uint G>
    struct Ntt32 {
        static constexpr uint NINV = []() { uint x = P; for (int i = 0; i < 5; ++i) { x *= 2 - P * x; } return -x; }();
        static constexpr uint R2 = uint((ulll(1) << 64) % P);
        static constexpr uint ONE = uint((ulng(1) << 32) % P);

        static uint red(ulng x) { uint r = uint((x + ulng(uint(x) * NINV) * P) >> 32); return r >= P ? r - P : r; }
        static uint mul(uint a, uint b) { return red(ulng(a) * b); }
        static uint from(uint a) { return mul(a, R2); }
        static vector<uint> &roots(bool inverse) { static vector<uint> rt, irt; return inverse ? irt : rt; }
        static void ensure(int n) {
            vector<uint> &rt = roots(false), &irt = roots(true);
            if (rt.empty()) { rt = {0, ONE}; irt = {0, ONE}; }
            if (int(rt.size()) >= n) { return; }
            int m = int(rt.size()); rt.resize(n); irt.resize(n);
            for (int len = m; len < n; len *= 2) {
                uint w = from(modPow32(G, (P - 1) / uint(2 * len), P)), iw = from(modPow32(G, P - 1 - (P - 1) / uint(2 * len), P));
                for (int i = len / 2; i < len; ++i) {
                    rt[2 * i] = rt[i]; rt[2 * i + 1] = mul(rt[i], w);
                    irt[2 * i] = irt[i]; irt[2 * i + 1] = mul(irt[i], iw);}}}
#ifdef __AVX2__
        static __m256i vred(__m256i x0, __m256i x1) {
            __m256i p32 = _mm256_set1_epi32(int(P)), p64 = _mm256_set1_epi64x(lng(P)), ninv = _mm256_set1_epi32(int(NINV));
            __m256i q0 = _mm256_mul_epu32(_mm256_mul_epu32(x0, ninv), p64), q1 = _mm256_mul_epu32(_mm256_mul_epu32(x1, ninv), p64);
            __m256i r0 = _mm256_srli_epi64(_mm256_add_epi64(x0, q0), 32), r1 = _mm256_srli_epi64(_mm256_add_epi64(x1, q1), 32);
            __m256i r = _mm256_blend_epi32(r0, _mm256_slli_epi64(r1, 32), 0xAA);
            return _mm256_min_epu32(r, _mm256_sub_epi32(r, p32));}
        static __m256i vadd(__m256i a, __m256i b) {
            __m256i s = _mm256_add_epi32(a, b);
            return _mm256_min_epu32(s, _mm256_sub_epi32(s, _mm256_set1_epi32(int(P))));}
        static __m256i vsub(__m256i a, __m256i b) {
            __m256i d = _mm256_sub_epi32(a, b);
            return _mm256_min_epu32(d, _mm256_add_epi32(d, _mm256_set1_epi32(int(P))));}
        static __m256i vmul(__m256i a, __m256i b) {
            return vred(_mm256_mul_epu32(a, b), _mm256_mul_epu32(_mm256_srli_epi64(a, 32), _mm256_srli_epi64(b, 32)));}
        static __m256i load(const uint *p) { return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p)); }
        static void store(uint *p, __m256i x) { _mm256_storeu_si256(reinterpret_cast<__m256i *>(p), x); }
        static __m256i wpair(const uint *w) { return _mm256_broadcastsi128_si256(_mm_shuffle_epi32(_mm_loadl_epi64(reinterpret_cast<const __m128i *>(w)), 0x44)); }
        static __m256i wquad(const uint *w) { return _mm256_broadcastsi128_si256(_mm_loadu_si128(reinterpret_cast<const __m128i *>(w))); }
#endif
        // Forward DIF to bit-reversed order, inverse DIT back, both in place on canonical residues.
        static void transform(uint *a, int n, bool inverse) {
            ensure(n);
            const uint *w = roots(inverse).data();
            if (!inverse) {
                int len = n / 2;
#ifdef __AVX2__
                for (; len >= 8; len /= 2) {
                    for (int i = 0; i < n; i += 2 * len) {
                        for (int j = 0; j < len; j += 8) {
                            __m256i u = load(a + i + j), v = load(a + i + j + len), x = load(w + len + j);
                            store(a + i + j, vadd(u, v)); store(a + i + j + len, vmul(vsub(u, v), x));}}}
                if (n >= 8) {
                    __m256i w2 = wpair(w + 2), w4 = wquad(w + 4);
                    for (int i = 0; i < n; i += 8) {
                        __m256i x = load(a + i), y = _mm256_permute2x128_si256(x, x, 0x01);
                        __m256i s = vadd(x, y), d = vsub(x, y);
                        x = _mm256_blend_epi32(s, vmul(_mm256_permute2x128_si256(d, d, 0x00), w4), 0xF0);
                        y = _mm256_shuffle_epi32(x, 0x4E); s = vadd(x, y); d = vsub(x, y);
                        x = _mm256_blend_epi32(s, vmul(_mm256_shuffle_epi32(d, 0x44), w2), 0xCC);
                        y = _mm256_shuffle_epi32(x, 0xB1); s = vadd(x, y); d = _mm256_shuffle_epi32(vsub(x, y), 0xA0);
                        store(a + i, _mm256_blend_epi32(s, d, 0xAA));}
                    return;}
#endif
                for (; len > 0; len /= 2) {
                    for (int i = 0; i < n; i += 2 * len) {
                        for (int j = 0; j < len; ++j) {
                            uint u = a[i + j], v = a[i + j + len], s = u + v;
                            a[i + j] = s >= P ? s - P : s; a[i + j + len] = mul(u - v + (u < v ? P : 0), w[len + j]);}}}
                return;}
            int len = 1;
#ifdef __AVX2__
            if (n >= 8) {
                __m256i w2 = wpair(w + 2), w4 = wquad(w + 4);
                for (int i = 0; i < n; i += 8) {
                    __m256i x = load(a + i), y = _mm256_shuffle_epi32(x, 0xB1);
                    __m256i s = vadd(x, y), d = _mm256_shuffle_epi32(vsub(x, y), 0xA0);
                    x = _mm256_blend_epi32(s, d, 0xAA);
                    __m256i u = _mm256_shuffle_epi32(x, 0x44), v = vmul(_mm256_shuffle_epi32(x, 0xEE), w2);
                    x = _mm256_blend_epi32(vadd(u, v), vsub(u, v), 0xCC);
                    u = _mm256_permute2x128_si256(x, x, 0x00); v = vmul(_mm256_permute2x128_si256(x, x, 0x11), w4);
                    store(a + i, _mm256_blend_epi32(vadd(u, v), vsub(u, v), 0xF0));}
                for (len = 8; len < n; len *= 2) {
                    for (int i = 0; i < n; i += 2 * len) {
                        for (int j = 0; j < len; j += 8) {
                            __m256i u = load(a + i + j), v = vmul(load(a + i + j + len), load(w + len + j));
                            store(a + i + j, vadd(u, v)); store(a + i + j + len, vsub(u, v));}}}}
#endif
            for (; len < n; len *= 2) {
                for (int i = 0; i < n; i += 2 * len) {
                    for (int j = 0; j < len; ++j) {
                        uint u = a[i + j], v = mul(a[i + j + len], w[len + j]), s = u + v;
                        a[i + j] = s >= P ? s - P : s; a[i + j + len] = u - v + (u < v ? P : 0);}}}
            uint inv = from(P - (P - 1) / uint(n));
            int i = 0;
#ifdef __AVX2__
            __m256i vi = _mm256_set1_epi32(int(inv));
            for (; i + 8 <= n; i += 8) { store(a + i, vmul(load(a + i), vi)); }
#endif
            for (; i < n; ++i) { a[i] = mul(a[i], inv); }}
        // Pointwise c = a * b (b may equal a) on transformed data.
        static void dot(uint *a, const uint *b, int n) {
            int i = 0;
#ifdef __AVX2__
            __m256i r2 = _mm256_set1_epi32(int(R2));
            for (; i + 8 <= n; i += 8) { store(a + i, vmul(load(a + i), vmul(load(b + i), r2))); }
#endif
            for (; i < n; ++i) { a[i] = uint(ulng(a[i]) * b[i] % P); }}
    };

    // T: O(len * log(len)), M: O(len); three-prime CRT: lo + hi * 2^64 holds each limb coefficient of the length-len cyclic
    // limb convolution (len a power of two <= 2^26, coefficients below 2^89).
    inline void ntt3Cyclic(vector<W> &lo, vector<W> &hi, const uint *a, int n, const uint *b, int m, int len) {
        using E0 = Ntt32<NTT_PRIMES[0], NTT_ROOTS[0]>;
        using E1 = Ntt32<NTT_PRIMES[1], NTT_ROOTS[1]>;
        using E2 = Ntt32<NTT_PRIMES[2], NTT_ROOTS[2]>;
        constexpr uint P0 = NTT_PRIMES[0], P1 = NTT_PRIMES[1], P2 = NTT_PRIMES[2];
        constexpr ulng P01 = ulng(P0) * P1;
        constexpr uint I0_1 = modPow32(P0, P1 - 2, P1), I01_2 = modPow32(uint(P01 % P2), P2 - 2, P2);
        static vector<uint> x[3], y[3];
        bool same = a == b && n == m;
        for (int k = 0; k < 3; ++k) {
            uint p = NTT_PRIMES[k];
            x[k].assign(len, 0);
            for (int i = 0; i < n; ++i) { x[k][i] = a[i] % p; }
            if (!same) {
                y[k].assign(len, 0);
                for (int i = 0; i < m; ++i) { y[k][i] = b[i] % p; }}}
        E0::transform(x[0].data(), len, false); E1::transform(x[1].data(), len, false); E2::transform(x[2].data(), len, false);
        if (!same) { E0::transform(y[0].data(), len, false); E1::transform(y[1].data(), len, false); E2::transform(y[2].data(), len, false); }
        E0::dot(x[0].data(), (same ? x : y)[0].data(), len); E1::dot(x[1].data(), (same ? x : y)[1].data(), len); E2::dot(x[2].data(), (same ? x : y)[2].data(), len);
        E0::transform(x[0].data(), len, true); E1::transform(x[1].data(), len, true); E2::transform(x[2].data(), len, true);
        lo.resize(len); hi.resize(len);
        for (int i = 0; i < len; ++i) {
            uint v0 = x[0][i], v1 = uint(ulng(x[1][i] + P1 - (v0 >= P1 ? v0 - P1 : v0)) * I0_1 % P1);
            ulng t = v0 + ulng(v1) * P0;
            uint v2 = uint((x[2][i] + P2 - uint(t % P2)) * ulng(I01_2) % P2);
            ulll e = t + ulll(v2) * P01;
            lo[i] = W(e); hi[i] = W(e >> 64);}}

    // S: O(len) roots, Q: O(len * log(len)), M: O(len); NTT modulo 2^64 - 2^32 + 1 on 16-bit pieces, length <= 2^32.
    struct Ntt64 {
        static constexpr ulng P = 18446744069414584321ULL, G = 7;
        static ulng red(ulll x) {
            ulng l = ulng(x), hl = uint(x >> 64), hh = ulng(x >> 96);
            ulll c = ulll(l) + (ulll(hl) << 32); ulng s = hl + hh;
            c += (c < s) * ulll(P); c -= s;
            ulng r = ulng(c) + 0xFFFFFFFFULL * ulng(c >> 64);
            return r >= P ? r - P : r;}
        static ulng mul(ulng a, ulng b) { return red(ulll(a) * b); }
        static ulng power(ulng a, ulng e) {
            ulng r = 1;
            for (; e; e >>= 1, a = mul(a, a)) { if (e & 1) { r = mul(r, a); } }
            return r;}
        static vector<ulng> &roots() { static vector<ulng> w = {0, 1}; return w; }
        static void ensure(int n) {
            vector<ulng> &w = roots();
            if (int(w.size()) >= n) { return; }
            int m = int(w.size()); w.resize(n);
            for (int len = m; len < n; len *= 2) {
                ulng w0 = power(G, (P - 1) / ulng(2 * len));
                for (int i = len / 2; i < len; ++i) { w[2 * i] = w[i]; w[2 * i + 1] = mul(w[i], w0); }}}
        static void transform(ulng *a, int n, bool inverse) {
            ensure(n);
            const ulng *w = roots().data();
            if (!inverse) {
                for (int len = n / 2; len > 0; len /= 2) {
                    for (int i = 0; i < n; i += 2 * len) {
                        for (int j = 0; j < len; ++j) {
                            ulng u = a[i + j], v = a[i + j + len], s = u + v;
                            a[i + j] = s - ((s < u) | (s >= P)) * P; a[i + j + len] = mul(u - v + (u < v) * P, w[len + j]);}}}
                return;}
            for (int len = 1; len < n; len *= 2) {
                for (int i = 0; i < n; i += 2 * len) {
                    for (int j = 0; j < len; ++j) {
                        ulng iw = j == 0 ? 1 : P - w[2 * len - j];
                        ulng u = a[i + j], v = mul(a[i + j + len], iw), s = u + v;
                        a[i + j] = s - ((s < u) | (s >= P)) * P; a[i + j + len] = u - v + (u < v) * P;}}}
            ulng inv = P - (P - 1) / ulng(n);
            for (int i = 0; i < n; ++i) { a[i] = mul(a[i], inv); }}
    };
    // T: O(len * log(len)), M: O(len); the Goldilocks NTT on 16-bit pieces folded into lo (hi = 0) limb coefficients.
    inline void ntt64Cyclic(vector<W> &lo, vector<W> &hi, const uint *a, int n, const uint *b, int m, int len) {
        static vector<ulng> x, y;
        bool same = a == b && n == m;
        int pieces = 2 * len;
        x.assign(pieces, 0);
        for (int i = 0; i < n; ++i) { x[2 * i] = a[i] & 0xFFFF; x[2 * i + 1] = a[i] >> 16; }
        Ntt64::transform(x.data(), pieces, false);
        if (same) { for (int i = 0; i < pieces; ++i) { x[i] = Ntt64::mul(x[i], x[i]); } }
        else {
            y.assign(pieces, 0);
            for (int i = 0; i < m; ++i) { y[2 * i] = b[i] & 0xFFFF; y[2 * i + 1] = b[i] >> 16; }
            Ntt64::transform(y.data(), pieces, false);
            for (int i = 0; i < pieces; ++i) { x[i] = Ntt64::mul(x[i], y[i]); }}
        Ntt64::transform(x.data(), pieces, true);
        lo.resize(len); hi.assign(len, 0);
        for (int i = 0; i < len; ++i) {
            ulll e = x[2 * i] + (ulll(x[2 * i + 1]) << 16);
            lo[i] = W(e); hi[i] = W(e >> 64);}}

    // S: O(len) roots, Q: O(len * log(len)), M: O(len); split re/im radix-2 FFT, forward to bit-reversed order.
    struct Fft {
        static vector<double> &roots(bool imag) { static vector<double> wr = {0, 1}, wi = {0, 0}; return imag ? wi : wr; }
        static void ensure(int n) {
            vector<double> &wr = roots(false), &wi = roots(true);
            if (int(wr.size()) >= n) { return; }
            int m = int(wr.size()); wr.resize(n); wi.resize(n);
            for (int len = m; len < n; len *= 2) {
                for (int j = 0; j < len; ++j) {
                    long double t = std::numbers::pi_v<long double> * j / len;
                    wr[len + j] = double(std::cos(t)); wi[len + j] = double(std::sin(t));}}}
        static int partner(int p) { return p < 2 ? p : 3 * (1 << (31 - std::countl_zero(uint(p)))) - 1 - p; }
        static void transform(double *re, double *im, int n, bool inverse) {
            ensure(n);
            const double *wr = roots(false).data(), *wi = roots(true).data();
            if (!inverse) {
                for (int len = n / 2; len > 0; len /= 2) {
                    for (int i = 0; i < n; i += 2 * len) {
                        for (int j = 0; j < len; ++j) {
                            double ur = re[i + j], ui = im[i + j], vr = re[i + j + len], vi = im[i + j + len];
                            re[i + j] = ur + vr; im[i + j] = ui + vi;
                            double dr = ur - vr, di = ui - vi;
                            re[i + j + len] = dr * wr[len + j] - di * wi[len + j]; im[i + j + len] = dr * wi[len + j] + di * wr[len + j];}}}
                return;}
            for (int len = 1; len < n; len *= 2) {
                for (int i = 0; i < n; i += 2 * len) {
                    for (int j = 0; j < len; ++j) {
                        double ur = re[i + j], ui = im[i + j], xr = re[i + j + len], xi = im[i + j + len];
                        double vr = xr * wr[len + j] + xi * wi[len + j], vi = xi * wr[len + j] - xr * wi[len + j];
                        re[i + j] = ur + vr; im[i + j] = ui + vi; re[i + j + len] = ur - vr; im[i + j + len] = ui - vi;}}}
            double inv = 1.0 / n;
            for (int i = 0; i < n; ++i) { re[i] *= inv; im[i] *= inv; }}
    };
    // T: O(len * log(len)), M: O(len); 16-bit pieces of a and b packed as one complex sequence; exact while
    // min(n, m) * 2^33 stays below 2^50, which the dispatcher guarantees.
    inline void fftCyclic(vector<W> &lo, vector<W> &hi, const uint *a, int n, const uint *b, int m, int len) {
        static vector<double> re, im;
        int pieces = 2 * len;
        re.assign(pieces, 0); im.assign(pieces, 0);
        for (int i = 0; i < n; ++i) { re[2 * i] = a[i] & 0xFFFF; re[2 * i + 1] = a[i] >> 16; }
        for (int i = 0; i < m; ++i) { im[2 * i] = b[i] & 0xFFFF; im[2 * i + 1] = b[i] >> 16; }
        Fft::transform(re.data(), im.data(), pieces, false);
        for (int p = 0; p < pieces; ++p) {
            int q = Fft::partner(p);
            if (q < p) { continue; }
            double pr = re[p], pi = im[p], qr = re[q], qi = im[q];
            double ar = (pr + qr) * 0.5, ai = (pi - qi) * 0.5, br = (pi + qi) * 0.5, bi = (qr - pr) * 0.5;
            re[p] = ar * br - ai * bi; im[p] = ar * bi + ai * br;
            if (q != p) {
                double cr = ar, ci = -ai, dr = br, di = -bi;
                re[q] = cr * dr - ci * di; im[q] = cr * di + ci * dr;}}
        Fft::transform(re.data(), im.data(), pieces, true);
        lo.resize(len); hi.assign(len, 0);
        for (int i = 0; i < len; ++i) {
            ulll e = ulll(std::llround(re[2 * i])) + (ulll(std::llround(re[2 * i + 1])) << 16);
            lo[i] = W(e); hi[i] = W(e >> 64);}}
    constexpr int FFT_DOUBLE_MAX = 1 << 14, NTT_MAX_WORDS = 1 << 29;
    // Cyclic limb convolution of length pow2 >= 2 * max(len, n, m) limbs of the limb images of a and b (words); lo + hi * 2^64 per limb.
    inline void nttCyclic(vector<W> &lo, vector<W> &hi, const W *a, int n, const W *b, int m, int len) {
        static vector<uint> la, lb;
        len = max({len, n, m});
        bool same = a == b && n == m;
        la.resize(2 * n);
        for (int i = 0; i < n; ++i) { la[2 * i] = uint(a[i]); la[2 * i + 1] = uint(a[i] >> 32); }
        if (!same) {
            lb.resize(2 * m);
            for (int i = 0; i < m; ++i) { lb[2 * i] = uint(b[i]); lb[2 * i + 1] = uint(b[i] >> 32); }}
        int limbs = int(std::bit_ceil(uint(2 * len)));
        const uint *pa = la.data(), *pb = same ? la.data() : lb.data();
        int na = 2 * n, nb = same ? 2 * n : 2 * m;
#ifndef __AVX2__
        if (min(n, m) <= FFT_DOUBLE_MAX) { fftCyclic(lo, hi, pa, na, pb, nb, limbs); return; }
#endif
        if (limbs <= (1 << NTT_MAX_LOG)) { ntt3Cyclic(lo, hi, pa, na, pb, nb, limbs); return; }
        ntt64Cyclic(lo, hi, pa, na, pb, nb, limbs);}
    // T: O(len * log(len)), M: O(len); r (n + m words) = a * b through the transform backends.
    inline void transformMul(W *r, const W *a, int n, const W *b, int m) {
        vector<W> lo, hi;
        nttCyclic(lo, hi, a, n, b, m, n + m);
        WW carry = 0;
        for (int i = 0; i < n + m; ++i) {
            WW c0 = lo[2 * i] + (WW(hi[2 * i]) << 64) + carry;
            W low = uint(c0); carry = c0 >> 32;
            WW c1 = lo[2 * i + 1] + (WW(hi[2 * i + 1]) << 64) + carry;
            r[i] = low | (W(uint(c1)) << 32); carry = c1 >> 32;}}
    // Transform lengths round up to a power of two; a product whose length wastes more than a third of the transform stays with Toom-3.
    inline bool padded(int len) { return 3 * ulng(len) >= 2 * ulng(std::bit_ceil(uint(len))); }
    // T: O(M(n)) word products by size: basecase, Karatsuba, Toom-3, transforms; n >= m >= 1, r has n + m words.
    inline void mul(W *r, const W *a, int n, const W *b, int m) {
        if (m == 1) { r[n] = mulWord(r, a, n, b[0]); return; }
        if (a == b && n == m) { square(r, a, n); return; }
        if (m < KARATSUBA) { mulBasecase(r, a, n, b, m); return; }
        if (n >= 2 * m) { unbalancedMul(r, a, n, b, m); return; }
        if (m >= FFT && n + m <= NTT_MAX_WORDS && padded(n + m)) { transformMul(r, a, n, b, m); return; }
        if (m >= TOOM3 && n == m) { toom3(r, a, b, n); return; }
        karatsuba(r, a, n, b, m);}
    inline void square(W *r, const W *a, int n) {
        if (n < KARATSUBA_SQR) { squareBasecase(r, a, n); return; }
        if (n >= FFT_SQR && 2 * n <= NTT_MAX_WORDS && padded(2 * n)) { transformMul(r, a, n, a, n); return; }
        if (n >= TOOM3_SQR) { toom3Square(r, a, n); return; }
        karatsubaSquare(r, a, n);}
    // Moller-Granlund reciprocal of a normalized divisor (top bit set).
    inline W invWord(W d) { return W(((WW(~d) << 64) | ~W(0)) / d); }
    // q = (u1:u0) / d, returns the remainder; d normalized, u1 < d, v = invWord(d).
    inline W div2by1(W &q, W u1, W u0, W d, W v) {
        WW t = WW(v) * u1 + ((WW(u1) << 64) | u0);
        W q1 = W(t >> 64) + 1, q0 = W(t), r = u0 - q1 * d;
        if (r > q0) { --q1; r += d; }
        if (r >= d) { ++q1; r -= d; }
        q = q1;
        return r;}
    // T: O(n), M: O(1); q = a / d (q may alias a), returns a % d; d != 0.
    inline W divWord(W *q, const W *a, int n, W d) {
        if (d == 0) { d = 1; }
        int s = std::countl_zero(d);
        W dn = d << s, v = invWord(dn), r = 0;
        if (s == 0) {
            for (int i = n - 1; i >= 0; --i) { r = div2by1(q[i], r, a[i], dn, v); }
            return r;}
        r = a[n - 1] >> (64 - s);
        for (int i = n - 1; i >= 0; --i) {
            W u = (a[i] << s) | (i ? a[i - 1] >> (64 - s) : 0);
            r = div2by1(q[i], r, u, dn, v);}
        return r >> s;}
    // T: O((n - m) * m) word products, M: O(n); Knuth D: q (n - m + 1 words), r (m words) for normalized b of m >= 2 words.
    inline void divBasecase(W *q, W *r, const W *a, int n, const W *b, int m) {
        vector<W> uv(a, a + n); uv.push_back(0);
        W *u = uv.data(), dt = b[m - 1], ds = b[m - 2], v = invWord(dt);
        for (int j = n - m; j >= 0; --j) {
            W qh, rh; bool refine = true;
            if (u[j + m] >= dt) { qh = ~W(0); rh = u[j + m - 1] + dt; refine = rh >= dt; }
            else { rh = div2by1(qh, u[j + m], u[j + m - 1], dt, v); }
            while (refine && WW(qh) * ds > ((WW(rh) << 64) | u[j + m - 2])) { --qh; rh += dt; refine = rh >= dt; }
            W borrow = subMul(u + j, b, m, qh), top = u[j + m];
            u[j + m] = top - borrow;
            if (top < borrow) { --qh; u[j + m] += add(u + j, u + j, m, b, m); }
            q[j] = qh;}
        std::copy(u, u + m, r);}
    inline void div2n1n(W *q, W *r, const W *a, const W *b, int n);
    // Burnikel-Ziegler 3n/2n step: a (3n words) < b * B^n, b (2n words) normalized; q n words, r 2n words.
    inline void div3n2n(W *q, W *r, const W *a, const W *b, int n) {
        vector<W> rr(3 * n + 1, 0), d(2 * n);
        std::copy(a, a + n, rr.data());
        if (cmp(a + 2 * n, n, b + n, n) < 0) { div2n1n(q, rr.data() + n, a + n, b + n, n); }
        else {
            std::fill(q, q + n, ~W(0));
            std::copy(a + n, a + 3 * n, rr.data() + n);
            subFrom(rr.data() + 2 * n, n, b + n, n); addInto(rr.data() + n, 2 * n + 1, b + n, n);}
        mulAny(d.data(), q, n, b, n);
        W borrow = sub(rr.data(), rr.data(), 3 * n + 1, d.data(), 2 * n);
        while (borrow) { subWord(q, n, 1); borrow -= add(rr.data(), rr.data(), 3 * n + 1, b, 2 * n); }
        std::copy(rr.data(), rr.data() + 2 * n, r);}
    // T: O(M(n) * log(n)), M: O(n); a (2n words) < b * B^n, b (n words) normalized; q n words, r n words.
    inline void div2n1n(W *q, W *r, const W *a, const W *b, int n) {
        if ((n & 1) || n <= BURNIKEL) {
            vector<W> qq(n + 1);
            divBasecase(qq.data(), r, a, 2 * n, b, n);
            std::copy(qq.data(), qq.data() + n, q);
            return;}
        int k = n / 2;
        vector<W> t(3 * k);
        div3n2n(q + k, t.data() + k, a + k, b, k);
        std::copy(a, a + k, t.data());
        div3n2n(q, r, t.data(), b, k);}
    // T: O(M(m) * log(m) * n / m), M: O(n); recursive division for normalized b of m words, n >= m.
    inline void divBurnikelZiegler(W *q, W *r, const W *a, int n, const W *b, int m) {
        int k = 0;
        while ((m + (1 << k) - 1) >> k > BURNIKEL) { ++k; }
        int bs = (m + (1 << k) - 1) >> k, blk = bs << k, pad = blk - m;
        vector<W> bb(blk, 0), aa(n + pad, 0), z(2 * blk, 0), qq;
        std::copy(b, b + m, bb.data() + pad); std::copy(a, a + n, aa.data() + pad);
        int na = n + pad, chunks = (na + blk - 1) / blk;
        if (na == chunks * blk && cmp(aa.data() + na - blk, blk, bb.data(), blk) >= 0) { ++chunks; }
        qq.assign(size_t(chunks) * blk, 0);
        for (int c = chunks - 1; c >= 0; --c) {
            int lo = c * blk, hi = min(lo + blk, na);
            std::copy(aa.data() + lo, aa.data() + hi, z.data()); std::fill(z.data() + (hi - lo), z.data() + blk, 0);
            if (c == chunks - 1) { std::copy(z.data(), z.data() + blk, z.data() + blk); continue; }
            div2n1n(qq.data() + lo, z.data() + blk, z.data(), bb.data(), blk);}
        std::copy(qq.data(), qq.data() + (n - m + 1), q);
        std::copy(z.data() + blk + pad, z.data() + 2 * blk, r);}
    // T: O(M(m)), M: O(m); x (m + 1 words) = floor(B^(2m) / b) for normalized b of m words: recursive Newton on the top half, the
    // residual B^(m + h) - b * xh read from a middle product with two guard words, then an exact correction.
    inline void reciprocal(W *x, const W *b, int m) {
        if (m <= BURNIKEL) {
            vector<W> num(2 * m + 1, 0), qq(m + 2), rr(m);
            num[2 * m] = 1;
            if (m == 1) { divWord(qq.data(), num.data(), 3, b[0]); }
            else { divBasecase(qq.data(), rr.data(), num.data(), 2 * m + 1, b, m); }
            std::copy(qq.data(), qq.data() + m + 1, x);
            return;}
        int h = (m + 1) / 2, l = m - h, cnt = l + 4;
        vector<W> xh(size_t(h) + 1), win(size_t(cnt), W(0)), e(size_t(l) + 2, 0), c(size_t(m) + 2, 0);
        reciprocal(xh.data(), b + l, h);
        middleProduct(win.data(), b, m, xh.data(), h + 1, h - 2, cnt);
        const W *pm = win.data() + 2;
        bool neg = pm[l + 1] != ~W(0);
        if (neg) { std::copy(pm, pm + l + 1, e.data()); }
        else { sub(e.data(), e.data(), l + 2, pm, l + 2); e[size_t(l) + 1] = 0; }
        int le = norm(e.data(), l + 1);
        if (le) { mulAny(c.data(), e.data(), le, xh.data(), h + 1); }
        std::fill(x, x + m + 1, 0); std::copy(xh.begin(), xh.end(), x + l);
        int lc = norm(c.data() + h, l + 2);
        if (neg) { subFrom(x, m + 1, c.data() + h, lc); }
        else { addInto(x, m + 1, c.data() + h, lc); }
        vector<W> prod(2 * m + 1, 0), num(2 * m + 1, 0);
        mulAny(prod.data(), b, m, x, m + 1); num[2 * m] = 1;
        while (cmp(prod.data(), norm(prod.data(), 2 * m + 1), num.data(), 2 * m + 1) > 0) { subWord(x, m + 1, 1); subFrom(prod.data(), 2 * m + 1, b, m); }
        subFrom(num.data(), 2 * m + 1, prod.data(), 2 * m + 1);
        while (cmp(num.data(), norm(num.data(), 2 * m + 1), b, m) >= 0) { addWord(x, m + 1, 1); subFrom(num.data(), 2 * m + 1, b, m); }}
    // T: O(M(m) * n / m), M: O(n); block division by the reciprocal inv of normalized b (m words), n >= m.
    inline void divNewton(W *q, W *r, const W *a, int n, const W *b, int m, const W *inv) {
        vector<W> z(2 * m, 0), t(2 * m + 2), qe(m + 1), qq;
        int chunks = (n + m - 1) / m;
        if (n == chunks * m && cmp(a + n - m, m, b, m) >= 0) { ++chunks; }
        qq.assign(size_t(chunks) * m, 0);
        for (int c = chunks - 1; c >= 0; --c) {
            int lo = c * m, hi = min(lo + m, n);
            std::copy(a + lo, a + hi, z.data()); std::fill(z.data() + (hi - lo), z.data() + m, 0);
            if (c == chunks - 1) { std::copy(z.data(), z.data() + m, z.data() + m); continue; }
            mulAny(t.data(), z.data() + m - 1, m + 1, inv, m + 1);
            std::copy(t.data() + m + 1, t.data() + 2 * m + 2, qe.data());
            int lq = norm(qe.data(), m + 1);
            std::fill(t.begin(), t.end(), 0);
            if (lq) { mulAny(t.data(), b, m, qe.data(), lq); }
            subFrom(z.data(), 2 * m, t.data(), 2 * m);
            while (cmp(z.data(), norm(z.data(), 2 * m), b, m) >= 0) { addWord(qe.data(), m + 1, 1); subFrom(z.data(), 2 * m, b, m); }
            std::copy(qe.data(), qe.data() + m, qq.data() + lo);
            std::copy(z.data(), z.data() + m, z.data() + m);}
        std::copy(qq.data(), qq.data() + (n - m + 1), q);
        std::copy(z.data() + m, z.data() + 2 * m, r);}
    // T: O(min(m, n - m) * m) word products below BURNIKEL, subquadratic above (a fresh reciprocal for quotients of at least 4m words
    // from NEWTON_LONG); q (n - m + 1 words), r (m words); b of m >= 1 words with b[m - 1] != 0, n >= m; q and r may be null.
    inline void divRem(W *q, W *r, const W *a, int n, const W *b, int m, const W *inv = nullptr) {
        vector<W> qq, rr;
        if (!q) { qq.resize(n - m + 1); q = qq.data(); }
        if (!r) { rr.resize(m); r = rr.data(); }
        if (m == 1) { r[0] = divWord(q, a, n, b[0]); return; }
        int s = std::countl_zero(b[m - 1]);
        vector<W> an(n + 1), bn(m);
        if (s) { an[n] = shl(an.data(), a, n, s); shl(bn.data(), b, m, s); }
        else { std::copy(a, a + n, an.data()); an[n] = 0; std::copy(b, b + m, bn.data()); }
        int na = an[n] ? n + 1 : n;
        vector<W> qt(na - m + 2), iv;
        if (!inv && m >= NEWTON_LONG && na - m >= 4 * m) { iv.resize(size_t(m) + 1); reciprocal(iv.data(), bn.data(), m); inv = iv.data(); }
        if (inv) { divNewton(qt.data(), r, an.data(), na, bn.data(), m, inv); }
        else if (min(m, na - m + 1) < BURNIKEL) { divBasecase(qt.data(), r, an.data(), na, bn.data(), m); }
        else { divBurnikelZiegler(qt.data(), r, an.data(), na, bn.data(), m); }
        std::copy(qt.data(), qt.data() + (n - m + 1), q);
        if (s) { shr(r, r, m, s); }}
    inline W invMod64(W b) {
        W x = b;
        for (int i = 0; i < 6; ++i) { x *= 2 - b * x; }
        return x;}
    // T: O(k * m) word products, M: O(n); q (k = n - m + 1 words) = a / b for odd b dividing a, word by word through the 2-adic inverse of b[0].
    inline void divExactHensel(W *q, const W *a, int n, const W *b, int m) {
        int k = n - m + 1;
        if (m == 1) { divWord(q, a, n, b[0]); return; }
        W inv0 = invMod64(b[0]);
        vector<W> u(a, a + n);
        for (int i = 0; i < k; ++i) {
            W qi = u[i] * inv0; q[i] = qi;
            int len = min(m, n - i);
            W borrow = subMul(u.data() + i, b, len, qi);
            if (i + len < n) { subWord(u.data() + i + len, n - i - len, borrow); }}}
    // Binary GCD of a (n words) and b (m words), both nonzero, result in a; returns its length.
    inline int gcdBinary(W *a, int n, W *b, int m) {
        W *out = a;
        auto ctz = [](const W *x, int len) { int c = 0; while (x[c] == 0 && c + 1 < len) { ++c; } return 64 * c + std::countr_zero(x[c]); };
        auto shiftRight = [](W *x, int &len, int bits) {
            int w = bits / 64, s = bits % 64;
            if (w) { std::copy(x + w, x + len, x); len -= w; }
            if (s) { shr(x, x, len, s); }
            len = norm(x, len);};
        int za = ctz(a, n), zb = ctz(b, m), z = min(za, zb);
        shiftRight(a, n, za); shiftRight(b, m, zb);
        while (m) {
            if (cmp(a, n, b, m) < 0) { std::swap(a, b); std::swap(n, m); }
            sub(a, a, n, b, m); n = norm(a, n);
            if (n) { shiftRight(a, n, ctz(a, n)); }
            else { std::swap(a, b); std::swap(n, m); break; }}
        int w = z / 64, s = z % 64;
        vector<W> res(a, a + n);
        if (s) { res.push_back(shl(res.data(), res.data(), n, s)); }
        res.insert(res.begin(), w, 0);
        n = norm(res.data(), int(res.size()));
        std::copy(res.begin(), res.begin() + n, out);
        return n;}
} // namespace infint_detail
template<int N> struct UInt;

// T: O(n) words per linear operation, M: O(n); sign-magnitude base 2^32 limbs n (no leading zeros, zero is {} or {0}) plus a signed infinity.
struct InfInt {
    using W = infint_detail::W;
    using WW = infint_detail::WW;
    static constexpr const char *DIGITS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static constexpr array<uint8_t, 256> DIGIT_VALUE = []() {
        array<uint8_t, 256> v{}; v.fill(255);
        for (int i = 0; i < 36; ++i) { v[size_t(DIGITS[i])] = uint8_t(i); if (i >= 10) { v[size_t(DIGITS[i] + 32)] = uint8_t(i); } }
        return v;}();
    static constexpr array<uint, 10000> ITOS4 = []() {
        array<uint, 10000> v{};
        for (int i = 0; i < 10000; ++i) { v[size_t(i)] = uint(0x30303030 + i / 1000 + (i / 100 % 10 << 8) + (i / 10 % 10 << 16) + (i % 10 << 24)); }
        return v;}();
    template<typename T> static constexpr bool isScalar = std::is_integral_v<T> && sizeof(T) <= 8 && !std::same_as<std::remove_cv_t<T>, bool>;
    template<typename T> static constexpr bool isIntegral = std::is_integral_v<T> || std::same_as<T, lll> || std::same_as<T, ulll>;

    int sgn = 1; bool is_inf = false;
    vector<uint> n;

    InfInt() {}
    template<typename T> requires isIntegral<T> InfInt(T x) { *this = x; }
    InfInt(string_view s, uint base = 10) { bool ok = read(s, base); assert(ok); (void)ok; }
    // T: O(M(n) * log(n)); digits most significant first, each below base in [2, 2^32); sign -1 or 1.
    InfInt(const vector<uint> &digits, uint base = 10, int sign = 1) {
        assert(base >= 2 && (sign == 1 || sign == -1) && std::ranges::all_of(digits, [&](uint d) { return d < base; }));
        n = fromDigits(int(digits.size()), base, [&](int i) { return digits[size_t(i)]; });
        trim(); sgn = isNil() ? 1 : sign;}
    template<int N> InfInt(const UInt<N> &u) { setWords(u.w.data(), N); }
    // T: O(n); little-endian magnitude limbs (base 2^32 or 2^64) with a sign.
    static InfInt fromLimbs(const vector<uint> &limbs, int sign = 1) {
        assert(sign == 1 || sign == -1);
        InfInt r; r.n = limbs; r.trim(); r.sgn = r.isNil() ? 1 : sign;
        return r;}
    static InfInt fromLimbs(const vector<ulng> &limbs, int sign = 1) {
        assert(sign == 1 || sign == -1);
        InfInt r; r.setWords(limbs.data(), int(limbs.size())); r.sgn = r.isNil() ? 1 : sign;
        return r;}
    // T: O(b / w); the integer part of a non-NaN double, infinities to the sentinel.
    static InfInt fromDouble(double d) {
        assert(!std::isnan(d));
        InfInt r;
        if (std::isinf(d)) { r.is_inf = true; r.sgn = d < 0 ? -1 : 1; return r; }
        d = std::trunc(d);
        if (d == 0) { return r; }
        int e; double f = std::frexp(std::fabs(d), &e);
        r = ulng(std::ldexp(f, 53));
        if (e >= 53) { r <<= e - 53; }
        else { r >>= 53 - e; }
        r.sgn = d < 0 ? -1 : 1;
        return r;}
    template<typename R> static ulng draw64(R &rng) {
        static_assert(R::min() == 0 && (R::max() == 0xFFFFFFFFu || R::max() == ~ulng(0)), "sample needs a 32- or 64-bit full-range generator");
        if constexpr (R::max() == ~ulng(0)) { return ulng(rng()); }
        else { return (ulng(rng()) << 32) | ulng(rng()); }}
    template<typename R> static InfInt randomBits(int bits, R &rng) {
        InfInt r;
        if (bits <= 0) { return r; }
        r.n.resize(size_t(bits + 31) / 32);
        for (size_t i = 0; i < r.n.size(); i += 2) {
            ulng x = draw64(rng); r.n[i] = uint(x);
            if (i + 1 < r.n.size()) { r.n[i + 1] = uint(x >> 32); }}
        r.n.back() &= (2u << ((bits - 1) % 32)) - 1;
        return r;}
    // T: O(b / w) expected; uniform on [0, bound) for finite bound > 0 (rejection sampling) and a value of exactly bits bits; rng is
    // std::mt19937, std::mt19937_64 or another generator with min() 0 and max() 2^32 - 1 or 2^64 - 1.
    template<typename R> friend InfInt sample(const InfInt &bound, R &rng) {
        assert(!bound.is_inf && bound.sgn == 1 && !bound.isNil());
        if (bound.is_inf || bound.sgn == -1 || bound.isNil()) { return 0; }
        int bits = bitLength(bound);
        while (true) {
            InfInt r = randomBits(bits, rng); r.trim();
            if (r < bound) { return r; }}}
    template<typename R> static InfInt sampleBits(int bits, R &rng) {
        InfInt r = randomBits(bits, rng);
        if (bits > 0) { r.n.back() |= 1u << ((bits - 1) % 32); }
        return r;}
    template<typename T> requires isIntegral<T> InfInt &operator=(T x) {
        is_inf = false; sgn = 1; ulll u = ulll(x);
        if constexpr (std::is_signed_v<T> || std::same_as<T, lll>) { if (x < 0) { sgn = -1; u = ulll(0) - u; } }
        n.assign(1, uint(u));
        while (u >>= 32) { n.push_back(uint(u)); }
        return *this;}

    // T: O(1)
    bool isNil() const { return !is_inf && (n.empty() || (n.size() == 1 && n[0] == 0)); }
    explicit operator bool() const { return !isNil(); }
    void trim() {
        if (is_inf) { n.clear(); return; }
        while (n.size() > 1 && n.back() == 0) { n.pop_back(); }
        if (n.empty()) { n.push_back(0); }
        if (n.size() == 1 && n[0] == 0) { sgn = 1; }}
    ulng low64() const { return n.empty() ? 0 : n[0] | (n.size() > 1 ? ulng(n[1]) << 32 : 0); }
    bool fitsWord() const { return n.size() <= 2; }
    int words() const { return int(n.size() + 1) / 2; }
    vector<W> toWords() const {
        vector<W> w(size_t(words()), 0);
        if (!n.empty()) { std::memcpy(w.data(), n.data(), 4 * n.size()); }
        return w;}
    void setWords(const W *w, int len) {
        n.resize(2 * size_t(len));
        if (len) { std::memcpy(n.data(), w, 8 * size_t(len)); }
        trim();}
    // T: O(1); finite values convert modulo 2^64 (two's complement), infinity saturates.
    explicit operator lng() const {
        if (is_inf) { return sgn == 1 ? LLONG_MAX : LLONG_MIN; }
        return lng(sgn == 1 ? low64() : ulng(0) - low64());}
    explicit operator ulng() const {
        if (is_inf) { return sgn == 1 ? ULLONG_MAX : 0; }
        return sgn == 1 ? low64() : ulng(0) - low64();}
    // T: O(1); true iff the value is representable in T (integral through 128 bits), then stores it.
    template<typename T> requires isIntegral<T> bool tryCast(T &out) const {
        if (is_inf || n.size() > 4) { return false; }
        ulll mag = 0;
        for (int i = int(n.size()) - 1; i >= 0; --i) { mag = (mag << 32) | n[size_t(i)]; }
        if constexpr (std::is_signed_v<T> || std::same_as<T, lll>) {
            ulll lim = ulll(std::numeric_limits<T>::max());
            if (mag > lim + (sgn == -1)) { return false; }
            out = sgn == -1 ? T(-lll(mag - 1) - 1) : T(mag);}
        else {
            if ((sgn == -1 && mag) || mag > ulll(std::numeric_limits<T>::max())) { return false; }
            out = T(mag);}
        return true;}
    // T: O(n); correctly rounded (nearest even) conversion, overflow and the sentinel give +-infinity.
    double toDouble() const {
        if (is_inf) { return sgn * std::numeric_limits<double>::infinity(); }
        if (isNil()) { return 0.0; }
        int bl = bitLength(*this), shift = max(0, bl - 64);
        ulng m = 0;
        if (shift == 0) { m = low64(); }
        else {
            InfInt t = abs(*this); bool sticky = false;
            for (int i = 0; i < shift / 32; ++i) { sticky |= t.n[size_t(i)] != 0; }
            if (shift % 32) { sticky |= (t.n[size_t(shift / 32)] & ((1u << (shift % 32)) - 1)) != 0; }
            t >>= shift; m = t.low64() | ulng(sticky);}
        return sgn * std::ldexp(double(m), shift);}
    template<typename T = uint> vector<T> toLimbs() const {
        assert(!is_inf);
        if constexpr (std::same_as<T, uint>) { return n.empty() ? vector<uint>{0} : n; }
        else { static_assert(std::same_as<T, ulng>); return toWords(); }}

    // Radix conversion: blocks of DIGITS digits in base BP = base^DIGITS, cached powers BP^(2^j) with reciprocals.
    struct Radix { W bp; int digits; vector<vector<W>> pow, inv; };
    static Radix &radix(uint base) {
        static unordered_map<uint, Radix> cache;
        Radix &r = cache[max(base, 2u)];
        if (r.digits == 0) {
            base = max(base, 2u); r.bp = base; r.digits = 1;
            while (r.bp <= ~W(0) / base) { r.bp *= base; ++r.digits; }
            r.pow.push_back({r.bp});}
        return r;}
    static const vector<W> &radixPow(Radix &r, int j) {
        while (int(r.pow.size()) <= j) {
            const vector<W> &p = r.pow.back(); vector<W> sq(2 * p.size());
            infint_detail::square(sq.data(), p.data(), int(p.size()));
            sq.resize(size_t(infint_detail::norm(sq.data(), int(sq.size()))));
            r.pow.push_back(std::move(sq));}
        return r.pow[size_t(j)];}
    static const vector<W> &radixInv(Radix &r, int j) {
        const vector<W> &p = radixPow(r, j);
        if (int(r.inv.size()) <= j) { r.inv.resize(size_t(j) + 1); }
        if (r.inv[size_t(j)].empty()) {
            int m = int(p.size()); vector<W> bn(p); int s = std::countl_zero(bn[size_t(m) - 1]);
            if (s) { infint_detail::shl(bn.data(), bn.data(), m, s); }
            r.inv[size_t(j)].resize(size_t(m) + 1);
            infint_detail::reciprocal(r.inv[size_t(j)].data(), bn.data(), m);}
        return r.inv[size_t(j)];}
    // T: O(M(n) * log(n)); little-endian blocks to words.
    static vector<W> fromBlocks(const W *blk, int cnt, Radix &r) {
        if (cnt <= infint_detail::RADIX) {
            vector<W> v(size_t(max(cnt, 1)), 0); int len = 0;
            for (int i = cnt - 1; i >= 0; --i) {
                W c = infint_detail::mulWord(v.data(), v.data(), len, r.bp);
                if (c) { v[size_t(len++)] = c; }
                c = infint_detail::addWord(v.data(), len, blk[i]);
                if (c) { v[size_t(len++)] = c; }}
            v.resize(size_t(len));
            return v;}
        int j = 31 - std::countl_zero(uint(cnt - 1)), h = 1 << j;
        vector<W> lo = fromBlocks(blk, h, r), hi = fromBlocks(blk + h, cnt - h, r);
        const vector<W> &p = radixPow(r, j);
        vector<W> res(hi.size() + p.size() + 1, 0);
        if (!hi.empty()) { infint_detail::mulAny(res.data(), hi.data(), int(hi.size()), p.data(), int(p.size())); }
        infint_detail::addInto(res.data(), int(res.size()), lo.data(), int(lo.size()));
        res.resize(size_t(infint_detail::norm(res.data(), int(res.size()))));
        return res;}
    // T: O(M(n) * log(n)); words to little-endian blocks, padded to 2^(j + 1) blocks when pad.
    static void toBlocks(vector<W> &out, vector<W> v, int j, bool pad, Radix &r) {
        int len = infint_detail::norm(v.data(), int(v.size()));
        if (j < 0 || len <= infint_detail::RADIX) {
            size_t start = out.size();
            while (len) { out.push_back(infint_detail::divWord(v.data(), v.data(), len, r.bp)); len = infint_detail::norm(v.data(), len); }
            if (pad) { out.resize(start + (size_t(2) << j), 0); }
            return;}
        const vector<W> &p = radixPow(r, j); int m = int(p.size());
        vector<W> q(size_t(max(len - m + 1, 1)), W(0)), rem(size_t(m), W(0));
        if (len < m) { q.assign(1, 0); std::copy(v.begin(), v.begin() + len, rem.begin()); std::fill(rem.begin() + len, rem.end(), 0); }
        else { infint_detail::divRem(q.data(), rem.data(), v.data(), len, p.data(), m, m >= infint_detail::NEWTON_DIV ? radixInv(r, j).data() : nullptr); }
        toBlocks(out, std::move(rem), j - 1, true, r);
        toBlocks(out, std::move(q), j - 1, pad, r);}
    static vector<W> toBlocks(const vector<W> &v, Radix &r) {
        vector<W> out;
        int len = infint_detail::norm(v.data(), int(v.size())), j = -1;
        if (len > infint_detail::RADIX) {
            while (true) {
                const vector<W> &p = radixPow(r, j + 1);
                if (infint_detail::cmp(v.data(), len, p.data(), int(p.size())) < 0) { break; }
                ++j;}}
        toBlocks(out, v, j, false, r);
        while (!out.empty() && out.back() == 0) { out.pop_back(); }
        return out;}
    template<typename G> static vector<uint> fromDigits(int len, uint base, const G &get) {
        Radix &r = radix(base);
        int cnt = (len + r.digits - 1) / r.digits;
        vector<W> blk(size_t(max(cnt, 1)), 0);
        for (int b = 0; b < cnt; ++b) {
            int hi = len - b * r.digits, lo = max(hi - r.digits, 0); W v = 0;
            for (int i = lo; i < hi; ++i) { v = v * base + get(i); }
            blk[size_t(b)] = v;}
        vector<W> w = fromBlocks(blk.data(), cnt, r);
        vector<uint> limbs(2 * w.size());
        if (!w.empty()) { std::memcpy(limbs.data(), w.data(), 8 * w.size()); }
        return limbs;}
    static ulng parse8(const char *p) {
        ulng v; std::memcpy(&v, p, 8);
        v = (v & 0x0F0F0F0F0F0F0F0FULL) * 2561 >> 8;
        v = (v & 0x00FF00FF00FF00FFULL) * 6553601 >> 16;
        return (v & 0x0000FFFF0000FFFFULL) * 42949672960001ULL >> 32;}
    static vector<uint> parsePow2(string_view s, int bits) {
        int len = int(s.size()); vector<uint> limbs(size_t(len) * size_t(bits) / 32 + 1, 0);
        int i = 0;
        if (bits == 4) {
#ifdef __AVX2__
            __m256i m0f = _mm256_set1_epi8(0x0F), v9 = _mm256_set1_epi8('9'), w16 = _mm256_set1_epi16(0x0110), w256 = _mm256_set1_epi32(0x00010100);
            __m256i rev = _mm256_setr_epi32(6, 4, 2, 0, 0, 0, 0, 0);
            for (; i + 32 <= len; i += 32) {
                __m256i x = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(s.data() + len - i - 32));
                x = _mm256_add_epi8(_mm256_and_si256(x, m0f), _mm256_and_si256(_mm256_cmpgt_epi8(x, v9), _mm256_set1_epi8(9)));
                x = _mm256_madd_epi16(_mm256_maddubs_epi16(x, w16), w256);
                __m256i y = _mm256_permutevar8x32_epi32(_mm256_or_si256(_mm256_slli_epi32(x, 16), _mm256_srli_epi64(x, 32)), rev);
                _mm_storeu_si128(reinterpret_cast<__m128i *>(&limbs[size_t(i) / 8]), _mm256_castsi256_si128(y));}
#endif
            for (; i + 8 <= len; i += 8) {
                ulng cur; std::memcpy(&cur, s.data() + len - i - 8, 8);
                cur = (cur & 0x0F0F0F0F0F0F0F0FULL) + 9 * ((cur & 0x4040404040404040ULL) >> 6);
                cur = ((cur << 4) | (cur >> 8)) & 0x00FF00FF00FF00FFULL;
                cur = ((cur << 8) | (cur >> 16)) & 0x0000FFFF0000FFFFULL;
                limbs[size_t(i) / 8] = uint((cur << 16) | (cur >> 32));}}
        for (; i < len; ++i) {
            uint d = DIGIT_VALUE[size_t(uint8_t(s[size_t(len - i - 1)]))]; int bit = i * bits;
            limbs[size_t(bit / 32)] |= d << (bit % 32);
            if (bit % 32 + bits > 32) { limbs[size_t(bit / 32) + 1] |= d >> (32 - bit % 32); }}
        return limbs;}
    // T: O(L) for power-of-two bases, O(M(L) * log(L)) otherwise; optional sign, digits of base 2-36 (either case) or, in bases up
    // to 23, "inf"; false and no change when malformed.
    bool read(string_view s, uint base = 10) {
        assert(base >= 2 && base <= 36);
        base = std::clamp(base, 2u, 36u);
        int sign = 1;
        if (!s.empty() && (s[0] == '+' || s[0] == '-')) { sign = s[0] == '-' ? -1 : 1; s.remove_prefix(1); }
        if (s.empty()) { return false; }
        if (base <= 23 && s.size() == 3 && (s[0] | 32) == 'i' && (s[1] | 32) == 'n' && (s[2] | 32) == 'f') { sgn = sign; is_inf = true; n.clear(); return true; }
        for (char c : s) { if (DIGIT_VALUE[size_t(uint8_t(c))] >= base) { return false; } }
        while (s.size() > 1 && s[0] == '0') { s.remove_prefix(1); }
        int len = int(s.size()); is_inf = false;
        if (std::has_single_bit(base)) { n = parsePow2(s, std::countr_zero(base)); }
        else if (base == 10 && len > 19) {
            Radix &r = radix(10); int cnt = (len + 18) / 19;
            vector<W> blk(size_t(cnt), W(0));
            for (int b = 0; b + 1 < cnt; ++b) {
                const char *p = s.data() + len - 19 * (b + 1);
                blk[size_t(b)] = (parse8(p) * 100000000 + parse8(p + 8)) * 1000 + ulng(p[16] - '0') * 100 + ulng(p[17] - '0') * 10 + ulng(p[18] - '0');}
            W top = 0;
            for (int i = 0; i < len - 19 * (cnt - 1); ++i) { top = top * 10 + ulng(s[size_t(i)] - '0'); }
            blk[size_t(cnt) - 1] = top;
            vector<W> w = fromBlocks(blk.data(), cnt, r);
            setWords(w.data(), int(w.size()));}
        else { n = fromDigits(len, base, [&](int i) { return uint(DIGIT_VALUE[size_t(uint8_t(s[size_t(i)]))]); }); }
        is_inf = false; trim(); sgn = isNil() ? 1 : sign;
        return true;}
    static void appendPow2(string &out, const vector<uint> &limbs, int bits) {
        int total = 32 * int(limbs.size()) - std::countl_zero(limbs.back()), cnt = (total + bits - 1) / bits;
        size_t start = out.size(); out.resize(start + size_t(cnt));
        int i = 0;
        if (bits == 4) {
            char *o = out.data() + start + cnt;
#ifdef __AVX2__
            __m256i m16 = _mm256_set1_epi64x(0x0000FFFF0000FFFFLL), m8 = _mm256_set1_epi64x(0x00FF00FF00FF00FFLL), m4 = _mm256_set1_epi64x(0x0F0F0F0F0F0F0F0FLL);
            for (; i + 4 <= int(limbs.size()) && (i + 4) * 8 <= cnt; i += 4) {
                __m256i cur = _mm256_cvtepu32_epi64(_mm_loadu_si128(reinterpret_cast<const __m128i *>(&limbs[size_t(i)])));
                cur = _mm256_and_si256(_mm256_or_si256(cur, _mm256_slli_epi64(cur, 16)), m16);
                cur = _mm256_and_si256(_mm256_or_si256(cur, _mm256_slli_epi64(cur, 8)), m8);
                cur = _mm256_and_si256(_mm256_or_si256(cur, _mm256_slli_epi64(cur, 4)), m4);
                __m256i alpha = _mm256_cmpgt_epi8(cur, _mm256_set1_epi8(9));
                cur = _mm256_add_epi8(_mm256_add_epi8(cur, _mm256_set1_epi8('0')), _mm256_and_si256(alpha, _mm256_set1_epi8(7)));
                char buf[32]; _mm256_storeu_si256(reinterpret_cast<__m256i *>(buf), cur);
                for (int k = 0; k < 4; ++k) { for (int d = 0; d < 8; ++d) { *--o = buf[8 * k + d]; } }}
#endif
            for (; i < int(limbs.size()) && (i + 1) * 8 <= cnt; ++i) {
                ulng cur = limbs[size_t(i)];
                cur = (cur | (cur << 16)) & 0x0000FFFF0000FFFFULL;
                cur = (cur | (cur << 8)) & 0x00FF00FF00FF00FFULL;
                cur = (cur | (cur << 4)) & 0x0F0F0F0F0F0F0F0FULL;
                cur = (cur + 0x3030303030303030ULL) + 7 * (((cur + 0x3636363636363636ULL) & 0x4040404040404040ULL) >> 6);
                char buf[8]; std::memcpy(buf, &cur, 8);
                for (int d = 0; d < 8; ++d) { *--o = buf[d]; }}
            i *= 8;}
        for (; i < cnt; ++i) {
            int bit = i * bits; uint d = limbs[size_t(bit / 32)] >> (bit % 32);
            if (bit % 32 + bits > 32 && size_t(bit / 32) + 1 < limbs.size()) { d |= limbs[size_t(bit / 32) + 1] << (32 - bit % 32); }
            out[start + size_t(cnt - 1 - i)] = DIGITS[d & ((1u << bits) - 1)];}}
    // T: O(L) for power-of-two bases, O(M(L) * log(L)) otherwise; uppercase digits, "inf" / "-inf" for the sentinel.
    string toString(uint base = 10) const {
        assert(base >= 2 && base <= 36);
        base = std::clamp(base, 2u, 36u);
        if (is_inf) { return sgn == -1 ? "-inf" : "inf"; }
        if (isNil()) { return "0"; }
        string out;
        if (sgn == -1) { out += '-'; }
        if (std::has_single_bit(base)) { appendPow2(out, n, std::countr_zero(base)); return out; }
        Radix &r = radix(base);
        vector<W> blk = toBlocks(toWords(), r);
        int cnt = int(blk.size());
        if (base == 10) {
            char buf[24]; int top = std::snprintf(buf, sizeof buf, "%llu", static_cast<unsigned long long>(blk[size_t(cnt) - 1]));
            out.reserve(out.size() + size_t(top) + 19 * size_t(cnt - 1)); out.append(buf, size_t(top));
            for (int b = cnt - 2; b >= 0; --b) {
                W v = blk[size_t(b)]; char d[19];
                for (int g = 3; g >= 0; --g) { std::memcpy(d + 3 + 4 * g, &ITOS4[size_t(v % 10000)], 4); v /= 10000; }
                d[0] = char('0' + v / 100); d[1] = char('0' + v / 10 % 10); d[2] = char('0' + v % 10);
                out.append(d, 19);}
            return out;}
        size_t start = out.size(); out.resize(start + size_t(cnt) * size_t(r.digits));
        char *o = out.data() + out.size();
        for (int b = 0; b < cnt; ++b) {
            W v = blk[size_t(b)];
            for (int d = 0; d < r.digits; ++d) { *--o = DIGITS[v % base]; v /= base; }}
        size_t z = start;
        while (z + 1 < out.size() && out[z] == '0') { ++z; }
        out.erase(start, z - start);
        return out;}
    // T: O(M(L) * log(L)); digits most significant first in base [2, 2^32), {0} for zero.
    vector<uint> toDigits(uint base) const {
        assert(!is_inf && base >= 2);
        base = max(base, 2u);
        if (isNil()) { return {0}; }
        Radix &r = radix(base);
        vector<W> blk = toBlocks(toWords(), r);
        vector<uint> out(blk.size() * size_t(r.digits));
        size_t o = out.size();
        for (W v : blk) { for (int d = 0; d < r.digits; ++d) { out[--o] = uint(v % base); v /= base; } }
        size_t z = 0;
        while (z + 1 < out.size() && out[z] == 0) { ++z; }
        out.erase(out.begin(), out.begin() + ptrdiff_t(z));
        return out;}
    // Magnitude helpers on limbs: |this| += o, |this| -= o (|this| >= o), |this| = o - |this| (o > |this|).
    void addMag(const vector<uint> &o) {
        if (n.size() < o.size()) { n.resize(o.size(), 0); }
        ulng c = 0; size_t i = 0;
        for (; i < o.size(); ++i) { c += ulng(n[i]) + o[i]; n[i] = uint(c); c >>= 32; }
        for (; c && i < n.size(); ++i) { c += n[i]; n[i] = uint(c); c >>= 32; }
        if (c) { n.push_back(uint(c)); }}
    void subMag(const vector<uint> &o) {
        ulng b = 0; size_t i = 0;
        for (; i < o.size(); ++i) { ulng c = ulng(n[i]) - o[i] - b; n[i] = uint(c); b = c >> 63; }
        for (; b && i < n.size(); ++i) { ulng c = ulng(n[i]) - b; n[i] = uint(c); b = c >> 63; }
        trim();}
    void rsubMag(const vector<uint> &o) {
        n.resize(o.size(), 0); ulng b = 0;
        for (size_t i = 0; i < o.size(); ++i) { ulng c = ulng(o[i]) - n[i] - b; n[i] = uint(c); b = c >> 63; }
        trim();}
    // inf + x = inf, inf + (-inf) = 0.
    InfInt &operator+=(const InfInt &o) {
        if (is_inf || o.is_inf) {
            if (is_inf && o.is_inf && sgn != o.sgn) { return *this = 0; }
            if (o.is_inf) { sgn = o.sgn; is_inf = true; }
            n.clear();
            return *this;}
        if (o.isNil()) { return *this; }
        if (isNil()) { return *this = o; }
        if (sgn == o.sgn) { addMag(o.n); return *this; }
        auto c = magCmp(*this, o);
        if (c == 0) { return *this = 0; }
        if (c > 0) { subMag(o.n); }
        else { rsubMag(o.n); sgn = o.sgn; }
        return *this;}
    InfInt &operator-=(const InfInt &o) {
        if (this == &o) { return *this = 0; }
        if (is_inf || o.is_inf) {
            if (is_inf && o.is_inf && sgn == o.sgn) { return *this = 0; }
            if (o.is_inf) { sgn = -o.sgn; is_inf = true; }
            n.clear();
            return *this;}
        if (o.isNil()) { return *this; }
        if (isNil()) { *this = o; sgn = -sgn; trim(); return *this; }
        if (sgn != o.sgn) { addMag(o.n); return *this; }
        auto c = magCmp(*this, o);
        if (c == 0) { return *this = 0; }
        if (c > 0) { subMag(o.n); }
        else { rsubMag(o.n); sgn = -sgn; }
        return *this;}
    InfInt &operator++() {
        if (is_inf) { return *this; }
        if (sgn == -1) { sgn = 1; --*this; sgn = isNil() ? 1 : -1; return *this; }
        for (size_t i = 0; i < n.size(); ++i) { if (++n[i] != 0) { return *this; } }
        n.push_back(1);
        return *this;}
    InfInt &operator--() {
        if (is_inf) { return *this; }
        if (isNil()) { n.assign(1, 1); sgn = -1; return *this; }
        if (sgn == -1) { sgn = 1; ++*this; sgn = -1; return *this; }
        for (size_t i = 0; i < n.size(); ++i) { if (n[i]-- != 0) { break; } }
        trim();
        return *this;}
    InfInt operator++(int) { InfInt r = *this; ++*this; return r; }
    InfInt operator--(int) { InfInt r = *this; --*this; return r; }

    template<typename T> requires isScalar<T> static pair<int, ulng> sgnMag(T x) {
        if constexpr (std::is_signed_v<T>) { if (x < 0) { return {-1, ulng(0) - ulng(x)}; } }
        return {1, ulng(x)};}
    // T: O(n); inf * 0 = 0.
    template<typename T> requires isScalar<T> InfInt &operator*=(T o) {
        auto [os, u] = sgnMag(o);
        if (isNil() || u == 0) { return *this = 0; }
        sgn *= os;
        if (is_inf || u == 1) { return *this; }
        if (std::has_single_bit(u)) { return *this <<= std::countr_zero(u); }
        ulng c = 0;
        if (u >> 32) {
            for (uint &x : n) { ulll t = ulll(x) * u + c; x = uint(t); c = ulng(t >> 32); }
            while (c) { n.push_back(uint(c)); c >>= 32; }
            return *this;}
        for (uint &x : n) { c += ulng(x) * u; x = uint(c); c >>= 32; }
        if (c) { n.push_back(uint(c)); }
        return *this;}
    // T: O(M(n)); squares when both operands are the same object or equal.
    InfInt &operator*=(const InfInt &o) {
        if (isNil() || o.isNil()) { return *this = 0; }
        if (is_inf || o.is_inf) { sgn *= o.sgn; is_inf = true; n.clear(); return *this; }
        if (o.fitsWord()) { int os = o.sgn; *this *= o.low64(); sgn *= os; return *this; }
        if (fitsWord()) { ulng u = low64(); int s = sgn; *this = o; *this *= u; sgn *= s; return *this; }
        int s = sgn * o.sgn;
        vector<W> a = toWords(), r(2 * a.size());
        if (this == &o || n == o.n) { infint_detail::square(r.data(), a.data(), int(a.size())); }
        else {
            vector<W> b = o.toWords(); r.resize(a.size() + b.size());
            if (a.size() >= b.size()) { infint_detail::mul(r.data(), a.data(), int(a.size()), b.data(), int(b.size())); }
            else { infint_detail::mul(r.data(), b.data(), int(b.size()), a.data(), int(a.size())); }}
        setWords(r.data(), int(r.size())); sgn = s;
        return *this;}
    // T: O(n); truncating; inf / x = inf, x / inf = 0.
    template<typename T> requires isScalar<T> InfInt &operator/=(T o) {
        auto [os, u] = sgnMag(o);
        assert(u != 0);
        if (is_inf) { sgn *= os; return *this; }
        if (isNil()) { return *this; }
        int s = sgn * os;
        if (std::has_single_bit(u)) { sgn = 1; *this >>= std::countr_zero(u); }
        else { vector<W> w = toWords(); infint_detail::divWord(w.data(), w.data(), int(w.size()), u); setWords(w.data(), int(w.size())); }
        sgn = isNil() ? 1 : s;
        return *this;}
    // T: O(n); the remainder keeps the dividend's sign; inf % x = 0, x % inf = x.
    template<typename T> requires isScalar<T> InfInt &operator%=(T o) {
        ulng u = sgnMag(o).second;
        assert(u != 0);
        if (is_inf || isNil()) { return *this = 0; }
        int s = sgn; ulng r;
        if (std::has_single_bit(u)) { r = low64() & (u - 1); }
        else { vector<W> w = toWords(); r = infint_detail::divWord(w.data(), w.data(), int(w.size()), u); }
        *this = r; sgn = isNil() ? 1 : s;
        return *this;}
    // Signs of a truncating quotient and remainder, including the sentinel rules of / and %; false when the operands settle it.
    static bool divSentinel(InfInt &a, InfInt &b, pair<InfInt, InfInt> &out) {
        assert(!b.isNil());
        if (b.isNil()) { out = {0, std::move(a)}; return true; }
        if (a.is_inf || b.is_inf) {
            if (a.is_inf && b.is_inf) { out = {a.sgn * b.sgn, 0}; }
            else if (b.is_inf) { out = {0, std::move(a)}; }
            else { a.sgn *= b.sgn; out = {std::move(a), 0}; }
            return true;}
        auto c = magCmp(a, b);
        if (c < 0) { out = {0, std::move(a)}; return true; }
        if (c == 0) { out = {a.sgn * b.sgn, 0}; return true; }
        return false;}
    // T: O(min(m, n - m) * m) word products; Knuth D only.
    friend pair<InfInt, InfInt> divModSlow(InfInt a, InfInt b) {
        pair<InfInt, InfInt> out;
        if (divSentinel(a, b, out)) { return out; }
        int qs = a.sgn * b.sgn, rs = a.sgn;
        vector<W> x = a.toWords(), y = b.toWords(), q(x.size() - y.size() + 1), r(y.size());
        int m = int(y.size()), nn = int(x.size());
        if (m == 1) { r[0] = infint_detail::divWord(q.data(), x.data(), nn, y[0]); }
        else {
            int s = std::countl_zero(y[size_t(m) - 1]);
            x.push_back(0);
            if (s) { x[size_t(nn)] = infint_detail::shl(x.data(), x.data(), nn, s); infint_detail::shl(y.data(), y.data(), m, s); }
            int na = x[size_t(nn)] ? nn + 1 : nn;
            vector<W> qt(size_t(na - m + 1));
            infint_detail::divBasecase(qt.data(), r.data(), x.data(), na, y.data(), m);
            std::copy(qt.begin(), qt.begin() + ptrdiff_t(nn - m + 1), q.begin());
            if (s) { infint_detail::shr(r.data(), r.data(), m, s); }}
        out.first.setWords(q.data(), int(q.size())); out.second.setWords(r.data(), int(r.size()));
        if (!out.first.isNil()) { out.first.sgn = qs; }
        if (!out.second.isNil()) { out.second.sgn = rs; }
        return out;}
    // T: O(M(n) * log(n)); truncating quotient and remainder (remainder keeps a's sign).
    friend pair<InfInt, InfInt> divMod(InfInt a, InfInt b) {
        pair<InfInt, InfInt> out;
        if (divSentinel(a, b, out)) { return out; }
        if (b.fitsWord()) {
            int qs = a.sgn * b.sgn, rs = a.sgn;
            vector<W> w = a.toWords();
            InfInt r = infint_detail::divWord(w.data(), w.data(), int(w.size()), b.low64());
            a.setWords(w.data(), int(w.size()));
            a.sgn = a.isNil() ? 1 : qs; r.sgn = r.isNil() ? 1 : rs;
            return {std::move(a), std::move(r)};}
        int qs = a.sgn * b.sgn, rs = a.sgn;
        vector<W> x = a.toWords(), y = b.toWords(), q(x.size() - y.size() + 1), r(y.size());
        infint_detail::divRem(q.data(), r.data(), x.data(), int(x.size()), y.data(), int(y.size()));
        out.first.setWords(q.data(), int(q.size())); out.second.setWords(r.data(), int(r.size()));
        if (!out.first.isNil()) { out.first.sgn = qs; }
        if (!out.second.isNil()) { out.second.sgn = rs; }
        return out;}
    InfInt &operator/=(const InfInt &o) {
        if (!o.is_inf && o.fitsWord()) { int os = o.sgn; *this /= o.low64(); sgn = isNil() ? 1 : sgn * os; return *this; }
        InfInt b = o;
        return *this = divMod(std::move(*this), std::move(b)).first;}
    InfInt &operator%=(const InfInt &o) {
        if (!o.is_inf && o.fitsWord()) { return *this %= o.low64(); }
        InfInt b = o;
        return *this = divMod(std::move(*this), std::move(b)).second;}

    InfInt operator+() const { return *this; }
    InfInt operator-() const & { InfInt r = *this; if (!r.isNil()) { r.sgn = -r.sgn; } return r; }
    InfInt operator-() && { if (!isNil()) { sgn = -sgn; } return std::move(*this); }
    friend InfInt operator+(InfInt a, const InfInt &b) { a += b; return a; }
    friend InfInt operator-(InfInt a, const InfInt &b) { a -= b; return a; }
    friend InfInt operator*(InfInt a, const InfInt &b) { a *= b; return a; }
    template<typename T> requires isScalar<T> friend InfInt operator*(InfInt a, T b) { a *= b; return a; }
    template<typename T> requires isScalar<T> friend InfInt operator*(T b, InfInt a) { a *= b; return a; }
    friend InfInt operator/(InfInt a, const InfInt &b) { a /= b; return a; }
    template<typename T> requires isScalar<T> friend InfInt operator/(InfInt a, T b) { a /= b; return a; }
    friend InfInt operator%(InfInt a, const InfInt &b) { a %= b; return a; }
    template<typename T> requires isScalar<T> friend InfInt operator%(InfInt a, T b) { a %= b; return a; }
    friend InfInt abs(InfInt a) { a.sgn = 1; return a; }
    // Rounded division modes on finite operands, b != 0: floor, ceiling and Euclidean (remainder in [0, |b|)).
    friend pair<InfInt, InfInt> divModFloor(const InfInt &a, const InfInt &b) {
        assert(!a.is_inf && !b.is_inf);
        auto [q, r] = divMod(a, b);
        if (!r.isNil() && r.sgn != b.sgn) { --q; r += b; }
        return {std::move(q), std::move(r)};}
    friend pair<InfInt, InfInt> divModCeil(const InfInt &a, const InfInt &b) {
        assert(!a.is_inf && !b.is_inf);
        auto [q, r] = divMod(a, b);
        if (!r.isNil() && r.sgn == b.sgn) { ++q; r -= b; }
        return {std::move(q), std::move(r)};}
    friend pair<InfInt, InfInt> divModEuclid(const InfInt &a, const InfInt &b) {
        assert(!a.is_inf && !b.is_inf);
        auto [q, r] = divMod(a, b);
        if (r.sgn == -1) { if (b.sgn == 1) { --q; r += b; } else { ++q; r -= b; } }
        return {std::move(q), std::move(r)};}
    static ulng residue61(const vector<W> &w) {
        constexpr ulng mod61 = (1ULL << 61) - 1; ulng r = 0;
        for (size_t i = w.size(); i-- > 0;) {
            ulll t = (ulll(r) << 64) | w[i], f = (t & mod61) + (t >> 61);
            f = (f & mod61) + (f >> 61); r = ulng(f >= mod61 ? f - mod61 : f);}
        return r;}
    // T: O(k * m) word products for quotients of k <= HENSEL words, else O(M(n) * log(n)); exact quotient a / b for finite b | a, b != 0:
    // Hensel (2-adic) division, then the measured faster quotient-only Burnikel-Ziegler division; the assertion checks modulo 2^61 - 1.
    friend InfInt divExact(const InfInt &a, const InfInt &b) {
        assert(!a.is_inf && !b.is_inf && !b.isNil());
        if (a.isNil()) { return 0; }
        if (b.fitsWord()) { InfInt q = a; q /= b.low64(); q.sgn = q.isNil() ? 1 : a.sgn * b.sgn; return q; }
        int tz = ctz(b);
        InfInt x = abs(a), y = abs(b); x >>= tz; y >>= tz;
        vector<W> xw = x.toWords(), yw = y.toWords();
        int nn = int(xw.size()), m = int(yw.size());
        if (nn < m) { return 0; }
        vector<W> q(size_t(nn - m + 1));
        if (nn - m + 1 <= infint_detail::HENSEL) { infint_detail::divExactHensel(q.data(), xw.data(), nn, yw.data(), m); }
        else { infint_detail::divRem(q.data(), nullptr, xw.data(), nn, yw.data(), m); }
        assert(ulng(ulll(residue61(q)) * residue61(yw) % ((1ULL << 61) - 1)) == residue61(xw));
        InfInt r; r.setWords(q.data(), int(q.size())); r.sgn = r.isNil() ? 1 : a.sgn * b.sgn;
        return r;}
    // T: O(k * M(n)) for k removed factors; divides x by f (|f| >= 2) while possible, returns the count; x finite nonzero.
    friend int removeFactor(InfInt &x, const InfInt &f) {
        assert(!x.is_inf && !f.is_inf && !x.isNil() && magCmp(f, 1) > 0);
        int cnt = 0;
        if (x.is_inf || f.is_inf || x.isNil() || magCmp(f, 1) <= 0) { return 0; }
        if (f.fitsWord() && std::has_single_bit(f.low64())) {
            int k = std::countr_zero(f.low64()), t = ctz(x) / k;
            x >>= t * k; cnt = t;
            if (f.sgn == -1 && (t & 1)) { x.sgn = -x.sgn; }
            return cnt;}
        while (true) {
            auto [q, r] = divMod(x, f);
            if (!r.isNil()) { return cnt; }
            x = std::move(q); ++cnt;}}

    friend std::strong_ordering magCmp(const InfInt &a, const InfInt &b) {
        if (a.is_inf || b.is_inf) { return int(a.is_inf) <=> int(b.is_inf); }
        bool az = a.isNil(), bz = b.isNil();
        if (az || bz) { return int(!az) <=> int(!bz); }
        if (a.n.size() != b.n.size()) { return a.n.size() <=> b.n.size(); }
        for (size_t i = a.n.size(); i-- > 0;) { if (a.n[i] != b.n[i]) { return a.n[i] <=> b.n[i]; } }
        return std::strong_ordering::equal;}
    friend std::strong_ordering operator<=>(const InfInt &a, const InfInt &b) {
        int as = a.isNil() ? 0 : a.sgn, bs = b.isNil() ? 0 : b.sgn;
        if (as != bs) { return as <=> bs; }
        if (as == 0) { return std::strong_ordering::equal; }
        return as == 1 ? magCmp(a, b) : magCmp(b, a);}
    friend bool operator==(const InfInt &a, const InfInt &b) {
        if (a.is_inf || b.is_inf) { return a.is_inf == b.is_inf && a.sgn == b.sgn; }
        if (a.isNil() || b.isNil()) { return a.isNil() && b.isNil(); }
        return a.sgn == b.sgn && a.n == b.n;}

    // T: O(n); shifts of the magnitude left, arithmetic (floor) right; the sentinel is unchanged.
    InfInt &operator<<=(int s) {
        assert(s >= 0);
        if (is_inf || isNil() || s <= 0) { return *this; }
        int q = s / 32, r = s % 32, len = int(n.size());
        if (r == 0) { n.insert(n.begin(), size_t(q), 0); return *this; }
        n.resize(size_t(len + q + 1), 0);
        for (int i = len - 1; i >= 0; --i) {
            ulng cur = ulng(n[size_t(i)]) << r;
            n[size_t(i + q + 1)] |= uint(cur >> 32); n[size_t(i + q)] = uint(cur);}
        std::fill(n.begin(), n.begin() + q, 0);
        trim();
        return *this;}
    InfInt &operator>>=(int s) {
        assert(s >= 0);
        if (is_inf || isNil() || s <= 0) { return *this; }
        int q = s / 32, r = s % 32, len = int(n.size());
        if (q >= len) { return *this = sgn == -1 ? -1 : 0; }
        bool lost = false;
        if (sgn == -1) {
            for (int i = 0; i < q && !lost; ++i) { lost = n[size_t(i)] != 0; }
            lost |= r > 0 && (n[size_t(q)] & ((1u << r) - 1)) != 0;}
        if (r == 0) { n.erase(n.begin(), n.begin() + q); }
        else {
            for (int i = 0; i < len - q; ++i) {
                ulng hi = i + q + 1 < len ? ulng(n[size_t(i + q + 1)]) : 0;
                n[size_t(i)] = uint((n[size_t(i + q)] >> r) | (hi << (32 - r)));}
            n.resize(size_t(len - q));}
        trim();
        if (lost) { --*this; }
        return *this;}
    friend InfInt operator<<(InfInt a, int s) { a <<= s; return a; }
    friend InfInt operator>>(InfInt a, int s) { a >>= s; return a; }
    // Two's-complement limb image of a finite value, ext = all-ones limb for negatives, and the inverse.
    static vector<uint> twosComplement(const InfInt &a, int &ext) {
        vector<uint> v = a.n.empty() ? vector<uint>{0} : a.n; ext = 0;
        if (a.sgn == -1 && !a.isNil()) {
            ext = -1; ulng c = 1;
            for (uint &x : v) { c += ulng(~x); x = uint(c); c >>= 32; }}
        return v;}
    static InfInt fromTwos(vector<uint> v, int ext) {
        InfInt r;
        if (ext) {
            ulng c = 1;
            for (uint &x : v) { c += ulng(~x); x = uint(c); c >>= 32; }
            if (c) { v.push_back(uint(c)); }
            r.sgn = -1;}
        r.n = std::move(v); r.trim();
        if (r.isNil()) { r.sgn = 1; }
        return r;}
    template<typename F> static InfInt bitOp(const InfInt &a, const InfInt &b, F op) {
        assert(!a.is_inf && !b.is_inf);
        int ea, eb; vector<uint> x = twosComplement(a, ea), y = twosComplement(b, eb);
        size_t len = max(x.size(), y.size());
        x.resize(len, uint(ea)); y.resize(len, uint(eb));
        for (size_t i = 0; i < len; ++i) { x[i] = op(x[i], y[i]); }
        return fromTwos(std::move(x), int(op(uint(ea), uint(eb))));}
    // T: O(n); two's-complement semantics on finite values.
    friend InfInt operator&(const InfInt &a, const InfInt &b) {
        if (a.sgn == 1 && b.sgn == 1 && !a.is_inf && !b.is_inf) {
            InfInt r; r.n.resize(min(a.n.size(), b.n.size()));
            for (size_t i = 0; i < r.n.size(); ++i) { r.n[i] = a.n[i] & b.n[i]; }
            r.trim(); return r;}
        return bitOp(a, b, std::bit_and<uint>());}
    friend InfInt operator|(const InfInt &a, const InfInt &b) {
        if (a.sgn == 1 && b.sgn == 1 && !a.is_inf && !b.is_inf) {
            InfInt r = a.n.size() >= b.n.size() ? a : b; const InfInt &o = a.n.size() >= b.n.size() ? b : a;
            for (size_t i = 0; i < o.n.size(); ++i) { r.n[i] |= o.n[i]; }
            r.trim(); return r;}
        return bitOp(a, b, std::bit_or<uint>());}
    friend InfInt operator^(const InfInt &a, const InfInt &b) {
        if (a.sgn == 1 && b.sgn == 1 && !a.is_inf && !b.is_inf) {
            InfInt r = a.n.size() >= b.n.size() ? a : b; const InfInt &o = a.n.size() >= b.n.size() ? b : a;
            for (size_t i = 0; i < o.n.size(); ++i) { r.n[i] ^= o.n[i]; }
            r.trim(); return r;}
        return bitOp(a, b, std::bit_xor<uint>());}
    InfInt operator~() const { assert(!is_inf); InfInt r = -*this; --r; return r; }
    InfInt &operator&=(const InfInt &o) { return *this = *this & o; }
    InfInt &operator|=(const InfInt &o) { return *this = *this | o; }
    InfInt &operator^=(const InfInt &o) { return *this = *this ^ o; }
    // T: O(n); bit queries on the magnitude (ctz, popcount, bitLength) and on the two's-complement image (testBit, scans).
    friend int ctz(const InfInt &a) {
        assert(!a.is_inf);
        if (a.is_inf || a.isNil()) { return 0; }
        int c = 0;
        while (a.n[size_t(c)] == 0) { ++c; }
        return 32 * c + std::countr_zero(a.n[size_t(c)]);}
    friend int popcount(const InfInt &a) {
        assert(!a.is_inf);
        int c = 0; size_t i = 0;
#ifdef __AVX2__
        __m256i lut = _mm256_broadcastsi128_si256(_mm_setr_epi8(0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4));
        __m256i msk = _mm256_set1_epi8(0x0F), acc = _mm256_setzero_si256();
        for (; i + 8 <= a.n.size(); i += 8) {
            __m256i x = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(&a.n[i]));
            __m256i cur = _mm256_add_epi8(_mm256_shuffle_epi8(lut, _mm256_and_si256(x, msk)), _mm256_shuffle_epi8(lut, _mm256_and_si256(_mm256_srli_epi16(x, 4), msk)));
            acc = _mm256_add_epi64(acc, _mm256_sad_epu8(cur, _mm256_setzero_si256()));}
        ulng res[4]; _mm256_storeu_si256(reinterpret_cast<__m256i *>(res), acc);
        c = int(res[0] + res[1] + res[2] + res[3]);
#endif
        for (; i < a.n.size(); ++i) { c += std::popcount(a.n[i]); }
        return c;}
    friend int bitLength(const InfInt &a) {
        assert(!a.is_inf);
        return a.is_inf || a.isNil() ? 0 : 32 * int(a.n.size()) - std::countl_zero(a.n.back());}
    friend bool testBit(const InfInt &a, int i) {
        assert(!a.is_inf && i >= 0);
        if (a.is_inf || a.isNil() || i < 0) { return false; }
        if (a.sgn == 1) { return size_t(i / 32) < a.n.size() && (a.n[size_t(i / 32)] >> (i % 32) & 1); }
        int t = ctz(a);
        if (i <= t) { return i == t; }
        return size_t(i / 32) >= a.n.size() || !(a.n[size_t(i / 32)] >> (i % 32) & 1);}
    friend int scan1(const InfInt &a, int i) {
        assert(!a.is_inf && i >= 0);
        if (a.is_inf) { return -1; }
        i = max(i, 0); int ext; vector<uint> v = twosComplement(a, ext);
        for (size_t k = size_t(i / 32); k < v.size(); ++k) {
            uint x = v[k] & (k == size_t(i / 32) ? ~0u << (i % 32) : ~0u);
            if (x) { return 32 * int(k) + std::countr_zero(x); }}
        return ext ? max(i, 32 * int(v.size())) : -1;}
    friend int scan0(const InfInt &a, int i) {
        assert(!a.is_inf && i >= 0);
        if (a.is_inf) { return -1; }
        i = max(i, 0); int ext; vector<uint> v = twosComplement(a, ext);
        for (size_t k = size_t(i / 32); k < v.size(); ++k) {
            uint x = ~v[k] & (k == size_t(i / 32) ? ~0u << (i % 32) : ~0u);
            if (x) { return 32 * int(k) + std::countr_zero(x); }}
        return ext ? -1 : max(i, 32 * int(v.size()));}
    InfInt &setBit(int i) {
        assert(!is_inf && i >= 0);
        if (is_inf || i < 0) { return *this; }
        if (sgn == -1 && !isNil()) { return *this |= InfInt(1) << i; }
        if (size_t(i / 32) >= n.size()) { n.resize(size_t(i / 32) + 1, 0); }
        n[size_t(i / 32)] |= 1u << (i % 32);
        return *this;}
    InfInt &clearBit(int i) {
        assert(!is_inf && i >= 0);
        if (is_inf || i < 0) { return *this; }
        if (sgn == -1 && !isNil()) { return *this &= ~(InfInt(1) << i); }
        if (size_t(i / 32) < n.size()) { n[size_t(i / 32)] &= ~(1u << (i % 32)); trim(); }
        return *this;}
    InfInt &flipBit(int i) { return testBit(*this, i) ? clearBit(i) : setBit(i); }
    // Half-GCD machinery: 2x2 cofactor matrices [[A, B], [C, D]] applied as (a, b) <- (A a + B b, C a + D b).
    static bool isIdentity(const InfInt &ma, const InfInt &mb, const InfInt &mc, const InfInt &md) { return ma == 1 && mb.isNil() && mc.isNil() && md == 1; }
    static void eucStep(InfInt q, InfInt &ma, InfInt &mb, InfInt &mc, InfInt &md) {
        std::swap(ma, mc); mc -= q * ma;
        std::swap(mb, md); md -= std::move(q) * mb;}
    static void eucBack(InfInt &ma, InfInt &mb, InfInt &mc, InfInt &md, InfInt &a, InfInt &b) {
        InfInt q = mb.sgn == -1 ? abs(mc) / ma : abs(md) / mb;
        mc += q * ma; std::swap(ma, mc);
        md += q * mb; std::swap(mb, md);
        b += q * a; std::swap(a, b);}
    template<typename T> static void matApply(const T &ma, const T &mb, const T &mc, const T &md, InfInt &a, InfInt &b) {
        InfInt a0 = std::move(a), b0 = std::move(b);
        auto mul = [](const T &k, const InfInt &x) -> InfInt {
            if constexpr (std::same_as<T, InfInt>) { return k.isNil() || x.isNil() ? InfInt(0) : k == 1 ? x : k == -1 ? -x : k * x; }
            else { return k == 0 || x.isNil() ? InfInt(0) : k == 1 ? x : k == -1 ? -x : x * k; }};
        a = mul(ma, a0); a += mul(mb, b0);
        b = mul(mc, a0); b += mul(md, b0);}
    static void adjust(const InfInt &ma, const InfInt &mb, const InfInt &mc, const InfInt &md, int shf, InfInt ha, InfInt hb, InfInt &a, InfInt &b) {
        if (shf == 0) { a = std::move(ha); b = std::move(hb); return; }
        InfInt la, lb;
        la.n.assign(a.n.begin(), a.n.begin() + min(shf, int(a.n.size()))); la.trim();
        lb.n.assign(b.n.begin(), b.n.begin() + min(shf, int(b.n.size()))); lb.trim();
        matApply(ma, mb, mc, md, la, lb);
        ha <<= 32 * shf; ha += la;
        hb <<= 32 * shf; hb += lb;
        a = std::move(ha); b = std::move(hb);}
    // T: O(n) per pass; Lehmer step on the leading 64 bits of equally long a >= b >= 0 (at least 3 limbs); false when no quotient is certain.
    template<bool TRACK> static bool lehmerHalfGcd(InfInt &a, InfInt &b, InfInt &ma, InfInt &mb, InfInt &mc, InfInt &md) {
        constexpr lll LIM = lll(1) << 60;
        if (a.n.size() < 3 || a.n.size() != b.n.size()) { return false; }
        lng ca = 1, cb = 0, cc = 0, cd = 1;
        ulng ah = a.low64From(int(a.n.size()) - 2), bh = b.low64From(int(b.n.size()) - 2);
        auto bounds = [&](lng x, lng y) {
            lll mid = lll(x) * lll(ah) + lll(y) * lll(bh);
            return pair<lll, lll>{mid + min<lng>(x, 0) + min<lng>(y, 0), mid + max<lng>(x, 0) + max<lng>(y, 0)};};
        bool changed = false;
        while (true) {
            auto [alo, ahi] = bounds(ca, cb);
            auto [blo, bhi] = bounds(cc, cd);
            if (alo <= 0 || blo <= 0) { break; }
            lll q = alo / bhi, qh = (ahi - 1) / blo;
            if (q <= 0 || q != qh || q > LIM) { break; }
            lll nc = lll(ca) - q * cc, nd = lll(cb) - q * cd;
            if (nc < -LIM || nc > LIM || nd < -LIM || nd > LIM) { break; }
            ca = cc; cb = cd; cc = lng(nc); cd = lng(nd);
            changed = true;}
        if (!changed) { return false; }
        matApply(ca, cb, cc, cd, a, b);
        if constexpr (TRACK) { matApply(ca, cb, cc, cd, ma, mc); matApply(ca, cb, cc, cd, mb, md); }
        return true;}
    ulng low64From(int i) const { return ulng(n[size_t(i)]) | (size_t(i) + 1 < n.size() ? ulng(n[size_t(i) + 1]) << 32 : 0); }
    // T: O(M(n) * log(n)); reduces finite a >= b >= 0 until b has at most half of a's limbs, returning the matrix when TRACK; false if unchanged.
    template<bool TRACK> static bool halfGcd(InfInt &a, InfInt &b, InfInt &ma, InfInt &mb, InfInt &mc, InfInt &md) {
        assert(!a.is_inf && a.sgn == 1 && !b.is_inf && b.sgn == 1 && a >= b);
        if constexpr (TRACK) { ma = 1; mb = 0; mc = 0; md = 1; }
        if (b.isNil()) { return false; }
        int req = (int(a.n.size()) + 1) / 2;
        if (int(b.n.size()) <= req) { return false; }
        bool changed = false;
        auto reduce = [&]() {
            auto [q, r] = divMod(std::move(a), b);
            if constexpr (TRACK) { eucStep(std::move(q), ma, mb, mc, md); }
            a = std::exchange(b, std::move(r));
            changed = true;};
        if (int(a.n.size()) <= infint_detail::HALFGCD) {
            while (!b.isNil() && int(b.n.size()) > req) {
                if (lehmerHalfGcd<TRACK>(a, b, ma, mb, mc, md)) { changed = true; }
                else { reduce(); }}
            return changed;}
        InfInt ha = 0, hb = 0, pa, pb, pc, pd;
        auto top = [&](const InfInt &x, int from) { InfInt t; if (from < int(x.n.size())) { t.n.assign(x.n.begin() + from, x.n.end()); } t.trim(); return t; };
        ha = top(a, req); hb = top(b, req);
        halfGcd<true>(ha, hb, pa, pb, pc, pd);
        if (!isIdentity(pa, pb, pc, pd)) {
            adjust(pa, pb, pc, pd, req, std::move(ha), std::move(hb), a, b);
            while ((a.sgn == -1 || b.sgn == -1 || a < b) && !isIdentity(pa, pb, pc, pd)) { eucBack(pa, pb, pc, pd, a, b); }
            if (!isIdentity(pa, pb, pc, pd)) {
                changed = true;
                if constexpr (TRACK) { ma = std::move(pa); mb = std::move(pb); mc = std::move(pc); md = std::move(pd); }}}
        if (!b.isNil() && int(b.n.size()) > req) { reduce(); }
        if (b.isNil() || int(b.n.size()) <= req) { return changed; }
        int shf = 2 * req - int(a.n.size());
        if (shf < 0) {
            while (!b.isNil() && int(b.n.size()) > req) { reduce(); }
            return changed;}
        ha = top(a, shf); hb = top(b, shf);
        halfGcd<true>(ha, hb, pa, pb, pc, pd);
        if (!isIdentity(pa, pb, pc, pd)) {
            adjust(pa, pb, pc, pd, shf, std::move(ha), std::move(hb), a, b);
            while ((a.sgn == -1 || b.sgn == -1 || a < b) && !isIdentity(pa, pb, pc, pd)) { eucBack(pa, pb, pc, pd, a, b); }
            if (!isIdentity(pa, pb, pc, pd)) {
                changed = true;
                if constexpr (TRACK) { matApply(pa, pb, pc, pd, ma, mc); matApply(pa, pb, pc, pd, mb, md); }}}
        while (!b.isNil() && int(b.n.size()) > req) { reduce(); }
        return changed;}
    // T: O(M(n) * log(n)); nonnegative gcd; gcd(inf, x) = |x| for x != 0.
    friend InfInt gcd(InfInt a, InfInt b) {
        if ((a.is_inf && !b.isNil()) || a.isNil()) { b.sgn = 1; return b; }
        if ((b.is_inf && !a.isNil()) || b.isNil()) { a.sgn = 1; return a; }
        a.sgn = 1; b.sgn = 1;
        if (a.fitsWord() && b.fitsWord()) { return std::gcd(a.low64(), b.low64()); }
        int za = ctz(a), zb = ctz(b), z = min(za, zb);
        a >>= za; b >>= zb;
        InfInt ma, mb, mc, md;
        while (!b.isNil()) {
            if (a < b) { std::swap(a, b); }
            if (b.isNil()) { break; }
            if (b.fitsWord()) { ulng u = b.low64(); a %= u; a = std::gcd(u, a.low64()); break; }
            if (a.words() <= infint_detail::BINARY_GCD) {
                vector<W> x = a.toWords(), y = b.toWords();
                int len = infint_detail::gcdBinary(x.data(), int(x.size()), y.data(), int(y.size()));
                a.setWords(x.data(), len); break;}
            if (int(b.n.size()) > infint_detail::HALFGCD && halfGcd<false>(a, b, ma, mb, mc, md)) { continue; }
            if (a.n.size() > b.n.size() || !lehmerHalfGcd<false>(a, b, ma, mb, mc, md)) { a %= b; }}
        return a << z;}
    // T: O(M(n) * log(n)); g = a x + b y with |x| <= |b| / g and |y| <= |a| / g when both are nonzero.
    friend InfInt exGcd(InfInt a, InfInt b, InfInt &x, InfInt &y) {
        if (a.is_inf) {
            if (b.is_inf || b.isNil()) { x = a.sgn; y = 0; a.sgn = 1; return a; }
            x = 0; y = b.sgn; b.sgn = 1;
            return b;}
        if (b.is_inf) {
            if (a.isNil()) { x = 0; y = b.sgn; b.sgn = 1; return b; }
            x = a.sgn; y = 0; a.sgn = 1;
            return a;}
        int as = a.sgn, bs = b.sgn;
        a.sgn = 1; b.sgn = 1;
        x = 1; y = 0; InfInt x1 = 0, y1 = 1, ma, mb, mc, md;
        while (!b.isNil()) {
            if (a < b) { std::swap(a, b); std::swap(x, x1); std::swap(y, y1); }
            if (b.isNil()) { break; }
            if (b.n.size() > 2 && halfGcd<true>(a, b, ma, mb, mc, md)) { matApply(ma, mb, mc, md, x, x1); matApply(ma, mb, mc, md, y, y1); continue; }
            auto [q, r] = divMod(std::move(a), b);
            a = std::exchange(b, std::move(r));
            std::swap(x, x1); x1 -= q * x;
            std::swap(y, y1); y1 -= std::move(q) * y;}
        if (as == -1) { x = -std::move(x); }
        if (bs == -1) { y = -std::move(y); }
        return a;}
    friend InfInt lcm(InfInt a, InfInt b) {
        if (a.isNil() || b.isNil()) { return 0; }
        if (a.is_inf || b.is_inf) { InfInt r; r.is_inf = true; return r; }
        InfInt g = gcd(a, b), r = (std::move(a) / std::move(g)) * b; r.sgn = 1;
        return r;}
    // T: O(M(n) * log(n)); inverse of a modulo |mod| in [0, |mod|), -1 when gcd(a, mod) != 1, 0 for |mod| = 1.
    friend InfInt inv(InfInt a, InfInt mod) {
        assert(!a.is_inf && !mod.is_inf && !mod.isNil());
        if (a.is_inf || mod.is_inf || mod.isNil()) { return -1; }
        mod.sgn = 1;
        if (mod == 1) { return 0; }
        a = divModEuclid(a, mod).second;
        if (a.isNil()) { return -1; }
        if (a.fitsWord() && mod.fitsWord()) {
            ulng r0 = a.low64(), r1 = mod.low64(); lll x0 = 1, x1 = 0;
            while (r1) { ulng q = r0 / r1; r0 = std::exchange(r1, r0 - q * r1); x0 = std::exchange(x1, x0 - lll(q) * x1); }
            if (r0 != 1) { return -1; }
            x0 %= lll(mod.low64());
            return x0 < 0 ? ulng(x0 + lll(mod.low64())) : ulng(x0);}
        InfInt x, y, g = exGcd(std::move(a), mod, x, y);
        if (g != 1) { return -1; }
        return divModEuclid(x, mod).second;}
    // T: O(M(n * e)); nonnegative exponent; inf^0 = 1.
    friend InfInt pow(InfInt a, lng e) {
        assert(e >= 0);
        if (e < 0) { return 0; }
        if (e == 0) { return 1; }
        if (a.is_inf) { if (!(e & 1)) { a.sgn = 1; } return a; }
        if (e == 1 || a.isNil() || a == 1) { return a; }
        if (a == -1) { return e & 1 ? -1 : 1; }
        InfInt r = a;
        for (int i = std::bit_width(ulng(e)) - 2; i >= 0; --i) { r *= r; if ((e >> i) & 1) { r *= a; } }
        return r;}
    // Modular context for a finite modulus of k words: Montgomery REDC (odd moduli, when asked) or reduction by a cached reciprocal.
    struct ModCtx {
        int k = 0, shift = 0; bool mont = false; vector<W> m, minv, r2, inv;
        ModCtx(const InfInt &modulus, bool montgomery) {
            m = modulus.toWords(); k = int(m.size()); mont = montgomery && (m[0] & 1) && k >= infint_detail::MONTGOMERY_POW;
            if (mont) {
                minv.assign(size_t(k), 0); vector<W> t(size_t(k), W(0)), y(size_t(k), W(0));
                minv[0] = ulng(0) - infint_detail::invMod64(m[0]);
                for (int p = 1; p < k; p *= 2) {
                    int np = min(2 * p, k);
                    infint_detail::truncatedMul(t.data(), m.data(), minv.data(), np);
                    std::copy(t.begin(), t.begin() + np, y.begin()); infint_detail::addWord(y.data(), np, 2);
                    infint_detail::truncatedMul(t.data(), minv.data(), y.data(), np);
                    std::copy(t.begin(), t.begin() + np, minv.begin());}
                vector<W> big(size_t(2 * k) + 1, 0); big[size_t(2 * k)] = 1;
                r2.assign(size_t(k), 0);
                infint_detail::divRem(nullptr, r2.data(), big.data(), 2 * k + 1, m.data(), k);}
            else if (k >= infint_detail::NEWTON_DIV) {
                vector<W> bn(m); shift = std::countl_zero(bn[size_t(k) - 1]);
                if (shift) { infint_detail::shl(bn.data(), bn.data(), k, shift); }
                inv.resize(size_t(k) + 1); infint_detail::reciprocal(inv.data(), bn.data(), k);}}
        // x = x * y mod m on k-word residues (Montgomery form when mont); y may alias x.
        void mul(vector<W> &x, const vector<W> &y, vector<W> &t) const {
            t.assign(size_t(2 * k) + 1, 0);
            if (&x == &y) { infint_detail::square(t.data(), x.data(), k); }
            else { infint_detail::mul(t.data(), x.data(), k, y.data(), k); }
            if (!mont) { infint_detail::divRem(nullptr, x.data(), t.data(), 2 * k, m.data(), k, inv.empty() ? nullptr : inv.data()); return; }
            vector<W> u(size_t(k), W(0)), v(size_t(2 * k), W(0));
            infint_detail::truncatedMul(u.data(), t.data(), minv.data(), k);
            infint_detail::mul(v.data(), u.data(), k, m.data(), k);
            infint_detail::addInto(t.data(), 2 * k + 1, v.data(), 2 * k);
            if (infint_detail::cmp(t.data() + k, infint_detail::norm(t.data() + k, k + 1), m.data(), k) >= 0) { infint_detail::subFrom(t.data() + k, k + 1, m.data(), k); }
            std::copy(t.data() + k, t.data() + 2 * k, x.data());}
        vector<W> residue(const InfInt &a) const {
            InfInt mod; mod.setWords(m.data(), k);
            vector<W> x = divModEuclid(a, mod).second.toWords(); x.resize(size_t(k), 0);
            return x;}
        vector<W> enter(const InfInt &a) const { vector<W> x = residue(a), t; if (mont) { mul(x, r2, t); } return x; }
        InfInt leave(vector<W> x) const {
            if (mont) { vector<W> t, u(size_t(k), 0); u[0] = 1; mul(x, u, t); }
            InfInt r; r.setWords(x.data(), k);
            return r;}
        // Plain modular product of residues in [0, m) without Montgomery form.
        InfInt mul(const InfInt &a, const InfInt &b) const {
            vector<W> x = residue(a), y = residue(b), t;
            infint_detail::divRem(nullptr, x.data(), [&] { t.assign(size_t(2 * k) + 1, 0); infint_detail::mul(t.data(), x.data(), k, y.data(), k); return t.data(); }(), 2 * k, m.data(), k, inv.empty() ? nullptr : inv.data());
            InfInt r; r.setWords(x.data(), k);
            return r;}
    };
    // T: O(log(e) * M(n)) with sliding windows; a^e mod |mod| in [0, |mod|), -1 when e < 0 and a is not a unit; |mod| = 1 gives 0.
    friend InfInt powMod(InfInt a, InfInt e, InfInt mod) {
        assert(!a.is_inf && !e.is_inf && !mod.is_inf && !mod.isNil());
        if (a.is_inf || e.is_inf || mod.is_inf || mod.isNil()) { return 0; }
        mod.sgn = 1;
        if (mod == 1) { return 0; }
        if (e.isNil()) { return 1; }
        if (e.sgn == -1) { a = inv(a, mod); if (a == -1) { return -1; } e.sgn = 1; }
        a = divModEuclid(a, mod).second;
        if (a.isNil() || a == 1 || e == 1) { return a; }
        int bits = bitLength(e);
        auto bit = [&](int i) { return int((e.n[size_t(i / 32)] >> (i % 32)) & 1); };
        int len = 1, best = INT_MAX;
        for (int w = 1; w <= min(bits, 8); ++w) {
            int l = max(bits - w, 0);
            while (!bit(l)) { ++l; }
            int cost = (1 << (w - 1)) - (w == 1) + l;
            for (int j = l - 1; j >= 0;) {
                if (!bit(j)) { --j; continue; }
                int k = max(j - w + 1, 0);
                while (!bit(k)) { ++k; }
                ++cost; j = k - 1;}
            if (cost < best) { len = w; best = cost; }}
        auto window = [&]<typename T>(T base, auto &&mulMod) -> T {
            if (len == 1) {
                T r = base;
                for (int i = bits - 2; i >= 0; --i) { mulMod(r, r); if (bit(i)) { mulMod(r, base); } }
                return r;}
            vector<T> odd(size_t(1) << (len - 1)); odd[0] = std::move(base);
            if (odd.size() > 1) {
                T sq = odd[0]; mulMod(sq, sq);
                for (size_t i = 1; i < odd.size(); ++i) { odd[i] = odd[i - 1]; mulMod(odd[i], sq); }}
            int l = max(bits - len, 0);
            while (!bit(l)) { ++l; }
            int k = 0;
            for (int j = bits - 1; j >= l; --j) { k = (k << 1) | bit(j); }
            T r = odd[size_t(k >> 1)];
            for (int i = l - 1; i >= 0;) {
                if (!bit(i)) { mulMod(r, r); --i; continue; }
                l = max(i - len + 1, 0);
                while (!bit(l)) { ++l; }
                k = 0;
                for (int j = i; j >= l; --j) { k = (k << 1) | bit(j); }
                for (int j = l; j <= i; ++j) { mulMod(r, r); }
                mulMod(r, odd[size_t(k >> 1)]); i = l - 1;}
            return r;};
        if (mod.fitsWord()) {
            ulng um = mod.low64();
            auto mulMod = [&](ulng &x, const ulng &y) { x = ulng(ulll(x) * y % um); };
            return window(a.low64(), mulMod);}
        ModCtx ctx(mod, true); vector<W> tmp;
        auto mulMod = [&](vector<W> &x, const vector<W> &y) { ctx.mul(x, y, tmp); };
        return ctx.leave(window(ctx.enter(a), mulMod));}
    // T: O(M(n)); Zimmermann's recursive square root with remainder on words, a has n = 4l words with a[n - 1] >= 2^62.
    static void sqrtRemWords(W *s, W *r, const W *a, int nn) {
        if (nn <= 2) {
            ulll v = nn == 2 ? (ulll(a[1]) << 64) | a[0] : a[0];
            long double f = std::sqrt(static_cast<long double>(v));
            ulng q = f >= 18446744073709551615.0L ? ~ulng(0) : ulng(f);
            while (ulll(q) * q > v) { --q; }
            while (q != ~ulng(0) && ulll(q + 1) * (q + 1) <= v) { ++q; }
            ulll rem = v - ulll(q) * q;
            s[0] = q; r[0] = ulng(rem); r[1] = ulng(rem >> 64);
            return;}
        int l = nn / 4, hn = 2 * l;
        vector<W> sh(size_t(l) + 1, 0), rh(size_t(hn) + 1, 0);
        if (hn % 4 == 0 || hn == 2) { sqrtRemWords(sh.data(), rh.data(), a + hn, hn); }
        else {
            vector<W> pad(size_t(hn) + 2, 0), s2(size_t(l) + 1), r2(size_t(hn) + 2);
            std::copy(a + hn, a + nn, pad.begin() + 2);
            sqrtRemWords(s2.data(), r2.data(), pad.data(), hn + 2);
            std::copy(s2.begin() + 1, s2.end(), sh.begin());
            vector<W> sq(size_t(2 * l)); infint_detail::square(sq.data(), sh.data(), l);
            std::copy(a + hn, a + nn, rh.begin()); infint_detail::subFrom(rh.data(), hn, sq.data(), 2 * l);}
        vector<W> num(size_t(hn) + 1, 0), d(size_t(l) + 1, 0), q(size_t(l) + 2, 0), u(size_t(l) + 1, 0);
        std::copy(a + l, a + hn, num.begin()); std::copy(rh.begin(), rh.begin() + l + 1, num.begin() + l);
        d[l] = infint_detail::shl(d.data(), sh.data(), l, 1);
        int ln = infint_detail::norm(num.data(), hn + 1), ld = infint_detail::norm(d.data(), l + 1);
        if (ln >= ld) { infint_detail::divRem(q.data(), u.data(), num.data(), ln, d.data(), ld); }
        else { std::copy(num.begin(), num.begin() + ln, u.begin()); }
        int lq = infint_detail::norm(q.data(), l + 2);
        vector<W> sx(size_t(hn) + 1, 0), rr(size_t(hn) + 2, 0), qq(size_t(2 * l) + 2, 0);
        std::copy(sh.begin(), sh.begin() + l, sx.begin() + l); sx[size_t(hn)] = infint_detail::add(sx.data(), sx.data(), hn, q.data(), lq);
        std::copy(a, a + l, rr.begin()); std::copy(u.begin(), u.end(), rr.begin() + l);
        if (lq) { infint_detail::square(qq.data(), q.data(), lq); }
        W borrow = infint_detail::sub(rr.data(), rr.data(), hn + 2, qq.data(), 2 * lq);
        while (borrow) {
            vector<W> t(size_t(hn) + 2, 0); t[size_t(hn) + 1] = infint_detail::shl(t.data(), sx.data(), hn + 1, 1);
            infint_detail::subWord(t.data(), hn + 2, 1); infint_detail::subWord(sx.data(), hn + 1, 1);
            borrow -= infint_detail::add(rr.data(), rr.data(), hn + 2, t.data(), hn + 2);}
        std::copy(sx.begin(), sx.begin() + hn, s); std::copy(rr.begin(), rr.begin() + hn + 1, r);}
    // T: O(M(n)); floor square root and remainder of a finite a >= 0.
    friend pair<InfInt, InfInt> isqrtRem(const InfInt &a) {
        assert(!a.is_inf && a.sgn == 1);
        if (a.is_inf) { return {a, 0}; }
        if (a.isNil()) { return {0, 0}; }
        vector<W> w = a.toWords(); int nn = int(w.size()), l = (nn + 3) / 4, total = 4 * l;
        int t = (std::countl_zero(w.back()) / 2) * 2, shift = 64 * (total - nn) + t;
        vector<W> x(size_t(total), 0);
        std::copy(w.begin(), w.end(), x.begin() + (total - nn));
        if (t) { infint_detail::shl(x.data(), x.data(), total, t); }
        vector<W> s(size_t(total) / 2), r(size_t(total) / 2 + 1);
        sqrtRemWords(s.data(), r.data(), x.data(), total);
        InfInt root; root.setWords(s.data(), total / 2); root >>= shift / 2;
        InfInt rem = a - root * root;
        return {std::move(root), std::move(rem)};}
    friend InfInt sqrt(const InfInt &a) {
        assert(a.sgn == 1);
        if (a.is_inf) { return a; }
        return isqrtRem(a).first;}
    // T: O(log(b) * M(n) * log(k)); truncated k-th root (k >= 1) and remainder a - root^k; a < 0 only for odd k. Integer Newton from a
    // floating-point upper estimate (2^-20 relative margin), so the iteration count is logarithmic in b for every k.
    friend pair<InfInt, InfInt> kthRootRem(const InfInt &a, int k) {
        assert(!a.is_inf && k >= 1 && (a.sgn == 1 || (k & 1)));
        k = max(k, 1);
        if (a.is_inf) { return {a, 0}; }
        if (a.sgn == -1) { auto [s, r] = kthRootRem(-a, k); return {-std::move(s), -std::move(r)}; }
        if (k == 1) { return {a, 0}; }
        if (k == 2) { return isqrtRem(a); }
        if (a.isNil()) { return {0, 0}; }
        int bl = bitLength(a);
        if (bl <= k) { return {1, a - 1}; }
        long double lg = (bl > 64 ? std::log2(static_cast<long double>((a >> (bl - 64)).low64())) + (bl - 64) : std::log2(static_cast<long double>(a.low64()))) / k;
        int ip = int(std::floor(lg)); long double mant = std::exp2(lg - ip) * (1 + std::ldexp(1.0L, -20));
        InfInt x = ulng(std::ceil(std::ldexp(mant, min(ip, 62))));
        if (ip > 62) { x <<= ip - 62; }
        while (true) {
            InfInt y = x * (k - 1); y += a / pow(x, k - 1); y /= k;
            if (y >= x) { break; }
            x = std::move(y);}
        while (pow(x + 1, k) <= a) { ++x; }
        InfInt rem = a - pow(x, k);
        return {std::move(x), std::move(rem)};}
    static uint residueSmall(const InfInt &a, uint m) {
        ulng r = 0;
        for (size_t i = a.n.size(); i-- > 0;) { r = ((r << 32) | a.n[i]) % m; }
        return uint(r);}
    // T: O(M(n)) after O(n) residue filters modulo 64, 63, 65 and 11.
    friend bool isPerfectSquare(const InfInt &a) {
        assert(!a.is_inf);
        if (a.is_inf || a.sgn == -1) { return false; }
        if (a.isNil()) { return true; }
        if (!((0x0202021202030213ULL >> (a.n[0] & 63)) & 1)) { return false; }
        static const auto build = [](uint m) { array<ulng, 2> v{}; for (ulng x = 0; x < m; ++x) { ulng q = x * x % m; v[size_t(q / 64)] |= 1ULL << (q % 64); } return v; };
        static const array<ulng, 2> Q63 = build(63), Q65 = build(65), Q11 = build(11);
        uint r = residueSmall(a, 45045);
        auto test = [&](const array<ulng, 2> &q, uint m) { uint v = r % m; return (q[size_t(v / 64)] >> (v % 64)) & 1; };
        if (!test(Q63, 63) || !test(Q65, 65) || !test(Q11, 11)) { return false; }
        return isqrtRem(a).second.isNil();}
    static bool isPrime32(uint q) {
        if (q < 2) { return false; }
        for (uint p : {2u, 3u, 5u, 7u, 11u, 13u, 17u, 19u, 23u, 29u, 31u, 37u}) { if (q % p == 0) { return q == p; } }
        uint d = q - 1; int s = std::countr_zero(d); d >>= s;
        for (ulng b : {2ULL, 7ULL, 61ULL}) {
            ulng x = powMod64(b % q, d, q);
            if (b % q == 0 || x == 1 || x == q - 1) { continue; }
            bool ok = false;
            for (int i = 1; i < s && !ok; ++i) { x = x * x % q; ok = x == q - 1; }
            if (!ok) { return false; }}
        return true;}
    static ulng powMod64(ulng b, ulng e, ulng m) {
        ulng r = 1; b %= m;
        for (; e; e >>= 1, b = ulng(ulll(b) * b % m)) { if (e & 1) { r = ulng(ulll(r) * b % m); } }
        return r;}
    // T: O(pi(b) * n + M(n) * log(b)) worst case; 0, 1 and every x^k with k >= 2 (odd k for negative values).
    friend bool isPerfectPower(const InfInt &a) {
        assert(!a.is_inf);
        InfInt m = abs(a);
        if (magCmp(m, 1) <= 0) { return true; }
        int bl = bitLength(m), t = ctz(m);
        auto check = [&](int p) { return p >= 2 && (a.sgn == 1 || (p & 1)) && kthRootRem(m, p).second.isNil(); };
        if (t > 0) {
            for (int p = 2; p <= t; ++p) { if (t % p == 0 && check(p)) { return true; } }
            return false;}
        vector<bool> comp(size_t(bl) + 1, false);
        for (int p = 2; p <= bl; ++p) {
            if (comp[size_t(p)]) { continue; }
            for (lng q = lng(p) * p; q <= bl; q += p) { comp[size_t(q)] = true; }
            if (a.sgn == -1 && p == 2) { continue; }
            bool reject = false;
            for (uint i = 1, found = 0; i <= 64 && found < 2 && !reject; ++i) {
                ulng q = 2ULL * i * ulng(p) + 1;
                if (q >= (1ULL << 31) || !isPrime32(uint(q))) { continue; }
                ++found;
                uint r = residueSmall(m, uint(q));
                reject = r != 0 && powMod64(r, (q - 1) / ulng(p), q) != 1;}
            if (!reject && check(p)) { return true; }}
        return false;}
    static int jacobi64(ulng a, ulng m) {
        int res = 1;
        while (a) {
            int t = std::countr_zero(a); a >>= t;
            if ((t & 1) && ((m & 7) == 3 || (m & 7) == 5)) { res = -res; }
            if ((a & 3) == 3 && (m & 3) == 3) { res = -res; }
            std::swap(a, m); a %= m;}
        return m == 1 ? res : 0;}
    // T: O(n^2) word operations; Jacobi symbol (a / m) for odd positive m; kronecker extends it to every finite m.
    friend int jacobi(const InfInt &a, const InfInt &m) {
        assert(!a.is_inf && !m.is_inf && m.sgn == 1 && !m.isNil() && (m.n[0] & 1));
        if (a.is_inf || m.is_inf || m.isNil()) { return 0; }
        InfInt x = divModEuclid(a, m).second, y = m;
        if (y.fitsWord()) { return jacobi64(x.low64(), y.low64()); }
        int res = 1;
        while (!x.isNil()) {
            int t = ctz(x); x >>= t;
            uint y8 = y.n[0] & 7;
            if ((t & 1) && (y8 == 3 || y8 == 5)) { res = -res; }
            if ((x.n[0] & 3) == 3 && (y8 & 3) == 3) { res = -res; }
            std::swap(x, y); x %= y;
            if (y.fitsWord()) { return res * jacobi64(x.low64(), y.low64()); }}
        return y == 1 ? res : 0;}
    friend int kronecker(const InfInt &a, const InfInt &m) {
        assert(!a.is_inf && !m.is_inf);
        if (a.is_inf || m.is_inf) { return 0; }
        if (m.isNil()) { return magCmp(a, 1) == 0 ? 1 : 0; }
        int res = m.sgn == -1 && a.sgn == -1 ? -1 : 1, t = ctz(m);
        InfInt y = abs(m); y >>= t;
        if (t) {
            if (a.isNil() || !(a.n[0] & 1)) { return 0; }
            uint a8 = uint((a & 7).low64());
            if ((t & 1) && (a8 == 3 || a8 == 5)) { res = -res; }}
        return res * jacobi(a, y);}
    // T: O(b * M(n)); Baillie-PSW: trial division, a strong base-2 test and a strong Lucas-Selfridge test; deterministic below 2^64,
    // Monte Carlo above (no counterexample is known); false for values below 2.
    friend bool isProbablePrime(const InfInt &a) {
        assert(!a.is_inf);
        if (a.is_inf || a.sgn == -1 || magCmp(a, 2) < 0) { return false; }
        if (a.fitsWord()) {
            ulng v = a.low64();
            if (v < 4) { return true; }
            if (!(v & 1)) { return false; }
            ulng d = v - 1; int s = std::countr_zero(d); d >>= s;
            for (ulng b : {2ULL, 325ULL, 9375ULL, 28178ULL, 450775ULL, 9780504ULL, 1795265022ULL}) {
                ulng x = powMod64(b % v, d, v);
                if (b % v == 0 || x == 1 || x == v - 1) { continue; }
                bool ok = false;
                for (int i = 1; i < s && !ok; ++i) { x = ulng(ulll(x) * x % v); ok = x == v - 1; }
                if (!ok) { return false; }}
            return true;}
        if (!(a.n[0] & 1)) { return false; }
        for (ulng prod : {614889782588491410ULL, 3749562977351496827ULL, 4343678784233766587ULL}) {
            vector<W> w = a.toWords();
            if (std::gcd(infint_detail::divWord(w.data(), w.data(), int(w.size()), prod), prod) != 1) { return false; }}
        if (isPerfectSquare(a)) { return false; }
        InfInt am1 = a - 1; int s = ctz(am1); InfInt d = am1 >> s;
        InfInt x = powMod(2, d, a);
        if (x != 1 && x != am1) {
            bool ok = false;
            for (int i = 1; i < s && !ok; ++i) { x = x * x % a; ok = x == am1; }
            if (!ok) { return false; }}
        lng disc = 5;
        while (true) {
            int j = jacobi(InfInt(disc), a);
            if (j == -1) { break; }
            if (j == 0) { return false; }
            disc = disc > 0 ? -(disc + 2) : -(disc - 2);}
        InfInt q = divModEuclid(InfInt((1 - disc) / 4), a).second, dm = divModEuclid(InfInt(disc), a).second;
        ModCtx ctx(a, false);
        auto half = [&](InfInt v) { if (v.n[0] & 1) { v += a; } v >>= 1; return divModEuclid(v, a).second; };
        auto dbl = [&](InfInt &u, InfInt &v, InfInt &qk) {
            InfInt uv = ctx.mul(u, v);
            v = ctx.mul(v, v) - ctx.mul(2, qk); if (v.sgn == -1) { v += a; }
            u = std::move(uv); qk = ctx.mul(qk, qk);};
        InfInt ap1 = a + 1; int s2 = ctz(ap1); InfInt dd = ap1 >> s2;
        InfInt u = 1, v = 1, qk = q;
        for (int i = bitLength(dd) - 2; i >= 0; --i) {
            dbl(u, v, qk);
            if (testBit(dd, i)) {
                InfInt nu = half(u + v), nv = half(ctx.mul(dm, u) + v);
                u = std::move(nu); v = std::move(nv); qk = ctx.mul(qk, q);}}
        if (u.isNil() || v.isNil()) { return true; }
        for (int r = 1; r < s2; ++r) {
            v = ctx.mul(v, v) - ctx.mul(2, qk); if (v.sgn == -1) { v += a; }
            qk = ctx.mul(qk, qk);
            if (v.isNil()) { return true; }}
        return false;}
    // T: O(M(n)); floor(2^(64 n) / a) for a finite positive a of n limbs with its top limb bit set (n + 1 limbs, exact).
    friend InfInt newtonRaphsonInv(const InfInt &a) {
        assert(!a.is_inf && a.sgn == 1 && !a.isNil() && (a.n.back() >> 31));
        if (a.is_inf || a.isNil() || !(a.n.back() >> 31)) { return 0; }
        int limbs = int(a.n.size());
        InfInt x = a; if (limbs & 1) { x <<= 32; }
        vector<W> w = x.toWords(), iv(w.size() + 1);
        infint_detail::reciprocal(iv.data(), w.data(), int(w.size()));
        InfInt r; r.setWords(iv.data(), int(iv.size()));
        if (limbs & 1) { r >>= 32; }
        return r;}

    // Stream base manipulator: per-stream iword, default 10; std::hex and std::oct flags also apply.
    struct SetBase { uint base; explicit SetBase(uint b) : base(b) {} };
    static int getBaseId() { static const int id = std::ios_base::xalloc(); return id; }
    static uint streamBase(std::ios_base &s) {
        uint base = uint(s.iword(getBaseId()));
        if (s.flags() & std::ios_base::hex) { return 16; }
        if (s.flags() & std::ios_base::oct) { return 8; }
        return base ? base : 10;}
    friend istream &operator>>(istream &is, SetBase sb) { is.iword(getBaseId()) = int(sb.base); return is; }
    friend ostream &operator<<(ostream &os, SetBase sb) { os.iword(getBaseId()) = int(sb.base); return os; }
    // One whitespace-delimited token; a malformed token sets failbit and leaves a unchanged.
    friend istream &operator>>(istream &is, InfInt &a) {
        string s;
        if (!(is >> s)) { return is; }
        if (!a.read(s, streamBase(is))) { is.setstate(std::ios_base::failbit); }
        return is;}
    friend ostream &operator<<(ostream &os, const InfInt &a) { return os << a.toString(streamBase(os)); }
};
using iint = InfInt;

// T: O(N) per linear operation and O(N^2) per product, M: O(1); fixed N-word unsigned integer, every operation wraps modulo 2^(64 N).
template<int N> struct UInt {
    static_assert(N >= 1);
    using W = ulng;
    array<W, N> w{};

    UInt() {}
    template<typename T> requires InfInt::isIntegral<T> UInt(T x) {
        bool neg = false;
        if constexpr (std::is_signed_v<T> || std::same_as<T, lll>) { neg = x < 0; }
        ulll u = ulll(x);
        if (neg) { w.fill(~W(0)); }
        w[0] = W(u);
        if constexpr (N > 1) { w[1] = W(u >> 64); }}
    // T: O(N); two's-complement image of a finite value modulo 2^(64 N); the string form parses as InfInt does.
    explicit UInt(const InfInt &a) {
        assert(!a.is_inf);
        vector<W> v = a.toWords();
        for (int i = 0; i < N && i < int(v.size()); ++i) { w[size_t(i)] = v[size_t(i)]; }
        if (a.sgn == -1 && !a.isNil()) { *this = -*this; }}
    explicit UInt(string_view s, uint base = 10) : UInt(InfInt(s, base)) {}
    string toString(uint base = 10) const { return InfInt(*this).toString(base); }
    explicit operator ulng() const { return w[0]; }
    explicit operator bool() const { return std::ranges::any_of(w, [](W x) { return x != 0; }); }
    int words() const { return infint_detail::norm(w.data(), N); }
    friend int bitLength(const UInt &a) { int k = a.words(); return k ? 64 * k - std::countl_zero(a.w[size_t(k) - 1]) : 0; }
    friend int ctz(const UInt &a) {
        for (int i = 0; i < N; ++i) { if (a.w[size_t(i)]) { return 64 * i + std::countr_zero(a.w[size_t(i)]); } }
        return 0;}
    friend int popcount(const UInt &a) { int c = 0; for (W x : a.w) { c += std::popcount(x); } return c; }

    UInt &operator+=(const UInt &o) { infint_detail::add(w.data(), w.data(), N, o.w.data(), N); return *this; }
    UInt &operator-=(const UInt &o) { infint_detail::sub(w.data(), w.data(), N, o.w.data(), N); return *this; }
    UInt &operator*=(const UInt &o) { array<W, N> r; infint_detail::truncatedMul(r.data(), w.data(), o.w.data(), N); w = r; return *this; }
    // T: O(N^2); quotient and remainder, o != 0.
    friend pair<UInt, UInt> divMod(const UInt &a, const UInt &o) {
        int n = a.words(), m = o.words();
        assert(m > 0);
        UInt q, r;
        if (n < m || m == 0) { r = a; return {q, r}; }
        vector<W> qq(size_t(n - m) + 1, W(0)), rr(size_t(m), W(0));
        infint_detail::divRem(qq.data(), rr.data(), a.w.data(), n, o.w.data(), m);
        std::copy(qq.begin(), qq.end(), q.w.begin()); std::copy(rr.begin(), rr.end(), r.w.begin());
        return {q, r};}
    UInt &operator/=(const UInt &o) { return *this = divMod(*this, o).first; }
    UInt &operator%=(const UInt &o) { return *this = divMod(*this, o).second; }
    // T: O(N); in-place quotient by a word d != 0, returns the remainder.
    W divWord(W d) {
        assert(d != 0);
        int n = d ? words() : 0;
        return n ? infint_detail::divWord(w.data(), w.data(), n, d) : 0;}
    UInt &operator&=(const UInt &o) { for (int i = 0; i < N; ++i) { w[size_t(i)] &= o.w[size_t(i)]; } return *this; }
    UInt &operator|=(const UInt &o) { for (int i = 0; i < N; ++i) { w[size_t(i)] |= o.w[size_t(i)]; } return *this; }
    UInt &operator^=(const UInt &o) { for (int i = 0; i < N; ++i) { w[size_t(i)] ^= o.w[size_t(i)]; } return *this; }
    // T: O(N); shift counts in [0, 64 N); bits leave at the ends.
    UInt &operator<<=(int s) {
        assert(s >= 0 && s < 64 * N);
        if (s < 0 || s >= 64 * N) { w.fill(0); return *this; }
        int q = s / 64, r = s % 64;
        for (int i = N - 1; i >= 0; --i) {
            W x = i - q >= 0 ? w[size_t(i - q)] << r : 0;
            if (r && i - q - 1 >= 0) { x |= w[size_t(i - q - 1)] >> (64 - r); }
            w[size_t(i)] = x;}
        return *this;}
    UInt &operator>>=(int s) {
        assert(s >= 0 && s < 64 * N);
        if (s < 0 || s >= 64 * N) { w.fill(0); return *this; }
        int q = s / 64, r = s % 64;
        for (int i = 0; i < N; ++i) {
            W x = i + q < N ? w[size_t(i + q)] >> r : 0;
            if (r && i + q + 1 < N) { x |= w[size_t(i + q + 1)] << (64 - r); }
            w[size_t(i)] = x;}
        return *this;}
    UInt operator~() const { UInt r; for (int i = 0; i < N; ++i) { r.w[size_t(i)] = ~w[size_t(i)]; } return r; }
    UInt operator-() const { UInt r = ~*this; W one[1] = {1}; infint_detail::add(r.w.data(), r.w.data(), N, one, 1); return r; }
    friend UInt operator+(UInt a, const UInt &b) { a += b; return a; }
    friend UInt operator-(UInt a, const UInt &b) { a -= b; return a; }
    friend UInt operator*(UInt a, const UInt &b) { a *= b; return a; }
    friend UInt operator/(const UInt &a, const UInt &b) { return divMod(a, b).first; }
    friend UInt operator%(const UInt &a, const UInt &b) { return divMod(a, b).second; }
    friend UInt operator&(UInt a, const UInt &b) { a &= b; return a; }
    friend UInt operator|(UInt a, const UInt &b) { a |= b; return a; }
    friend UInt operator^(UInt a, const UInt &b) { a ^= b; return a; }
    friend UInt operator<<(UInt a, int s) { a <<= s; return a; }
    friend UInt operator>>(UInt a, int s) { a >>= s; return a; }
    // T: O(N^2); the high N words of the 2N-word product.
    friend UInt mulHigh(const UInt &a, const UInt &b) {
        W r[2 * N]; infint_detail::mulBasecase(r, a.w.data(), N, b.w.data(), N);
        UInt h; std::copy(r + N, r + 2 * N, h.w.begin());
        return h;}
    friend std::strong_ordering operator<=>(const UInt &a, const UInt &b) {
        for (int i = N - 1; i >= 0; --i) { if (a.w[size_t(i)] != b.w[size_t(i)]) { return a.w[size_t(i)] <=> b.w[size_t(i)]; } }
        return std::strong_ordering::equal;}
    friend bool operator==(const UInt &a, const UInt &b) { return a.w == b.w; }
    friend ostream &operator<<(ostream &os, const UInt &a) { return os << InfInt(a); }
};
using u256 = UInt<4>;

template<> struct std::hash<InfInt> {
    size_t operator()(const InfInt &a) const noexcept {
        ulng h = 0x9E3779B97F4A7C15ULL ^ ulng(a.is_inf ? 2 : 0) ^ ulng(a.sgn == -1 && !a.isNil() ? 1 : 0);
        auto mix = [&](ulng x) { h ^= x; h *= 0xBF58476D1CE4E5B9ULL; h ^= h >> 31; };
        if (a.n.empty()) { mix(0); }
        for (uint x : a.n) { mix(x); }
        return size_t(h);}
};
template<int N> struct std::hash<UInt<N>> {
    size_t operator()(const UInt<N> &a) const noexcept {
        ulng h = 0x9E3779B97F4A7C15ULL;
        for (ulng x : a.w) { h ^= x; h *= 0xBF58476D1CE4E5B9ULL; h ^= h >> 31; }
        return size_t(h);}
};
