#pragma once
#include "01-template.hpp"
#include "03-barrett.hpp"
#include "05-modint.hpp"
#if defined(__AVX2__) || defined(__PCLMUL__)
#include <immintrin.h>
#endif

template<typename T> struct Poly;
template<typename T> struct PolyModulus;
template<typename T> struct SemiRelaxedMul;
template<typename T> struct RelaxedMul;
namespace poly_detail {
    template<typename T> inline constexpr bool is_complex = false;
    template<typename U> inline constexpr bool is_complex<std::complex<U>> = true;
    template<typename T> concept Approx = std::is_floating_point_v<T> || is_complex<T>;
    template<typename T> concept BigInt = requires(T x) { { x.n } -> std::same_as<vector<uint> &>; { x.sgn } -> std::same_as<int &>; x.trim(); x.isNil(); };
    template<typename T> concept Ring = std::is_integral_v<T> || BigInt<T>;
    template<typename T> concept Field = !Ring<T>;
    template<typename T> concept Fftable = std::is_same_v<T, double> || std::is_same_v<T, std::complex<double>>;
    // Descending NTT primes p = c * 2^23 + 1 within a factor two of each other; three cover n * m^2 < 2^89, six 2^177.
    inline constexpr uint PRIMES[6] = {998244353, 897581057, 880803841, 754974721, 645922817, 595591169};
    static_assert(modint_detail::isPrime(PRIMES[0]) && modint_detail::isPrime(PRIMES[1]) && modint_detail::isPrime(PRIMES[2]));
    static_assert(modint_detail::isPrime(PRIMES[3]) && modint_detail::isPrime(PRIMES[4]) && modint_detail::isPrime(PRIMES[5]));
    inline constexpr int KARATSUBA = 40, HGCD_BASE = 128, FFT_MOD_MAX = 1 << 20;
#ifdef __AVX2__
    inline constexpr int SCHOOLBOOK = 16, COMPOSE_BK = 512, GCD_FAST = 1024, LARGE_RATIO = 16;
#else
    inline constexpr int SCHOOLBOOK = 32, COMPOSE_BK = 1024, GCD_FAST = 4096, LARGE_RATIO = 64;
#endif

    constexpr int ceilPow2(int n) { return n <= 1 ? 1 : 1 << (32 - std::countl_zero(uint(n - 1))); }
    constexpr int log2Of(int n) { return std::countr_zero(uint(n)); }

    // T: O(n + log(characteristic)), M: O(n); r[i] = 1/i for 1 <= i < n, needs T(1)..T(n - 1) nonzero.
    template<typename T> vector<T> inverses(int n) {
        vector<T> r(max(n, 1), T(1));
        T p = 1;
        for (int i = 1; i < n; ++i) { r[i] = p; p *= T(i); }
        T ip = T(1) / p;
        for (int i = n - 1; i >= 1; --i) { r[i] *= ip; ip *= T(i); }
        return r;}

    // T: O(1) per operation, M: O(1); odd p < 2^(B - 2), lazy words in [0, 2p), products need a * b < p * 2^B.
    template<typename W> struct Mont {
        using D = std::conditional_t<std::is_same_v<W, uint>, ulng, ulll>;
        static constexpr int B = 8 * sizeof(W);
        W p = 1, ninv = 1, r2 = 0, one = 0;

        constexpr Mont() = default;
        constexpr explicit Mont(W m) : p(m), ninv(m) {
            for (int i = 0; i < 6; ++i) { ninv *= 2 - m * ninv; }
            one = (W(0) - m) % m; r2 = W(D(one) * one % m);}

        W reduce(D t) const { W q = W(t) * ninv; return W(t >> B) - W((D(q) * p) >> B) + p; }
        W mul(W a, W b) const { return reduce(D(a) * b); }
        W canon(W a) const { return a >= p ? a - p : a; }
        W from(W a) const { return canon(mul(a, r2)); }
        W to(W a) const { return canon(reduce(a)); }
        W add(W a, W b) const { W s = a + b; return s >= 2 * p ? s - 2 * p : s; }
        W sub(W a, W b) const { W d = a + 2 * p - b; return d >= 2 * p ? d - 2 * p : d; }
        W diff(W a, W b) const { return a + 2 * p - b; }
    };

#ifdef __AVX2__
    // Eight 32-bit lanes of Mont<uint> with the same domains.
    struct Mont8 {
        __m256i p, p2, ninv;

        explicit Mont8(const Mont<uint> &m) : p(_mm256_set1_epi32(int(m.p))), p2(_mm256_set1_epi32(int(2 * m.p))), ninv(_mm256_set1_epi32(int(m.ninv))) {}

        static __m256i load(const void *q) { return _mm256_loadu_si256(static_cast<const __m256i *>(q)); }
        static void store(void *q, __m256i x) { _mm256_storeu_si256(static_cast<__m256i *>(q), x); }
        __m256i mul(__m256i a, __m256i b) const {
            __m256i te = _mm256_mul_epu32(a, b), to = _mm256_mul_epu32(_mm256_srli_epi64(a, 32), _mm256_srli_epi64(b, 32));
            __m256i ue = _mm256_mul_epu32(_mm256_mul_epu32(te, ninv), p), uo = _mm256_mul_epu32(_mm256_mul_epu32(to, ninv), p);
            __m256i ht = _mm256_blend_epi32(_mm256_srli_epi64(te, 32), to, 0xAA), hu = _mm256_blend_epi32(_mm256_srli_epi64(ue, 32), uo, 0xAA);
            return _mm256_add_epi32(_mm256_sub_epi32(ht, hu), p);}
        __m256i canon(__m256i a) const { return _mm256_min_epu32(a, _mm256_sub_epi32(a, p)); }
        __m256i add(__m256i a, __m256i b) const { __m256i s = _mm256_add_epi32(a, b); return _mm256_min_epu32(s, _mm256_sub_epi32(s, p2)); }
        __m256i sub(__m256i a, __m256i b) const { return _mm256_min_epu32(diff(a, b), _mm256_sub_epi32(diff(a, b), p2)); }
        __m256i diff(__m256i a, __m256i b) const { return _mm256_sub_epi32(_mm256_add_epi32(a, p2), b); }
    };
#endif

    // S: O(n) roots per modulus, Q: O(n * log(n)) per transform, M: O(n) roots plus O(n) workspace.
    // Prime modulus with 2^k | mod - 1; transforms of length <= min(2^k, 2^30) in bit-reversed order; cache keyed by mod().
    template<ModularInt T> struct Ntt {
        using W = typename T::Word;
        static constexpr int B = 8 * sizeof(W);
        struct Cache {
            W mod = 0; bool mont = false; Mont<W> m; int cap = 0;
            vector<W> w, iw; vector<T> tw, tiw; vector<T> pad[2]; vector<W> buf[2];
        };

        static Cache &cache() { static Cache c; return c; }
        static int maxLength() { return T::is_prime ? 1 << min(std::countr_zero(W(T::mod() - 1)), 30) : 0; }
        static bool usable(int n) { return n <= maxLength(); }
        static void ensure(int n) {
            Cache &c = cache();
            if (c.mod != T::mod()) {
                c = Cache(); c.mod = T::mod(); c.mont = (c.mod & 1) && c.mod < (W(1) << (B - 2));
                if (c.mont) { c.m = Mont<W>(c.mod); }
                c.cap = 1; c.w.assign(2, 0); c.iw.assign(2, 0); c.tw.assign(2, T(1)); c.tiw.assign(2, T(1));
                c.w[1] = c.iw[1] = c.mont ? c.m.one : W(1);}
            if (n <= c.cap) { return; }
            assert(usable(n));
            int k = std::countr_zero(W(c.mod - 1));
            T z = 2;
            for (int t = 0; t < 1024 && pow(z, (c.mod - 1) / 2) == T(1); ++t) { ++z; }
            T g = pow(z, (c.mod - 1) >> k), ig = T(1) / g;
            c.w.resize(n); c.iw.resize(n); c.tw.resize(n); c.tiw.resize(n);
            for (int len = c.cap; len < n; len *= 2) {
                T om = g, iom = ig;
                for (int i = log2Of(len) + 1; i < k; ++i) { om *= om; iom *= iom; }
                T cur = 1, icur = 1;
                for (int j = 0; j < len; ++j, cur *= om, icur *= iom) {
                    if (c.mont) { c.w[len + j] = c.m.from(cur.val()); c.iw[len + j] = c.m.from(icur.val()); }
                    else { c.tw[len + j] = cur; c.tiw[len + j] = icur; }}}
            c.cap = n;}

        // Lazy Montgomery DIF; in != nullptr converts canonical values during the first level.
        static void dif(const T *in, W *a, int n, const W *w, const Mont<W> &m) {
            int len = n / 2;
            if (in && n < 32) {
                for (int i = 0; i < n; ++i) { a[i] = m.from(in[i].val()); }
                in = nullptr;}
#ifdef __AVX2__
            if constexpr (B == 32) {
                if (n >= 8) {
                    Mont8 v(m); __m256i r2 = _mm256_set1_epi32(int(m.r2));
                    for (; len >= 16; len /= 4) {
                        int h = len / 2;
                        for (int i = 0; i < n; i += 2 * len) {
                            for (int j = 0; j < h; j += 8) {
                                W *p0 = a + i + j, *p1 = p0 + h, *p2 = p0 + len, *p3 = p2 + h;
                                __m256i a0, a1, a2, a3;
                                if (in) {
                                    const T *q = in + i + j;
                                    a0 = v.mul(v.load(q), r2); a1 = v.mul(v.load(q + h), r2); a2 = v.mul(v.load(q + len), r2); a3 = v.mul(v.load(q + len + h), r2);}
                                else { a0 = v.load(p0); a1 = v.load(p1); a2 = v.load(p2); a3 = v.load(p3); }
                                __m256i x0 = v.add(a0, a2), x2 = v.mul(v.diff(a0, a2), v.load(w + len + j));
                                __m256i x1 = v.add(a1, a3), x3 = v.mul(v.diff(a1, a3), v.load(w + len + j + h)), wh = v.load(w + h + j);
                                v.store(p0, v.add(x0, x1)); v.store(p1, v.mul(v.diff(x0, x1), wh));
                                v.store(p2, v.add(x2, x3)); v.store(p3, v.mul(v.diff(x2, x3), wh));}}
                        in = nullptr;}
                    if (len == 8) {
                        __m256i w8 = v.load(w + 8);
                        for (int i = 0; i < n; i += 16) {
                            __m256i x = v.load(a + i), y = v.load(a + i + 8);
                            v.store(a + i, v.add(x, y)); v.store(a + i + 8, v.mul(v.diff(x, y), w8));}
                        len = 4;}
                    __m256i r4 = _mm256_broadcastsi128_si256(_mm_loadu_si128(reinterpret_cast<const __m128i *>(w + 4)));
                    __m256i r2w = _mm256_set1_epi64x(lng(ulng(w[2]) | ulng(w[3]) << 32));
                    for (int i = 0; i < n; i += 8) {
                        __m256i x = v.load(a + i);
                        __m256i lo = _mm256_permute2x128_si256(x, x, 0x00), hi = _mm256_permute2x128_si256(x, x, 0x11);
                        x = _mm256_blend_epi32(v.add(lo, hi), v.mul(v.diff(lo, hi), r4), 0xF0);
                        lo = _mm256_shuffle_epi32(x, 0x44); hi = _mm256_shuffle_epi32(x, 0xEE);
                        x = _mm256_blend_epi32(v.add(lo, hi), v.mul(v.diff(lo, hi), r2w), 0xCC);
                        lo = _mm256_shuffle_epi32(x, 0xA0); hi = _mm256_shuffle_epi32(x, 0xF5);
                        v.store(a + i, _mm256_blend_epi32(v.add(lo, hi), v.sub(lo, hi), 0xAA));}
                    len = 0;}}
#endif
            for (; len >= 2; len /= 4) {
                int h = len / 2;
                for (int i = 0; i < n; i += 2 * len) {
                    for (int j = 0; j < h; ++j) {
                        W *p0 = a + i + j, *p1 = p0 + h, *p2 = p0 + len, *p3 = p2 + h, a0, a1, a2, a3;
                        if (in) { a0 = m.from(in[i + j].val()); a1 = m.from(in[i + j + h].val()); a2 = m.from(in[i + j + len].val()); a3 = m.from(in[i + j + len + h].val()); }
                        else { a0 = *p0; a1 = *p1; a2 = *p2; a3 = *p3; }
                        W x0 = m.add(a0, a2), x2 = m.mul(m.diff(a0, a2), w[len + j]), x1 = m.add(a1, a3), x3 = m.mul(m.diff(a1, a3), w[len + j + h]);
                        *p0 = m.add(x0, x1); *p1 = m.mul(m.diff(x0, x1), w[h + j]); *p2 = m.add(x2, x3); *p3 = m.mul(m.diff(x2, x3), w[h + j]);}}
                in = nullptr;}
            if (len == 1) {
                for (int i = 0; i < n; i += 2) { W x = a[i], y = a[i + 1]; a[i] = m.add(x, y); a[i + 1] = m.sub(x, y); }}}

        // Lazy Montgomery DIT with inverse roots; scale is plain n^-1 when out != nullptr (canonical output) else Montgomery form (lazy in a).
        static void dit(W *a, T *out, int n, const W *w, W scale, const Mont<W> &m) {
            bool fuse = n >= 32; int len = 1;
            auto emit = [&](W *p, W x, int i) { W y = m.mul(x, scale); if (out) { out[i] = T::init(m.canon(y)); } else { *p = y; } };
#ifdef __AVX2__
            if constexpr (B == 32) {
                if (n >= 8) {
                    Mont8 v(m); __m256i s8 = _mm256_set1_epi32(int(scale));
                    __m256i r4 = _mm256_broadcastsi128_si256(_mm_loadu_si128(reinterpret_cast<const __m128i *>(w + 4)));
                    __m256i r2w = _mm256_set1_epi64x(lng(ulng(w[2]) | ulng(w[3]) << 32));
                    for (int i = 0; i < n; i += 8) {
                        __m256i x = v.load(a + i);
                        __m256i lo = _mm256_shuffle_epi32(x, 0xA0), hi = _mm256_shuffle_epi32(x, 0xF5);
                        x = _mm256_blend_epi32(v.add(lo, hi), v.sub(lo, hi), 0xAA);
                        lo = _mm256_shuffle_epi32(x, 0x44); hi = v.mul(_mm256_shuffle_epi32(x, 0xEE), r2w);
                        x = _mm256_blend_epi32(v.add(lo, hi), v.sub(lo, hi), 0xCC);
                        lo = _mm256_permute2x128_si256(x, x, 0x00); hi = v.mul(_mm256_permute2x128_si256(x, x, 0x11), r4);
                        v.store(a + i, _mm256_blend_epi32(v.add(lo, hi), v.sub(lo, hi), 0xF0));}
                    len = 8;
                    if ((log2Of(n) - 3) & 1) {
                        __m256i w8 = v.load(w + 8);
                        for (int i = 0; i < n; i += 16) {
                            __m256i x = v.load(a + i), y = v.mul(v.load(a + i + 8), w8);
                            v.store(a + i, v.add(x, y)); v.store(a + i + 8, v.sub(x, y));}
                        len = 16;}
                    for (; len < n; len *= 4) {
                        int h = len, l2 = 2 * len; bool last = fuse && 4 * len == n;
                        auto put = [&](W *p, __m256i x, int i) {
                            if (!last) { v.store(p, x); }
                            else if (out) { v.store(out + i, v.canon(v.mul(x, s8))); }
                            else { v.store(p, v.mul(x, s8)); }};
                        for (int i = 0; i < n; i += 2 * l2) {
                            for (int j = 0; j < h; j += 8) {
                                W *p0 = a + i + j, *p1 = p0 + h, *p2 = p0 + l2, *p3 = p2 + h;
                                __m256i w0 = v.load(w + h + j), t1 = v.mul(v.load(p1), w0), t3 = v.mul(v.load(p3), w0), a0 = v.load(p0), a2 = v.load(p2);
                                __m256i x0 = v.add(a0, t1), x1 = v.sub(a0, t1), x2 = v.add(a2, t3), x3 = v.sub(a2, t3);
                                __m256i u2 = v.mul(x2, v.load(w + l2 + j)), u3 = v.mul(x3, v.load(w + l2 + j + h));
                                put(p0, v.add(x0, u2), i + j); put(p1, v.add(x1, u3), i + j + h); put(p2, v.sub(x0, u2), i + j + l2); put(p3, v.sub(x1, u3), i + j + l2 + h);}}}
                    len = n;}}
#endif
            if (len < n && (log2Of(n) & 1)) {
                for (int i = 0; i < n; i += 2) { W x = a[i], y = a[i + 1]; a[i] = m.add(x, y); a[i + 1] = m.sub(x, y); }
                len = 2;}
            for (; len < n; len *= 4) {
                int h = len, l2 = 2 * len; bool last = fuse && 4 * len == n;
                for (int i = 0; i < n; i += 2 * l2) {
                    for (int j = 0; j < h; ++j) {
                        W *p0 = a + i + j, *p1 = p0 + h, *p2 = p0 + l2, *p3 = p2 + h;
                        W t1 = m.mul(*p1, w[h + j]), t3 = m.mul(*p3, w[h + j]), x0 = m.add(*p0, t1), x1 = m.sub(*p0, t1), x2 = m.add(*p2, t3), x3 = m.sub(*p2, t3);
                        W u2 = m.mul(x2, w[l2 + j]), u3 = m.mul(x3, w[l2 + j + h]), y0 = m.add(x0, u2), y1 = m.add(x1, u3), y2 = m.sub(x0, u2), y3 = m.sub(x1, u3);
                        if (last) { emit(p0, y0, i + j); emit(p1, y1, i + j + h); emit(p2, y2, i + j + l2); emit(p3, y3, i + j + l2 + h); }
                        else { *p0 = y0; *p1 = y1; *p2 = y2; *p3 = y3; }}}}
            if (!fuse) { for (int i = 0; i < n; ++i) { emit(a + i, a[i], i); } }}

        // Generic radix-2 kernels on T for moduli outside the Montgomery domain.
        static void difT(T *a, int n, const T *w) {
            for (int len = n / 2; len >= 1; len /= 2) {
                for (int i = 0; i < n; i += 2 * len) {
                    for (int j = 0; j < len; ++j) { T x = a[i + j], y = a[i + j + len]; a[i + j] = x + y; a[i + j + len] = (x - y) * w[len + j]; }}}}
        static void ditT(T *a, int n, const T *w) {
            for (int len = 1; len < n; len *= 2) {
                for (int i = 0; i < n; i += 2 * len) {
                    for (int j = 0; j < len; ++j) { T x = a[i + j], y = a[i + j + len] * w[len + j]; a[i + j] = x + y; a[i + j + len] = x - y; }}}}

        // In-place transforms on canonical values: forward/inverse (DIF/DIT) and their transposes (DIT with w / DIF with iw).
        static void transform(vector<T> &a, bool inverse, bool transposed) {
            int n = int(a.size()); assert(std::has_single_bit(uint(n)));
            ensure(n); Cache &c = cache();
            T sc = inverse ? T(1) / T(n) : T(1);
            if (!c.mont) {
                const T *w = (inverse ? c.tiw : c.tw).data();
                if (inverse != transposed) { ditT(a.data(), n, w); }
                else { difT(a.data(), n, w); }
                if (inverse) { for (T &x : a) { x *= sc; } }
                return;}
            const W *w = (inverse ? c.iw : c.w).data(); vector<W> &b = c.buf[0]; b.resize(n);
            if (inverse != transposed) {
                for (int i = 0; i < n; ++i) { b[i] = c.m.from(a[i].val()); }
                dit(b.data(), a.data(), n, w, sc.val(), c.m);}
            else {
                dif(a.data(), b.data(), n, w, c.m);
                W s = c.m.from(sc.val());
                for (int i = 0; i < n; ++i) { a[i] = T::init(c.m.to(c.m.mul(b[i], s))); }}}

        // T: O(n * log(n)), M: O(n); full product of a and b (b may alias a for squaring) of length |a| + |b| - 1.
        static vector<T> convolve(const vector<T> &a, const vector<T> &b) {
            int n = int(a.size()), m = int(b.size()), len = ceilPow2(n + m - 1);
            assert(n && m);
            if (len > maxLength()) { return convolveLarge(a, b); }
            ensure(len); Cache &c = cache();
            bool same = &a == &b;
            vector<T> res(len);
            T sc = T(1) / T(len);
            if (!c.mont) {
                vector<T> x(a.begin(), a.end()), y; x.resize(len); difT(x.data(), len, c.tw.data());
                if (!same) { y.assign(b.begin(), b.end()); y.resize(len); difT(y.data(), len, c.tw.data()); }
                const vector<T> &z = same ? x : y;
                for (int i = 0; i < len; ++i) { res[i] = x[i] * z[i]; }
                ditT(res.data(), len, c.tiw.data());
                for (T &v : res) { v *= sc; }
                res.resize(n + m - 1); return res;}
            vector<T> &pa = c.pad[0], &pb = c.pad[1]; vector<W> &x = c.buf[0], &y = c.buf[1];
            pa.assign(len, T()); std::copy(a.begin(), a.end(), pa.begin()); x.resize(len); dif(pa.data(), x.data(), len, c.w.data(), c.m);
            if (!same) { pb.assign(len, T()); std::copy(b.begin(), b.end(), pb.begin()); y.resize(len); dif(pb.data(), y.data(), len, c.w.data(), c.m); }
            const W *z = same ? x.data() : y.data(); int i = 0;
#ifdef __AVX2__
            if constexpr (B == 32) {
                Mont8 v(c.m);
                for (; i + 8 <= len; i += 8) { v.store(x.data() + i, v.mul(v.load(x.data() + i), v.load(z + i))); }}
#endif
            for (; i < len; ++i) { x[i] = c.m.mul(x[i], z[i]); }
            dit(x.data(), res.data(), len, c.iw.data(), sc.val(), c.m);
            res.resize(n + m - 1); return res;}
        // T: O((n + m)^2 / L + (n + m) * log(L)) with L = maxLength / 2, M: O(n + m); blocks of L terms share transforms of the full length.
        static vector<T> convolveLarge(const vector<T> &a, const vector<T> &b) {
            int cap = maxLength(), half = cap / 2, n = int(a.size()), m = int(b.size()), na = (n + half - 1) / half, nb = (m + half - 1) / half;
            assert(half >= 1);
            auto blocks = [&](const vector<T> &v, int cnt) {
                vector<vector<T>> f(cnt);
                for (int i = 0; i < cnt; ++i) { f[i].assign(v.begin() + i * half, v.begin() + min(int(v.size()), (i + 1) * half)); f[i].resize(cap); transform(f[i], false, false); }
                return f;};
            vector<vector<T>> fa = blocks(a, na), fo;
            if (&a != &b) { fo = blocks(b, nb); }
            const vector<vector<T>> &fb = &a == &b ? fa : fo;
            vector<T> res(n + m - 1), acc(cap);
            for (int d = 0; d < na + nb - 1; ++d) {
                fill(acc.begin(), acc.end(), T(0));
                for (int i = max(0, d - nb + 1); i <= min(d, na - 1); ++i) {
                    const vector<T> &x = fa[i], &y = fb[d - i];
                    for (int j = 0; j < cap; ++j) { acc[j] += x[j] * y[j]; }}
                transform(acc, true, false);
                for (int j = 0; j < cap && d * half + j < n + m - 1; ++j) { res[d * half + j] += acc[j]; }}
            return res;}
    };

    // S: O(n) roots, Q: O(n * log(n)), M: O(n); complex doubles as split re/im arrays, bit-reversed order, forward root exp(i * pi * j / len).
    struct Fft {
        struct Cache { vector<double> wr, wi; int cap = 1; vector<double> re[2], im[2]; };
        using Pd = std::complex<double>;

        static Cache &cache() { static Cache c{{0, 1}, {0, 0}, 1, {}, {}}; return c; }
        static void ensure(int n) {
            Cache &c = cache();
            if (n <= c.cap) { return; }
            c.wr.resize(n); c.wi.resize(n);
            for (int len = c.cap; len < n; len *= 2) {
                for (int j = 0; j < len; ++j) {
                    long double t = std::numbers::pi_v<long double> * j / len;
                    c.wr[len + j] = double(std::cos(t)); c.wi[len + j] = double(std::sin(t));
                    if (2 * j == len) { c.wr[len + j] = 0; c.wi[len + j] = 1; }}}
            c.cap = n;}
        static int partner(int p) { return p < 2 ? p : 3 * (1 << (31 - std::countl_zero(uint(p)))) - 1 - p; }

#ifdef __AVX2__
        static __m256d load(const double *q) { return _mm256_loadu_pd(q); }
        static void store(double *q, __m256d x) { _mm256_storeu_pd(q, x); }
        // (xr + i xi) * (wr + i wi) with conj selecting the conjugate root.
        static void cmul(__m256d &xr, __m256d &xi, __m256d wr, __m256d wi, bool conj) {
            __m256d a = _mm256_mul_pd(xi, wi), b = _mm256_mul_pd(xi, wr), rr, ri;
#ifdef __FMA__
            rr = conj ? _mm256_fmadd_pd(xr, wr, a) : _mm256_fmsub_pd(xr, wr, a);
            ri = conj ? _mm256_fnmadd_pd(xr, wi, b) : _mm256_fmadd_pd(xr, wi, b);
#else
            rr = conj ? _mm256_add_pd(_mm256_mul_pd(xr, wr), a) : _mm256_sub_pd(_mm256_mul_pd(xr, wr), a);
            ri = conj ? _mm256_sub_pd(b, _mm256_mul_pd(xr, wi)) : _mm256_add_pd(_mm256_mul_pd(xr, wi), b);
#endif
            xr = rr; xi = ri;}
#endif
        static void dif(double *re, double *im, int n, bool conj) {
            Cache &c = cache(); const double *wr = c.wr.data(), *wi = c.wi.data(); int len = n / 2;
#ifdef __AVX2__
            if (n >= 4) {
                for (; len >= 4; len /= 2) {
                    for (int i = 0; i < n; i += 2 * len) {
                        for (int j = 0; j < len; j += 4) {
                            double *r0 = re + i + j, *i0 = im + i + j, *r1 = r0 + len, *i1 = i0 + len;
                            __m256d xr = load(r0), xi = load(i0), yr = load(r1), yi = load(i1);
                            store(r0, _mm256_add_pd(xr, yr)); store(i0, _mm256_add_pd(xi, yi));
                            __m256d dr = _mm256_sub_pd(xr, yr), di = _mm256_sub_pd(xi, yi);
                            cmul(dr, di, load(wr + len + j), load(wi + len + j), conj);
                            store(r1, dr); store(i1, di);}}}
                __m256d w2r = _mm256_set_pd(wr[3], wr[2], wr[3], wr[2]), w2i = _mm256_set_pd(wi[3], wi[2], wi[3], wi[2]);
                for (int i = 0; i < n; i += 4) {
                    __m256d xr = load(re + i), xi = load(im + i);
                    __m256d lr = _mm256_permute2f128_pd(xr, xr, 0x00), li = _mm256_permute2f128_pd(xi, xi, 0x00);
                    __m256d hr = _mm256_permute2f128_pd(xr, xr, 0x11), hi = _mm256_permute2f128_pd(xi, xi, 0x11);
                    __m256d dr = _mm256_sub_pd(lr, hr), di = _mm256_sub_pd(li, hi);
                    cmul(dr, di, w2r, w2i, conj);
                    xr = _mm256_blend_pd(_mm256_add_pd(lr, hr), dr, 0xC); xi = _mm256_blend_pd(_mm256_add_pd(li, hi), di, 0xC);
                    lr = _mm256_permute_pd(xr, 0x0); li = _mm256_permute_pd(xi, 0x0); hr = _mm256_permute_pd(xr, 0xF); hi = _mm256_permute_pd(xi, 0xF);
                    store(re + i, _mm256_blend_pd(_mm256_add_pd(lr, hr), _mm256_sub_pd(lr, hr), 0xA));
                    store(im + i, _mm256_blend_pd(_mm256_add_pd(li, hi), _mm256_sub_pd(li, hi), 0xA));}
                len = 0;}
#endif
            for (; len >= 1; len /= 2) {
                for (int i = 0; i < n; i += 2 * len) {
                    for (int j = 0; j < len; ++j) {
                        int u = i + j, v = u + len;
                        double xr = re[u], xi = im[u], yr = re[v], yi = im[v], dr = xr - yr, di = xi - yi, r = wr[len + j], s = conj ? -wi[len + j] : wi[len + j];
                        re[u] = xr + yr; im[u] = xi + yi; re[v] = dr * r - di * s; im[v] = dr * s + di * r;}}}}
        static void dit(double *re, double *im, int n, bool conj) {
            Cache &c = cache(); const double *wr = c.wr.data(), *wi = c.wi.data(); int len = 1;
#ifdef __AVX2__
            if (n >= 4) {
                __m256d w2r = _mm256_set_pd(wr[3], wr[2], wr[3], wr[2]), w2i = _mm256_set_pd(wi[3], wi[2], wi[3], wi[2]);
                for (int i = 0; i < n; i += 4) {
                    __m256d xr = load(re + i), xi = load(im + i);
                    __m256d lr = _mm256_permute_pd(xr, 0x0), li = _mm256_permute_pd(xi, 0x0), hr = _mm256_permute_pd(xr, 0xF), hi = _mm256_permute_pd(xi, 0xF);
                    xr = _mm256_blend_pd(_mm256_add_pd(lr, hr), _mm256_sub_pd(lr, hr), 0xA); xi = _mm256_blend_pd(_mm256_add_pd(li, hi), _mm256_sub_pd(li, hi), 0xA);
                    lr = _mm256_permute2f128_pd(xr, xr, 0x00); li = _mm256_permute2f128_pd(xi, xi, 0x00);
                    hr = _mm256_permute2f128_pd(xr, xr, 0x11); hi = _mm256_permute2f128_pd(xi, xi, 0x11);
                    cmul(hr, hi, w2r, w2i, conj);
                    store(re + i, _mm256_blend_pd(_mm256_add_pd(lr, hr), _mm256_sub_pd(lr, hr), 0xC));
                    store(im + i, _mm256_blend_pd(_mm256_add_pd(li, hi), _mm256_sub_pd(li, hi), 0xC));}
                for (len = 4; len < n; len *= 2) {
                    for (int i = 0; i < n; i += 2 * len) {
                        for (int j = 0; j < len; j += 4) {
                            double *r0 = re + i + j, *i0 = im + i + j, *r1 = r0 + len, *i1 = i0 + len;
                            __m256d xr = load(r0), xi = load(i0), yr = load(r1), yi = load(i1);
                            cmul(yr, yi, load(wr + len + j), load(wi + len + j), conj);
                            store(r0, _mm256_add_pd(xr, yr)); store(i0, _mm256_add_pd(xi, yi)); store(r1, _mm256_sub_pd(xr, yr)); store(i1, _mm256_sub_pd(xi, yi));}}}}
#endif
            for (; len < n; len *= 2) {
                for (int i = 0; i < n; i += 2 * len) {
                    for (int j = 0; j < len; ++j) {
                        int u = i + j, v = u + len;
                        double r = wr[len + j], s = conj ? -wi[len + j] : wi[len + j], yr = re[v] * r - im[v] * s, yi = re[v] * s + im[v] * r;
                        re[v] = re[u] - yr; im[v] = im[u] - yi; re[u] += yr; im[u] += yi;}}}}

        static void forward(vector<Pd> &a, bool inverse) {
            int n = int(a.size()); assert(std::has_single_bit(uint(n))); ensure(n);
            Cache &c = cache(); c.re[0].resize(n); c.im[0].resize(n);
            for (int i = 0; i < n; ++i) { c.re[0][i] = a[i].real(); c.im[0][i] = a[i].imag(); }
            if (inverse) { dit(c.re[0].data(), c.im[0].data(), n, true); }
            else { dif(c.re[0].data(), c.im[0].data(), n, false); }
            double sc = inverse ? 1.0 / n : 1;
            for (int i = 0; i < n; ++i) { a[i] = Pd(c.re[0][i] * sc, c.im[0][i] * sc); }}

        // T: O(n * log(n)), M: O(n); real sequences packed as re + i im, one forward and one inverse transform.
        static vector<double> convolveReal(const double *a, int n, const double *b, int m) {
            int len = ceilPow2(n + m - 1); ensure(len); Cache &c = cache();
            vector<double> &re = c.re[0], &im = c.im[0];
            re.assign(len, 0); im.assign(len, 0); std::copy(a, a + n, re.begin()); std::copy(b, b + m, im.begin());
            dif(re.data(), im.data(), len, false);
            for (int p = 0; p < len; ++p) {
                int q = partner(p); if (q < p) { continue; }
                Pd zp(re[p], im[p]), zq(re[q], im[q]);
                Pd ap = (zp + std::conj(zq)) * 0.5, bp = (zp - std::conj(zq)) * Pd(0, -0.5), aq = (zq + std::conj(zp)) * 0.5, bq = (zq - std::conj(zp)) * Pd(0, -0.5);
                Pd cp = ap * bp, cq = aq * bq;
                re[p] = cp.real(); im[p] = cp.imag(); re[q] = cq.real(); im[q] = cq.imag();}
            dit(re.data(), im.data(), len, true);
            vector<double> res(n + m - 1);
            for (int i = 0; i < n + m - 1; ++i) { res[i] = re[i] / len; }
            return res;}
        static vector<Pd> convolveComplex(const Pd *a, int n, const Pd *b, int m) {
            int len = ceilPow2(n + m - 1); ensure(len); Cache &c = cache();
            vector<double> &xr = c.re[0], &xi = c.im[0], &yr = c.re[1], &yi = c.im[1];
            xr.assign(len, 0); xi.assign(len, 0); yr.assign(len, 0); yi.assign(len, 0);
            for (int i = 0; i < n; ++i) { xr[i] = a[i].real(); xi[i] = a[i].imag(); }
            for (int i = 0; i < m; ++i) { yr[i] = b[i].real(); yi[i] = b[i].imag(); }
            dif(xr.data(), xi.data(), len, false); dif(yr.data(), yi.data(), len, false);
            for (int i = 0; i < len; ++i) { Pd z = Pd(xr[i], xi[i]) * Pd(yr[i], yi[i]); xr[i] = z.real(); xi[i] = z.imag(); }
            dit(xr.data(), xi.data(), len, true);
            vector<Pd> res(n + m - 1);
            for (int i = 0; i < n + m - 1; ++i) { res[i] = Pd(xr[i] / len, xi[i] / len); }
            return res;}
        // T: O(n * log(n)), M: O(n); residues < mod < 2^30 split into 15-bit halves, four products rounded from two inverse transforms.
        static vector<uint> convolveMod(const uint *a, int n, const uint *b, int m, uint mod) {
            assert(mod < (1u << 30));
            int len = ceilPow2(n + m - 1); ensure(len); Cache &c = cache();
            vector<double> &lr = c.re[0], &li = c.im[0], &rr = c.re[1], &ri = c.im[1];
            lr.assign(len, 0); li.assign(len, 0); rr.assign(len, 0); ri.assign(len, 0);
            for (int i = 0; i < n; ++i) { lr[i] = a[i] & 32767; li[i] = a[i] >> 15; }
            for (int i = 0; i < m; ++i) { rr[i] = b[i] & 32767; ri[i] = b[i] >> 15; }
            dif(lr.data(), li.data(), len, false); dif(rr.data(), ri.data(), len, false);
            for (int p = 0; p < len; ++p) {
                int q = partner(p); if (q < p) { continue; }
                Pd lp(lr[p], li[p]), lq(lr[q], li[q]), rp(rr[p], ri[p]), rq(rr[q], ri[q]);
                Pd a0p = (lp + std::conj(lq)) * 0.5, a1p = (lp - std::conj(lq)) * Pd(0, -0.5), a0q = (lq + std::conj(lp)) * 0.5, a1q = (lq - std::conj(lp)) * Pd(0, -0.5);
                Pd op = a0p * rp, oq = a0q * rq, sp = a1p * rp, sq = a1q * rq;
                lr[p] = op.real(); li[p] = op.imag(); lr[q] = oq.real(); li[q] = oq.imag();
                rr[p] = sp.real(); ri[p] = sp.imag(); rr[q] = sq.real(); ri[q] = sq.imag();}
            dit(lr.data(), li.data(), len, true); dit(rr.data(), ri.data(), len, true);
            vector<uint> res(n + m - 1); double inv = 1.0 / len;
            for (int i = 0; i < n + m - 1; ++i) {
                ulng a0b0 = ulng(std::llround(lr[i] * inv)) % mod, a0b1 = ulng(std::llround(li[i] * inv)) % mod, a1b0 = ulng(std::llround(rr[i] * inv)) % mod, a1b1 = ulng(std::llround(ri[i] * inv)) % mod;
                res[i] = uint((((a1b1 << 15) + a0b1 + a1b0) % mod << 15) % mod + a0b0) % mod;}
            return res;}
    };

    // T: O(K^2) per coefficient, M: O(1); mixed-radix digits of residues modulo the first K primes.
    template<int K> struct Garner {
        Mont<uint> m[K]; uint inv[K][K] = {};
        ulng wrap[K + 1] = {};

        Garner() {
            for (int i = 0; i < K; ++i) {
                m[i] = Mont<uint>(PRIMES[i]); wrap[i + 1] = wrap[i] ? wrap[i] * PRIMES[i] : PRIMES[i];
                for (int j = 0; j < i; ++j) { inv[j][i] = m[i].from(uint(Barrett32(PRIMES[i]).pow(PRIMES[j], PRIMES[i] - 2))); }}
            wrap[0] = 1;}
        static const Garner &get() { static const Garner g; return g; }
        // In place: c[i] residues mod PRIMES[i] become digits with value sum c[i] * PRIMES[0..i).
        void digits(uint *c) const {
            for (int i = 1; i < K; ++i) {
                uint t = c[i], p = PRIMES[i];
                for (int j = 0; j < i; ++j) { uint d = c[j] >= p ? c[j] - p : c[j]; t = m[i].canon(m[i].mul(t + p - d, inv[j][i])); }
                c[i] = t;}}
    };

    // T: O(n * log(n)), M: O(n); residues of the product modulo P from any integer-valued accessors.
    template<uint P, typename A, typename Bf> vector<uint> convolvePrime(int n, int m, A fa, Bf fb) {
        using M = ModInt<P>;
        vector<M> a(n), b(m);
        for (int i = 0; i < n; ++i) { a[i] = M(fa(i)); }
        for (int i = 0; i < m; ++i) { b[i] = M(fb(i)); }
        vector<M> c = Ntt<M>::convolve(a, b);
        vector<uint> r(c.size());
        for (int i = 0; i < int(c.size()); ++i) { r[i] = c[i].val(); }
        return r;}
    // T: O(K * n * log(n) + K^2 * n), M: O(K * n); out(i, digits) receives the Garner digits of coefficient i.
    template<int K, typename A, typename Bf, typename F> void convolveMulti(int n, int m, A fa, Bf fb, F out) {
        static_assert(1 <= K && K <= 6);
        vector<uint> r[6];
        r[0] = convolvePrime<PRIMES[0]>(n, m, fa, fb);
        if constexpr (K > 1) { r[1] = convolvePrime<PRIMES[1]>(n, m, fa, fb); }
        if constexpr (K > 2) { r[2] = convolvePrime<PRIMES[2]>(n, m, fa, fb); }
        if constexpr (K > 3) { r[3] = convolvePrime<PRIMES[3]>(n, m, fa, fb); }
        if constexpr (K > 4) { r[4] = convolvePrime<PRIMES[4]>(n, m, fa, fb); }
        if constexpr (K > 5) { r[5] = convolvePrime<PRIMES[5]>(n, m, fa, fb); }
        const Garner<K> &g = Garner<K>::get(); uint c[K];
        for (int i = 0; i < n + m - 1; ++i) {
            for (int k = 0; k < K; ++k) { c[k] = r[k][i]; }
            g.digits(c); out(i, c);}}

    // Signed integers accumulate in the unsigned twin so partial sums wrap instead of overflowing; the final value is exact when it fits.
    template<typename T> struct Wrap { using type = T; };
    template<std::signed_integral T> struct Wrap<T> { using type = std::make_unsigned_t<T>; };
    template<typename T> void schoolbook(const T *a, int n, const T *b, int m, T *res) {
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < m; ++j) { res[i + j] += a[i] * b[j]; }}}
    // T: O(n^1.585), M: O(n * log(n)); equal lengths, res has 2n - 1 zeroed entries.
    template<typename T> void karatsuba(const T *a, const T *b, int n, T *res) {
        if (n <= KARATSUBA) { schoolbook(a, n, b, n, res); return; }
        int h = n / 2, t = n - h;
        vector<T> z0(2 * h - 1, T(0)), z2(2 * t - 1, T(0)), z1(2 * t - 1, T(0)), sa(a + h, a + n), sb(b + h, b + n);
        for (int i = 0; i < h; ++i) { sa[i] += a[i]; sb[i] += b[i]; }
        karatsuba(a, b, h, z0.data()); karatsuba(a + h, b + h, t, z2.data()); karatsuba(sa.data(), sb.data(), t, z1.data());
        for (int i = 0; i < 2 * h - 1; ++i) { z1[i] -= z0[i]; res[i] += z0[i]; }
        for (int i = 0; i < 2 * t - 1; ++i) { z1[i] -= z2[i]; res[2 * h + i] += z2[i]; }
        for (int i = 0; i < 2 * t - 1; ++i) { res[h + i] += z1[i]; }}

    // T: O(M(n * (la + lb))) big-integer work for limb counts la, lb; M: O(n * (la + lb)); Kronecker packing with offsets for signs.
    template<BigInt T> vector<T> kronecker(const vector<T> &a, const vector<T> &b) {
        int n = int(a.size()), m = int(b.size());
        auto limbs = [](const vector<T> &v) { int l = 1; for (const T &x : v) { l = max(l, int(x.n.size())); } return l; };
        auto power = [](int l) { T c; c.n.assign(l, 0); c.n.push_back(1); c.sgn = 1; return c; };
        int la = limbs(a), lb = limbs(b), s = la + lb + 3;
        T ca = power(la), cb = power(lb);
        auto pack = [&](const vector<T> &v, const T &off) {
            T big; big.n.assign(size_t(s) * v.size(), 0); big.sgn = 1;
            for (int i = 0; i < int(v.size()); ++i) { T x = v[i] + off; assert(x.sgn == 1 && int(x.n.size()) <= s); std::copy(x.n.begin(), x.n.end(), big.n.begin() + size_t(s) * i); }
            big.trim(); return big;};
        T prod = pack(a, ca) * pack(b, cb); prod.n.resize(size_t(s) * (n + m - 1), 0);
        vector<T> pa(n + 1), pb(m + 1), res(n + m - 1);
        for (int i = 0; i < n; ++i) { pa[i + 1] = pa[i] + a[i]; }
        for (int i = 0; i < m; ++i) { pb[i + 1] = pb[i] + b[i]; }
        T cc = ca * cb;
        for (int k = 0; k < n + m - 1; ++k) {
            int lo = max(0, k - m + 1), hi = min(k, n - 1);
            T &r = res[k]; r.n.assign(prod.n.begin() + size_t(s) * k, prod.n.begin() + size_t(s) * (k + 1)); r.sgn = 1; r.trim();
            r -= ca * (pb[k - lo + 1] - pb[k - hi]); r -= cb * (pa[hi + 1] - pa[lo]); r -= cc * T(hi - lo + 1);}
        return res;}
    template<typename T> bool fastSpec(int n) {
        if constexpr (ModularInt<T>) { return Ntt<T>::usable(n); }
        else { return false; }}
    // Transform-domain helpers of power-of-two length N; without an NTT the vectors stay coefficients and dot is a cyclic product.
    template<typename T> vector<T> spec(const vector<T> &a, int n) {
        vector<T> r(a.begin(), a.begin() + min(size_t(n), a.size())); r.resize(n);
        if constexpr (ModularInt<T>) { if (fastSpec<T>(n)) { Ntt<T>::transform(r, false, false); } }
        return r;}
    template<typename T> vector<T> unspec(vector<T> a, int n) {
        if constexpr (ModularInt<T>) { if (fastSpec<T>(n)) { Ntt<T>::transform(a, true, false); } }
        return a;}
    template<typename T> void dot(vector<T> &a, const vector<T> &b, int n) {
        if (fastSpec<T>(n)) { for (int i = 0; i < n; ++i) { a[i] *= b[i]; } return; }
        vector<T> c = convolution(Poly<T>(a), Poly<T>(b)).v;
        a.assign(n, T(0));
        for (int i = 0; i < int(c.size()); ++i) { a[i % n] += c[i]; }}
    template<typename T> vector<T> slice(const vector<T> &a, int l, int r) {
        vector<T> res(r - l);
        for (int i = l; i < r; ++i) { if (i < int(a.size())) { res[i - l] = a[i]; } }
        return res;}
    // T: O(log(e)), M: O(1).
    template<typename T> T power(T a, ulng e) {
        T r = 1;
        for (; e; e >>= 1, a *= a) { if (e & 1) { r *= a; } }
        return r;}
    // Square root of a constant: modular types through trySqrt, floating point through std::sqrt, otherwise only 1.
    template<typename T> bool sqrtConst(const T &c, T &out) {
        if constexpr (ModularInt<T>) { return trySqrt(c, out); }
        else if constexpr (std::is_floating_point_v<T>) { if (c < 0) { return false; } out = std::sqrt(c); return true; }
        else if constexpr (is_complex<T>) { out = std::sqrt(c); return true; }
        else { if (c != T(1)) { return false; } out = T(1); return true; }}
    template<typename T> int valuation(const Poly<T> &a) {
        int v = 0;
        while (v < a.size() && a.v[v] == T(0)) { ++v; }
        return v;}
    // T: O(sqrt(p) + log(p)^2), M: O(1); smallest generator of the multiplicative group of the prime p.
    inline uint primitiveRootMod(ulng p) {
        if (p == 2) { return 1; }
        vector<ulng> f; ulng r = p - 1;
        for (ulng q = 2; q * q <= r; ++q) { if (r % q == 0) { f.push_back(q); while (r % q == 0) { r /= q; } } }
        if (r > 1) { f.push_back(r); }
        auto pw = [&](ulng a, ulng e) { ulng x = 1; for (; e; e >>= 1, a = ulng(ulll(a) * a % p)) { if (e & 1) { x = ulng(ulll(x) * a % p); } } return x; };
        for (uint g = 2; g < p; ++g) {
            if (pw(g, p - 1) != 1) { return 0; }
            bool ok = true;
            for (ulng q : f) { if (pw(g, (p - 1) / q) == 1) { ok = false; break; } }
            if (ok) { return g; }}
        return 0;}
    template<typename T> vector<T> dft(const vector<T> &a, const T &w);
    // T: O(p^2) below 32, O(M(p)) via Rader above; prime length p with w a primitive p-th root.
    template<typename T> vector<T> dftPrime(const vector<T> &a, const T &w) {
        int p = int(a.size());
        vector<T> pw(p), res(p);
        pw[0] = 1;
        for (int e = 1; e < p; ++e) { pw[e] = pw[e - 1] * w; }
        if (p < 32) {
            for (int k = 0; k < p; ++k) { T s = 0; for (int j = 0, e = 0; j < p; ++j, e = e + k >= p ? e + k - p : e + k) { s += a[j] * pw[e]; } res[k] = s; }
            return res;}
        int g = int(primitiveRootMod(ulng(p)));
        vector<int> gp(p - 1); gp[0] = 1;
        for (int u = 1; u < p - 1; ++u) { gp[u] = int(lng(gp[u - 1]) * g % p); }
        Poly<T> bq(p - 1), cq(p - 1);
        for (int u = 0; u < p - 1; ++u) { bq.v[u] = a[gp[u]]; cq.v[u] = pw[gp[(p - 1 - u) % (p - 1)]]; }
        Poly<T> conv = cyclic(bq, cq, p - 1);
        T s = 0;
        for (const T &x : a) { s += x; }
        res[0] = s;
        for (int v = 0; v < p - 1; ++v) { res[gp[(p - 1 - v) % (p - 1)]] = a[0] + conv.v[v]; }
        return res;}
    // T: O(n * sum of prime factors with Rader for large ones), M: O(n); w a primitive n-th root of unity.
    template<typename T> vector<T> dft(const vector<T> &a, const T &w) {
        int n = int(a.size());
        if (n <= 1) { return a; }
        int p = 2;
        while (lng(p) * p <= n && n % p) { ++p; }
        if (lng(p) * p > n) { return dftPrime(a, w); }
        int m = n / p;
        T wp = power(w, ulng(p)), wm = power(w, ulng(m));
        vector<vector<T>> parts(p);
        vector<T> sub(m), res(n), col(p), wr(p), cur(p, T(1));
        for (int r = 0; r < p; ++r) {
            for (int j = 0; j < m; ++j) { sub[j] = a[j * p + r]; }
            parts[r] = dft(sub, wp);}
        wr[0] = 1;
        for (int r = 1; r < p; ++r) { wr[r] = wr[r - 1] * w; }
        for (int k1 = 0; k1 < m; ++k1) {
            for (int r = 0; r < p; ++r) { col[r] = parts[r][k1] * cur[r]; cur[r] *= wr[r]; }
            vector<T> col2 = dftPrime(col, wm);
            for (int k2 = 0; k2 < p; ++k2) { res[k1 + m * k2] = col2[k2]; }}
        return res;}
    // T: O(1), M: O(1); (lo, hi) carry-less product of two GF(2)[x] words.
    inline ulll clmul(ulng a, ulng b) {
        ulll r = 0;
#ifdef __PCLMUL__
        __m128i p = _mm_clmulepi64_si128(_mm_cvtsi64_si128(lng(a)), _mm_cvtsi64_si128(lng(b)), 0);
        r = ulll(ulng(_mm_cvtsi128_si64(_mm_srli_si128(p, 8)))) << 64 | ulng(_mm_cvtsi128_si64(p));
#else
        for (int i = 0; i < 64; ++i) { if (b >> i & 1) { r ^= ulll(a) << i; } }
#endif
        return r;}
    // T: O(n^1.585) word products, M: O(n * log(n)); equal lengths, res has 2n - 1 entries that are xor-accumulated.
    inline void karatsubaGf2(const ulng *a, const ulng *b, int n, ulll *res) {
        if (n <= 24) {
            for (int i = 0; i < n; ++i) { for (int j = 0; j < n; ++j) { res[i + j] ^= clmul(a[i], b[j]); } }
            return;}
        int h = n / 2, t = n - h;
        vector<ulll> z0(2 * h - 1, 0), z2(2 * t - 1, 0), z1(2 * t - 1, 0);
        vector<ulng> sa(a + h, a + n), sb(b + h, b + n);
        for (int i = 0; i < h; ++i) { sa[i] ^= a[i]; sb[i] ^= b[i]; }
        karatsubaGf2(a, b, h, z0.data()); karatsubaGf2(a + h, b + h, t, z2.data()); karatsubaGf2(sa.data(), sb.data(), t, z1.data());
        for (int i = 0; i < 2 * h - 1; ++i) { z1[i] ^= z0[i]; res[i] ^= z0[i]; }
        for (int i = 0; i < 2 * t - 1; ++i) { z1[i] ^= z2[i]; res[2 * h + i] ^= z2[i]; }
        for (int i = 0; i < 2 * t - 1; ++i) { res[h + i] ^= z1[i]; }}
    // T: O(M(n) * log(n)) build, M: O(n * log(n)); subproduct tree over m leaves padded to N, t[v] = product of the leaves under v.
    template<typename T> struct Tree {
        int m, nleaf; vector<Poly<T>> t;

        explicit Tree(const vector<T> &xs) : m(int(xs.size())), nleaf(ceilPow2(max(m, 1))), t(2 * nleaf) {
            for (int i = 0; i < nleaf; ++i) { t[nleaf + i] = i < m ? Poly<T>{-xs[i], T(1)} : Poly<T>{T(1)}; }
            build();}
        explicit Tree(const vector<Poly<T>> &given) : m(int(given.size())), nleaf(ceilPow2(max(m, 1))), t(2 * nleaf) {
            for (int i = 0; i < nleaf; ++i) { t[nleaf + i] = i < m ? given[i] : Poly<T>{T(1)}; }
            build();}
        void build() { for (int i = nleaf - 1; i >= 1; --i) { t[i] = t[2 * i] * t[2 * i + 1]; } }
        int cnt(int v) const { return t[v].size() - 1; }

        // b[i] = sum_{j >= i} a[j] * d[j - i] for i < len.
        static vector<T> corr(const vector<T> &a, const vector<T> &d, int len) {
            if (!len) { return {}; }
            Poly<T> x(a); x.resize(int(d.size()) - 1 + len);
            Poly<T> y(d); y.reverse();
            return middleProduct(x, y).v;}
        // Transposed evaluation down the tree; nleaf hold the values a(x_i).
        vector<T> eval(const Poly<T> &a) const {
            if (!m) { return {}; }
            Poly<T> f = a.size() > m ? a % t[1] : a;
            f.trim();
            int n = f.size();
            vector<T> res(m);
            if (!n) { return res; }
            vector<T> root = corr(f.v, inv(Poly<T>(t[1]).reverse(), n).v, m);
            auto down = [&](auto &&self, int v, vector<T> b) -> void {
                if (v >= nleaf) { if (v - nleaf < m) { res[v - nleaf] = b[0]; } return; }
                int l = 2 * v, r = l + 1, cl = cnt(l), cr = cnt(r);
                if (!cl) { return; }
                vector<T> bl = corr(b, Poly<T>(t[r]).reverse().v, cl), br = corr(b, Poly<T>(t[l]).reverse().v, cr);
                self(self, l, std::move(bl)); self(self, r, std::move(br));};
            down(down, 1, std::move(root));
            return res;}
        // sum over nleaf of c_i * t[1] / t[leaf] with polynomial weights c_i, bottom up.
        Poly<T> combine(const vector<Poly<T>> &c) const {
            vector<Poly<T>> f(2 * nleaf);
            for (int i = 0; i < nleaf; ++i) { f[nleaf + i] = i < m ? c[i] : Poly<T>(); }
            for (int i = nleaf - 1; i >= 1; --i) { f[i] = f[2 * i] * t[2 * i + 1] + f[2 * i + 1] * t[2 * i]; }
            return f[1];}
        Poly<T> combine(const vector<T> &c) const {
            vector<Poly<T>> w(m);
            for (int i = 0; i < m; ++i) { w[i] = Poly<T>{c[i]}; }
            return combine(w);}
        // a mod t[leaf] for every leaf, top down.
        vector<Poly<T>> remainders(const Poly<T> &a) const {
            vector<Poly<T>> r(2 * nleaf);
            r[1] = a % t[1];
            for (int v = 2; v < nleaf + m; ++v) { r[v] = r[v / 2] % t[v]; }
            return vector<Poly<T>>(r.begin() + nleaf, r.begin() + nleaf + m);}
    };
    template<typename T> vector<T> inverseList(const vector<T> &a) {
        int n = int(a.size());
        vector<T> pre(n + 1, T(1));
        for (int i = 0; i < n; ++i) { pre[i + 1] = pre[i] * a[i]; }
        T inv = T(1) / pre[n];
        vector<T> res(n);
        for (int i = n - 1; i >= 0; --i) { res[i] = inv * pre[i]; inv *= a[i]; }
        return res;}
    // Factorials and inverse factorials for 0..n-1; needs 1..n-1 invertible.
    template<typename T> pair<vector<T>, vector<T>> factorials(int n) {
        vector<T> f(max(n, 1), T(1)), g = inverses<T>(n);
        for (int i = 1; i < n; ++i) { f[i] = f[i - 1] * T(i); g[i] *= g[i - 1]; }
        return {f, g};}
    template<typename T> Poly<T> polyPow(Poly<T> a, int e) {
        Poly<T> r{T(1)};
        for (; e; e >>= 1, a = a * a) { if (e & 1) { r = r * a; } }
        return r;}
    // Falling factorial c^(t) / t! for t < n without dividing by anything but t.
    template<typename T> vector<T> binomialsOf(const T &c, int n) {
        vector<T> b(max(n, 1), T(1)), inv = inverses<T>(n);
        for (int t = 1; t < n; ++t) { b[t] = b[t - 1] * (c - T(t - 1)) * inv[t]; }
        return b;}
    // 2x2 polynomial matrix with an identity flag; step() left-multiplies by the Euclid step [[0, 1], [1, -q]].
    template<typename T> struct Mat2 {
        Poly<T> a[2][2]; bool id = true;

        pair<Poly<T>, Poly<T>> apply(const Poly<T> &p, const Poly<T> &q) const {
            if (id) { return {p, q}; }
            return {(a[0][0] * p + a[0][1] * q).trim(), (a[1][0] * p + a[1][1] * q).trim()};}
        void swapRows() {
            if (id) { id = false; a[0][0] = {T(0)}; a[0][1] = {T(1)}; a[1][0] = {T(1)}; a[1][1] = {T(0)}; return; }
            swap(a[0][0], a[1][0]); swap(a[0][1], a[1][1]);}
        void step(const Poly<T> &q) {
            swapRows();
            a[1][0] = (a[1][0] - q * a[0][0]).trim(); a[1][1] = (a[1][1] - q * a[0][1]).trim();}
        friend Mat2 operator*(const Mat2 &x, const Mat2 &y) {
            if (x.id) { return y; }
            if (y.id) { return x; }
            Mat2 r; r.id = false;
            for (int i = 0; i < 2; ++i) { for (int j = 0; j < 2; ++j) { r.a[i][j] = (x.a[i][0] * y.a[0][j] + x.a[i][1] * y.a[1][j]).trim(); } }
            return r;}
    };
    template<typename T> Poly<T> shiftDown(const Poly<T> &a, int s) { return s > 0 ? Poly<T>(a) >> s : a; }
    // T: O(M(n) * log(n)), M: O(n); deg r0 > deg r1; M with M * (r0, r1) = (r_h, r_{h+1}) for the largest h with deg r0 - deg r_h <= k; quotients appended.
    template<typename T> Mat2<T> fastEuclid(const Poly<T> &r0, const Poly<T> &r1, int k, vector<Poly<T>> *qs = nullptr) {
        int n0 = r0.deg(), n1 = r1.deg();
        Mat2<T> mat;
        if (n1 < 0 || k < n0 - n1) { return mat; }
        if (n0 <= HGCD_BASE) {
            Poly<T> a = r0, b = r1; a.trim(); b.trim();
            while (b.deg() >= 0 && n0 - b.deg() <= k) {
                auto [q, r] = divMod(a, b);
                if (qs) { qs->push_back(q); }
                mat.step(q); a = std::move(b); b = std::move(r);}
            return mat;}
        int d = (k + 1) / 2, s = n0 - (2 * d - 2);
        Mat2<T> left = fastEuclid(shiftDown(r0, s), shiftDown(r1, s), d - 1, qs);
        auto [a, b] = left.apply(r0, r1);
        if (b.deg() < 0 || k < n0 - b.deg()) { return left; }
        auto [q, r] = divMod(a, b);
        if (qs) { qs->push_back(q); }
        left.step(q);
        int ds = k - (n0 - b.deg()), s2 = b.deg() - 2 * ds;
        return fastEuclid(shiftDown(b, s2), shiftDown(r, s2), ds, qs) * left;}
    // Euclid state (p, q) = M * (a, b) with deg p > deg q after at most one preliminary step, then the fast chain under bound k.
    template<typename T> Mat2<T> euclidMatrix(const Poly<T> &a, const Poly<T> &b, int k, vector<Poly<T>> *qs = nullptr) {
        Poly<T> p = a, q = b; p.trim(); q.trim();
        Mat2<T> mat;
        if (p.deg() < q.deg()) { swap(p, q); mat.swapRows(); }
        else if (p.deg() == q.deg() && q.deg() >= 0) {
            auto [qq, r] = divMod(p, q);
            if (qs) { qs->push_back(qq); }
            mat.step(qq); p = q; q = r;}
        if (q.deg() >= 0) { return fastEuclid(p, q, k, qs) * mat; }
        return mat;}
    template<typename T> T leadSign(const T &x) {
        if constexpr (Ring<T>) { return x < T(0) ? T(-1) : T(1); }
        else { return T(1); }}
    template<typename T> bool lessPoly(const Poly<T> &x, const Poly<T> &y) {
        if (x.deg() != y.deg()) { return x.deg() < y.deg(); }
        for (int i = x.deg(); i >= 0; --i) { if (x.v[i] != y.v[i]) { return x.v[i].val() < y.v[i].val(); } }
        return false;}
    // Equal-degree splitting of a monic product g of irreducibles of degree d (Cantor–Zassenhaus; trace map for p = 2), Las Vegas.
    template<ModularInt T> void equalDegree(const Poly<T> &g, int d, std::mt19937_64 &rng, vector<Poly<T>> &out) {
        if (g.deg() == d) { out.push_back(g); return; }
        PolyModulus<T> pm(g);
        ulng p = T::mod();
        for (int tries = 0; tries < 256; ++tries) {
            Poly<T> r(g.deg());
            for (T &x : r.v) { x = T(rng()); }
            Poly<T> t;
            if (p == 2) {
                Poly<T> w = pm.reduce(r); t = w;
                for (int i = 1; i < d; ++i) { w = pm.mul(w, w); t += w; }}
            else {
                Poly<T> u = pm.pow(r, (p - 1) / 2), w = u; t = u;
                for (int i = 1; i < d; ++i) { w = pm.pow(w, p); t = pm.mul(t, w); }
                t -= Poly<T>{T(1)};}
            Poly<T> h = gcd(t, g);
            if (h.deg() > 0 && h.deg() < g.deg()) { equalDegree(h, d, rng, out); equalDegree(g / h, d, rng, out); return; }}
        out.push_back(g);}
    // Bivariate arrays are row-major with the x index as the row; pack with the y stride S and multiply as one polynomial.
    template<typename T> vector<T> pack(const vector<T> &a, int rows, int cols, int stride) {
        vector<T> r(size_t(rows) * stride);
        for (int i = 0; i < rows; ++i) { std::copy(a.begin() + size_t(i) * cols, a.begin() + size_t(i) * cols + cols, r.begin() + size_t(i) * stride); }
        return r;}
    // Rows [0, keep) of the stride-S product; entries past column S - 1 of a row spill into the next row.
    template<typename T> vector<T> mul2d(const vector<T> &a, int ra, int ca, const vector<T> &b, int rb, int cb, int stride, int keep) {
        Poly<T> p = Poly<T>(pack(a, ra, ca, stride)) * Poly<T>(pack(b, rb, cb, stride));
        p.resize(keep * stride);
        return p.v;}
    // Q_i(x, y) for every level of the Kinoshita–Li recursion on Q = 1 - y * g(x) mod x^N, N a power of two; level i has N >> i rows and 2^i + 1 columns.
    template<typename T> vector<vector<T>> projectionLevels(const Poly<T> &g, int len) {
        vector<vector<T>> q;
        vector<T> cur(size_t(len) * 2);
        cur[0] = 1;
        for (int a = 0; a < len; ++a) { cur[2 * a + 1] = -g.coef(a); }
        q.push_back(cur);
        for (int rows = len, k = 1; rows > 1; rows /= 2, k *= 2) {
            int cols = k + 1;
            vector<T> qm = cur;
            for (int a = 1; a < rows; a += 2) { for (int b = 0; b < cols; ++b) { qm[size_t(a) * cols + b] = -qm[size_t(a) * cols + b]; } }
            vector<T> v = mul2d(cur, rows, cols, qm, rows, cols, 2 * cols - 1, rows), next(size_t(rows / 2) * (2 * k + 1));
            for (int j = 0; j < rows / 2; ++j) { std::copy(v.begin() + size_t(2 * j) * (2 * cols - 1), v.begin() + size_t(2 * j) * (2 * cols - 1) + 2 * k + 1, next.begin() + size_t(j) * (2 * k + 1)); }
            cur = std::move(next);
            q.push_back(cur);}
        return q;}
    inline vector<uint> convolutionFftMod(const vector<uint> &a, const vector<uint> &b, uint mod) {
        assert(mod >= 1 && mod < (1u << 30));
        if (a.empty() || b.empty()) { return {}; }
        assert(a.size() + b.size() - 1 <= size_t(1) << 20);
        assert(std::ranges::all_of(a, [&](uint x) { return x < mod; }) && std::ranges::all_of(b, [&](uint x) { return x < mod; }));
        return poly_detail::Fft::convolveMod(a.data(), int(a.size()), b.data(), int(b.size()), mod);}
    inline vector<lng> convolutionLng(const vector<lng> &a, const vector<lng> &b) {
        if (a.empty() || b.empty()) { return {}; }
        int n = int(a.size()), m = int(b.size());
        vector<lng> res(size_t(n + m) - 1);
        const poly_detail::Garner<3> &g = poly_detail::Garner<3>::get();
        poly_detail::convolveMulti<3>(n, m, [&](int i) { return a[i]; }, [&](int i) { return b[i]; }, [&](int i, const uint *c) {
            ulng x = c[0] + ulng(c[1]) * g.wrap[1] + ulng(c[2]) * g.wrap[2];
            if (c[2] > poly_detail::PRIMES[2] / 2) { x -= g.wrap[3]; }
            res[i] = lng(x);});
        return res;}
    inline vector<ulng> convolution2p64(const vector<ulng> &a, const vector<ulng> &b) {
        if (a.empty() || b.empty()) { return {}; }
        int n = int(a.size()), m = int(b.size());
        vector<ulng> res(size_t(n + m) - 1);
        const poly_detail::Garner<3> &g = poly_detail::Garner<3>::get();
        auto lo = [](ulng x) { return uint(x); };
        auto hi = [](ulng x) { return uint(x >> 32); };
        auto value = [&](const uint *c) { return c[0] + ulng(c[1]) * g.wrap[1] + ulng(c[2]) * g.wrap[2]; };
        poly_detail::convolveMulti<3>(n, m, [&](int i) { return lo(a[i]); }, [&](int i) { return lo(b[i]); }, [&](int i, const uint *c) { res[i] = value(c); });
        vector<uint> ah(n), bh(m), al(n), bl(m);
        for (int i = 0; i < n; ++i) { ah[i] = hi(a[i]); al[i] = lo(a[i]); }
        for (int i = 0; i < m; ++i) { bh[i] = hi(b[i]); bl[i] = lo(b[i]); }
        using poly_detail::PRIMES;
        auto cross = [&]<uint P>() {
            using M = ModInt<P>;
            vector<M> x(n), y(m), z(n), w(m);
            for (int i = 0; i < n; ++i) { x[i] = M(al[i]); z[i] = M(ah[i]); }
            for (int i = 0; i < m; ++i) { y[i] = M(bh[i]); w[i] = M(bl[i]); }
            vector<M> c = poly_detail::Ntt<M>::convolve(x, y), d = poly_detail::Ntt<M>::convolve(z, w);
            vector<uint> r(c.size());
            for (int i = 0; i < int(c.size()); ++i) { r[i] = (c[i] + d[i]).val(); }
            return r;};
        vector<uint> r0 = cross.template operator()<PRIMES[0]>(), r1 = cross.template operator()<PRIMES[1]>(), r2 = cross.template operator()<PRIMES[2]>();
        for (int i = 0; i < n + m - 1; ++i) { uint c[3] = {r0[i], r1[i], r2[i]}; g.digits(c); res[i] += value(c) << 32; }
        return res;}
    // T: O(n * k) with k nonzero terms, M: O(n); u^k * c for a unit u and an exponent k in T, the sparse recurrence behind powSparse and sqrtSparse.
    template<typename T> Poly<T> powSparseUnit(const Poly<T> &u, const T &k, const T &c, int n) {
        assert(n >= 0 && u.size() && u.v[0] != T(0));
        if (!n) { return {}; }
        vector<pair<int, T>> s;
        for (int i = 1; i < u.size(); ++i) { if (u.v[i] != T(0)) { s.emplace_back(i, u.v[i]); } }
        vector<T> inv = inverses<T>(n);
        T iu = T(1) / u.coef(0);
        Poly<T> g(n); g.v[0] = c;
        for (int i = 1; i < n; ++i) {
            T acc = 0;
            for (auto &[j, x] : s) { if (j > i) { break; } acc += (k * T(j) - T(i - j)) * x * g.v[i - j]; }
            g.v[i] = acc * inv[i] * iu;}
        return g;}
} // namespace poly_detail

// T: O(M(n)) per arithmetic operation with M the convolution cost, O(n) access; M: O(n) coefficients.
// v[i] is the coefficient of x^i; size() stored terms, deg() ignores trailing zeros (-1 for zero); "mod x^n" keeps n terms.
template<typename T>
struct Poly {
    vector<T> v;

    Poly() = default;
    Poly(vector<T> a) : v(std::move(a)) {}
    Poly(std::initializer_list<T> a) : v(a) {}
    explicit Poly(int n, const T &x = T(0)) : v(size_t(n), x) {}

    int size() const { return int(v.size()); }
    int deg() const { int d = size(); while (d > 0 && v[d - 1] == T(0)) { --d; } return d - 1; }
    T lead() const { int d = deg(); return d < 0 ? T(0) : v[d]; }
    T coef(int i) const { return 0 <= i && i < size() ? v[i] : T(0); }
    T &operator[](int i) { assert(0 <= i && i < size()); return v[i]; }
    const T &operator[](int i) const { assert(0 <= i && i < size()); return v[i]; }
    bool isNil() const { return deg() < 0; }

    Poly &trim() { v.resize(size_t(deg() + 1)); return *this; }
    Poly &resize(int n) { assert(n >= 0); v.resize(size_t(n)); return *this; }
    Poly &truncate(int n) { assert(n >= 0); if (size() > n) { v.resize(size_t(n)); } return *this; }
    Poly &normalize() { T l = lead(); assert(l != T(0)); trim(); return *this /= l; }
    Poly &reverse(int n = -1) { if (n >= 0) { resize(n); } std::reverse(v.begin(), v.end()); return *this; }

    Poly &operator+=(const Poly &b) {
        if (size() < b.size()) { v.resize(b.v.size()); }
        for (int i = 0; i < b.size(); ++i) { v[i] += b.v[i]; }
        return *this;}
    Poly &operator-=(const Poly &b) {
        if (size() < b.size()) { v.resize(b.v.size()); }
        for (int i = 0; i < b.size(); ++i) { v[i] -= b.v[i]; }
        return *this;}
    Poly &operator*=(const T &c) { for (T &x : v) { x *= c; } return *this; }
    Poly &operator/=(const T &c) {
        if constexpr (poly_detail::Field<T>) { return *this *= T(1) / c; }
        else { for (T &x : v) { x /= c; } return *this; }}
    Poly &operator*=(const Poly &b) { return *this = convolution(*this, b); }
    Poly &operator/=(const Poly &b) { return *this = divMod(*this, b).first; }
    Poly &operator%=(const Poly &b) { return *this = divMod(*this, b).second; }
    Poly &operator<<=(int k) { assert(k >= 0); v.insert(v.begin(), size_t(k), T(0)); return *this; }
    Poly &operator>>=(int k) { assert(k >= 0); v.erase(v.begin(), v.begin() + min(k, size())); return *this; }

    Poly operator-() const { Poly r = *this; for (T &x : r.v) { x = -x; } return r; }
    friend Poly operator+(Poly a, const Poly &b) { return a += b; }
    friend Poly operator-(Poly a, const Poly &b) { return a -= b; }
    friend Poly operator*(const Poly &a, const Poly &b) { return convolution(a, b); }
    friend Poly operator*(Poly a, const T &c) { return a *= c; }
    friend Poly operator*(const T &c, Poly a) { return a *= c; }
    friend Poly operator/(Poly a, const T &c) { return a /= c; }
    friend Poly operator/(const Poly &a, const Poly &b) { return divMod(a, b).first; }
    friend Poly operator%(const Poly &a, const Poly &b) { return divMod(a, b).second; }
    friend Poly operator<<(Poly a, int k) { return a <<= k; }
    friend Poly operator>>(Poly a, int k) { return a >>= k; }
    friend bool operator==(const Poly &a, const Poly &b) {
        for (int i = 0; i < max(a.size(), b.size()); ++i) { if (a.coef(i) != b.coef(i)) { return false; } }
        return true;}
    friend ostream &operator<<(ostream &os, const Poly &a) {
        for (int i = 0; i < a.size(); ++i) { os << (i ? " " : "") << a.v[i]; }
        return os;}

    friend T eval(const Poly &a, const T &x) {
        T r = 0;
        for (int i = a.size() - 1; i >= 0; --i) { r = r * x + a.v[i]; }
        return r;}
    friend Poly deriv(const Poly &a) {
        Poly r(max(a.size() - 1, 0));
        for (int i = 1; i < a.size(); ++i) { r.v[i - 1] = a.v[i] * T(i); }
        return r;}
    // Fields divide by 1..n through one inversion; rings divide exactly.
    friend Poly integ(const Poly &a, const T &c = T(0)) {
        Poly r(a.size() + 1); r.v[0] = c;
        if constexpr (poly_detail::Field<T>) {
            vector<T> inv = poly_detail::inverses<T>(a.size() + 1);
            for (int i = 0; i < a.size(); ++i) { r.v[i + 1] = a.v[i] * inv[i + 1]; }}
        else { for (int i = 0; i < a.size(); ++i) { r.v[i + 1] = a.v[i] / T(i + 1); } }
        return r;}

    // T: O(n * m), M: O(n + m).
    friend Poly schoolbook(const Poly &a, const Poly &b) {
        if (!a.size() || !b.size()) { return {}; }
        if constexpr (std::signed_integral<T>) {
            using U = std::make_unsigned_t<T>;
            vector<U> x(a.v.begin(), a.v.end()), y(b.v.begin(), b.v.end()), z(a.v.size() + b.v.size() - 1);
            poly_detail::schoolbook(x.data(), a.size(), y.data(), b.size(), z.data());
            return Poly(vector<T>(z.begin(), z.end()));}
        Poly r(a.size() + b.size() - 1);
        poly_detail::schoolbook(a.v.data(), a.size(), b.v.data(), b.size(), r.v.data());
        return r;}

    // T: O(n * m^0.585) for n >= m in blocks of m terms, M: O(n + m); any coefficient ring.
    friend Poly karatsuba(const Poly &a, const Poly &b) {
        if (!a.size() || !b.size()) { return {}; }
        if (a.size() < b.size()) { return karatsuba(b, a); }
        using U = typename poly_detail::Wrap<T>::type;
        int n = a.size(), m = b.size();
        vector<U> x(a.v.begin(), a.v.end()), y(b.v.begin(), b.v.end()), r(size_t(n + m) - 1, U(0)), xa(m), block(2 * size_t(m) - 1);
        for (int l = 0; l < n; l += m) {
            int len = min(m, n - l);
            fill(xa.begin(), xa.end(), U(0)); std::copy(x.begin() + l, x.begin() + l + len, xa.begin());
            fill(block.begin(), block.end(), U(0));
            poly_detail::karatsuba(xa.data(), y.data(), m, block.data());
            for (int i = 0; i < len + m - 1; ++i) { r[l + i] += block[i]; }}
        return Poly(vector<T>(r.begin(), r.end()));}

    // T: O(1), M: O(1); largest admissible transform length (2^k with 2^k | mod - 1, capped at 2^30), 0 for composite moduli.
    static int nttMaxLength() requires ModularInt<T> { return poly_detail::Ntt<T>::maxLength(); }

    // T: O(n * log(n)), M: O(n); n a power of two <= nttMaxLength; ntt leaves bit-reversed order, intt consumes it.
    static void ntt(vector<T> &a) requires ModularInt<T> { poly_detail::Ntt<T>::transform(a, false, false); }

    static void intt(vector<T> &a) requires ModularInt<T> { poly_detail::Ntt<T>::transform(a, true, false); }

    // The transposed maps: transposedNtt takes bit-reversed input to natural order; transposedIntt inverts it.
    static void transposedNtt(vector<T> &a) requires ModularInt<T> { poly_detail::Ntt<T>::transform(a, false, true); }

    static void transposedIntt(vector<T> &a) requires ModularInt<T> { poly_detail::Ntt<T>::transform(a, true, true); }

    friend void ntt(Poly &a) requires ModularInt<T> { ntt(a.v); }

    friend void intt(Poly &a) requires ModularInt<T> { intt(a.v); }

    // T: O(n * log(n)), M: O(n); double or complex<double> coefficients, relative error about log(n) * 2^-53 of the coefficient magnitude sum.
    friend Poly convolutionFft(const Poly &a, const Poly &b) requires poly_detail::Fftable<T> {
        if (!a.size() || !b.size()) { return {}; }
        if constexpr (std::is_same_v<T, double>) { return Poly(poly_detail::Fft::convolveReal(a.v.data(), a.size(), b.v.data(), b.size())); }
        else { return Poly(poly_detail::Fft::convolveComplex(a.v.data(), a.size(), b.v.data(), b.size())); }}

    friend Poly convolutionArbitraryMod(const Poly &a, const Poly &b) requires ModularInt<T> {
        if (!a.size() || !b.size()) { return {}; }
        int n = a.size(), m = b.size();
        Poly res(n + m - 1);
        auto fa = [&](int i) { return a.v[i].val(); };
        auto fb = [&](int i) { return b.v[i].val(); };
        auto run = [&]<int K>() {
            T pre[K]; pre[0] = 1;
            for (int k = 1; k < K; ++k) { pre[k] = pre[k - 1] * T(poly_detail::PRIMES[k - 1]); }
            poly_detail::convolveMulti<K>(n, m, fa, fb, [&](int i, const uint *c) {
                T x = 0;
                for (int k = 0; k < K; ++k) { x += T(c[k]) * pre[k]; }
                res.v[i] = x;});};
        if constexpr (sizeof(typename T::Word) == 4) { run.template operator()<3>(); }
        else { run.template operator()<6>(); }
        return res;}

    // T: O(n * log(n)) over NTT primes within the transform limit (O(n * m / L + (n + m) * log(L)) in blocks of L = nttMaxLength / 2 beyond it) and doubles, O(n^1.585) otherwise; M: O(n); dispatcher behind operator*.
    friend Poly convolution(const Poly &a, const Poly &b) {
        int n = a.size(), m = b.size();
        if (!n || !m) { return {}; }
        if (min(n, m) <= poly_detail::SCHOOLBOOK) { return schoolbook(a, b); }
        if constexpr (ModularInt<T>) {
            int len = poly_detail::ceilPow2(n + m - 1);
            int cap = poly_detail::Ntt<T>::maxLength();
            if (cap && len / poly_detail::LARGE_RATIO <= cap) { return Poly(poly_detail::Ntt<T>::convolve(a.v, b.v)); }
#ifndef __AVX2__
            if constexpr (sizeof(typename T::Word) == 4) {
                if (T::mod() < (1u << 30) && n + m - 1 <= poly_detail::FFT_MOD_MAX) {
                    vector<uint> x(n), y(m);
                    for (int i = 0; i < n; ++i) { x[i] = a.v[i].val(); }
                    for (int i = 0; i < m; ++i) { y[i] = b.v[i].val(); }
                    vector<uint> r = poly_detail::convolutionFftMod(x, y, T::mod());
                    Poly res(n + m - 1);
                    for (int i = 0; i < n + m - 1; ++i) { res.v[i] = T::init(r[i]); }
                    return res;}}
#endif
            return convolutionArbitraryMod(a, b);}
        else if constexpr (poly_detail::Fftable<T>) { return convolutionFft(a, b); }
        else if constexpr (std::is_same_v<T, ulng>) { return Poly(poly_detail::convolution2p64(a.v, b.v)); }
        else if constexpr (std::is_integral_v<T>) {
            vector<lng> x(a.v.begin(), a.v.end()), y(b.v.begin(), b.v.end()), r = poly_detail::convolutionLng(x, y);
            return Poly(vector<T>(r.begin(), r.end()));}
        else if constexpr (poly_detail::BigInt<T>) { return Poly(poly_detail::kronecker(a.v, b.v)); }
        else { return karatsuba(a, b); }}

    // T: O(n * log(n)), M: O(n); a with |a| <= n and b with |b| <= n multiplied modulo x^n - 1 (NTT-friendly power of two n) or by folding.
    friend Poly cyclic(const Poly &a, const Poly &b, int n) {
        assert(n > 0 && a.size() <= n && b.size() <= n);
        if (std::has_single_bit(uint(n)) && poly_detail::fastSpec<T>(n) && min(a.size(), b.size()) > poly_detail::SCHOOLBOOK) {
            vector<T> x = poly_detail::spec(a.v, n), y = poly_detail::spec(b.v, n);
            poly_detail::dot(x, y, n); return Poly(poly_detail::unspec(x, n));}
        Poly c = a * b, r(n);
        for (int i = 0; i < c.size(); ++i) { r.v[i % n] += c.v[i]; }
        return r;}

    // T: O(n * log(n)), M: O(n); product modulo x^n + 1, twisted by a 2n-th root of unity when one exists.
    friend Poly negacyclic(const Poly &a, const Poly &b, int n) {
        assert(n > 0 && a.size() <= n && b.size() <= n);
        if constexpr (ModularInt<T>) {
            if (std::has_single_bit(uint(n)) && poly_detail::fastSpec<T>(2 * n) && min(a.size(), b.size()) > poly_detail::SCHOOLBOOK) {
                poly_detail::Ntt<T>::ensure(2 * n);
                const auto &c = poly_detail::Ntt<T>::cache();
                T psi = c.mont ? T::init(c.m.to(c.w[n + 1])) : c.tw[n + 1], ipsi = T(1) / psi, p = 1, ip = 1;
                Poly x(n), y(n);
                for (int i = 0; i < n; ++i, p *= psi) { x.v[i] = a.coef(i) * p; y.v[i] = b.coef(i) * p; }
                Poly r = cyclic(x, y, n);
                for (int i = 0; i < n; ++i, ip *= ipsi) { r.v[i] *= ip; }
                return r;}}
        Poly c = a * b, r(n);
        for (int i = 0; i < c.size(); ++i) { if ((i / n) & 1) { r.v[i % n] -= c.v[i]; } else { r.v[i % n] += c.v[i]; } }
        return r;}

    // T: O(M(n)), M: O(n); a * a with one forward transform.
    friend Poly square(const Poly &a) {
        if constexpr (ModularInt<T>) {
            if (a.size() > poly_detail::SCHOOLBOOK && poly_detail::Ntt<T>::usable(poly_detail::ceilPow2(2 * a.size() - 1))) { return Poly(poly_detail::Ntt<T>::convolve(a.v, a.v)); }}
        return a * a;}

    // T: O(M(n)), M: O(n); (a * b) mod x^n from the inputs truncated to n terms.
    friend Poly truncatedMul(const Poly &a, const Poly &b, int n) {
        Poly x = a, y = b;
        return (x.truncate(n) * y.truncate(n)).truncate(n).resize(min(n, x.size() && y.size() ? n : 0));}

    // T: O(M(n + m)), M: O(n + m); coefficients of x^k, k >= n, of a * b.
    friend Poly mulHigh(const Poly &a, const Poly &b, int n) { return (a * b) >> n; }

    // T: O(M(n)), M: O(n); |a| >= |b|: coefficients [|b| - 1, |a|) of a * b through one cyclic product of length ceilPow2(|a|).
    friend Poly middleProduct(const Poly &a, const Poly &b) {
        int n = a.size(), m = b.size(); assert(m >= 1 && n >= m);
        Poly c = cyclic(a, b, poly_detail::ceilPow2(n));
        return Poly(vector<T>(c.v.begin() + (m - 1), c.v.begin() + n));}

    // T: O(n * k) for k sparse terms, M: O(n + deg); b given as (exponent, coefficient) pairs.
    friend Poly sparseMul(const Poly &a, const vector<pair<int, T>> &b) {
        if (!a.size() || b.empty()) { return {}; }
        int d = 0;
        for (auto &[e, c] : b) { assert(e >= 0); d = max(d, e); }
        Poly r(a.size() + d);
        for (auto &[e, c] : b) { for (int i = 0; i < a.size(); ++i) { r.v[i + e] += a.v[i] * c; } }
        return r;}

    // T: O(M(r * c)), M: O(r * c); row-major r1 x c1 and r2 x c2 arrays, full 2-D product (r1 + r2 - 1) x (c1 + c2 - 1) by Kronecker packing.
    static vector<T> convolution2d(const vector<T> &a, int r1, int c1, const vector<T> &b, int r2, int c2) {
        assert(int(a.size()) == r1 * c1 && int(b.size()) == r2 * c2);
        if (a.empty() || b.empty()) { return {}; }
        int w = c1 + c2 - 1, rows = r1 + r2 - 1;
        Poly x(r1 * w), y(r2 * w);
        for (int i = 0; i < r1; ++i) { for (int j = 0; j < c1; ++j) { x.v[i * w + j] = a[i * c1 + j]; } }
        for (int i = 0; i < r2; ++i) { for (int j = 0; j < c2; ++j) { y.v[i * w + j] = b[i * c2 + j]; } }
        Poly z = x * y; z.resize(rows * w);
        return z.v;}

    // T: O(k * M(N) + k^2 * N) for k variables and N = product of the shape (k^2 products without an NTT), M: O(k * N); product truncated to the shape.
    static vector<T> multivariate(const vector<T> &a, const vector<T> &b, const vector<int> &shape) {
        int k = int(shape.size()), total = 1;
        for (int s : shape) { assert(s > 0); total *= s; }
        assert(int(a.size()) == total && int(b.size()) == total);
        if (k <= 1) { Poly c = Poly(a) * Poly(b); c.resize(total); return c.v; }
        vector<int> chi(total);
        for (int i = 0; i < total; ++i) {
            int s = 1, c = 0;
            for (int j = 0; j + 1 < k; ++j) { s *= shape[j]; c += i / s; }
            chi[i] = c % k;}
        int len = poly_detail::ceilPow2(2 * total - 1);
        vector<vector<T>> fa(k), fb(k), fc(k);
        for (int c = 0; c < k; ++c) {
            vector<T> x(total), y(total);
            for (int i = 0; i < total; ++i) { if (chi[i] == c) { x[i] = a[i]; y[i] = b[i]; } }
            fa[c] = poly_detail::spec(x, len); fb[c] = poly_detail::spec(y, len); fc[c].assign(len, T(0));}
        for (int i = 0; i < k; ++i) {
            for (int j = 0; j < k; ++j) {
                vector<T> t = fa[i]; poly_detail::dot(t, fb[j], len);
                vector<T> &d = fc[(i + j) % k];
                for (int p = 0; p < len; ++p) { d[p] += t[p]; }}}
        vector<T> res(total);
        for (int c = 0; c < k; ++c) {
            vector<T> t = poly_detail::unspec(fc[c], len);
            for (int i = 0; i < total; ++i) { if (chi[i] == c) { res[i] = t[i]; } }}
        return res;}

    // T: O(M(n)), M: O(n); fields use Newton division, rings schoolbook with exact coefficient division; b nonzero.
    friend pair<Poly, Poly> divMod(const Poly &a, const Poly &b) {
        int db = b.deg(), da = a.deg(); assert(db >= 0);
        if (db < 0 || da < db) { return {Poly(), Poly(a).trim()}; }
        int n = da - db + 1;
        if constexpr (poly_detail::Field<T>) {
            if (min(n, db) > poly_detail::SCHOOLBOOK) {
                Poly ra = Poly(a).trim().reverse().truncate(n), rb = Poly(b).trim().reverse().truncate(n);
                Poly q = truncatedMul(ra, inv(rb, n), n).resize(n).reverse();
                Poly r = (Poly(a).truncate(db) - truncatedMul(q, b, db)).resize(db).trim();
                return {q, r};}}
        Poly r = a, q(n); r.trim();
        T l = b.v[db];
        for (int i = da; i >= db; --i) {
            T c = r.v[i];
            if constexpr (poly_detail::Field<T>) { c *= T(1) / l; }
            else { c /= l; }
            q.v[i - db] = c;
            if (c != T(0)) { for (int j = 0; j <= db; ++j) { r.v[i - db + j] -= c * b.v[j]; } }}
        r.resize(db); r.trim();
        return {q, r};}

    // T: O(M(n)), M: O(n); b monic, so no coefficient division happens and every ring is allowed.
    friend Poly monicDiv(const Poly &a, const Poly &b) {
        int db = b.deg(), da = a.deg(); assert(db >= 0 && b.v[db] == T(1));
        if (db < 0 || da < db) { return {}; }
        int n = da - db + 1;
        if constexpr (poly_detail::Field<T>) { return divMod(a, b).first; }
        Poly r = a, q(n); r.trim();
        for (int i = da; i >= db; --i) {
            T c = r.v[i]; q.v[i - db] = c;
            if (c != T(0)) { for (int j = 0; j <= db; ++j) { r.v[i - db + j] -= c * b.v[j]; } }}
        return q;}

    // T: O(n * (n - m + 1)) for n = deg a, m = deg b, M: O(n); fraction-free: lead(b)^(n - m + 1) * a = q * b + r with deg r < m.
    friend pair<Poly, Poly> pseudoDiv(const Poly &a, const Poly &b) {
        int db = b.deg(), da = a.deg(); assert(db >= 0);
        if (db < 0 || da < db) { return {Poly(), Poly(a).trim()}; }
        T l = b.v[db];
        Poly r = a, q(da - db + 1); r.trim();
        for (int i = da; i >= db; --i) {
            T c = r.v[i];
            for (int j = 0; j <= i; ++j) { r.v[j] *= l; }
            for (int j = i - db + 1; j <= da - db; ++j) { q.v[j] *= l; }
            q.v[i - db] = c;
            for (int j = 0; j <= db; ++j) { r.v[i - db + j] -= c * b.v[j]; }}
        r.resize(db); r.trim();
        return {q, r};}

    // T: O(n * (n - m + 1)), M: O(n); the remainder of pseudoDiv.
    friend Poly pseudoRem(const Poly &a, const Poly &b) { return pseudoDiv(a, b).second; }

    // T: O(M(n)), M: O(n); true with the exact quotient in q when b divides a (q unchanged otherwise).
    friend bool tryDivide(const Poly &a, const Poly &b, Poly &q) {
        if (b.isNil()) { return false; }
        auto [d, r] = divMod(a, b);
        if (!r.isNil()) { return false; }
        if constexpr (poly_detail::Ring<T>) { if (!(d * b == a)) { return false; } }
        q = d; return true;}

    // T: O(n), M: O(n); quotient of a by x - c with the remainder a(c) as the second value.
    friend pair<Poly, T> divRoot(const Poly &a, const T &c) {
        int d = a.deg();
        if (d < 1) { return {Poly(), d < 0 ? T(0) : a.v[0]}; }
        Poly q(d); T r = a.v[d];
        for (int i = d - 1; i >= 0; --i) { q.v[i] = r; r = r * c + a.v[i]; }
        return {q, r};}

    // T: O(M(n)), M: O(n); a / b mod x^n with b[0] invertible.
    friend Poly divSeries(const Poly &a, const Poly &b, int n) { return truncatedMul(a, inv(b, n), n).resize(n); }

    // T: O(M(n)), M: O(n); Newton iteration with five transforms of length 2k per doubling; a[0] must be invertible.
    friend Poly inv(const Poly &a, int n) {
        assert(n >= 0);
        if (!n) { return {}; }
        assert(a.size() && a.v[0] != T(0));
        Poly g = {T(1) / a.coef(0)};
        for (int k = 1; k < n; k *= 2) {
            int len = 2 * k;
            vector<T> fa = poly_detail::spec(a.v, len), fg = poly_detail::spec(g.v, len);
            poly_detail::dot(fa, fg, len);
            vector<T> h = poly_detail::unspec(fa, len);
            fill(h.begin(), h.begin() + k, T(0));
            vector<T> fh = poly_detail::spec(h, len);
            poly_detail::dot(fh, fg, len); h = poly_detail::unspec(fh, len);
            g.v.resize(len);
            for (int i = k; i < len; ++i) { g.v[i] = -h[i]; }}
        g.resize(n); return g;}

    // T: O(M(n)), M: O(n); a[0] must be 1.
    friend Poly log(const Poly &a, int n) {
        static_assert(poly_detail::Field<T>);
        assert(n >= 0 && a.size() && a.v[0] == T(1));
        if (n <= 1) { return Poly(n); }
        return integ(truncatedMul(deriv(a), inv(a, n - 1), n - 1)).resize(n);}

    // T: O(M(n)), M: O(n); Newton with the inverse lifted once per doubling (five transforms of length m, eight of 2m); a[0] must be 0.
    friend Poly exp(const Poly &a, int n) {
        static_assert(poly_detail::Field<T>);
        assert(n >= 0 && (!a.size() || a.v[0] == T(0)));
        if (!n) { return {}; }
        Poly da = deriv(a);
        vector<T> f = {T(1)}, g = {T(1)};
        for (int m = 1; m < n; m *= 2) {
            if (m > 1) {
                int h = m / 2;
                vector<T> fa1 = poly_detail::spec(f, m), fg1 = poly_detail::spec(g, m), t = fa1;
                poly_detail::dot(t, fg1, m); t = poly_detail::unspec(t, m);
                vector<T> fe = poly_detail::spec(poly_detail::slice(t, h, m), m);
                poly_detail::dot(fe, fg1, m); vector<T> u = poly_detail::unspec(fe, m);
                g.resize(m);
                for (int i = 0; i < h; ++i) { g[h + i] = -u[i]; }}
            int len = 2 * m;
            vector<T> fa = poly_detail::spec(f, len), fd = poly_detail::spec(poly_detail::slice(da.v, 0, len - 1), len);
            poly_detail::dot(fd, fa, len); vector<T> z = poly_detail::unspec(fd, len);
            vector<T> zh(m);
            for (int i = 0; i < m; ++i) { zh[i] = -z[m - 1 + i]; }
            vector<T> fg = poly_detail::spec(g, len), fz = poly_detail::spec(zh, len);
            poly_detail::dot(fz, fg, len); vector<T> q = poly_detail::unspec(fz, len);
            vector<T> inv = poly_detail::inverses<T>(len), r(m);
            for (int i = 0; i < m; ++i) { r[i] = q[i] * inv[m + i]; }
            vector<T> fr = poly_detail::spec(r, len);
            poly_detail::dot(fr, fa, len); vector<T> w = poly_detail::unspec(fr, len);
            f.resize(len);
            for (int i = 0; i < m; ++i) { f[m + i] = -w[i]; }}
        f.resize(n); return Poly(f);}

    // T: O(M(n)), M: O(n); exponent as an element of T (integers, rationals in fields); u[0] = 1 required, valuation handled by the callers.
    friend Poly powUnit(const Poly &u, const T &k, int n) {
        static_assert(poly_detail::Field<T>);
        assert(n >= 0 && u.size() && u.v[0] == T(1));
        if (!n) { return {}; }
        return exp(log(u, n) * k, n);}

    // T: O(M(n)) for fields, O(M(n) * log(k)) for rings (k > 0); zero when v * k >= n for the valuation v; negative k inverts (needs v = 0).
    friend Poly pow(const Poly &a, lng k, int n) {
        assert(n >= 0);
        if (!n) { return {}; }
        if (k == 0) { Poly r(n); r.v[0] = 1; return r; }
        int v = poly_detail::valuation(a);
        assert(k > 0 || (v == 0 && a.size()));
        if (v == a.size() || (v && k >= (n + v - 1) / v)) { return Poly(n); }
        int shift = int(v * k);
        if (k == 1) { return (Poly(a).truncate(n)).resize(n); }
        if constexpr (poly_detail::Ring<T>) {
            assert(k > 0);
            Poly r = {T(1)}, b = Poly(a).truncate(n);
            for (ulng e = ulng(k); e; e >>= 1, b = truncatedMul(b, b, n)) { if (e & 1) { r = truncatedMul(r, b, n); } }
            return r.resize(n);}
        else {
            T c = a.v[v], ck = k > 0 ? poly_detail::power(c, ulng(k)) : T(1) / poly_detail::power(c, ulng(0) - ulng(k));
            Poly r = powUnit((a >> v) / c, T(k), n - shift) * ck;
            return (r << shift).resize(n);}}

    // T: O(M(n)), M: O(n); a^(p/q) for a with a[0] = 1 after removing a valuation divisible by q; q must be invertible in T.
    friend Poly powRational(const Poly &a, lng p, lng q, int n) {
        static_assert(poly_detail::Field<T>);
        assert(q > 0 && n >= 0);
        if (!n) { return {}; }
        int v = poly_detail::valuation(a);
        if (v == a.size()) { assert(p > 0); return Poly(n); }
        lll s = lll(v) * p;
        assert(s % q == 0 && a.v[v] == T(1));
        s /= q;
        if (s >= n) { return Poly(n); }
        assert(s >= 0);
        int shift = int(s);
        Poly r = powUnit(a >> v, T(p) / T(q), n - shift);
        return (r << shift).resize(n);}

    // T: O(M(n)), M: O(n); k-th root with u[0] = 1 and valuation divisible by k.
    friend Poly kthRoot(const Poly &a, lng k, int n) { return powRational(a, 1, k, n); }

    // T: O(M(n)), M: O(n); false when the valuation is odd or the leading constant has no square root; the root with the smaller canonical constant.
    friend bool trySqrt(const Poly &a, int n, Poly &out) {
        static_assert(poly_detail::Field<T>);
        assert(n >= 0);
        int v = poly_detail::valuation(a);
        if (!n || v == a.size()) { out = Poly(n); return true; }
        if (v & 1) { return false; }
        T c;
        if (!poly_detail::sqrtConst(a.v[v], c)) { return false; }
        int m2 = n - v / 2;
        if (m2 <= 0) { out = Poly(n); return true; }
        Poly u = a >> v;
        T half = T(1) / T(2);
        vector<T> g = {c}, h = {T(1) / c};
        for (int m = 1; m < m2; m *= 2) {
            int len = 2 * m;
            vector<T> fg = poly_detail::spec(g, len), q = fg;
            poly_detail::dot(q, fg, len); q = poly_detail::unspec(q, len);
            vector<T> d(m);
            for (int i = 0; i < m; ++i) { d[i] = u.coef(m + i) - q[m + i]; }
            vector<T> fh = poly_detail::spec(h, len), fd = poly_detail::spec(d, len);
            poly_detail::dot(fd, fh, len); vector<T> w = poly_detail::unspec(fd, len);
            g.resize(len);
            for (int i = 0; i < m; ++i) { g[m + i] = w[i] * half; }
            if (len >= m2) { break; }
            vector<T> fg2 = poly_detail::spec(g, len), t = fg2;
            poly_detail::dot(t, fh, len); t = poly_detail::unspec(t, len);
            vector<T> fe = poly_detail::spec(poly_detail::slice(t, m, len), len);
            poly_detail::dot(fe, fh, len); vector<T> y = poly_detail::unspec(fe, len);
            h.resize(len);
            for (int i = 0; i < m; ++i) { h[m + i] = -y[i]; }}
        g.resize(m2);
        out = (Poly(g) << (v / 2)).resize(n); return true;}

    // T: O(M(n)), M: O(n); empty when no square root exists (indistinguishable from n = 0, use trySqrt there).
    friend Poly sqrt(const Poly &a, int n) { Poly r; return trySqrt(a, n, r) ? r : Poly(); }

    // T: O(n * k) for k nonzero terms of a, M: O(n); the sparse recurrences share the dense contracts.
    friend Poly invSparse(const Poly &a, int n) {
        assert(n >= 0 && a.size() && a.v[0] != T(0));
        vector<pair<int, T>> s;
        for (int i = 1; i < a.size(); ++i) { if (a.v[i] != T(0)) { s.emplace_back(i, a.v[i]); } }
        Poly g(n); T c = T(1) / a.coef(0);
        if (n) { g.v[0] = c; }
        for (int i = 1; i < n; ++i) {
            T acc = 0;
            for (auto &[j, x] : s) { if (j > i) { break; } acc += x * g.v[i - j]; }
            g.v[i] = -acc * c;}
        return g;}

    // T: O(n * k) with k nonzero terms of a, M: O(n); a[0] must be 1.
    friend Poly logSparse(const Poly &a, int n) {
        static_assert(poly_detail::Field<T>);
        assert(n >= 0 && a.size() && a.v[0] == T(1));
        if (n <= 1) { return Poly(n); }
        vector<pair<int, T>> s;
        for (int i = 1; i < a.size(); ++i) { if (a.v[i] != T(0)) { s.emplace_back(i, a.v[i]); } }
        Poly h(n - 1);
        for (int i = 0; i < n - 1; ++i) {
            T acc = a.coef(i + 1) * T(i + 1);
            for (auto &[j, x] : s) { if (j > i) { break; } acc -= x * h.v[i - j]; }
            h.v[i] = acc;}
        return integ(h).resize(n);}

    // T: O(n * k) with k nonzero terms of a, M: O(n); a[0] must be 0.
    friend Poly expSparse(const Poly &a, int n) {
        static_assert(poly_detail::Field<T>);
        assert(n >= 0 && (!a.size() || a.v[0] == T(0)));
        if (!n) { return {}; }
        vector<pair<int, T>> s;
        for (int i = 1; i < a.size(); ++i) { if (a.v[i] != T(0)) { s.emplace_back(i, a.v[i] * T(i)); } }
        vector<T> inv = poly_detail::inverses<T>(n);
        Poly g(n); g.v[0] = 1;
        for (int i = 1; i < n; ++i) {
            T acc = 0;
            for (auto &[j, x] : s) { if (j > i) { break; } acc += x * g.v[i - j]; }
            g.v[i] = acc * inv[i];}
        return g;}

    // T: O(n * k') with k' nonzero terms of a, M: O(n); k >= 0, valuation handled as in pow.
    friend Poly powSparse(const Poly &a, lng k, int n) {
        static_assert(poly_detail::Field<T>);
        assert(n >= 0 && k >= 0);
        if (!n) { return {}; }
        if (k == 0) { Poly r(n); r.v[0] = 1; return r; }
        int v = poly_detail::valuation(a);
        if (v == a.size() || (v && k >= (n + v - 1) / v)) { return Poly(n); }
        int shift = int(v * k);
        Poly r = poly_detail::powSparseUnit(a >> v, T(k), poly_detail::power(a.v[v], ulng(k)), n - shift);
        return (r << shift).resize(n);}

    // T: O(n * k) with k nonzero terms of a, M: O(n); empty when no square root exists, as sqrt.
    friend Poly sqrtSparse(const Poly &a, int n) {
        static_assert(poly_detail::Field<T>);
        assert(n >= 0);
        int v = poly_detail::valuation(a);
        if (!n || v == a.size()) { return Poly(n); }
        T c;
        if ((v & 1) || !poly_detail::sqrtConst(a.v[v], c)) { return {}; }
        if (n - v / 2 <= 0) { return Poly(n); }
        return (poly_detail::powSparseUnit(a >> v, T(1) / T(2), c, n - v / 2) << (v / 2)).resize(n);}

    // T: O(M(n)), M: O(n); (cos a, sin a) mod x^n by Newton over T[i] without needing sqrt(-1); a[0] must be 0.
    friend pair<Poly, Poly> circular(const Poly &a, int n) {
        static_assert(poly_detail::Field<T>);
        assert(n >= 0 && (!a.size() || a.v[0] == T(0)));
        if (!n) { return {}; }
        vector<T> c = {T(1)}, s = {T(0)};
        T half = T(1) / T(2);
        for (int m = 1; m < n; m *= 2) {
            int len = 2 * m;
            vector<T> fc = poly_detail::spec(c, len), fs = poly_detail::spec(s, len), fdc = poly_detail::spec(deriv(Poly(c)).v, len), fds = poly_detail::spec(deriv(Poly(s)).v, len);
            vector<T> u = fds, w = fdc, q = fc, q2 = fs;
            poly_detail::dot(u, fc, len); poly_detail::dot(w, fs, len); poly_detail::dot(q, fc, len); poly_detail::dot(q2, fs, len);
            for (int i = 0; i < len; ++i) { u[i] -= w[i]; q[i] += q2[i]; }
            u = poly_detail::unspec(u, len); q = poly_detail::unspec(q, len);
            vector<T> fe = poly_detail::spec(poly_detail::slice(q, m, len), len), fu = poly_detail::spec(poly_detail::slice(u, 0, m - 1), len), z = fe;
            poly_detail::dot(z, fu, len); z = poly_detail::unspec(z, len);
            for (int i = m; i < len - 1; ++i) { u[i] -= z[i - m]; }
            u.resize(len - 1);
            Poly t = Poly(a).truncate(len).resize(len) - integ(Poly(u));
            vector<T> ft = poly_detail::spec(poly_detail::slice(t.v, m, len), len), x = fs, y = fc, ce = fc, se = fs;
            poly_detail::dot(x, ft, len); poly_detail::dot(y, ft, len); poly_detail::dot(ce, fe, len); poly_detail::dot(se, fe, len);
            for (int i = 0; i < len; ++i) { x[i] += ce[i] * half; y[i] -= se[i] * half; }
            x = poly_detail::unspec(x, len); y = poly_detail::unspec(y, len);
            c.resize(len); s.resize(len);
            for (int i = 0; i < m; ++i) { c[m + i] = -x[i]; s[m + i] = y[i]; }}
        c.resize(n); s.resize(n);
        return {Poly(c), Poly(s)};}

    // T: O(M(n)), M: O(n) for each of the twelve series below; a[0] must be 0.
    friend Poly sin(const Poly &a, int n) { return circular(a, n).second; }

    friend Poly cos(const Poly &a, int n) { return circular(a, n).first; }

    friend Poly tan(const Poly &a, int n) { auto [c, s] = circular(a, n); return divSeries(s, c, n); }

    friend Poly asin(const Poly &a, int n) {
        static_assert(poly_detail::Field<T>);
        assert(n >= 0 && (!a.size() || a.v[0] == T(0)));
        if (n <= 1) { return Poly(n); }
        Poly one(n - 1); one.v[0] = 1;
        return integ(truncatedMul(deriv(a), inv(sqrt(one - truncatedMul(a, a, n - 1), n - 1), n - 1), n - 1)).resize(n);}

    friend Poly atan(const Poly &a, int n) {
        static_assert(poly_detail::Field<T>);
        assert(n >= 0 && (!a.size() || a.v[0] == T(0)));
        if (n <= 1) { return Poly(n); }
        Poly one(n - 1); one.v[0] = 1;
        return integ(divSeries(deriv(a), one + truncatedMul(a, a, n - 1), n - 1)).resize(n);}

    friend Poly sinh(const Poly &a, int n) { Poly e = exp(a, n); return (e - inv(e, n)) * (T(1) / T(2)); }

    friend Poly cosh(const Poly &a, int n) { Poly e = exp(a, n); return (e + inv(e, n)) * (T(1) / T(2)); }

    friend Poly tanh(const Poly &a, int n) { Poly e = exp(a, n), f = inv(e, n); return divSeries(e - f, e + f, n); }

    friend Poly asinh(const Poly &a, int n) {
        static_assert(poly_detail::Field<T>);
        assert(n >= 0 && (!a.size() || a.v[0] == T(0)));
        if (n <= 1) { return Poly(n); }
        Poly one(n - 1); one.v[0] = 1;
        return integ(truncatedMul(deriv(a), inv(sqrt(one + truncatedMul(a, a, n - 1), n - 1), n - 1), n - 1)).resize(n);}

    friend Poly atanh(const Poly &a, int n) {
        static_assert(poly_detail::Field<T>);
        assert(n >= 0 && (!a.size() || a.v[0] == T(0)));
        if (n <= 1) { return Poly(n); }
        Poly one(n - 1); one.v[0] = 1;
        return integ(divSeries(deriv(a), one - truncatedMul(a, a, n - 1), n - 1)).resize(n);}

    // T: O(n * log(n)), M: O(n); a holds the bit-reversed NTT of a polynomial with fewer than n terms and becomes its 2n-point NTT.
    static void nttDoubling(vector<T> &a) requires ModularInt<T> {
        int n = int(a.size()); assert(n >= 1 && poly_detail::Ntt<T>::usable(2 * n));
        vector<T> b = a; intt(b);
        poly_detail::Ntt<T>::ensure(2 * n);
        const auto &c = poly_detail::Ntt<T>::cache();
        T psi = n == 1 ? T(0) : c.mont ? T::init(c.m.to(c.w[n + 1])) : c.tw[n + 1], p = 1;
        for (int i = 0; i < n; ++i, p *= psi) { b[i] *= p; }
        ntt(b); a.insert(a.end(), b.begin(), b.end());}

    // T: O((n + m)^2 / L + (n + m) * log(L)) with L = nttMaxLength / 2, M: O(n + m); explicit block product.
    friend Poly convolutionLarge(const Poly &a, const Poly &b) requires ModularInt<T> {
        if (!a.size() || !b.size()) { return {}; }
        return Poly(poly_detail::Ntt<T>::convolveLarge(a.v, b.v));}

    // T: O(M(n)) in each prime length p via Rader and O(n * p) per small factor, M: O(n); w a primitive n-th root of unity; inverse divides by n.
    static vector<T> mixedRadix(const vector<T> &a, const T &w, bool inverse = false) {
        int n = int(a.size());
        if (!n) { return {}; }
        vector<T> r = poly_detail::dft(a, inverse ? T(1) / w : w);
        if (inverse) { T s = T(1) / T(n); for (T &x : r) { x *= s; } }
        return r;}

    // T: O(M(n)), M: O(n); prime length n through mixedRadix.
    static vector<T> rader(const vector<T> &a, const T &w, bool inverse = false) { return mixedRadix(a, w, inverse); }

    // T: O(M(n + m)), M: O(n + m); a(w^k) for k in [0, m) for any w, through evalMultiGeometric.
    static vector<T> bluestein(const vector<T> &a, const T &w, int m) { return evalMultiGeometric(Poly(a), T(1), w, m); }

    // T: O(N * sum(n_i) + k * N) for N = product of the shape, M: O(N); product in T[x_1..x_k] / (x_i^{n_i} - 1) with n_i | mod - 1, prime 32-bit modulus.
    static vector<T> multivariateCyclic(const vector<T> &a, const vector<T> &b, const vector<int> &shape) requires ModularInt<T> {
        static_assert(sizeof(typename T::Word) == 4);
        int k = int(shape.size()), total = 1;
        for (int s : shape) { assert(s > 0 && (T::mod() - 1) % uint(s) == 0); total *= s; }
        assert(int(a.size()) == total && int(b.size()) == total && T::is_prime);
        T g = T(poly_detail::primitiveRootMod(T::mod()));
        auto axis = [&](vector<T> &v, bool inverse) {
            for (int i = 0, stride = 1; i < k; stride *= shape[i], ++i) {
                int s = shape[i]; T w = pow(g, (T::mod() - 1) / uint(s));
                if (inverse) { w = T(1) / w; }
                vector<T> line(s);
                for (int base = 0; base < total; base += stride * s) {
                    for (int o = 0; o < stride; ++o) {
                        for (int t = 0; t < s; ++t) { line[t] = v[base + o + t * stride]; }
                        vector<T> y = poly_detail::dft(line, w);
                        for (int t = 0; t < s; ++t) { v[base + o + t * stride] = y[t]; }}}}};
        vector<T> x = a, y = b;
        axis(x, false); axis(y, false);
        T inv = T(1) / T(total);
        for (int i = 0; i < total; ++i) { x[i] *= y[i] * inv; }
        axis(x, true);
        return x;}

    // T: O(M(n) * log(n)) for n = total size, M: O(n); product of all polynomials by merging the two shortest first.
    friend Poly productOfSequence(const vector<Poly> &f) {
        if (f.empty()) { return Poly{T(1)}; }
        std::priority_queue<pair<int, int>, vector<pair<int, int>>, std::greater<>> q;
        vector<Poly> w = f;
        for (int i = 0; i < int(w.size()); ++i) { q.emplace(w[i].size(), i); }
        while (q.size() > 1) {
            int i = q.top().second; q.pop();
            int j = q.top().second; q.pop();
            w[i] = w[i] * w[j]; w[j] = Poly();
            q.emplace(w[i].size(), i);}
        return w[q.top().second];}

    // T: O(M(n) * log(n)) with n = |a| + |xs|, M: O(n * log(n)); a(x_i) for every point through the transposed subproduct tree.
    friend vector<T> evalMulti(const Poly &a, const vector<T> &xs) {
        if (xs.size() <= 32 || a.size() <= 32 || poly_detail::Approx<T>) {
            vector<T> res(xs.size());
            for (int i = 0; i < int(xs.size()); ++i) { res[i] = eval(a, xs[i]); }
            return res;}
        return poly_detail::Tree<T>(xs).eval(a);}

    // T: O(M(n) * log(n)), M: O(n * log(n)); the polynomial of degree < n through distinct points (asserted).
    static Poly interpolate(const vector<T> &xs, const vector<T> &ys) {
        int n = int(xs.size()); assert(int(ys.size()) == n);
        if (!n) { return {}; }
        if constexpr (poly_detail::Approx<T>) {
            vector<T> d = ys;
            for (int k = 1; k < n; ++k) { for (int i = n - 1; i >= k; --i) { d[i] = (d[i] - d[i - 1]) / (xs[i] - xs[i - k]); } }
            Poly res = {d[n - 1]};
            for (int i = n - 2; i >= 0; --i) { res = (res << 1) - res * xs[i]; res.v[0] += d[i]; }
            return res.resize(n);}
        poly_detail::Tree<T> tr(xs);
        vector<T> d = tr.eval(deriv(tr.t[1]));
        for (int i = 0; i < n; ++i) { assert(d[i] != T(0)); d[i] = ys[i] / d[i]; }
        return tr.combine(d).resize(n);}

    // T: O(M(n + m)), M: O(n + m); a(c * r^i) for i in [0, m) by the chirp-z factorization i * j = C(i + j, 2) - C(i, 2) - C(j, 2).
    friend vector<T> evalMultiGeometric(const Poly &a, const T &c, const T &r, int m) {
        assert(m >= 0);
        int n = a.size();
        vector<T> res(m);
        if (!m || !n) { return res; }
        if (r == T(0) || c == T(0)) {
            res[0] = eval(a, c); fill(res.begin() + 1, res.end(), a.v[0]);
            return res;}
        T ir = T(1) / r, rp = 1, irp = 1, cp = 1, rk = 1, irk = 1;
        Poly u(n), v(n + m - 1);
        for (int j = 0; j < n; ++j, cp *= c, irp *= irk, irk *= ir) { u.v[n - 1 - j] = a.v[j] * cp * irp; }
        for (int k = 0; k < n + m - 1; ++k, rp *= rk, rk *= r) { v.v[k] = rp; }
        Poly w = middleProduct(v, u);
        irp = 1; irk = 1;
        for (int i = 0; i < m; ++i, irp *= irk, irk *= ir) { res[i] = w.v[i] * irp; }
        return res;}

    // T: O(n) by the q-binomial theorem when r^k != 1 for 0 < k < n, else O(M(n) * log(n)); prod (x - c * r^i) over i < n.
    static Poly productOfGeometric(const T &c, const T &r, int n) {
        assert(n >= 0);
        vector<T> q(n + 1, T(1)), rp(n + 1, T(1));
        bool regular = true;
        for (int k = 1; k <= n; ++k) { rp[k] = rp[k - 1] * r; q[k] = q[k - 1] * (rp[k] - T(1)); if (k < n && q[k] == T(0)) { regular = false; } }
        if (!regular) {
            vector<Poly> f(n);
            T x = c;
            for (int i = 0; i < n; ++i, x *= r) { f[i] = Poly{-x, T(1)}; }
            return productOfSequence(f);}
        vector<T> iq = poly_detail::inverseList(vector<T>(q.begin(), q.begin() + n));
        Poly res(n + 1);
        T ck = 1, tri = 1;
        for (int k = 0; k <= n; ++k) {
            T binom = k == 0 || k == n ? T(1) : q[n] * iq[k] * iq[n - k];
            res.v[n - k] = ck * tri * binom;
            ck *= -c; tri *= rp[k];}
        return res;}

    // T: O(M(n)), M: O(n); polynomial of degree < n through (c * r^i, y_i); needs c, r nonzero and r^k != 1 for 0 < k < n (asserted).
    static Poly interpolateGeometric(const T &c, const T &r, const vector<T> &ys) {
        int n = int(ys.size());
        if (n <= 1) { return Poly(ys); }
        assert(c != T(0) && r != T(0));
        vector<T> q(n, T(1)), rp(n, T(1));
        for (int k = 1; k < n; ++k) { rp[k] = rp[k - 1] * r; q[k] = q[k - 1] * (rp[k] - T(1)); assert(q[k] != T(0)); }
        vector<T> iq = poly_detail::inverseList(q), denom(n);
        T tri = 1;
        for (int i = 0; i < n; ++i) { denom[i] = tri * poly_detail::power(r, ulng(i) * ulng(n - 1 - i)); tri *= rp[i]; }
        vector<T> idenom = poly_detail::inverseList(denom);
        T icn = T(1) / poly_detail::power(c, ulng(n - 1));
        Poly w(n);
        for (int i = 0; i < n; ++i) { w.v[i] = ys[i] * icn * iq[i] * iq[n - 1 - i] * idenom[i] * ((n - 1 - i) & 1 ? T(-1) : T(1)); }
        vector<T> s = evalMultiGeometric(w, T(1), r, n);
        T cp = 1;
        for (int k = 0; k < n; ++k, cp *= c) { s[k] *= cp; }
        Poly p = productOfGeometric(c, r, n);
        p.reverse();
        return truncatedMul(p, Poly(s), n).resize(n).reverse();}

    // T: O(M(n)) for fields through the binomial convolution, O(M(n) * log(n)) for rings by splitting; a(x + c).
    friend Poly taylorShift(const Poly &a, const T &c) {
        int n = a.size();
        if (n <= 1 || c == T(0)) { return a; }
        if constexpr (poly_detail::Field<T>) {
            auto [f, g] = poly_detail::factorials<T>(n);
            Poly x(n), y(n);
            T cp = 1;
            for (int j = 0; j < n; ++j, cp *= c) { x.v[n - 1 - j] = a.v[j] * f[j]; y.v[j] = cp * g[j]; }
            Poly z = truncatedMul(x, y, n);
            Poly res(n);
            for (int k = 0; k < n; ++k) { res.v[k] = z.coef(n - 1 - k) * g[k]; }
            return res;}
        else {
            int len = poly_detail::ceilPow2(n);
            vector<Poly> pw;
            for (Poly p = {c, T(1)}; int(pw.size()) < std::countr_zero(uint(len)); p = p * p) { pw.push_back(p); }
            auto rec = [&](auto &&self, int l, int r, int lv) -> Poly {
                if (r - l == 1) { return Poly{a.coef(l)}; }
                int mid = std::midpoint(l, r);
                return self(self, l, mid, lv - 1) + self(self, mid, r, lv - 1) * pw[lv];};
            return rec(rec, 0, len, std::countr_zero(uint(len)) - 1).resize(n);}}

    // T: O(M(n) * log(n)), M: O(n * log(n)); c with a(x) = sum c_k prod_{j<k} (x - xs_j), needs |xs| >= |a| - 1.
    friend vector<T> monomialToNewton(const Poly &a, const vector<T> &xs) {
        int n = a.size();
        if (n <= 1) { return a.v; }
        assert(int(xs.size()) >= n - 1);
        poly_detail::Tree<T> tr(vector<T>(xs.begin(), xs.begin() + n - 1));
        vector<T> res(n);
        auto rec = [&](auto &&self, int v, Poly f, int pos) -> void {
            int s = tr.cnt(v);
            if (!s) { res[pos] = f.coef(0); return; }
            if (v >= tr.nleaf) { res[pos + 1] = f.coef(1); res[pos] = f.coef(0) + f.coef(1) * xs[v - tr.nleaf]; return; }
            int sl = tr.cnt(2 * v);
            auto [q, r] = divMod(f, tr.t[2 * v]);
            self(self, 2 * v, r, pos); self(self, 2 * v + 1, q, pos + sl);};
        rec(rec, 1, Poly(a).trim(), 0);
        return res;}

    // T: O(M(n) * log(n)), M: O(n * log(n)); inverse of monomialToNewton.
    static Poly newtonToMonomial(const vector<T> &c, const vector<T> &xs) {
        int n = int(c.size());
        if (n <= 1) { return Poly(c); }
        assert(int(xs.size()) >= n - 1);
        poly_detail::Tree<T> tr(vector<T>(xs.begin(), xs.begin() + n - 1));
        auto rec = [&](auto &&self, int v, int pos, bool last) -> Poly {
            int s = tr.cnt(v);
            T top = last ? c[pos + s] : T(0);
            if (!s) { return Poly{top}; }
            if (v >= tr.nleaf) { return Poly{c[pos] - top * xs[v - tr.nleaf], top}; }
            int sl = tr.cnt(2 * v);
            return self(self, 2 * v, pos, false) + self(self, 2 * v + 1, pos + sl, last) * tr.t[2 * v];};
        return rec(rec, 1, 0, true).resize(n);}

    // T: O(n), M: O(n); the points 0..n-1, and the Newton conversions on them (O(M(n) * log(n))).
    static vector<T> iotaPoints(int n) { vector<T> xs(max(n, 0)); for (int i = 0; i < n; ++i) { xs[i] = T(i); } return xs; }

    friend vector<T> monomialToFactorial(const Poly &a) { return monomialToNewton(a, iotaPoints(a.size() - 1)); }

    static Poly factorialToMonomial(const vector<T> &c) { return newtonToMonomial(c, iotaPoints(int(c.size()) - 1)); }

    // T: O(M(n) * log(n)), M: O(n * log(n)); polynomial of degree < n with a(i) = ys[i] for i < n, through forward differences.
    static Poly interpolateIota(const vector<T> &ys) {
        int n = int(ys.size());
        if (n <= 1) { return Poly(ys); }
        auto [f, g] = poly_detail::factorials<T>(n);
        Poly x(n), y(n);
        for (int i = 0; i < n; ++i) { x.v[i] = ys[i] * g[i]; y.v[i] = i & 1 ? -g[i] : g[i]; }
        return factorialToMonomial(truncatedMul(x, y, n).resize(n).v);}

    // T: O(M(n + m)), M: O(n + m); values a(c), ..., a(c + m - 1) of the polynomial of degree < n with a(i) = ys[i].
    static vector<T> shiftSamplingPoints(const vector<T> &ys, const T &c, int m) {
        int n = int(ys.size()); assert(m >= 0);
        if (!m) { return {}; }
        if (!n) { return vector<T>(m); }
        auto [f, g] = poly_detail::factorials<T>(max(n, m));
        Poly x(n), y(n);
        for (int i = 0; i < n; ++i) { x.v[i] = ys[i] * g[i]; y.v[i] = i & 1 ? -g[i] : g[i]; }
        Poly d = truncatedMul(x, y, n).resize(n);
        for (int k = 0; k < n; ++k) { d.v[k] *= f[k]; }
        Poly e = truncatedMul(d.reverse(), Poly(poly_detail::binomialsOf(c, n)), n).resize(n).reverse();
        for (int j = 0; j < n; ++j) { e.v[j] *= g[j]; }
        Poly h = truncatedMul(e, Poly(vector<T>(g.begin(), g.begin() + m)), m).resize(m);
        for (int i = 0; i < m; ++i) { h.v[i] *= f[i]; }
        return h.v;}

    // T: O(M(n) * log(n)) for n = sum of multiplicities, M: O(n * log(n)); a with a(x_i + t) = c_i(t) mod t^{|c_i|}, distinct x_i.
    static Poly hermiteInterpolate(const vector<T> &xs, const vector<vector<T>> &c) {
        int k = int(xs.size()); assert(int(c.size()) == k);
        vector<Poly> leaves(k), leaves2(k);
        for (int i = 0; i < k; ++i) { assert(!c[i].empty()); leaves[i] = poly_detail::polyPow(Poly{-xs[i], T(1)}, int(c[i].size())); leaves2[i] = leaves[i] * leaves[i]; }
        poly_detail::Tree<T> tr(leaves);
        vector<Poly> rem = poly_detail::Tree<T>(leaves2).remainders(tr.t[1]), u(k);
        for (int i = 0; i < k; ++i) {
            int mi = int(c[i].size());
            Poly s = taylorShift(rem[i], xs[i]) >> mi;
            s.resize(mi);
            u[i] = taylorShift(truncatedMul(Poly(c[i]), inv(s, mi), mi).resize(mi), -xs[i]);}
        return tr.combine(u);}

    // T: O(M(n) * log(n)), M: O(n * log(n)); sum_i c_i * x_i^j for j < d, the transpose of evalMulti.
    static vector<T> transposedEvalMulti(const vector<T> &c, const vector<T> &xs, int d) {
        int n = int(xs.size()); assert(int(c.size()) == n && d >= 0);
        if (!d) { return {}; }
        if (!n) { return vector<T>(d); }
        poly_detail::Tree<T> tr(xs);
        return divSeries(tr.combine(c).resize(n).reverse(), Poly(tr.t[1]).reverse(), d).v;}

    // T: O(M(n) * log(n)), M: O(n * log(n)); y with y_i = sum_j b_j [x^j] L_i for the Lagrange basis L_i of the points, the transpose of interpolate.
    static vector<T> transposedInterpolate(const vector<T> &b, const vector<T> &xs) {
        int n = int(xs.size()); assert(int(b.size()) == n);
        if (!n) { return {}; }
        poly_detail::Tree<T> tr(xs);
        Poly q = truncatedMul(Poly(tr.t[1]).reverse(), Poly(b), n).resize(n).reverse();
        vector<T> w = tr.eval(deriv(tr.t[1])), y = tr.eval(q);
        for (int i = 0; i < n; ++i) { assert(w[i] != T(0)); y[i] /= w[i]; }
        return y;}

    // T: O(M(n) * log(n)), M: O(n); matrix [m00, m01, m10, m11] with (a', b') = M (a, b), deg a' >= ceil(deg a / 2) > deg b'; deg a > deg b.
    friend std::array<Poly, 4> halfGcd(const Poly &a, const Poly &b) {
        assert(a.deg() > b.deg());
        poly_detail::Mat2<T> mat = poly_detail::fastEuclid(a, b, a.deg() / 2);
        if (mat.id) { return {Poly{T(1)}, Poly{}, Poly{}, Poly{T(1)}}; }
        return {mat.a[0][0], mat.a[0][1], mat.a[1][0], mat.a[1][1]};}

    // T: O(M(n) * log(n)) for fields (plain Euclid below the measured degree 1024 or 2048), O(n^2) ring operations; monic (fields) or primitive with positive lead (rings), zero for (0, 0).
    friend Poly gcd(const Poly &a, const Poly &b) {
        Poly p = a, q = b; p.trim(); q.trim();
        if constexpr (poly_detail::Field<T>) {
            if (p.deg() < q.deg()) { swap(p, q); }
            else if (p.deg() == q.deg() && q.deg() >= 0) { p = p % q; swap(p, q); }
            if (p.deg() >= poly_detail::GCD_FAST && q.deg() >= 0) { p = poly_detail::fastEuclid(p, q, p.deg()).apply(p, q).first; }
            else { while (q.deg() >= 0) { Poly r = p % q; p = std::move(q); q = std::move(r); } }
            if (p.deg() >= 0) { p.normalize(); }
            return p;}
        else {
            if (q.isNil()) { return p.isNil() ? p : primitivePart(p); }
            if (p.isNil()) { return primitivePart(q); }
            return primitivePart(subresultants(p, q).second);}}

    // T: O(M(n) * log(n)), M: O(n); monic g = x * a + y * b, the cofactors of the extended Euclidean algorithm (deg x < deg b - deg g unless b | a); fields only.
    friend Poly exGcd(const Poly &a, const Poly &b, Poly &x, Poly &y) {
        static_assert(poly_detail::Field<T>);
        poly_detail::Mat2<T> mat = poly_detail::euclidMatrix(a, b, max(a.deg(), b.deg()));
        Poly g = mat.apply(a, b).first;
        if (g.isNil()) { x = y = Poly(); return g; }
        T il = T(1) / g.lead();
        x = mat.id ? Poly{il} : mat.a[0][0] * il; y = mat.id ? Poly() : mat.a[0][1] * il;
        g *= il; g.trim(); x.trim(); y.trim();
        return g;}

    // T: O(M(n) * log(n)), M: O(n); false when gcd(a, m) is not constant; out = a^-1 mod m with deg out < deg m.
    friend bool tryInvMod(const Poly &a, const Poly &m, Poly &out) {
        assert(m.deg() >= 1);
        Poly x, y, g = exGcd(a % m, m, x, y);
        if (g.deg() != 0) { return false; }
        out = x; return true;}

    // T: as tryInvMod; asserts invertibility.
    friend Poly invMod(const Poly &a, const Poly &m) { Poly r; if (!tryInvMod(a, m, r)) { assert(false); } return r; }

    // T: O(M(n) * log(n)) for fields, O(n^2) ring operations; a * b / gcd normalized like gcd; zero when either is zero.
    friend Poly lcm(const Poly &a, const Poly &b) {
        if (a.isNil() || b.isNil()) { return {}; }
        Poly g = gcd(a, b), q;
        if (!tryDivide(a * b, g, q)) { assert(false); }
        if constexpr (poly_detail::Field<T>) { q.normalize(); }
        else { q = primitivePart(q); }
        return q;}

    // T: O(n^2) coefficient operations, M: O(n^2); (resultant, last nonzero remainder) of the subresultant sequence, optionally recording it.
    friend pair<Poly, Poly> subresultants(const Poly &a, const Poly &b, vector<Poly> *seq = nullptr) {
        Poly p = a, q = b; p.trim(); q.trim();
        assert(!p.isNil() && !q.isNil());
        T s = 1;
        if (p.deg() < q.deg()) { swap(p, q); if (p.deg() & q.deg() & 1) { s = -s; } }
        if (seq) { seq->assign({p, q}); }
        T g = 1, h = 1;
        while (q.deg() > 0) {
            int d = p.deg() - q.deg();
            Poly r = pseudoRem(p, q);
            if (p.deg() & q.deg() & 1) { s = -s; }
            T div = g * poly_detail::power(h, ulng(d));
            p = std::move(q); q = r;
            if constexpr (poly_detail::Field<T>) { q *= T(1) / div; }
            else { q /= div; }
            q.trim();
            g = p.lead();
            if (d) { if constexpr (poly_detail::Field<T>) { h = poly_detail::power(g, ulng(d)) / poly_detail::power(h, ulng(d - 1)); } else { h = poly_detail::power(g, ulng(d)) / poly_detail::power(h, ulng(d - 1)); } }
            if (q.isNil()) { return {Poly{T(0)}, p}; }
            if (seq) { seq->push_back(q); }}
        T res = poly_detail::power(q.v[0], ulng(p.deg()));
        if constexpr (poly_detail::Field<T>) { res *= T(1) / poly_detail::power(h, ulng(p.deg() - 1)); }
        else { res /= poly_detail::power(h, ulng(p.deg() - 1)); }
        return {Poly{s * res}, q};}

    // T: O(M(n) * log(n)) for fields through the fast Euclid quotients, O(n^2) ring operations; Sylvester resultant, 0 when either is zero.
    friend T resultant(const Poly &a, const Poly &b) {
        Poly p = a, q = b; p.trim(); q.trim();
        int da = p.deg(), db = q.deg();
        if (da < 0 || db < 0) { return T(0); }
        if (da == 0 || db == 0) { return da == 0 ? poly_detail::power(p.v[0], ulng(db)) : poly_detail::power(q.v[0], ulng(da)); }
        if constexpr (poly_detail::Ring<T>) { return subresultants(p, q).first.coef(0); }
        else {
            T res = 1;
            if (da < db) { swap(p, q); swap(da, db); if (da & db & 1) { res = -res; } }
            if (da == db) {
                Poly r = p % q;
                if (r.isNil()) { return T(0); }
                res *= poly_detail::power(q.lead(), ulng(da - r.deg())) * (da & db & 1 ? T(-1) : T(1));
                p = std::move(q); q = std::move(r); da = p.deg(); db = q.deg();}
            vector<Poly> qs;
            poly_detail::fastEuclid(p, q, da, &qs);
            vector<int> n = {da, db}; vector<T> lc = {p.lead(), q.lead()};
            for (int i = 1; i < int(qs.size()); ++i) { n.push_back(n.back() - qs[i].deg()); lc.push_back(lc.back() / qs[i].lead()); }
            int h = int(qs.size()) + 1;
            for (int i = 0; i + 2 <= h - 1; ++i) { res *= poly_detail::power(lc[i + 1], ulng(n[i] - n[i + 2])) * (n[i] & n[i + 1] & 1 ? T(-1) : T(1)); }
            return n[h - 1] == 0 ? res * poly_detail::power(lc[h - 1], ulng(n[h - 2])) : T(0);}}

    // T: as resultant; (-1)^(n(n-1)/2) res(a, a') / lc(a) with the formal degree n - 1 for a'; deg a >= 1.
    friend T discriminant(const Poly &a) {
        Poly p = a; p.trim();
        int n = p.deg(); assert(n >= 1);
        Poly d = deriv(p); d.trim();
        if (d.isNil()) { return T(0); }
        T r = resultant(p, d) * poly_detail::power(p.lead(), ulng(n - 1 - d.deg()));
        if constexpr (poly_detail::Field<T>) { r *= T(1) / p.lead(); }
        else { r /= p.lead(); }
        return (n / 2) & 1 ? -r : r;}

    // T: O(n) gcds for rings (signed like the leading coefficient), the leading coefficient for fields; primitivePart divides it out.
    friend T content(const Poly &a) {
        if constexpr (poly_detail::Field<T>) { return a.lead(); }
        else {
            T g = 0;
            for (const T &x : a.v) { if constexpr (poly_detail::BigInt<T>) { g = gcd(g, x); } else { g = std::gcd(g, x); } }
            return g * poly_detail::leadSign(a.lead());}}

    // T: O(n) plus content, M: O(n); a divided by its content, zero for zero.
    friend Poly primitivePart(const Poly &a) {
        Poly r = a; r.trim();
        if (r.isNil()) { return r; }
        return r /= content(a);}

    // T: O(n * d(n)), M: O(n); the n-th cyclotomic polynomial over T, n >= 1.
    static Poly cyclotomic(int n) {
        assert(n >= 1);
        vector<int> mu(n + 1, 0); mu[1] = 1;
        for (int i = 1; i <= n; ++i) { for (int j = 2 * i; j <= n; j += i) { mu[j] -= mu[i]; } }
        Poly res = {T(1)};
        for (int d = 1; d <= n; ++d) {
            if (n % d || mu[n / d] != 1) { continue; }
            Poly next(res.size() + d);
            for (int i = 0; i < res.size(); ++i) { next.v[i + d] += res.v[i]; next.v[i] -= res.v[i]; }
            res = std::move(next);}
        for (int d = 1; d <= n; ++d) {
            if (n % d || mu[n / d] != -1) { continue; }
            Poly next(res.size() - d);
            for (int i = 0; i < next.size(); ++i) { next.v[i] = (i >= d ? next.v[i - d] : T(0)) - res.v[i]; }
            res = std::move(next);}
        return res;}

    // T: O(M(n)), M: O(n); false unless a is the square of a polynomial, which is then out with the sqrt branch of its lowest coefficient.
    friend bool trySqrtExact(const Poly &a, Poly &out) {
        static_assert(poly_detail::Field<T>);
        Poly p = a; p.trim();
        if (p.isNil()) { out = Poly(); return true; }
        int v = poly_detail::valuation(p), d = p.deg();
        if ((v & 1) || ((d - v) & 1)) { return false; }
        Poly r;
        if (!trySqrt(p, (d - v) / 2 + 1 + v / 2, r)) { return false; }
        r.trim();
        if (!(r * r == p)) { return false; }
        out = r; return true;}

    // T: O(M(d)) for d = deg m, M: O(d); (a * b) mod m.
    friend Poly mulMod(const Poly &a, const Poly &b, const Poly &m) { return PolyModulus<T>(m).mul(a, b); }

    // T: O(M(d) * log(e)), M: O(d); a^e mod m for e >= 0, negative e through invMod (asserted invertible).
    friend Poly powMod(const Poly &a, lng e, const Poly &m) {
        PolyModulus<T> pm(m);
        if (e < 0) { return pm.pow(invMod(a, m), ulng(0) - ulng(e)); }
        return pm.pow(a, ulng(e));}

    // T: O(k * M(d)), M: O(k * d); g^0, ..., g^k mod m.
    friend vector<Poly> powersMod(const Poly &g, int k, const Poly &m) {
        assert(k >= 0);
        PolyModulus<T> pm(m);
        vector<Poly> r(k + 1);
        r[0] = pm.reduce(Poly{T(1)});
        for (int i = 1; i <= k; ++i) { r[i] = pm.mul(r[i - 1], g); }
        return r;}

    // T: O(sqrt(n) * M(d) + n * d) for n = |f|, M: O(sqrt(n) * d); f(g) mod m by Brent–Kung baby steps and giant steps.
    friend Poly composeMod(const Poly &f, const Poly &g, const Poly &m) {
        PolyModulus<T> pm(m);
        int n = f.size(), d = pm.d;
        if (!n) { return {}; }
        int t = max(1, int(std::sqrt(double(n)))), nb = (n + t - 1) / t;
        vector<Poly> pw = powersMod(g, t, m);
        for (Poly &x : pw) { x.resize(d); }
        Poly res;
        for (int j = nb - 1; j >= 0; --j) {
            Poly blk(d);
            for (int i = 0; i < t && j * t + i < n; ++i) {
                T c = f.v[j * t + i];
                if (c == T(0)) { continue; }
                for (int e = 0; e < d; ++e) { blk.v[e] += c * pw[i].v[e]; }}
            res = pm.mul(res, pw[t]) + blk;}
        return res.trim();}

    // T: O(M(n) * log(n)) gcds per Yun step, M: O(n); (monic square-free factor, multiplicity) pairs with product a / lc(a); prime fields or characteristic zero.
    friend vector<pair<Poly, int>> squareFree(const Poly &a) {
        static_assert(poly_detail::Field<T>);
        Poly f = a; f.trim();
        vector<pair<Poly, int>> res;
        if (f.deg() < 1) { return res; }
        f.normalize();
        lng p = 0;
        if constexpr (ModularInt<T>) { assert(T::is_prime); p = lng(T::mod()); }
        auto push = [&](const Poly &g, int mult) {
            for (auto &[h, e] : res) { if (e == mult) { h = h * g; return; } }
            res.emplace_back(g, mult);};
        auto deflate = [&](const Poly &g) {
            Poly r((g.size() - 1) / int(p) + 1);
            for (int i = 0; i < r.size(); ++i) { r.v[i] = g.v[i * int(p)]; }
            return r;};
        auto run = [&](auto &&self, Poly g, int scale) -> void {
            Poly dg = deriv(g); dg.trim();
            if (dg.isNil()) { self(self, deflate(g), scale * int(p)); return; }
            Poly c = gcd(g, dg), w = g / c;
            for (int i = 1; w.deg() > 0; ++i) {
                Poly y = gcd(w, c), z = w / y;
                if (z.deg() > 0) { push(z, i * scale); }
                w = y; c = c / y;}
            if (c.deg() > 0) { self(self, deflate(c), scale * int(p)); }};
        run(run, f, 1);
        sort(res.begin(), res.end(), [](const auto &x, const auto &y) { return x.second < y.second; });
        return res;}

    // T: O(n * log(p) * M(n) + expected n * M(n) * log(n)) over F_p, M: O(n); sorted (monic irreducible factor, multiplicity) pairs; Las Vegas with the given seed.
    friend vector<pair<Poly, int>> factor(const Poly &a, ulng seed = 1) requires ModularInt<T> {
        assert(T::is_prime);
        std::mt19937_64 rng(seed);
        vector<pair<Poly, int>> res;
        for (auto &[f, e] : squareFree(a)) {
            PolyModulus<T> pm(f);
            Poly g = f, h = pm.reduce(Poly{T(0), T(1)}), x = {T(0), T(1)};
            for (int i = 1; g.deg() >= 2 * i; ++i) {
                h = pm.pow(h, T::mod());
                Poly c = gcd(h - x, g);
                if (c.deg() > 0) {
                    vector<Poly> parts; poly_detail::equalDegree(c, i, rng, parts);
                    for (const Poly &q : parts) { res.emplace_back(q, e); }
                    g = g / c; pm = PolyModulus<T>(g.deg() >= 1 ? g : Poly{T(0), T(1)}); h = pm.reduce(h);}}
            if (g.deg() > 0) { res.emplace_back(g, e); }}
        sort(res.begin(), res.end(), [](const auto &x, const auto &y) { return poly_detail::lessPoly(x.first, y.first); });
        return res;}

    // T: O(log(p) * M(n) + expected M(n) * log(n)^2), M: O(n); the distinct roots of a in F_p sorted by value; Las Vegas with the given seed.
    friend vector<T> roots(const Poly &a, ulng seed = 1) requires ModularInt<T> {
        assert(T::is_prime);
        Poly f = a; f.trim();
        vector<T> res;
        if (f.deg() < 1) { return res; }
        f.normalize();
        Poly x = {T(0), T(1)}, g = gcd(PolyModulus<T>(f).pow(x, T::mod()) - x, f);
        if (g.deg() < 1) { return res; }
        std::mt19937_64 rng(seed);
        vector<Poly> parts; poly_detail::equalDegree(g, 1, rng, parts);
        for (const Poly &q : parts) { res.push_back(-q.v[0]); }
        sort(res.begin(), res.end(), [](const T &u, const T &v) { return u.val() < v.val(); });
        return res;}

    // T: O(n * log(p) * M(n)), M: O(n^2); Rabin's test over F_p with all Frobenius images stored; constants are not irreducible.
    friend bool isIrreducible(const Poly &a) requires ModularInt<T> {
        assert(T::is_prime);
        Poly f = a; f.trim();
        int n = f.deg();
        if (n < 1) { return false; }
        if (n == 1) { return true; }
        PolyModulus<T> pm(f);
        Poly x = {T(0), T(1)};
        vector<Poly> h(n + 1); h[0] = x;
        for (int i = 1; i <= n; ++i) { h[i] = pm.pow(h[i - 1], T::mod()); }
        if (!(h[n] == x)) { return false; }
        for (int q = 2, r = n; q <= r; ++q) {
            if (r % q) { continue; }
            while (r % q == 0) { r /= q; }
            if (gcd(h[n / q] - x, f).deg() > 0) { return false; }}
        return true;}

    // T: O(M(n) * log(n)), M: O(n * log(n)); s_k = sum_{i<n} w_i [x^i] g^k for k < n (Kinoshita–Li bivariate Bostan–Mori).
    friend vector<T> powerProjection(const Poly &w, const Poly &g, int n) {
        assert(n >= 0);
        if (!n) { return {}; }
        int len = poly_detail::ceilPow2(n);
        vector<vector<T>> q = poly_detail::projectionLevels(g, len);
        vector<T> p(len);
        for (int a = 0; a < len; ++a) { p[a] = len - 1 - a < n ? w.coef(len - 1 - a) : T(0); }
        for (int rows = len, k = 1, lv = 0; rows > 1; rows /= 2, k *= 2, ++lv) {
            int cols = k + 1;
            vector<T> qm = q[lv];
            for (int a = 1; a < rows; a += 2) { for (int b = 0; b < cols; ++b) { qm[size_t(a) * cols + b] = -qm[size_t(a) * cols + b]; } }
            vector<T> v = poly_detail::mul2d(p, rows, k, qm, rows, cols, 2 * k, rows), next(size_t(rows / 2) * 2 * k);
            for (int j = 0; j < rows / 2; ++j) { std::copy(v.begin() + size_t(2 * j + 1) * 2 * k, v.begin() + size_t(2 * j + 2) * 2 * k, next.begin() + size_t(j) * 2 * k); }
            p = std::move(next);}
        Poly qb(vector<T>(q.back().begin(), q.back().begin() + len + 1));
        return truncatedMul(Poly(p), inv(qb, n), n).resize(n).v;}

    // T: O(M(N) * log(N)) with N = max(n, |f|), M: O(N * log(N)); f(g(x)) mod x^n with f a polynomial (Kinoshita–Li, the transpose of powerProjection).
    friend Poly compose(const Poly &f, const Poly &g, int n) {
        assert(n >= 0);
        if (!n) { return {}; }
        if (max(n, f.size()) < poly_detail::COMPOSE_BK) { return composeBrentKung(f, g, n); }
        int len = poly_detail::ceilPow2(max(n, f.size()));
        vector<vector<T>> q = poly_detail::projectionLevels(g, len);
        int depth = int(q.size()) - 1;
        vector<T> fv(len), u = inv(Poly(vector<T>(q[depth].begin(), q[depth].begin() + len + 1)), len).v;
        for (int k = 0; k < len; ++k) { fv[k] = f.coef(k); }
        vector<T> p = poly_detail::Tree<T>::corr(fv, u, len);
        for (int lv = depth - 1, rows = 2, k = len / 2; lv >= 0; --lv, rows *= 2, k /= 2) {
            int cols = k + 1;
            vector<T> at(size_t(rows) * 2 * k), qr(size_t(rows) * cols);
            for (int j = 0; j < rows / 2; ++j) { std::copy(p.begin() + size_t(j) * 2 * k, p.begin() + size_t(j + 1) * 2 * k, at.begin() + size_t(2 * j + 1) * 2 * k); }
            for (int a = 0; a < rows; ++a) {
                for (int b = 0; b <= k; ++b) {
                    T x = q[lv][size_t(rows - 1 - a) * cols + (k - b)];
                    qr[size_t(a) * cols + b] = (rows - 1 - a) & 1 ? -x : x;}}
            vector<T> v = poly_detail::mul2d(at, rows, 2 * k, qr, rows, cols, 2 * k, 2 * rows);
            vector<T> next(size_t(rows) * k);
            for (int a = 0; a < rows; ++a) { std::copy(v.begin() + size_t(a + rows - 1) * 2 * k + k, v.begin() + size_t(a + rows - 1) * 2 * k + 2 * k, next.begin() + size_t(a) * k); }
            p = std::move(next);}
        Poly res(n);
        for (int i = 0; i < n; ++i) { res.v[i] = p[len - 1 - i]; }
        return res;}

    // T: O(sqrt(n) * M(n) + n^2), M: O(sqrt(n) * n); the Brent–Kung baseline that compose uses below the measured threshold.
    friend Poly composeBrentKung(const Poly &f, const Poly &g, int n) {
        assert(n >= 0);
        int m = f.size();
        if (!n || !m) { return Poly(n); }
        int t = max(1, int(std::sqrt(double(m)))), nb = (m + t - 1) / t;
        vector<Poly> pw(t + 1);
        pw[0] = Poly(n); pw[0].v[0] = 1;
        for (int i = 1; i <= t; ++i) { pw[i] = truncatedMul(pw[i - 1], g, n).resize(n); }
        Poly res(n);
        for (int j = nb - 1; j >= 0; --j) {
            Poly blk(n);
            for (int i = 0; i < t && j * t + i < m; ++i) {
                T c = f.v[j * t + i];
                if (c == T(0)) { continue; }
                for (int e = 0; e < n; ++e) { blk.v[e] += c * pw[i].v[e]; }}
            res = truncatedMul(res, pw[t], n).resize(n) + blk;}
        return res;}

    // T: O(M(n) * log(n)), M: O(n * log(n)); h with h(g(x)) = x mod x^n for g(0) = 0 and g'(0) invertible; needs characteristic > n.
    friend Poly compositionalInverse(const Poly &g, int n) {
        assert(n >= 0 && g.coef(0) == T(0) && g.coef(1) != T(0));
        if (n <= 1) { return Poly(n); }
        int m = n - 1;
        Poly w(m + 1); w.v[m] = 1;
        vector<T> s = powerProjection(w, g, m + 1);
        Poly c(m);
        T im = T(1) / T(m), g1 = g.coef(1), ig = T(1) / g1;
        for (int k = 1; k <= m; ++k) { c.v[m - k] = s[k] * T(m) * (T(1) / T(k)); }
        c *= poly_detail::power(ig, ulng(m));
        Poly xh = powUnit(c, im, m) * g1;
        return (inv(xh, m) << 1).resize(n);}

    // T: O(M(n)), M: O(n); [x^n] h^k for the compositional inverse h of g through Lagrange inversion; k in [0, n], needs T(n) invertible for n >= 1.
    friend T lagrangeInversionCoefficient(const Poly &g, int k, int n) {
        assert(n >= 0 && 0 <= k && g.coef(0) == T(0) && g.coef(1) != T(0));
        if (k > n) { return T(0); }
        if (n == 0) { return T(k == 0); }
        if (k == 0) { return T(0); }
        int m = n - k + 1;
        Poly u = pow(inv(g >> 1, m), n, m);
        return u.coef(n - k) * T(k) / T(n);}

    // T: O(M(n * m)), M: O(n * m); 1 / f mod (x^n, y^m) for an n x m row-major array with f(0, 0) invertible (Newton in x over T[y] / y^m).
    static vector<T> inv2d(const vector<T> &f, int n, int m) {
        assert(n >= 1 && m >= 1 && int(f.size()) == n * m);
        vector<T> g = inv(Poly(vector<T>(f.begin(), f.begin() + m)), m).v;
        int stride = 2 * m - 1;
        for (int k = 1; k < n; k *= 2) {
            int rows = min(2 * k, n);
            vector<T> fg = poly_detail::mul2d(vector<T>(f.begin(), f.begin() + size_t(rows) * m), rows, m, g, k, m, stride, rows), e(size_t(rows) * m);
            for (int a = 0; a < rows; ++a) { for (int b = 0; b < m; ++b) { e[size_t(a) * m + b] = -fg[size_t(a) * stride + b]; } }
            e[0] += 2;
            vector<T> ng = poly_detail::mul2d(g, k, m, e, rows, m, stride, rows);
            g.assign(size_t(rows) * m, T(0));
            for (int a = 0; a < rows; ++a) { for (int b = 0; b < m; ++b) { g[size_t(a) * m + b] = ng[size_t(a) * stride + b]; } }}
        return g;}

    // T: O(M(d) * log(k)) for d = deg q, M: O(d); [x^k] p / q with q(0) invertible (Bostan–Mori).
    friend T coefOfRationalFps(Poly p, Poly q, lng k) {
        assert(k >= 0 && q.size() && q.v[0] != T(0));
        p.trim(); q.trim();
        for (; k > 0; k >>= 1) {
            Poly qm = q;
            for (int i = 1; i < qm.size(); i += 2) { qm.v[i] = -qm.v[i]; }
            Poly u = p * qm, v = q * qm, np((u.size() + 1) / 2), nq((v.size() + 1) / 2);
            for (int i = int(k & 1); i < u.size(); i += 2) { np.v[i / 2] = u.v[i]; }
            for (int i = 0; i < v.size(); i += 2) { nq.v[i / 2] = v.v[i]; }
            p = std::move(np); q = std::move(nq); p.trim();}
        return p.coef(0) / q.coef(0);}

    // T: O(M(d) * log(l) + M(d + m)) for d = deg q and m = r - l, M: O(d + m); coefficients [l, r) of p / q with q(0) invertible.
    friend vector<T> sliceRationalFps(const Poly &p, const Poly &q, lng l, lng r) {
        assert(0 <= l && l <= r && q.size() && q.v[0] != T(0));
        int m = int(r - l);
        vector<T> res(m);
        if (!m) { return res; }
        Poly qq = q; qq.trim();
        int d = qq.deg();
        T iq = T(1) / qq.coef(0);
        if (d == 0) {
            for (int i = 0; i < m; ++i) { res[i] = p.coef(int(min<lng>(l + i, INT32_MAX))) * iq; }
            return res;}
        auto [u, pp] = divMod(p, qq);
        for (int i = 0; i < m; ++i) { res[i] = u.coef(int(min<lng>(l + i, INT32_MAX))); }
        if (pp.isNil()) { return res; }
        Poly c = Poly(qq).reverse() * iq;
        Poly rl = powMod(Poly{T(0), T(1)}, l, c);
        rl.resize(d);
        vector<T> a = divSeries(pp, qq, d + m - 1).v, w = poly_detail::Tree<T>::corr(a, rl.v, m);
        for (int i = 0; i < m; ++i) { res[i] += w[i]; }
        return res;}

    // T: O(M(d) * log(k)), M: O(d); a_k for a_i = sum_{j=1}^{d} c[j-1] * a_{i-j} from the first d terms (more terms are accepted and used directly).
    static T linearRecurrenceKth(const vector<T> &a, const vector<T> &c, lng k) {
        int d = int(c.size()); assert(k >= 0 && int(a.size()) >= d);
        if (k < lng(a.size())) { return a[size_t(k)]; }
        if (!d) { return T(0); }
        Poly q(d + 1); q.v[0] = 1;
        for (int j = 1; j <= d; ++j) { q.v[j] = -c[j - 1]; }
        Poly p = truncatedMul(Poly(vector<T>(a.begin(), a.begin() + d)), q, d);
        return coefOfRationalFps(p, q, k);}

    // T: O(M(d) * log(l) + M(d + m)), M: O(d + m); a_l, ..., a_{l+m-1} of the same recurrence.
    static vector<T> consecutiveTerms(const vector<T> &a, const vector<T> &c, lng l, int m) {
        int d = int(c.size()); assert(l >= 0 && m >= 0 && int(a.size()) >= d);
        if (!d) { vector<T> r(m); for (int i = 0; i < m; ++i) { r[i] = l + i < lng(a.size()) ? a[size_t(l + i)] : T(0); } return r; }
        Poly q(d + 1); q.v[0] = 1;
        for (int j = 1; j <= d; ++j) { q.v[j] = -c[j - 1]; }
        Poly p = truncatedMul(Poly(vector<T>(a.begin(), a.begin() + d)), q, d);
        return sliceRationalFps(p, q, l, l + m);}

    // T: O(n^2), M: O(n); shortest c with a_i = sum_{j=1}^{|c|} c[j-1] * a_{i-j} for |c| <= i < n (Berlekamp–Massey over a field).
    static vector<T> findLinearRecurrence(const vector<T> &a) {
        vector<T> c, b = {T(1)};
        T bd = 1; int cur = 0, m = 1;
        for (int i = 0; i < int(a.size()); ++i, ++m) {
            T d = a[i];
            for (int j = 1; j <= cur && j <= int(c.size()); ++j) { d += c[j - 1] * a[i - j]; }
            if (d == T(0)) { continue; }
            vector<T> t = c;
            T coef = d / bd;
            if (c.size() < b.size() + m - 1) { c.resize(b.size() + m - 1); }
            for (int j = 0; j < int(b.size()); ++j) { c[j + m - 1] -= coef * b[j]; }
            if (2 * cur <= i) { cur = i + 1 - cur; b = t; b.insert(b.begin(), T(1)); bd = d; m = 0; }}
        c.resize(cur);
        for (T &x : c) { x = -x; }
        return c;}

    // T: O(M(n) * log(n)), M: O(n); [m / n] Padé approximant p / q of f mod x^(m + n + 1) with q(0) = 1; false when no such q(0) != 0 exists.
    friend bool pade(const Poly &f, int m, int n, Poly &p, Poly &q) {
        assert(m >= 0 && n >= 0);
        int len = m + n + 1;
        Poly xn(len + 1), fx = Poly(f).truncate(len).trim();
        xn.v[len] = 1;
        poly_detail::Mat2<T> mat = poly_detail::euclidMatrix(xn, fx, n);
        if (mat.id) { p = fx; q = {T(1)}; return true; }
        p = mat.apply(xn, fx).second; q = mat.a[1][1]; q.trim();
        if (q.coef(0) == T(0)) { return false; }
        T s = T(1) / q.coef(0); p *= s; q *= s;
        return true;}

    // T: O(M(N) * log(N)) for N = m + n + 1 points, M: O(N * log(N)); p / q with deg p <= m, deg q <= n through the points; false when no reduced solution exists.
    friend bool rationalInterpolate(const vector<T> &xs, const vector<T> &ys, int m, int n, Poly &p, Poly &q) {
        int len = int(xs.size()); assert(len == m + n + 1 && int(ys.size()) == len);
        poly_detail::Tree<T> tr(xs);
        Poly f = interpolate(xs, ys);
        poly_detail::Mat2<T> mat = poly_detail::euclidMatrix(tr.t[1], f, n);
        if (mat.id) { p = f; q = {T(1)}; }
        else { p = mat.apply(tr.t[1], f).second; q = mat.a[1][1]; q.trim(); }
        vector<T> qv = tr.eval(q), pv = tr.eval(p);
        if (p.deg() > m) { return false; }
        for (int i = 0; i < len; ++i) { if (qv[i] == T(0) || pv[i] != ys[i] * qv[i]) { return false; } }
        T s = T(1) / q.lead(); p *= s; q *= s;
        return true;}

    // T: O(M(n) * log(n)) for n = deg p + sum of multiplicities, M: O(n * log(n)); p / prod (x - r_i)^{m_i} = u + sum c[i][j-1] / (x - r_i)^j with distinct r_i.
    friend pair<Poly, vector<vector<T>>> partialFractions(const Poly &p, const vector<pair<T, int>> &rts) {
        int k = int(rts.size());
        vector<Poly> l1(k), l2(k);
        for (int i = 0; i < k; ++i) { assert(rts[i].second >= 1); l1[i] = poly_detail::polyPow(Poly{-rts[i].first, T(1)}, rts[i].second); l2[i] = l1[i] * l1[i]; }
        poly_detail::Tree<T> t1(l1), t2(l2);
        auto [u, pp] = divMod(p, t1.t[1]);
        vector<Poly> rq = t2.remainders(t1.t[1]), rp = t1.remainders(pp);
        vector<vector<T>> c(k);
        for (int i = 0; i < k; ++i) {
            int mi = rts[i].second;
            Poly qs = taylorShift(rq[i], rts[i].first) >> mi, ps = taylorShift(rp[i], rts[i].first);
            qs.resize(mi); ps.resize(mi);
            Poly h = truncatedMul(ps, inv(qs, mi), mi).resize(mi);
            c[i].resize(mi);
            for (int j = 1; j <= mi; ++j) { c[i][j - 1] = h.v[mi - j]; }}
        return {u, c};}

    // T: O(M(n) * log(n)), M: O(n); (a * b) mod x^n through the online structures, for cross-checks.
    friend Poly relaxedMul(const Poly &a, const Poly &b, int n) {
        RelaxedMul<T> r;
        Poly res(n);
        for (int i = 0; i < n; ++i) { res.v[i] = r.push(a.coef(i), b.coef(i)); }
        return res;}

    friend Poly semiRelaxedMul(const Poly &a, const Poly &b, int n) {
        SemiRelaxedMul<T> r(b);
        Poly res(n);
        for (int i = 0; i < n; ++i) { res.v[i] = r.push(a.coef(i)); }
        return res;}

    // T: O(M(n) * log(n)) through the semi-relaxed product, M: O(n); f with f' = a * f + b and f(0) = c mod x^n (fields with characteristic > n).
    friend Poly linearOde(const Poly &a, const Poly &b, const T &c, int n) {
        assert(n >= 0);
        if (!n) { return {}; }
        SemiRelaxedMul<T> r(a);
        vector<T> inv = poly_detail::inverses<T>(n + 1);
        Poly f(n); f.v[0] = c;
        for (int i = 0; i + 1 < n; ++i) { f.v[i + 1] = (r.push(f.v[i]) + b.coef(i)) * inv[i + 1]; }
        return f;}

    // T: O(M(n)), M: O(n); f with f' / f = h and f(0) = 1 mod x^n.
    friend Poly fromLogDerivative(const Poly &h, int n) { return exp(integ(Poly(h).truncate(max(n - 1, 0))).truncate(n), n); }

    // T: O(n), M: O(n); divide or multiply coefficient i by i!; fields with characteristic > n.
    friend Poly ogfToEgf(const Poly &a) {
        auto [f, g] = poly_detail::factorials<T>(a.size());
        Poly r = a;
        for (int i = 0; i < r.size(); ++i) { r.v[i] *= g[i]; }
        return r;}

    friend Poly egfToOgf(const Poly &a) {
        auto [f, g] = poly_detail::factorials<T>(a.size());
        Poly r = a;
        for (int i = 0; i < r.size(); ++i) { r.v[i] *= f[i]; }
        return r;}

    // T: O(M(n)) for fields with d != 0 (doubling with Taylor shifts), O(M(n) * log(n)) otherwise; prod_{i<n} (x + a + i * d).
    static Poly productOfArithmeticProgression(const T &a, const T &d, int n) {
        assert(n >= 0);
        if constexpr (poly_detail::Field<T>) {
            if (d != T(0)) {
                T c = a / d;
                auto rec = [&](auto &&self, int k) -> Poly {
                    if (!k) { return Poly{T(1)}; }
                    if (k & 1) { return self(self, k - 1) * Poly{c + T(k - 1), T(1)}; }
                    Poly f = self(self, k / 2);
                    return f * taylorShift(f, T(k / 2));};
                Poly f = rec(rec, n);
                T dp = 1;
                for (int j = n; j >= 0; --j, dp *= d) { f.v[j] *= dp; }
                return f;}}
        vector<Poly> f(n);
        T x = a;
        for (int i = 0; i < n; ++i, x += d) { f[i] = Poly{x, T(1)}; }
        return productOfSequence(f);}

    // T: O(M(n) * log(n)) for n = total size, M: O(n); (numerator, denominator) of sum p_i / q_i without reduction.
    friend pair<Poly, Poly> sumOfRationals(const vector<pair<Poly, Poly>> &f) {
        if (f.empty()) { return {Poly(), Poly{T(1)}}; }
        auto rec = [&](auto &&self, int l, int r) -> pair<Poly, Poly> {
            if (r - l == 1) { return f[l]; }
            int mid = std::midpoint(l, r);
            auto [p1, q1] = self(self, l, mid);
            auto [p2, q2] = self(self, mid, r);
            return {p1 * q2 + p2 * q1, q1 * q2};};
        return rec(rec, 0, int(f.size()));}

    // T: O(M(n) * log(n)), M: O(n * log(n)); [x^(n-1)] g * f^k for k < n (powerProjection with the reversed weights).
    friend vector<T> polynomialPowerEnumerate(const Poly &f, const Poly &g, int n) {
        Poly w(n);
        for (int i = 0; i < n; ++i) { w.v[i] = g.coef(n - 1 - i); }
        return powerProjection(w, f, n);}

    // T: O(M(n)), M: O(n); B_0..B_{n-1} with B_1 = -1/2; fields with characteristic > n.
    static vector<T> bernoulliNumbers(int n) {
        assert(n >= 0);
        if (!n) { return {}; }
        auto [f, g] = poly_detail::factorials<T>(n + 1);
        Poly e(n);
        for (int i = 0; i < n; ++i) { e.v[i] = g[i + 1]; }
        Poly b = inv(e, n);
        for (int i = 0; i < n; ++i) { b.v[i] *= f[i]; }
        return b.v;}

    // T: O(M(n)), M: O(n); p(0..n-1) through Euler's pentagonal series.
    static vector<T> partitionNumbers(int n) {
        assert(n >= 0);
        if (!n) { return {}; }
        Poly e(n);
        e.v[0] = 1;
        for (int k = 1; k * (3 * k - 1) / 2 < n; ++k) {
            T s = k & 1 ? T(-1) : T(1);
            e.v[k * (3 * k - 1) / 2] += s;
            if (k * (3 * k + 1) / 2 < n) { e.v[k * (3 * k + 1) / 2] += s; }}
        return inv(e, n).v;}

    // T: O(M(n)), M: O(n); signed Stirling numbers of the first kind s(n, k) for k <= n, the coefficients of x (x - 1) ... (x - n + 1).
    static vector<T> stirling1Row(int n) { return productOfArithmeticProgression(T(0), T(-1), n).resize(n + 1).v; }

    // T: O(M(n)), M: O(n); S(n, k) for k <= n; fields with characteristic > n.
    static vector<T> stirling2Row(int n) {
        assert(n >= 0);
        auto [f, g] = poly_detail::factorials<T>(n + 1);
        Poly a(n + 1), b(n + 1);
        for (int i = 0; i <= n; ++i) { a.v[i] = i & 1 ? -g[i] : g[i]; b.v[i] = poly_detail::power(T(i), ulng(n)) * g[i]; }
        return truncatedMul(a, b, n + 1).resize(n + 1).v;}

    // T: O(M(n)), M: O(n); Eulerian numbers A(n, k) for k < n (A(0, 0) = 1); fields with characteristic > n + 1.
    static vector<T> eulerianRow(int n) {
        assert(n >= 0);
        if (!n) { return {T(1)}; }
        auto [f, g] = poly_detail::factorials<T>(n + 2);
        Poly a(n), b(n);
        for (int j = 0; j < n; ++j) { a.v[j] = f[n + 1] * g[j] * g[n + 1 - j] * (j & 1 ? T(-1) : T(1)); b.v[j] = poly_detail::power(T(j + 1), ulng(n)); }
        return truncatedMul(a, b, n).resize(n).v;}

    // T: O(M(n)), M: O(n); g with g(k) = sum_{i<k} f(i) for every integer k, by Faulhaber with Bernoulli numbers; characteristic > deg f + 1.
    friend Poly prefixSumOfPolynomial(const Poly &f) {
        int n = f.size();
        if (!n) { return {}; }
        auto [fa, g] = poly_detail::factorials<T>(n + 2);
        vector<T> bern = bernoulliNumbers(n + 1);
        vector<T> af(n), bz(n + 1);
        for (int k = 0; k < n; ++k) { af[k] = f.v[k] * fa[k]; }
        for (int s = 0; s <= n; ++s) { bz[s] = bern[s] * g[s]; }
        vector<T> w = poly_detail::Tree<T>::corr(af, bz, n);
        Poly res(n + 1);
        for (int t = 1; t <= n; ++t) { res.v[t] = w[t - 1] * g[t]; }
        return res;}
};
template<typename T> using FPS = Poly<T>;

// T: NA, M: NA; stateless companion for the double FFT and the exact integer and GF(2^k) products, each function with its own bound.
struct PolyCompanion {
    // T: O(n * log(n)), M: O(n); complex doubles, n a power of two, same bit-reversed convention as ntt.
    static void fft(vector<std::complex<double>> &a) { poly_detail::Fft::forward(a, false); }
    static void ifft(vector<std::complex<double>> &a) { poly_detail::Fft::forward(a, true); }
    // T: O(n * log(n)), M: O(n); residues below mod with 1 <= mod < 2^30 via split 15-bit halves; exact for n + m - 1 <= 2^20 (asserted).
    static vector<uint> convolutionFftMod(const vector<uint> &a, const vector<uint> &b, uint mod) { return poly_detail::convolutionFftMod(a, b, mod); }
    // T: O(n * log(n)) up to 2^23 terms, O(n * m / L + (n + m) * log(L)) with L = 2^22 beyond; M: O(n); any modulus in [1, 2^64) through three (mod < 2^32) or six NTT primes and Garner.
    static vector<ulng> convolutionArbitraryMod(const vector<ulng> &a, const vector<ulng> &b, ulng mod) {
        assert(mod);
        if (a.empty() || b.empty()) { return {}; }
        int n = int(a.size()), m = int(b.size());
        vector<ulng> res(size_t(n + m) - 1); Barrett64 br(mod);
        auto fa = [&](int i) { return br.reduce(a[i]); };
        auto fb = [&](int i) { return br.reduce(b[i]); };
        auto run = [&]<int K>() {
            ulng pre[K]; pre[0] = 1 % mod;
            for (int k = 1; k < K; ++k) { pre[k] = br.mul(pre[k - 1], poly_detail::PRIMES[k - 1] % mod); }
            poly_detail::convolveMulti<K>(n, m, fa, fb, [&](int i, const uint *c) {
                ulng x = 0;
                for (int k = 0; k < K; ++k) { ulng t = br.mul(c[k], pre[k]); x = x >= mod - t ? x - (mod - t) : x + t; }
                res[i] = x;});};
        if (mod < (ulng(1) << 32)) { run.template operator()<3>(); }
        else { run.template operator()<6>(); }
        return res;}
    // T: O(n * log(n)), M: O(n); exact whenever every true coefficient lies in [-2^63, 2^63).
    static vector<lng> convolutionLng(const vector<lng> &a, const vector<lng> &b) { return poly_detail::convolutionLng(a, b); }
    // T: O(n * log(n)), M: O(n); product modulo 2^64 from 32-bit halves, lo * lo and the cross terms through three primes each.
    static vector<ulng> convolution2p64(const vector<ulng> &a, const vector<ulng> &b) { return poly_detail::convolution2p64(a, b); }
    // T: O(n * m^0.585) carry-less word products for n >= m plus O((n + m) * k) reduction, M: O(n + m); GF(2^k) elements as k-bit words, 1 <= k <= 64, x^k = red.
    static vector<ulng> convolutionGF2k(const vector<ulng> &a, const vector<ulng> &b, int k, ulng red) {
        assert(1 <= k && k <= 64);
        if (a.empty() || b.empty()) { return {}; }
        if (a.size() < b.size()) { return convolutionGF2k(b, a, k, red); }
        int n = int(a.size()), m = int(b.size());
        vector<ulll> prod(size_t(n + m) - 1, 0), block(2 * size_t(m) - 1);
        vector<ulng> xa(m);
        for (int l = 0; l < n; l += m) {
            int len = min(m, n - l);
            fill(xa.begin(), xa.end(), 0); std::copy(a.begin() + l, a.begin() + l + len, xa.begin());
            fill(block.begin(), block.end(), 0);
            poly_detail::karatsubaGf2(xa.data(), b.data(), m, block.data());
            for (int i = 0; i < len + m - 1; ++i) { prod[l + i] ^= block[i]; }}
        ulll modulus = ulll(red) | ulll(1) << k;
        vector<ulng> res(n + m - 1);
        for (int i = 0; i < n + m - 1; ++i) {
            ulll v = prod[i];
            for (int t = 2 * k - 2; t >= k; --t) { if (v >> t & 1) { v ^= modulus << (t - k); } }
            res[i] = ulng(v);}
        return res;}
};

// S: O(M(n)), Q: O(M(n)) per reduction of a product, M: O(n); precomputed reversed inverse of a monic-normalized modulus of degree d >= 1.
template<typename T> struct PolyModulus {
    static_assert(poly_detail::Field<T>);
    Poly<T> m, rinv; int d;

    explicit PolyModulus(const Poly<T> &mod) : m(mod), d(mod.deg()) {
        assert(d >= 1);
        m.normalize();
        if (d > poly_detail::SCHOOLBOOK) { rinv = inv(Poly<T>(m).reverse(), d); }}

    Poly<T> reduce(const Poly<T> &a) const {
        int da = a.deg(), n = da - d + 1;
        if (da < d) { return Poly<T>(a).trim(); }
        if (n > d || d <= poly_detail::SCHOOLBOOK) { return a % m; }
        Poly<T> q = truncatedMul(Poly<T>(a).trim().reverse().truncate(n), Poly<T>(rinv).truncate(n), n).resize(n).reverse();
        return (Poly<T>(a).truncate(d) - truncatedMul(q, m, d)).resize(d).trim();}
    Poly<T> mul(const Poly<T> &a, const Poly<T> &b) const { return reduce(a * b); }
    Poly<T> pow(Poly<T> a, ulng e) const {
        Poly<T> r = {T(1)};
        a = reduce(a);
        for (; e; e >>= 1) { if (e & 1) { r = mul(r, a); } if (e > 1) { a = mul(a, a); } }
        return r;}
};

// S: O(1), U: O(M(2^v)) at the i-th push with 2^v the lowest set bit of i + 1 (O(M(n) * log(n)) total), M: O(n); online product with a known g.
template<typename T> struct SemiRelaxedMul {
    Poly<T> g; vector<T> f, h; vector<vector<T>> spec;

    explicit SemiRelaxedMul(Poly<T> known) : g(std::move(known)) {}

    // sum_{j<i} f_j g_{i-j} for the next index i, before f_i is known.
    T partial() const { int i = int(f.size()); return i < int(h.size()) ? h[i] : T(0); }
    // Appends f_i and returns the final h_i = sum_{j<=i} f_j g_{i-j}.
    T push(const T &fi) {
        int i = int(f.size()), mid = i + 1, v = std::countr_zero(uint(mid)), len = 2 << v, l = mid - (1 << v);
        f.push_back(fi);
        if (int(h.size()) < mid + len / 2) { h.resize(size_t(mid + len / 2)); }
        h[i] += fi * g.coef(0);
        vector<T> blk(f.begin() + l, f.begin() + mid), gs(len);
        for (int t = 0; t < len; ++t) { gs[t] = g.coef(t); }
        vector<T> w;
        if (poly_detail::fastSpec<T>(len) && len >= 2 * poly_detail::SCHOOLBOOK) {
            if (int(spec.size()) <= v) { spec.resize(v + 1); }
            if (spec[v].empty()) { spec[v] = poly_detail::spec(gs, len); }
            vector<T> x = poly_detail::spec(blk, len);
            poly_detail::dot(x, spec[v], len); w = poly_detail::unspec(x, len);}
        else { w = cyclic(Poly<T>(gs), Poly<T>(blk), len).v; }
        for (int t = 0; t < len / 2; ++t) { h[mid + t] += w[len / 2 + t]; }
        return h[i];}
};

// S: O(1), U: as SemiRelaxedMul, M: O(n); online product of two sequences revealed one coefficient each per push.
template<typename T> struct RelaxedMul {
    vector<T> f, g, h;

    // sum of f_j g_k over j + k = i with 0 < j, k < i for the next index i.
    T partial() const { int i = int(f.size()); return i < int(h.size()) ? h[i] : T(0); }
    T push(const T &fi, const T &gi) {
        int i = int(f.size()), mid = i + 1, v = std::countr_zero(uint(mid)), len = 2 << v, l = mid - (1 << v);
        f.push_back(fi); g.push_back(gi);
        if (int(h.size()) < 2 * mid) { h.resize(size_t(2 * mid)); }
        h[i] += i ? f[0] * gi + fi * g[0] : fi * gi;
        if (!l) {
            vector<T> x(f.begin(), f.end()), y(g.begin(), g.end());
            vector<T> w = cyclic(Poly<T>(x), Poly<T>(y), 2 * mid).v;
            for (int t = 0; t < mid; ++t) { h[mid + t] += w[mid + t]; }}
        else {
            vector<T> fb(f.begin() + l, f.begin() + mid), gb(g.begin() + l, g.begin() + mid), fp(f.begin(), f.begin() + len), gp(g.begin(), g.begin() + len);
            vector<T> w1 = cyclic(Poly<T>(gp), Poly<T>(fb), len).v, w2 = cyclic(Poly<T>(fp), Poly<T>(gb), len).v;
            for (int t = 0; t < len / 2; ++t) { h[mid + t] += w1[len / 2 + t] + w2[len / 2 + t]; }}
        return h[i];}
};
