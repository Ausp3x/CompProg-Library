#line 2 "1-Core\\01-template.hpp"

// 知彼知己，百战不殆
#include <bits/stdc++.h>
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/tree_policy.hpp>
using namespace std;
using namespace __gnu_pbds;

#define fi    first
#define se    second
#define pb    push_back
using uint = uint32_t;
using lng = int64_t;    using ulng = uint64_t;
using lll = __int128_t; using ulll = __uint128_t;
template<typename T> 
using indexed_set = tree<T, null_type, std::less<T>, rb_tree_tag, tree_order_statistics_node_update>;

constexpr int INF32 = 0x3f3f3f3f;
constexpr lng INF64 = 0x3f3f3f3f3f3f3f3f;

template<typename T> 
constexpr inline bool chmax(T &a, const T &b) { return a < b ? a = b, 1 : 0; }
template<typename T> 
constexpr inline bool chmin(T &a, const T &b) { return a > b ? a = b, 1 : 0; }
#line 3 "1-Core\\05-infint.hpp"

#ifdef __AVX2__
#include <immintrin.h>
namespace FastIintMul {
    constexpr uint P1 = 2013265921, G1 = 31;
    constexpr uint P2 = 1811939329, G2 = 13;
    constexpr uint P3 = 469762049,  G3 = 3;
    static_assert(ulll(P1) * P2 * P3 > (ulll(1) << 89));
    constexpr uint getIMOD(uint p) {
        uint res = p;
        for (int i = 0; i < 4; ++i) { res *= 2 - p * res; }
        return -res;}
    constexpr uint getRSQ(uint p) { return uint((1ULL << 32) * ((1ULL << 32) % p) % p); }
    constexpr uint modPow(uint a, uint b, uint p) {
        if (b == 0) { return 1; }
        a %= p; uint res = a;
        for (int i = std::bit_width(b) - 2; i >= 0; i--) {
            res = uint(ulng(res) * res % p);
            if ((b >> i) & 1) { res = uint(ulng(res) * a % p); }}
        return res;}

    template<uint MOD, uint GEN>
    struct FastNTT {
        static constexpr uint IMOD = getIMOD(MOD);
        static constexpr uint RSQ = getRSQ(MOD);

        static inline uint red(ulng x) {
            uint q = uint(x) * IMOD, res = uint((x + ulng(q) * MOD) >> 32);
            return res - (res >= MOD) * MOD;}
        static inline uint init(uint a) { return red(ulng(a) * RSQ); }
        static inline uint mul(uint a, uint b) { return red(ulng(a) * b); }
        static inline __m256i vred(__m256i x0, __m256i x1) {
            __m256i vmod32 = _mm256_set1_epi32(MOD);
            __m256i vmod64 = _mm256_set1_epi64x(MOD);
            __m256i vimod = _mm256_set1_epi32(IMOD);
            __m256i q0 = _mm256_mul_epu32(_mm256_mul_epu32(x0, vimod), vmod64);
            __m256i q1 = _mm256_mul_epu32(_mm256_mul_epu32(x1, vimod), vmod64);
            __m256i r0 = _mm256_srli_epi64(_mm256_add_epi64(x0, q0), 32);
            __m256i r1 = _mm256_srli_epi64(_mm256_add_epi64(x1, q1), 32);
            __m256i res = _mm256_blend_epi32(r0, _mm256_slli_epi64(r1, 32), 0xAA);
            __m256i msk = _mm256_cmpeq_epi32(_mm256_max_epu32(res, vmod32), res);
            return _mm256_sub_epi32(res, _mm256_and_si256(msk, vmod32));}
        static inline __m256i vadd(__m256i a, __m256i b) {
            __m256i vmod32 = _mm256_set1_epi32(MOD);
            __m256i res = _mm256_add_epi32(a, b);
            __m256i msk = _mm256_cmpeq_epi32(_mm256_max_epu32(res, vmod32), res);
            return _mm256_sub_epi32(res, _mm256_and_si256(msk, vmod32));}
        static inline __m256i vsub(__m256i a, __m256i b) {
            __m256i res = _mm256_sub_epi32(a, b);
            return _mm256_add_epi32(res, _mm256_and_si256(_mm256_srai_epi32(res, 31), _mm256_set1_epi32(MOD)));}
        static inline __m256i vmul(__m256i a, __m256i b) {
            __m256i x0 = _mm256_mul_epu32(a, b);
            __m256i x1 = _mm256_mul_epu32(_mm256_srli_epi64(a, 32), _mm256_srli_epi64(b, 32));
            return vred(x0, x1);}
        static inline __m256i vbutterfly(__m256i x) {
            __m256i y = _mm256_shuffle_epi32(x, 0xB1);
            __m256i add = vadd(x, y), sub = _mm256_shuffle_epi32(vsub(x, y), 0xA0);
            return _mm256_blend_epi32(add, sub, 0xAA);}

        static inline vector<uint> rt, irt;
        static void calc(int n) {
            assert(n > 0 && n <= (1 << 26) && std::has_single_bit(uint(n)) &&
                   "InfInt NTT error: Length must be a power of two at most 2^26.");
            if (rt.empty()) { rt = {init(0), init(1)}; irt = {init(0), init(1)}; }
            if (rt.size() < n) {
                int m = int(rt.size()); rt.resize(n); irt.resize(n);
                for (int len = m; len < n; len <<= 1) {
                    uint w0 = init(modPow(GEN, (MOD - 1) / (2 * len), MOD));
                    for (int i = len / 2; i < len; ++i) {
                        uint e = rt[i], o = mul(e, w0);
                        rt[2 * i] = e; rt[2 * i + 1] = o;
                        if (2 * i == len) { irt[len] = e; }
                        else { irt[3 * len - 2 * i] = MOD - e; }
                        irt[3 * len - 2 * i - 1] = MOD - o;}}}}
        static void ntt(vector<uint> &a, bool is_inv) {
            assert(!a.empty() && a.size() <= (1U << 26) && std::has_single_bit(a.size()) &&
                   "InfInt NTT error: Length must be a power of two at most 2^26.");
            int n = int(a.size()); calc(n);
            uint *A = a.data();
            const uint *W = is_inv ? irt.data() : rt.data();
            if (!is_inv) {
                if (n < 8) {
                    for (int len = n / 2; len > 0; len >>= 1) {
                        for (int i = 0; i < n; i += 2 * len) {
                            for (int j = 0; j < len; j++) {
                                uint u = A[i + j], v = A[i + j + len], w = W[len + j];
                                A[i + j]       = u + v - (u + v >= MOD) * MOD;
                                A[i + j + len] = mul(u - v + (u < v) * MOD, w);}}}}
                else {
                    for (int len = n / 2; len >= 8; len >>= 1) {
                        for (int i = 0; i < n; i += 2 * len) {
                            for (int j = 0; j < len; j += 8) {
                                __m256i u = _mm256_loadu_si256((__m256i*)(&A[i + j]));
                                __m256i v = _mm256_loadu_si256((__m256i*)(&A[i + j + len]));
                                __m256i w = _mm256_loadu_si256((__m256i*)(&W[len + j]));
                                _mm256_storeu_si256((__m256i*)(&A[i + j]), vadd(u, v));
                                _mm256_storeu_si256((__m256i*)(&A[i + j + len]), vmul(vsub(u, v), w));}}}
                    __m256i w2 = _mm256_broadcastsi128_si256(_mm_shuffle_epi32(
                                 _mm_loadl_epi64((const __m128i*)(&W[2])), 0x44));
                    __m256i w4 = _mm256_broadcastsi128_si256(
                                 _mm_loadu_si128((const __m128i*)(&W[4])));
                    for (int i = 0; i < n; i += 8) {
                        __m256i x = _mm256_loadu_si256((__m256i*)(&A[i]));
                        __m256i y = _mm256_permute2x128_si256(x, x, 0x01);
                        __m256i add = vadd(x, y), dif = vsub(x, y);
                        __m256i sub = _mm256_permute2x128_si256(dif, dif, 0x00);
                        _mm256_storeu_si256((__m256i*)(&A[i]), _mm256_blend_epi32(add, vmul(sub, w4), 0xF0));}
                    for (int i = 0; i < n; i += 8) {
                        __m256i x = _mm256_loadu_si256((__m256i*)(&A[i]));
                        __m256i y = _mm256_shuffle_epi32(x, 0x4E);
                        __m256i add = vadd(x, y);
                        __m256i sub = _mm256_shuffle_epi32(vsub(x, y), 0x44);
                        _mm256_storeu_si256((__m256i*)(&A[i]), _mm256_blend_epi32(add, vmul(sub, w2), 0xCC));}
                    for (int i = 0; i < n; i += 8) {
                        __m256i x = _mm256_loadu_si256((__m256i*)(&A[i]));
                        _mm256_storeu_si256((__m256i*)(&A[i]), vbutterfly(x));}}}
            else {
                if (n < 8) {
                    for (int len = 1; len < n; len <<= 1) {
                        for (int i = 0; i < n; i += 2 * len) {
                            for (int j = 0; j < len; j++) {
                                uint u = A[i + j], vw = mul(A[i + j + len], W[len + j]);
                                A[i + j]       = u + vw - (u + vw >= MOD) * MOD;
                                A[i + j + len] = u - vw + (u < vw) * MOD;}}}}
                else {
                    __m256i w2 = _mm256_broadcastsi128_si256(_mm_shuffle_epi32(
                                 _mm_loadl_epi64((const __m128i*)(&W[2])), 0x44));
                    __m256i w4 = _mm256_broadcastsi128_si256(
                                 _mm_loadu_si128((const __m128i*)(&W[4])));
                    for (int i = 0; i < n; i += 8) {
                        __m256i x = _mm256_loadu_si256((__m256i*)(&A[i]));
                        _mm256_storeu_si256((__m256i*)(&A[i]), vbutterfly(x));}
                    for (int i = 0; i < n; i += 8) {
                        __m256i x = _mm256_loadu_si256((__m256i*)(&A[i]));
                        __m256i u = _mm256_shuffle_epi32(x, 0x44);
                        __m256i v = _mm256_shuffle_epi32(x, 0xEE);
                        __m256i vw = vmul(v, w2);
                        _mm256_storeu_si256((__m256i*)(&A[i]), _mm256_blend_epi32(vadd(u, vw), vsub(u, vw), 0xCC));}
                    for (int i = 0; i < n; i += 8) {
                        __m256i x = _mm256_loadu_si256((__m256i*)(&A[i]));
                        __m256i u = _mm256_permute2x128_si256(x, x, 0x00);
                        __m256i v = _mm256_permute2x128_si256(x, x, 0x11);
                        __m256i vw = vmul(v, w4);
                        _mm256_storeu_si256((__m256i*)(&A[i]), _mm256_blend_epi32(vadd(u, vw), vsub(u, vw), 0xF0));}
                    for (int len = 8; len < n; len <<= 1) {
                        for (int i = 0; i < n; i += 2 * len) {
                            for (int j = 0; j < len; j += 8) {
                                __m256i u = _mm256_loadu_si256((__m256i*)(&A[i + j]));
                                __m256i v = _mm256_loadu_si256((__m256i*)(&A[i + j + len]));
                                __m256i w = _mm256_loadu_si256((__m256i*)(&W[len + j]));
                                __m256i vw = vmul(v, w);
                                _mm256_storeu_si256((__m256i*)(&A[i + j]), vadd(u, vw));
                                _mm256_storeu_si256((__m256i*)(&A[i + j + len]), vsub(u, vw));}}}}
                uint ninv = init(MOD - ((MOD - 1) >> std::countr_zero(uint(n))));
                __m256i vninv = _mm256_set1_epi32(ninv);
                int i = 0;
                for (; i + 7 < n; i += 8) {
                    __m256i u = _mm256_loadu_si256((__m256i*)(&A[i]));
                    _mm256_storeu_si256((__m256i*)(&A[i]), vmul(u, vninv));}
                for (; i < n; ++i) { A[i] = mul(A[i], ninv); }}}};
    static void multiply(const vector<uint> &a, const vector<uint> &b, vector<uint> &res) {
        using E1 = FastNTT<P1, G1>;
        using E2 = FastNTT<P2, G2>;
        using E3 = FastNTT<P3, G3>;
        constexpr ulng P1P2 = ulng(P1) * P2;
        constexpr uint IP1_P2 = modPow(P1, P2 - 2, P2);
        constexpr uint IP1P2_P3 = modPow(P1P2 % P3, P3 - 2, P3);

        assert(!a.empty() && !b.empty() && "InfInt NTT error: Empty multiplicand.");
        bool is_square = &a == &b || (a.size() == b.size() && a == b);
        ulng req64 = ulng(a.size()) + b.size() - 1;
        assert(req64 <= (1U << 26) && "InfInt NTT error: Transform length exceeds 2^26.");
        int req = int(req64), len = int(std::bit_ceil(uint(req)));
        static vector<uint> a1, b1, a2, b2, a3, b3;
        a1.resize(len); a2.resize(len); a3.resize(len);
        std::fill(a1.begin() + a.size(), a1.end(), 0);
        std::fill(a2.begin() + a.size(), a2.end(), 0);
        std::fill(a3.begin() + a.size(), a3.end(), 0);
        if (!is_square) {
            b1.resize(len); b2.resize(len); b3.resize(len);
            std::fill(b1.begin() + b.size(), b1.end(), 0);
            std::fill(b2.begin() + b.size(), b2.end(), 0);
            std::fill(b3.begin() + b.size(), b3.end(), 0);}
        for (int i = 0; i < a.size(); i++) {
            a1[i] = a[i] % P1; a2[i] = a[i] % P2; a3[i] = a[i] % P3;}
        if (!is_square) {
            for (int i = 0; i < b.size(); i++) {
                b1[i] = b[i] % P1; b2[i] = b[i] % P2; b3[i] = b[i] % P3;}}
        E1::ntt(a1, false); E2::ntt(a2, false); E3::ntt(a3, false);
        __m256i r1 = _mm256_set1_epi32(E1::RSQ);
        __m256i r2 = _mm256_set1_epi32(E2::RSQ);
        __m256i r3 = _mm256_set1_epi32(E3::RSQ);
        int i = 0;
        if (!is_square) {
            E1::ntt(b1, false); E2::ntt(b2, false); E3::ntt(b3, false);
            for (; i + 7 < len; i += 8) {
                __m256i x1 = _mm256_loadu_si256((__m256i*)(&a1[i])), y1 = _mm256_loadu_si256((__m256i*)(&b1[i]));
                __m256i x2 = _mm256_loadu_si256((__m256i*)(&a2[i])), y2 = _mm256_loadu_si256((__m256i*)(&b2[i]));
                __m256i x3 = _mm256_loadu_si256((__m256i*)(&a3[i])), y3 = _mm256_loadu_si256((__m256i*)(&b3[i]));
                y1 = E1::vmul(y1, r1); y2 = E2::vmul(y2, r2); y3 = E3::vmul(y3, r3);
                _mm256_storeu_si256((__m256i*)(&a1[i]), E1::vmul(x1, y1));
                _mm256_storeu_si256((__m256i*)(&a2[i]), E2::vmul(x2, y2));
                _mm256_storeu_si256((__m256i*)(&a3[i]), E3::vmul(x3, y3));}
            for (; i < len; i++) {
                a1[i] = uint(ulng(a1[i]) * b1[i] % P1);
                a2[i] = uint(ulng(a2[i]) * b2[i] % P2);
                a3[i] = uint(ulng(a3[i]) * b3[i] % P3);}}
        else {
            for (; i + 7 < len; i += 8) {
                __m256i x1 = _mm256_loadu_si256((__m256i*)(&a1[i]));
                __m256i x2 = _mm256_loadu_si256((__m256i*)(&a2[i]));
                __m256i x3 = _mm256_loadu_si256((__m256i*)(&a3[i]));
                _mm256_storeu_si256((__m256i*)(&a1[i]), E1::vmul(x1, E1::vmul(x1, r1)));
                _mm256_storeu_si256((__m256i*)(&a2[i]), E2::vmul(x2, E2::vmul(x2, r2)));
                _mm256_storeu_si256((__m256i*)(&a3[i]), E3::vmul(x3, E3::vmul(x3, r3)));}
            for (; i < len; i++) {
                a1[i] = uint(ulng(a1[i]) * a1[i] % P1);
                a2[i] = uint(ulng(a2[i]) * a2[i] % P2);
                a3[i] = uint(ulng(a3[i]) * a3[i] % P3);}}
        E1::ntt(a1, true); E2::ntt(a2, true); E3::ntt(a3, true);
        res.resize(req); ulng carry = 0;
        for (int i = 0; i < req; ++i) {
            uint v1 = a1[i], v1_P2 = v1 - P2;
            v1_P2 += (0U - (v1_P2 >> 31)) & P2;
            ulng v2 = ulng(a2[i] + P2 - v1_P2) * IP1_P2 % P2;
            ulng v3 = (a3[i] + P3 - (v1 + v2 * P1) % P3) * IP1P2_P3 % P3;
            ulll exact = v1 + v2 * P1 + ulll(v3) * P1P2; exact += carry;
            res[i] = uint(exact); carry = ulng(exact >> 32);}
        while (carry) { res.push_back(uint(carry)); carry >>= 32; }}
}
#endif

// T: O(n) or O(n * log(n)), M: O(n)
struct InfInt {
    static constexpr const char *DIG = "0123456789ABCDEF";
    static constexpr ulng NTT_P = 18446744069414584321ULL, NTT_G = 7;
    static constexpr array<uint, 10000> itos10 = []() {
        array<uint, 10000> v{};
        for (int i = 0; i < 10000; i++) {
            int a = i / 1000 % 10, b = i / 100 % 10, c = i / 10 % 10, d = i % 10;
            v[i] = uint(a + (b << 8) + (c << 16) + (d << 24) + 0x30303030);}
        return v;}();
    template<typename T>
    static constexpr bool isFastScalar = std::is_integral_v<T> && sizeof(T) <= 8 &&
                                         !std::same_as<std::remove_cv_t<T>, bool>;

    int sgn; bool is_inf = false;
    vector<uint> n;
    // T: O(1)
    InfInt() : sgn(1) {}
    // T: O(1)
    template<typename T>
    requires std::is_integral_v<T> || std::is_same_v<T, lll> || std::is_same_v<T, ulll>
    InfInt(T N) { *this = N; }
    // T: O(n) or O(n * log(n)^2)
    InfInt(std::string_view N, uint base = 10) { read(N, base); }
    // T: O(n) or O(n * log(n)^2)
    InfInt(const vector<uint> &a, uint base = 10, int sgn = 1) { init(a, base, sgn); }

    // T: O(1)
    explicit operator lng() const {
        if (is_inf) { return sgn == 1 ? LLONG_MAX : LLONG_MIN; }
        if (isNil()) { return 0; }
        ulng res = ulng(n[0]);
        if (n.size() > 1) { res |= ulng(n[1]) << 32; }
        return sgn == 1 ? lng(res) : lng(-res);}
    // T: O(1)
    explicit operator ulng() const {
        if (is_inf) { return sgn == 1 ? ULLONG_MAX : 0; }
        if (isNil()) { return 0; }
        ulng res = ulng(n[0]);
        if (n.size() > 1) { res |= ulng(n[1]) << 32; }
        return sgn == 1 ? res : -res;}
    // T: O(1)
    static vector<pair<InfInt, InfInt>> &getBpow2(uint base) {
        assert(base >= 2 && "InfInt base error: Base must be >= 2.");
        static vector<pair<InfInt, InfInt>> b10pow2;
        static unordered_map<uint, vector<pair<InfInt, InfInt>>> bspow2;
        auto &res = base == 10 ? b10pow2 : bspow2[base];
        if (res.empty()) {
            ulng b = ulng(base);
            while (b * base <= 0xFFFFFFFFULL) { b *= base; }
            InfInt b_norm = b; b_norm <<= __builtin_clz(uint(b));
            InfInt b_inv = newtonRaphsonInv(b_norm);
            res.push_back({std::move(b), std::move(b_inv)});}
        return res;}
    // T: O(n * log(n)^2)
    void initFromBlks(vector<uint> &blks, uint base, int sgn) {
        assert(base >= 2 && "InfInt base error: Base must be >= 2.");
        assert((sgn == 1 || sgn == -1) && "InfInt sign error: Sign must be -1 or 1.");
        ulng B = ulng(base);
        while (B * base <= 0xFFFFFFFFULL) { B *= base; }
        int p = 0;
        while ((1U << p) < blks.size()) { p++; }
        blks.resize(1U << p, 0);
        auto &pows = getBpow2(base);
        while (pows.size() <= p) {
            InfInt b = pows.back().first * pows.back().first;
            InfInt b_norm = b; b_norm <<= __builtin_clz(b.n.back());
            InfInt b_inv = newtonRaphsonInv(b_norm);
            pows.push_back({std::move(b), std::move(b_inv)});}
        auto build = [&](auto &&build, int l, int r, int p) -> InfInt {
            if (l == r) { return InfInt(blks[l]); }
            if (r - l + 1 <= 256) {
                InfInt res = 0; res.n.reserve(r - l + 2);
                for (int i = r; i >= l; i--) {
                    res *= B; ulng carry = ulng(blks[i]);
                    for (int j = 0; j < res.n.size() && carry; j++) {
                        carry += res.n[j]; res.n[j] = uint(carry); carry >>= 32;}
                    while (carry) { res.n.push_back(uint(carry)); carry >>= 32; }}
                res.trim();
                return res;}
            int md = std::midpoint(l, r);
            InfInt L = build(build, l,      md, p - 1);
            InfInt R = build(build, md + 1, r,  p - 1);
            if (R.isNil()) { return L; }
            if (L.isNil()) { R *= pows[p - 1].first; return R; }
            R *= pows[p - 1].first;
            int req = int(max(R.n.size(), L.n.size())) + 1; R.n.reserve(req);
            R += std::move(L);
            return R;};
        *this = build(build, 0, int((1U << p) - 1), p);
        this->sgn = isNil() ? 1 : sgn;}
    // T: O(n) or O(n * log(n)^2)
    void init(const vector<uint> &a, uint base = 10, int sgn = 1) {
        assert(base >= 2 && "InfInt base error: Base must be >= 2.");
        assert((sgn == 1 || sgn == -1) && "InfInt sign error: Sign must be -1 or 1.");
        is_inf = false;
        if (a.empty()) { *this = 0; return; }
        if (base == 2 || base == 8 || base == 16) {
            int len = int(a.size());
            if (base == 2) {
                n.assign((len + 31) >> 5, 0);
                for (int i = 0; i < len; i++) {
                    n[i >> 5] |= (a[len - i - 1] << (i & 31));}}
            else if (base == 8) {
                n.assign((3 * len + 31) >> 5, 0);
                int bits = 0, j = 0; ulng cur = 0;
                for (int i = 0; i < len; i++) {
                    cur |= ulng(a[len - i - 1]) << bits; bits += 3;
                    if (bits >= 32) {
                        n[j++] = uint(cur & 0xFFFFFFFFULL);
                        cur >>= 32; bits -= 32;}}
                if (bits > 0 && j < n.size()) { n[j] = uint(cur); }}
            else if (base == 16) {
                n.assign((len + 7) >> 3, 0);
                for (int i = 0; i < len; i++) {
                    n[i >> 3] |= a[len - i - 1] << ((i & 7) << 2);}}
            trim(); this->sgn = isNil() ? 1 : sgn;
            return;}
        int gsz = 1;
        for (ulng i = ulng(base); i * base <= 0xFFFFFFFFULL; i *= base) { gsz++; }
        static vector<uint> blks; blks.clear(); blks.reserve((a.size() + gsz - 1) / gsz);
        for (int i = int(a.size()) - 1; i >= 0; i -= gsz) {
            uint cur = 0;
            for (int j = max(i - gsz + 1, 0); j <= i; j++) { cur = cur * base + a[j]; }
            blks.push_back(cur);}
        initFromBlks(blks, base, sgn);}
    // T: O(n) or O(n * log(n)^2)
    void read(std::string_view s, uint base = 10) {
        assert(base >= 2 && "InfInt base error: Base must be >= 2.");
        sgn = 1; is_inf = false; n.clear();
        if (s.empty()) { *this = 0; return; }
        int pos = 0;
        while (pos < s.size() && (s[pos] == '+' || s[pos] == '-')) {
            if (s[pos++] == '-') { sgn = -sgn; }}
        if (pos == s.size()) { *this = 0; return; }
        if (s.substr(pos) == "INF" || s.substr(pos) == "inf") { is_inf = true; return; }
        if (base == 2 || base == 8 || base == 16) {
            int len = int(s.size()) - pos;
            if (base == 2) {
                n.assign((len + 31) >> 5, 0);
                for (int i = 0; i < len; i++) {
                    n[i >> 5] |= uint(s[s.size() - i - 1] - '0') << (i & 31);}}
            else if (base == 8) {
                n.assign((3 * len + 31) >> 5, 0);
                int bits = 0, j = 0; ulng cur = 0;
                for (int i = 0; i < len; i++) {
                    cur |= ulng(s[s.size() - i - 1] - '0') << bits; bits += 3;
                    if (bits >= 32) {
                        n[j++] = uint(cur & 0xFFFFFFFFULL);
                        cur >>= 32; bits -= 32;}}
                if (bits > 0 && j < n.size()) { n[j] = uint(cur); }}
            else if (base == 16) {
                n.assign((len + 7) >> 3, 0);
                int i = 0;
                #ifdef __AVX2__
                __m256i v0f = _mm256_set1_epi8(0x0F), v9 = _mm256_set1_epi8('9');
                __m256i w16 = _mm256_set1_epi16(0x0110), w256 = _mm256_set1_epi32(0x00010100);
                __m256i rev = _mm256_setr_epi32(6, 4, 2, 0, 0, 0, 0, 0);
                for (; i + 32 <= len; i += 32) {
                    __m256i x = _mm256_loadu_si256((const __m256i*)(s.data() + s.size() - i - 32));
                    x = _mm256_add_epi8(_mm256_and_si256(x, v0f),
                                        _mm256_and_si256(_mm256_cmpgt_epi8(x, v9), _mm256_set1_epi8(9)));
                    x = _mm256_madd_epi16(_mm256_maddubs_epi16(x, w16), w256);
                    __m256i y = _mm256_or_si256(_mm256_slli_epi32(x, 16), _mm256_srli_epi64(x, 32));
                    y = _mm256_permutevar8x32_epi32(y, rev);
                    _mm_storeu_si128((__m128i*)(&n[i >> 3]), _mm256_castsi256_si128(y));}
                #endif
                for (; i + 8 <= len; i += 8) {
                    ulng cur; std::memcpy(&cur, s.data() + s.size() - i - 8, 8);
                    cur = (cur & 0x0F0F0F0F0F0F0F0FULL) + 9 * ((cur & 0x4040404040404040ULL) >> 6);
                    cur = ((cur << 4) | (cur >> 8)) & 0x00FF00FF00FF00FFULL;
                    cur = ((cur << 8) | (cur >> 16)) & 0x0000FFFF0000FFFFULL;
                    n[i >> 3] = uint((cur << 16) | (cur >> 32));}
                for (; i < len; i++) {
                    char c = s[s.size() - i - 1];
                    n[i >> 3] |= ((c & 0x0F) + 9 * ((c & 0x40) > 0)) << ((i & 7) << 2);}}
            trim(); sgn = isNil() ? 1 : sgn;
            return;}
        int gsz = 1;
        for (ulng i = ulng(base); i * base <= 0xFFFFFFFFULL; i *= base) { gsz++; }
        static vector<uint> blks, digs; blks.clear();
        if (base <= 16) {
            blks.reserve((s.size() - pos + gsz - 1) / gsz);
            for (int i = int(s.size()); i > pos; i -= gsz) {
                uint cur = 0;
                for (int j = max(i - gsz, pos); j < i; j++) {
                    char c = s[j]; uint d = 0;
                    if ('0' <= c && c <= '9') { d = c - '0'; }
                    else if ('A' <= c && c <= 'F') { d = 10 + (c - 'A'); }
                    else if ('a' <= c && c <= 'f') { d = 10 + (c - 'a'); }
                    cur = cur * base + d;}
                blks.push_back(cur);}}
        else {
            if (pos < s.size() && s[pos] == '[') { pos++; }
            digs.clear(); bool chk = false; uint cur = 0;
            for (; pos < s.size(); pos++) {
                char c = s[pos];
                if (c >= '0' && c <= '9') { chk = true; cur = 10 * cur + (c - '0'); }
                else if (c == ',' || c == ']') {
                    if (chk) { digs.push_back(cur); chk = false; cur = 0; }
                    if (c == ']') { break; }}}
            if (chk) { digs.push_back(cur); }
            blks.reserve((digs.size() + gsz - 1) / gsz);
            for (int i = int(digs.size()) - 1; i >= 0; i -= gsz) {
                cur = 0;
                for (int j = max(i - gsz + 1, 0); j <= i; j++) { cur = cur * base + digs[j]; }
                blks.push_back(cur);}}
        initFromBlks(blks, base, sgn);}
    // T: O(n) or O(n * log(n)^2)
    string toString(uint base = 10) const {
        assert(base >= 2 && "InfInt base error: Base must be >= 2.");
        if (is_inf) { return (sgn == -1 ? "-inf" : "inf"); }
        if (isNil()) { return "0"; }
        if (base == 2) { return bin(*this); }
        if (base == 8) { return oct(*this); }
        if (base == 16) { return hex(*this); }
        int gsz = 1; ulng b = ulng(base);
        while (b * base <= 0xFFFFFFFFULL) { gsz++; b *= base; }
        int p = 0; InfInt ua = abs(*this);
        auto &pows = getBpow2(base);
        while (true) {
            if (p >= pows.size()) {
                InfInt b = pows.back().first * pows.back().first;
                InfInt b_norm = b; b_norm <<= __builtin_clz(b.n.back());
                InfInt b_inv = newtonRaphsonInv(b_norm);
                pows.push_back({std::move(b), std::move(b_inv)});}
            if (pows[p].first > ua) { break; }
            p++;}
        static vector<uint> res; res.clear();
        auto extract = [&](auto &&extract, InfInt a, int p, bool pad) -> void {
            if (a.isNil()) { if (pad) { res.insert(res.end(), 1U << p, 0); } return; }
            if (a.n.size() <= 128) {
                int cnt = 0, id = int(res.size());
                if (base == 10) {
                    while (!a.isNil()) {
                        ulng r = 0;
                        for (int i = int(a.n.size()) - 1; i >= 0; i--) {
                            ulng cur = a.n[i] + (r << 32);
                            ulng q = ulng(18446744074ULL * ulll(cur) >> 64);
                            r = cur - 1000000000ULL * q;
                            if (r >= 1000000000ULL) { q--; r += 1000000000ULL; }
                            a.n[i] = uint(q);}
                        a.trim(); res.push_back(uint(r)); cnt++;}}
                else {
                    while (!a.isNil()) {
                        ulng r = 0;
                        for (int i = int(a.n.size()) - 1; i >= 0; i--) {
                            ulng cur = a.n[i] + (r << 32);
                            a.n[i] = uint(cur / b); r = cur % b;}
                        a.trim(); res.push_back(uint(r)); cnt++;}}
                reverse(res.begin() + id, res.end());
                if (pad) { res.insert(res.begin() + id, (1U << p) - cnt, 0); }
                return;}
            auto [q, r] = divMod(std::move(a), pows[p - 1].first, &pows[p - 1].second);
            if (!q.isNil() || pad) {
                extract(extract, std::move(q), p - 1, pad);
                extract(extract, std::move(r), p - 1, true);}
            else { extract(extract, std::move(r), p - 1, pad); }};
        extract(extract, std::move(ua), p, false);
        if (base == 10) {
            if (res.empty()) { return "0"; }
            string s; s.reserve(9 * res.size() + 2);
            if (sgn == -1) { s += '-'; }
            s += std::to_string(res[0]); char buf[12];
            for (int i = 1; i < res.size(); i++) {
                uint cur = res[i];
                uint lo = cur % 10000, md = cur / 10000 % 10000, hi = cur / 100000000;
                buf[0] = char('0' + hi);
                std::memcpy(buf + 1, &itos10[md], 4);
                std::memcpy(buf + 5, &itos10[lo], 4);
                s.append(buf, 9);}
            return s;}
        static vector<uint> digs; digs.clear(); digs.reserve(res.size() * gsz);
        for (int i = 0; i < res.size(); i++) {
            uint cur = res[i]; uint buf[32];
            for (int j = gsz - 1; j >= 0; j--) { buf[j] = cur % base; cur /= base; }
            int pos = 0;
            if (i == 0) {
                while (pos < gsz && buf[pos] == 0) { pos++; }
                if (pos == gsz && res.size() == 1) { pos--; }}
            for (int j = pos; j < gsz; j++) { digs.push_back(buf[j]); }}
        if (digs.empty()) { digs.push_back(0); }
        if (base <= 16) {
            string s; s.reserve(digs.size() + 1);
            if (sgn == -1) { s += '-'; }
            for (uint d : digs) { s += DIG[d]; }
            return s;}
        else {
            base--; int w = 3;
            while (base >= 10) { base /= 10; w++; }
            string s; s.reserve(digs.size() * w + 2);
            if (sgn == -1) { s += '-'; }
            s += '[';
            for (uint d : digs) { s += std::to_string(d); s += ", "; }
            s.pop_back(); s.pop_back(); s += ']';
            return s;}}
    void trim() {
        if (is_inf) { n.clear(); return; }
        if (n.empty()) { n.push_back(0); sgn = 1; return; }
        int pos = int(n.size()) - 1;
        while (pos > 0 && n[pos] == 0) { pos--; }
        if (pos + 1 < n.size()) { n.resize(pos + 1); }
        if (n.size() == 1 && n[0] == 0) { sgn = 1; }}
    // T: O(1)
    template<typename T>
    requires std::is_integral_v<T> || std::is_same_v<T, lll> || std::is_same_v<T, ulll>
    InfInt &operator=(T N) {
        sgn = N < 0 ? -1 : 1; is_inf = false;
        ulll uN = N < 0 ? -ulll(N) : ulll(N);
        n.assign(1, uint(uN));
        while (uN >>= 32) { n.push_back(uint(uN)); }
        return *this;}
    // T: O(n * log(n)^2)
    InfInt &operator=(std::string_view N) { read(N); return *this; }

    InfInt &operator++() {
        if (is_inf) { return *this; }
        if (sgn == -1) { sgn = 1; --(*this); sgn = isNil() ? 1 : -1; return *this; }
        for (int i = 0; i < n.size(); i++) { if (++n[i] != 0) { return *this; } }
        n.push_back(1);
        return *this;}
    InfInt &operator--() {
        if (is_inf) { return *this; }
        if (isNil()) { n.assign(1, 1); sgn = -1; return *this; }
        if (sgn == -1) { sgn = 1; ++(*this); sgn = -1; return *this; }
        for (int i = 0; i < n.size(); i++) {
            if (n[i]-- != 0) {
                if (n.size() > 1 && n.back() == 0) { n.pop_back(); }
                return *this;}}
        return *this;}
    InfInt operator++(int) { InfInt res = *this; ++(*this); return res; }
    InfInt operator--(int) { InfInt res = *this; --(*this); return res; }
    void addMag(const vector<uint> &o) {
        if (is_inf || o.empty()) { return; }
        int n_len = int(n.size()), o_len = int(o.size());
        if (n_len < o_len) { n.resize(o_len, 0); n_len = o_len; }
        ulng carry = 0;
        for (int i = 0; i < o_len; i++) {
            ulng cur = ulng(n[i]) + o[i] + carry;
            n[i] = uint(cur); carry = cur >> 32;}
        for (int i = o_len; i < n_len && carry; i++) { if (++n[i] != 0) { carry = 0; } }
        if (carry) { n.push_back(1); }}
    void subMag(const vector<uint> &o) {
        if (is_inf || o.empty()) { return; }
        int n_len = int(n.size()), o_len = int(o.size());
        ulng carry = 0;
        for (int i = 0; i < o_len; i++) {
            ulng cur = ulng(n[i]) - o[i] - carry;
            n[i] = uint(cur); carry = cur >> 63;}
        for (int i = o_len; i < n_len && carry; i++) { if (n[i]-- != 0) { carry = 0; } }
        trim();}
    void rsubMag(const vector<uint> &o) {
        if (is_inf || o.empty()) { return; }
        int o_len = int(o.size()); n.resize(o_len, 0);
        ulng carry = 0;
        for (int i = 0; i < o_len; i++) {
            ulng cur = ulng(o[i]) - n[i] - carry;
            n[i] = uint(cur); carry = cur >> 63;}
        trim();}
    InfInt &operator+=(const InfInt &o) {
        if (is_inf || o.is_inf) {
            if (is_inf && o.is_inf && sgn != o.sgn) { return *this = 0; }
            if (o.is_inf) { sgn = o.sgn; is_inf = true; }
            n.clear();
            return *this;}
        if (o.isNil()) { return *this; }
        if (isNil()) { return *this = o; }
        if (sgn == o.sgn) { addMag(o.n); return *this; }
        auto cmp = magCmp(*this, o);
        if (cmp == 0) { return *this = 0; }
        if (cmp > 0) { subMag(o.n); }
        else { rsubMag(o.n); sgn = o.sgn; }
        return *this;}
    InfInt &operator-=(const InfInt &o) {
        if (is_inf || o.is_inf) {
            if (is_inf && o.is_inf && sgn == o.sgn) { return *this = 0; }
            if (o.is_inf) { sgn = -o.sgn; is_inf = true; }
            n.clear();
            return *this;}
        if (o.isNil()) { return *this; }
        if (isNil()) { *this = o; sgn = -sgn; return *this; }
        if (sgn != o.sgn) { addMag(o.n); return *this; }
        auto cmp = magCmp(*this, o);
        if (cmp == 0) { return *this = 0; }
        if (cmp > 0) { subMag(o.n); }
        else { rsubMag(o.n); sgn = -sgn; }
        return *this;}

    template<typename T> requires isFastScalar<T>
    static pair<int, ulng> sgnMag(T o) {
        using U = std::make_unsigned_t<std::remove_cv_t<T>>;
        if constexpr (std::is_signed_v<std::remove_cv_t<T>>) {
            if (o < 0) { return {-1, ulng(U(0) - U(o))}; }}
        return {1, ulng(U(o))};}
    template<typename T> requires isFastScalar<T>
    InfInt &operator*=(T o) {
        auto [o_sgn, uo] = sgnMag(o);
        if (isNil() || uo == 0) { return *this = 0; }
        if (is_inf) { sgn *= o_sgn; n.clear(); return *this; }
        sgn *= o_sgn;
        if (uo == 1) { return *this; }
        if (std::has_single_bit(uo)) { return *this <<= std::countr_zero(uo); }
        int len = int(n.size()); ulng carry = 0;
        if (uo <= 0xFFFFFFFFULL) {
            uint uo32 = uint(uo);
            for (int i = 0; i < len; i++) {
                ulng cur = ulng(n[i]) * uo32 + carry;
                n[i] = uint(cur); carry = ulng(cur >> 32);}
            if (carry) { n.push_back(uint(carry)); }}
        else {
            for (int i = 0; i < len; i++) {
                ulll cur = ulll(n[i]) * uo + carry;
                n[i] = uint(cur); carry = ulng(cur >> 32);}
            while (carry) { n.push_back(uint(carry)); carry >>= 32; }}
        return *this;}
    // T: O(n^1.585)
    static void karatsuba(int len, const ulng *__restrict a,     const ulng *__restrict b,     ulll *__restrict res,
                                         ulng *__restrict tmp_a,       ulng *__restrict tmp_b, ulll *__restrict tmp_res) {
        if (len <= 32) {
            std::fill(res, res + 2 * len, 0);
            for (int i = 0; i < len; i++) {
                for (int j = 0; j < len; j++) { res[i + j] += ulll(a[i]) * b[j]; }}
            return;}
        int m = len / 2;
        karatsuba(m, a,     b,     res,         tmp_a, tmp_b, tmp_res);
        karatsuba(m, a + m, b + m, res + 2 * m, tmp_a, tmp_b, tmp_res);
        for (int i = 0; i < m; i++) {
            tmp_a[i] = a[i] + a[i + m];
            tmp_b[i] = b[i] + b[i + m];}
        karatsuba(m, tmp_a, tmp_b, tmp_res, tmp_a + m, tmp_b + m, tmp_res + 2 * m);
        for (int i = 0; i < 2 * m; i++) { tmp_res[i] -= res[i] + res[i + 2 * m]; }
        for (int i = 0; i < 2 * m; i++) { res[i + m] += tmp_res[i]; }}
    // T: O(n^1.585)
    static void karatsubaSquare(int len, const ulng *__restrict a,     ulll *__restrict res,
                                               ulng *__restrict tmp_a, ulll *__restrict tmp_res) {
        if (len <= 32) {
            std::fill(res, res + 2 * len, 0);
            for (int i = 0; i < len; i++) {
                res[2 * i] += ulll(a[i]) * a[i];
                for (int j = i + 1; j < len; j++) { res[i + j] += ulll(2) * a[i] * a[j]; }}
            return;}
        int m = len / 2;
        karatsubaSquare(m, a,     res,         tmp_a, tmp_res);
        karatsubaSquare(m, a + m, res + 2 * m, tmp_a, tmp_res);
        for (int i = 0; i < m; i++) { tmp_a[i] = a[i] + a[i + m]; }
        karatsubaSquare(m, tmp_a, tmp_res, tmp_a + m, tmp_res + 2 * m);
        for (int i = 0; i < 2 * m; i++) { tmp_res[i] -= res[i] + res[i + 2 * m]; }
        for (int i = 0; i < 2 * m; i++) { res[i + m] += tmp_res[i]; }}
    // T: O(1)
    static inline ulng modP(ulll a) {
        ulng l = ulng(a), hl = uint(a >> 64), hh = ulng(a >> 96);
        ulll cur = ulll(l) + (ulll(hl) << 32); ulng sub = hl + hh;
        cur += (cur < sub) * NTT_P; cur -= sub;
        ulng res = ulng(cur);
        res += ulng(0xFFFFFFFFULL * (cur >> 64)); res -= (res >= NTT_P) * NTT_P;
        return res;}
    // T: O(1)
    static ulng nttPow(ulng a, ulng b) {
        if (b == 0) { return 1; }
        a = modP(a); ulng res = a;
        for (int i = std::bit_width(b) - 2; i >= 0; i--) {
            res = modP(ulll(res) * res);
            if ((b >> i) & 1) { res = modP(ulll(res) * a); }}
        return res;}
    static void ntt(vector<ulng> &a, bool is_inv) {
        assert(!a.empty() && std::has_single_bit(a.size()) && a.size() <= (1U << 30) &&
               "InfInt NTT error: Length must be a power of two at most 2^30.");
        int n = int(a.size());
        static vector<ulng> w = {0, 1};
        if (w.size() < n) {
            int m = int(w.size()); w.resize(n);
            for (int len = m; len < n; len <<= 1) {
                ulng w0 = nttPow(NTT_G, (NTT_P - 1) / (2 * len));
                for (int i = len / 2; i < len; i++) {
                    w[2 * i] = w[i];
                    w[2 * i + 1] = modP(ulll(w[i]) * w0);}}}
        if (!is_inv) {
            for (int len = n / 2; len > 0; len >>= 1) {
                for (int i = 0; i < n; i += 2 * len) {
                    for (int j = 0; j < len; j++) {
                        ulng u = a[i + j], v = a[i + j + len], uv = u + v;
                        a[i + j]       = uv - ((uv < u) | (uv >= NTT_P)) * NTT_P;
                        a[i + j + len] = modP(ulll(u - v + (u < v) * NTT_P) * w[len + j]);}}}}
        else {
            for (int len = 1; len < n; len <<= 1) {
                for (int i = 0; i < n; i += 2 * len) {
                    for (int j = 0; j < len; j++) {
                        ulng iw = j == 0 ? 1 : NTT_P - w[2 * len - j];
                        ulng u = a[i + j], v = modP(ulll(a[i + j + len]) * iw), uv = u + v;
                        a[i + j]       = uv - ((uv < u) | (uv >= NTT_P)) * NTT_P;
                        a[i + j + len] = u - v + (u < v) * NTT_P;}}}
            ulng n_inv = NTT_P - ((NTT_P - 1) >> std::countr_zero(uint(n)));
            for (ulng &x : a) { x = modP(ulll(x) * n_inv); }}}
    InfInt &operator*=(const InfInt &o) {
        if (isNil() || o.isNil()) { return *this = 0; }
        if (is_inf || o.is_inf) { sgn *= o.sgn; is_inf = true; n.clear(); return *this; }
        if (o.n.size() <= 2) {
            int o_sgn = o.sgn;
            ulng uo = o.n[0] | (o.n.size() == 2 ? ulng(o.n[1]) << 32 : 0);
            *this *= uo; sgn *= o_sgn;
            return *this;}
        if (n.size() <= 2) {
            int a_sgn = sgn;
            ulng ua = n[0] | (n.size() == 2 ? ulng(n[1]) << 32 : 0);
            *this = o; *this *= ua; sgn *= a_sgn;
            return *this;}
        int n_len = int(n.size()), o_len = int(o.n.size());
        bool is_square = this == &o || (n_len >= 16 && n_len == o_len && n == o.n); sgn *= o.sgn;
        bool use_naive = min(n_len, o_len) < 64; int len = 1, req;
        if (!use_naive) {
        #ifdef __AVX2__
        ulng req64 = ulng(n_len) + o_len - 1;
        if (req64 > (1U << 26)) { use_naive = true; }
        else {
            req = int(req64); len = int(std::bit_ceil(uint(req)));
            use_naive = !(n_len == 64 && o_len == 64) &&
                        ulng(n_len) * o_len <= ulng(4) * len * std::countr_zero(uint(len));}
        #else
        if (max(n_len, o_len) < 1024) {
            req = max(n_len, o_len); len = int(std::bit_ceil(uint(req)));
            ulng cost = 1024;
            for (int i = 32; i < len; i <<= 1) { cost *= 3; }
            if (is_square) {
                use_naive = len <= 32 || ulng(2) * n_len * n_len + ulng(48) * len <= ulng(3) * cost;}
            else {
                use_naive = !(n_len == 64 && o_len == 64) &&
                            ulng(2) * n_len * o_len <= ulng(3) * cost;}}
        else {
            ulng req64 = ulng(2) * (ulng(n_len) + o_len) - 1;
            if (req64 > (1U << 30)) { use_naive = true; }
            else {
                req = int(req64); len = int(std::bit_ceil(uint(req)));
                use_naive = ulng(n_len) * o_len <= ulng(10) * len * std::countr_zero(uint(len));}}
        #endif
        }
        if (use_naive) {
            if (is_square && n_len >= 16) {
                static vector<ulll> sq;
                sq.assign(2 * n_len, 0);
                for (int i = 0; i < n_len; i++) {
                    sq[2 * i] += ulll(n[i]) * n[i];
                    for (int j = i + 1; j < n_len; j++) { sq[i + j] += ulll(2) * n[i] * n[j]; }}
                n.resize(2 * n_len); ulll carry = 0;
                for (int i = 0; i < 2 * n_len; i++) {
                    ulll cur = sq[i] + carry;
                    n[i] = uint(cur); carry = cur >> 32;}
                assert(carry == 0);
                trim();
                return *this;}
            static vector<uint> res;
            res.assign(n_len + o_len, 0);
            const uint *n_ptr = n.data(); const uint *o_ptr = o.n.data();
            uint *res_ptr = res.data();
            for (int i = 0; i < n_len; i++) {
                uint ni = n_ptr[i];
                if (!ni) { continue; }
                ulng carry = 0;
                for (int j = 0; j < o_len; j++) {
                    ulng cur = ulng(res_ptr[i + j]) + ulng(ni) * o_ptr[j] + carry;
                    res_ptr[i + j] = uint(cur); carry = cur >> 32;}
                res_ptr[i + o_len] = uint(carry);}
            n.swap(res); trim();
            return *this;}

        #ifdef __AVX2__
        if (max(n_len, o_len) < 65) {
        #else
        if (max(n_len, o_len) < 1024) {
        #endif
            req = max(n_len, o_len); len = int(std::bit_ceil(uint(req)));
            static vector<ulng> a, b, tmp_a, tmp_b;
            static vector<ulll> res, tmp_res;
            a.resize(len); std::copy(n.begin(), n.end(), a.begin());
            std::fill(a.begin() + n_len, a.end(), 0); tmp_a.resize(len);
            res.resize(2 * len); tmp_res.resize(2 * len);
            if (is_square) { karatsubaSquare(len, a.data(), res.data(), tmp_a.data(), tmp_res.data()); }
            else {
                b.resize(len); std::copy(o.n.begin(), o.n.end(), b.begin());
                std::fill(b.begin() + o_len, b.end(), 0); tmp_b.resize(len);
                karatsuba(len, a.data(), b.data(), res.data(), tmp_a.data(), tmp_b.data(), tmp_res.data());}
            n.resize(n_len + o_len);
            ulng carry = 0;
            for (int i = 0; i < n.size(); i++) {
                ulll cur = res[i] + carry;
                n[i] = uint(cur); carry = ulng(cur >> 32);}
            trim();
            return *this;}

        #ifdef __AVX2__
        FastIintMul::multiply(n, is_square ? n : o.n, n);
        #else
        ulng req64 = ulng(2) * (ulng(n_len) + o_len) - 1;
        assert(req64 <= (1U << 30) && "InfInt multiplication error: Transform length exceeds 2^30.");
        req = int(req64); len = int(std::bit_ceil(uint(req)));
        static vector<ulng> fa, fb;
        fa.resize(len);
        for (int i = 0; i < n_len; i++) { fa[2 * i] = n[i] & 0xFFFF; fa[2 * i + 1] = n[i] >> 16; }
        std::fill(fa.begin() + 2 * n_len, fa.end(), 0);
        if (!is_square) {
            fb.resize(len);
            for (int i = 0; i < o_len; i++) { fb[2 * i] = o.n[i] & 0xFFFF; fb[2 * i + 1] = o.n[i] >> 16; }
            std::fill(fb.begin() + 2 * o_len, fb.end(), 0);}
        ntt(fa, false);
        if (is_square) {
            for (int i = 0; i < len; i++) { fa[i] = modP(ulll(fa[i]) * fa[i]); }}
        else {
            ntt(fb, false);
            for (int i = 0; i < len; i++) { fa[i] = modP(ulll(fa[i]) * fb[i]); }}
        ntt(fa, true);
        n.resize(n_len + o_len);
        ulng carry = 0;
        for (int i = 0; i < n.size(); i++) {
            ulng cur = fa[2 * i] + carry;
            ulng nxt = (2 * i + 1 < req) * fa[2 * i + 1] + (cur >> 16);
            n[i] = uint((cur & 0xFFFF) | ((nxt & 0xFFFF) << 16)); carry = nxt >> 16;}
        while (carry) { n.push_back(uint(carry & 0xFFFFFFFFULL)); carry >>= 32; }
        #endif
        trim();
        return *this;}

    template<typename T> requires isFastScalar<T>
    InfInt &operator/=(T o) {
        assert(o != 0 && "InfInt scalar division error: Division by zero.");
        auto [o_sgn, uo] = sgnMag(o);
        if (is_inf) { sgn *= o_sgn; n.clear(); return *this; }
        if (isNil()) { return *this; }
        sgn *= o_sgn;
        if (uo == 1) { return *this; }
        if (std::has_single_bit(uo)) {
            int res_sgn = sgn; sgn = 1;
            *this >>= std::countr_zero(uo);
            if (!isNil()) { sgn = res_sgn; }
            return *this;}
        ulng rem = 0;
        if (uo <= 0xFFFFFFFFULL) {
            uint uo32 = uint(uo); ulng mgc = ULLONG_MAX / uo32 + 1;
            for (int i = int(n.size()) - 1; i >= 0; i--) {
                ulng cur = n[i] + (rem << 32);
                ulng qi = ulng(ulll(cur) * mgc >> 64), ri = cur - qi * uo32;
                if (ri >= uo32) { qi--; ri += uo32; }
                n[i] = uint(qi); rem = ri;}}
        else {
            int i = int(n.size()) - 1;
            if (n.size() & 1) { rem = n[i]; n[i--] = 0; }
            for (; i > 0; i -= 2) {
                ulng blk = (ulng(n[i]) << 32) | n[i - 1];
                ulll cur = (ulll(rem) << 64) | blk;
                ulng qi = ulng(cur / uo); rem = ulng(cur % uo);
                n[i] = uint(qi >> 32); n[i - 1] = uint(qi);}}
        trim();
        return *this;}
    template<typename T> requires isFastScalar<T>
    InfInt &operator%=(T o) {
        assert(o != 0 && "InfInt scalar modulo error: Modulo by zero.");
        ulng uo = sgnMag(o).second;
        if (is_inf) { return *this = 0; }
        if (isNil() || uo == 1) { return *this = 0; }
        int a_sgn = sgn; ulng rem = 0;
        if (std::has_single_bit(uo)) {
            rem = n[0] | (n.size() > 1 ? ulng(n[1]) << 32 : 0); rem &= uo - 1;
            *this = rem; 
            if (a_sgn == -1 && !isNil()) { sgn = -1; }
            return *this;}
        if (uo <= 0xFFFFFFFFULL) {
            uint uo32 = uint(uo); ulng mgc = ULLONG_MAX / uo32 + 1;
            for (int i = int(n.size()) - 1; i >= 0; i--) {
                ulng cur = n[i] + (rem << 32);
                ulng qi = ulng(ulll(cur) * mgc >> 64);
                rem = cur - qi * uo32;
                if (rem >= uo32) { rem += uo32; }}}
        else {
            int i = int(n.size()) - 1;
            if (n.size() & 1) { rem = n[i--]; }
            for (; i > 0; i -= 2) {
                ulng blk = (ulng(n[i]) << 32) | n[i - 1];
                rem = ulng(((ulll(rem) << 64) | blk) % uo);}}
        *this = rem;
        if (a_sgn == -1 && !isNil()) { sgn = -1; }
        return *this;}
    // T: O(n^2)
    friend pair<InfInt, InfInt> divModSlow(InfInt a, InfInt b) {
        assert(!b.isNil() && "InfInt divMod error: Division by zero.");
        if (a.is_inf || b.is_inf) {
            if (a.is_inf && b.is_inf) { return {a.sgn * b.sgn, 0}; }
            if (b.is_inf) { return {0, std::move(a)}; }
            a.sgn *= b.sgn;
            return {std::move(a), 0};}
        int a_sgn = a.sgn, b_sgn = b.sgn;
        a.sgn = 1; b.sgn = 1;
        auto cmp = magCmp(a, b);
        if (cmp < 0) { a.sgn = a_sgn; a.trim(); return {0, std::move(a)}; }
        if (cmp == 0) { return {a_sgn * b_sgn, 0}; }
        if (b.n.size() == 1) {
            ulng r = 0; uint d = b.n[0];
            if (d == 1) { a.sgn = a_sgn * b_sgn; return {std::move(a), 0}; }
            ulng mgc = ULLONG_MAX / d + 1;
            for (int i = int(a.n.size()) - 1; i >= 0; i--) {
                ulng cur = (r << 32) | a.n[i];
                ulng qi = ulng(ulll(cur) * mgc >> 64), ri = cur - qi * d;
                if (ri >= d) { qi--; ri += d; }
                a.n[i] = uint(qi); r = ri;}
            a.sgn = a_sgn * b_sgn; a.trim();
            InfInt rem = r; rem.sgn = a_sgn; rem.trim();
            return {std::move(a), std::move(rem)};}
        int norm = __builtin_clz(b.n.back());
        a <<= norm; b <<= norm;
        int n = int(a.n.size()), m = int(b.n.size());
        ulng mgc = ULLONG_MAX / b.n.back() + 1;
        InfInt q, r = std::move(a);
        q.n.assign(n - m + 1, 0); r.n.push_back(0);
        const uint *b_ptr = b.n.data();
        uint *r_ptr = r.n.data(); uint *q_ptr = q.n.data();
        for (int i = n - m; i >= 0; i--) {
            ulng d = ulng(((ulll(r_ptr[i + m]) << 32) | r_ptr[i + m - 1]) * mgc >> 64);
            if (d > 0xFFFFFFFFU) { d = 0xFFFFFFFFU; }
            if (d > 0) {
                ulng carry = 0;
                for (int j = 0; j < m; j++) {
                    ulng sub = ulng(b_ptr[j]) * d + carry;
                    ulng cur = r_ptr[i + j] - (sub & 0xFFFFFFFFULL);
                    r_ptr[i + j] = uint(cur); carry = (sub >> 32) + (cur >> 63);}
                ulng rem = r_ptr[i + m] - carry;
                r_ptr[i + m] = uint(rem);
                while (r_ptr[i + m] != 0) {
                    d--; ulng carry = 0;
                    for (int j = 0; j < m; j++) {
                        ulng cur = ulng(r_ptr[i + j]) + b_ptr[j] + carry;
                        r_ptr[i + j] = uint(cur); carry = cur >> 32;}
                    r_ptr[i + m] += uint(carry);}}
            q_ptr[i] = uint(d);}
        q.sgn = a_sgn * b_sgn; q.trim();
        r.n.resize(m); r.trim();
        r >>= norm; r.sgn = a_sgn; r.trim();
        return {std::move(q), std::move(r)};}
    InfInt &shiftBlocksLeft(int shf) {
        if (is_inf || isNil() || shf <= 0) { return *this; }
        n.insert(n.begin(), shf, 0);
        return *this;}
    InfInt &shiftBlocksRight(int shf) {
        if (is_inf || isNil() || shf <= 0) { return *this; }
        if (shf >= n.size()) { return *this = 0; }
        n.erase(n.begin(), n.begin() + shf);
        return *this;}
    static InfInt newtonRaphsonInv(const InfInt &a) {
        assert(!a.is_inf && a.sgn == 1 && !a.isNil() && (a.n.back() >> 31) &&
               "InfInt reciprocal error: Input must be positive and normalized.");
        int len = int(a.n.size());
        if (len <= 256) {
            InfInt b; b.n.assign(2 * len + 1, 0); b.n.back() = 1;
            return divModSlow(std::move(b), a).first;}
        int md = (len + 2) / 2;
        InfInt a_hi; a_hi.sgn = a.sgn; a_hi.n.assign(a.n.begin() + (len - md), a.n.end());
        InfInt b = newtonRaphsonInv(a_hi), cur;
        cur.n.assign(2 * md + 1, 0); cur.n.back() = 1;
        cur -= std::move(a_hi) * b; cur.shiftBlocksLeft(len - md);
        InfInt a_lo; a_lo.sgn = a.sgn; a_lo.n.assign(a.n.begin(), a.n.begin() + (len - md)); a_lo.trim();
        cur -= std::move(a_lo) * b; cur.shiftBlocksRight(md);
        cur *= b; cur.shiftBlocksRight(md);
        b.shiftBlocksLeft(len - md); b += cur;
        return b;}
    friend pair<InfInt, InfInt> divMod(InfInt a, InfInt b, const InfInt *b_inv = nullptr) {
        assert(!b.isNil() && "InfInt divMod error: Division by zero.");
        if (a.is_inf || b.is_inf) {
            if (a.is_inf && b.is_inf) { return {a.sgn * b.sgn, 0}; }
            if (b.is_inf) { return {0, std::move(a)}; }
            a.sgn *= b.sgn;
            return {std::move(a), 0};}
        int a_sgn = a.sgn, b_sgn = b.sgn;
        a.sgn = 1; b.sgn = 1;
        auto cmp = magCmp(a, b);
        if (cmp < 0) { a.sgn = a_sgn; return {0, std::move(a)}; }
        if (cmp == 0) { return {a_sgn * b_sgn, 0}; }
        int b_pow2 = std::has_single_bit(b.n.back()) ? 0 : -1;
        for (int i = 0; b_pow2 >= 0 && i + 1 < b.n.size(); i++) {
            if (b.n[i] != 0) { b_pow2 = -1; }}
        if (b_pow2 >= 0) {
            b_pow2 = 32 * (int(b.n.size()) - 1) + std::countr_zero(b.n.back());
            if (b_pow2 == 0) { a.sgn = a_sgn * b_sgn; return {std::move(a), 0}; }
            InfInt q = a; q >>= b_pow2;
            int r_len = (b_pow2 + 31) >> 5;
            if (a.n.size() > r_len) { a.n.resize(r_len); }
            int r_bits = b_pow2 & 31;
            if (r_bits > 0) { a.n.back() &= (1U << r_bits) - 1; }
            q.sgn = a_sgn * b_sgn; a.sgn = a_sgn; q.trim(); a.trim();
            return {std::move(q), std::move(a)};}
        if (b.n.size() <= 256 || a.n.size() - b.n.size() <= 256) {
            auto [q, r] = divModSlow(std::move(a), std::move(b));
            q.sgn = a_sgn * b_sgn; r.sgn = a_sgn;
            q.trim(); r.trim();
            return {std::move(q), std::move(r)};}

        int norm = __builtin_clz(b.n.back());
        a <<= norm; b <<= norm;
        int n = int(a.n.size()), m = int(b.n.size()); InfInt q;
        if (b_inv && n <= 2 * m) {
            q = (*b_inv) * a;
            if (q.n.size() > 2 * m) { q.shiftBlocksRight(2 * m); }
            else { q = 0; }}
        else {
            InfInt b_pad = b; b_pad.shiftBlocksLeft(n - m);
            q = newtonRaphsonInv(b_pad) * a;
            if (q.n.size() > n + m) { q.shiftBlocksRight(n + m); }
            else { q = 0; }}
        InfInt r = std::move(a); r -= q * b;
        while (r.sgn == -1) { q--; r += b; }
        while (r >= b) { q++; r -= b; }
        r >>= norm;
        q.sgn = a_sgn * b_sgn; r.sgn = a_sgn;
        q.trim(); r.trim();
        return {std::move(q), std::move(r)};}
    InfInt &operator/=(InfInt o) {
        assert(!o.isNil() && "InfInt division error: Division by zero.");
        if (is_inf || o.is_inf) {
            if (is_inf && o.is_inf) { return *this = sgn * o.sgn; }
            if (o.is_inf) { return *this = 0; }
            sgn *= o.sgn; n.clear();
            return *this;}
        if (isNil()) { return *this; }
        if (o.n.size() <= 2) {
            int o_sgn = o.sgn;
            ulng uo = o.n[0] | (o.n.size() == 2 ? ulng(o.n[1]) << 32 : 0);
            *this /= uo;
            if (o_sgn == -1 && !isNil()) { sgn = -sgn; }
            return *this;}
        return *this = divMod(std::move(*this), std::move(o)).first;}
    InfInt &operator%=(InfInt o) {
        assert(!o.isNil() && "InfInt modulo error: Modulo by zero.");
        if (is_inf) { return *this = 0; }
        if (o.is_inf) { return *this; }
        if (isNil()) { return *this; }
        if (o.n.size() <= 2) {
            ulng uo = o.n[0] | (o.n.size() == 2 ? ulng(o.n[1]) << 32 : 0);
            return *this %= uo;}
        return *this = divMod(std::move(*this), std::move(o)).second;}

    InfInt operator+() const { return *this; }
    InfInt operator-() const & {
        InfInt res = *this;
        if (!res.isNil()) { res.sgn = -res.sgn; }
        return res;}
    // T: O(1)
    InfInt operator-() && {
        if (!isNil()) { sgn = -sgn; }
        return std::move(*this);}
    friend InfInt operator+(InfInt a, const InfInt &b) { a += b; return a; }
    friend InfInt operator-(InfInt a, const InfInt &b) { a -= b; return a; }
    template<typename T> requires isFastScalar<T>
    friend InfInt operator*(InfInt a, T b) { a *= b; return a; }
    template<typename T> requires isFastScalar<T>
    friend InfInt operator*(T b, InfInt a) { a *= b; return a; }
    friend InfInt operator*(InfInt a, const InfInt &b) { a *= b; return a; }
    template<typename T> requires isFastScalar<T>
    friend InfInt operator/(InfInt a, T b) { a /= b; return a; }
    friend InfInt operator/(InfInt a, InfInt b) { a /= std::move(b); return a; }
    template<typename T> requires isFastScalar<T>
    friend InfInt operator%(InfInt a, T b) { a %= b; return a; }
    friend InfInt operator%(InfInt a, InfInt b) { a %= std::move(b); return a; }

    friend InfInt abs(InfInt a) { a.sgn = 1; return a; }
    static bool isIdMat(const InfInt &A, const InfInt &B, const InfInt &C, const InfInt &D) {
        return A == 1 && B.isNil() && C.isNil() && D == 1;}
    static void leftEucMul(InfInt q, InfInt &A, InfInt &B, InfInt &C, InfInt &D) {
        std::swap(A, C); C -= q * A;
        std::swap(B, D); D -= std::move(q) * B;}
    static void backEucMul(InfInt &A, InfInt &B, InfInt &C, InfInt &D, InfInt &a, InfInt &b) {
        InfInt q = B.sgn == -1 ? abs(C) / A : abs(D) / B;
        C += q * A; std::swap(A, C);
        D += q * B; std::swap(B, D);
        b += q * a; std::swap(a, b);}
    template<typename T>
    requires isFastScalar<std::remove_cvref_t<T>> || std::same_as<std::remove_cvref_t<T>, InfInt>
    static void leftMatMul(const T &A, const T &B, const T &C, const T &D, InfInt &a, InfInt &b) {
        InfInt a0 = std::move(a), b0 = std::move(b);
        auto mul = [](const T &k, const InfInt &x) -> InfInt {
            if constexpr (std::same_as<std::remove_cvref_t<T>, InfInt>) {
                return k.isNil() || x.isNil() ? InfInt(0) : k == 1 ? x : k == -1 ? -x : k * x;}
            else if constexpr (std::is_signed_v<std::remove_cvref_t<T>>) {
                return k == 0 || x.isNil() ? InfInt(0) : k == 1 ? x : k == -1 ? -x : x * k;}
            else { return k == 0 || x.isNil() ? InfInt(0) : k == 1 ? x : x * k; }};
        a = mul(A, a0); a += mul(B, b0);
        b = mul(C, a0); b += mul(D, b0);}
    static void adjustHalfGcd(const InfInt &A, const InfInt &B, const InfInt &C, const InfInt &D,
                              int shf, InfInt ha, InfInt hb, InfInt &a, InfInt &b) {
        if (shf == 0) { a = std::move(ha); b = std::move(hb); return; }
        InfInt la, lb;
        la.n.assign(a.n.begin(), a.n.begin() + min(shf, int(a.n.size()))); la.trim();
        lb.n.assign(b.n.begin(), b.n.begin() + min(shf, int(b.n.size()))); lb.trim();
        leftMatMul(A, B, C, D, la, lb);
        ha.shiftBlocksLeft(shf); ha += la;
        hb.shiftBlocksLeft(shf); hb += lb;
        a = std::move(ha); b = std::move(hb);}
    template<bool track_matrix = true>
    static bool lehmerHalfGcd(InfInt &a, InfInt &b, InfInt &A, InfInt &B, InfInt &C, InfInt &D) {
        static constexpr lll LIM = lll(1) << 60;
        assert(!a.is_inf && a.sgn == 1 && !b.is_inf && b.sgn == 1 &&
               "InfInt Lehmer error: Inputs must be finite and nonnegative.");
        if (a.n.size() < 3 || a.n.size() != b.n.size()) { return false; }
        lng cA = 1, cB = 0, cC = 0, cD = 1;
        ulng a_back = (ulng(a.n.back()) << 32) | a.n[a.n.size() - 2];
        ulng b_back = (ulng(b.n.back()) << 32) | b.n[b.n.size() - 2];
        auto bounds = [&](lng x, lng y) -> pair<lll, lll> {
            lll md = lll(x) * a_back + lll(y) * b_back;
            lll lo = md + min<lng>(x, 0) + min<lng>(y, 0);
            lll hi = md + max<lng>(x, 0) + max<lng>(y, 0);
            return pair<lll, lll>{lo, hi};};
        bool changed = false;
        while (true) {
            auto [a_lo, a_hi] = bounds(cA, cB);
            auto [b_lo, b_hi] = bounds(cC, cD);
            if (a_lo <= 0 || b_lo <= 0) { break; }
            lll q = a_lo / b_hi, q_hi = (a_hi - 1) / b_lo;
            if (q <= 0 || q != q_hi || q > LIM) { break; }
            lll nC = lll(cA) - q * cC, nD = lll(cB) - q * cD;
            if (nC < -LIM || nC > LIM || nD < -LIM || nD > LIM) { break; }
            cA = cC; cB = cD; cC = lng(nC); cD = lng(nD);
            changed = true;}
        if (!changed) { return false; }
        leftMatMul(cA, cB, cC, cD, a, b);
        if constexpr (track_matrix) {
            leftMatMul(cA, cB, cC, cD, A, C);
            leftMatMul(cA, cB, cC, cD, B, D);}
        return true;}
    // T: O(n * log(n)^2)
    template<bool track_matrix = true>
    static bool halfGcd(InfInt &a, InfInt &b, InfInt &A, InfInt &B, InfInt &C, InfInt &D) {
        assert(!a.is_inf && a.sgn == 1 && !b.is_inf && b.sgn == 1 && a >= b &&
               "InfInt halfGcd error: Inputs must be finite, nonnegative, and ordered.");
        if constexpr (track_matrix) { A = 1; B = 0; C = 0; D = 1; }
        if (b.isNil()) { return false; }
        int req = (int(a.n.size()) + 1) / 2;
        if (int(b.n.size()) <= req) { return false; }
        bool changed = false;
        auto reduce = [&]() -> void {
            auto [q, r] = divMod(std::move(a), b);
            if constexpr (track_matrix) { leftEucMul(std::move(q), A, B, C, D); }
            a = std::exchange(b, std::move(r));
            changed = true;};
        if (a.n.size() <= 32) {
            while (!b.isNil() && int(b.n.size()) > req) {
                if (lehmerHalfGcd<track_matrix>(a, b, A, B, C, D)) { changed = true; }
                else { reduce(); }}
            return changed;}
        InfInt ha = 0, hb = 0, hA, hB, hC, hD;
        if (req < a.n.size()) { ha.n.assign(a.n.begin() + req, a.n.end()); }
        if (req < b.n.size()) { hb.n.assign(b.n.begin() + req, b.n.end()); }
        halfGcd(ha, hb, hA, hB, hC, hD);
        if (!isIdMat(hA, hB, hC, hD)) {
            adjustHalfGcd(hA, hB, hC, hD, req, std::move(ha), std::move(hb), a, b);
            while ((a.sgn == -1 || b.sgn == -1 || a < b) && !isIdMat(hA, hB, hC, hD)) {
                backEucMul(hA, hB, hC, hD, a, b);}
            if (!isIdMat(hA, hB, hC, hD)) {
                changed = true;
                if constexpr (track_matrix) {
                    A = std::move(hA); B = std::move(hB); C = std::move(hC); D = std::move(hD);}}}
        if (!b.isNil() && int(b.n.size()) > req) { reduce(); }
        if (b.isNil() || int(b.n.size()) <= req) { return changed; }
        int shf = 2 * req - int(a.n.size());
        if (shf < 0) {
            while (!b.isNil() && int(b.n.size()) > req) { reduce(); }
            return changed;}
        ha = 0; hb = 0;
        if (shf < a.n.size()) { ha.n.assign(a.n.begin() + shf, a.n.end()); }
        if (shf < b.n.size()) { hb.n.assign(b.n.begin() + shf, b.n.end()); }
        halfGcd(ha, hb, hA, hB, hC, hD);
        if (!isIdMat(hA, hB, hC, hD)) {
            adjustHalfGcd(hA, hB, hC, hD, shf, std::move(ha), std::move(hb), a, b);
            while ((a.sgn == -1 || b.sgn == -1 || a < b) && !isIdMat(hA, hB, hC, hD)) {
                backEucMul(hA, hB, hC, hD, a, b);}
            if (!isIdMat(hA, hB, hC, hD)) {
                changed = true;
                if constexpr (track_matrix) {
                    leftMatMul(hA, hB, hC, hD, A, C);
                    leftMatMul(hA, hB, hC, hD, B, D);}}}
        while (!b.isNil() && int(b.n.size()) > req) { reduce(); }
        return changed;}
    // T: O(n * log(n)^2)
    friend InfInt exGcd(InfInt a, InfInt b, InfInt &x, InfInt &y) {
        if (a.is_inf) {
            if (b.is_inf || b.isNil()) { x = a.sgn; y = 0; a.sgn = 1; return a; }
            x = 0; y = b.sgn; b.sgn = 1;
            return b;}
        if (b.is_inf) {
            if (a.isNil()) { x = 0; y = b.sgn; b.sgn = 1; return b; }
            x = a.sgn; y = 0; a.sgn = 1;
            return a;}
        if (a.n.size() == 1 && a.n[0] == 1) { x = a.sgn; y = 0; return 1; }
        if (b.n.size() == 1 && b.n[0] == 1) { x = 0; y = b.sgn; return 1; }
        int a_sgn = a.sgn, b_sgn = b.sgn;
        a.sgn = 1; b.sgn = 1;
        x = 1; y = 0; InfInt X = 0, Y = 1;
        while (!b.isNil()) {
            if (a < b) { std::swap(a, b); std::swap(x, X); std::swap(y, Y); }
            if (b.isNil()) { break; }
            if (b.n.size() > 2) {
                InfInt A, B, C, D;
                if (halfGcd(a, b, A, B, C, D)) {
                    leftMatMul(A, B, C, D, x, X);
                    leftMatMul(A, B, C, D, y, Y);
                    continue;}}
            auto [q, r] = divMod(std::move(a), b);
            a = std::exchange(b, std::move(r));
            std::swap(x, X); X -= q * x;
            std::swap(y, Y); Y -= std::move(q) * y;}
        if (a_sgn == -1) { x = -std::move(x); }
        if (b_sgn == -1) { y = -std::move(y); }
        return a;}
    // T: O(n * log(n)^2)
    friend InfInt gcd(InfInt a, InfInt b) {
        if ((a.is_inf && !b.isNil()) || a.isNil()) { b.sgn = 1; return b; }
        if ((b.is_inf && !a.isNil()) || b.isNil()) { a.sgn = 1; return a; }
        if (a.n.size() <= 2 && b.n.size() <= 2) {
            ulng ua = a.n[0] | (a.n.size() == 2 ? ulng(a.n[1]) << 32 : 0);
            ulng ub = b.n[0] | (b.n.size() == 2 ? ulng(b.n[1]) << 32 : 0);
            return std::gcd(ua, ub);}
        a.sgn = 1; b.sgn = 1;
        int a_shf = ctz(a), b_shf = ctz(b), shf = min(a_shf, b_shf);
        a >>= a_shf; b >>= b_shf;
        while (!b.isNil()) {
            if (a < b) { std::swap(a, b); }
            if (b.n.size() > 256) {
                InfInt A, B, C, D;
                if (halfGcd<false>(a, b, A, B, C, D)) { continue; }
                a %= b;
                continue;}
            if (a.n.size() > b.n.size() + 64) { a %= b; continue; }
            int a_ext = ctz(a), b_ext = ctz(b), ext = min(a_ext, b_ext);
            a >>= a_ext; b >>= b_ext;
            while (!b.isNil()) {
                if (a > b) { std::swap(a, b); }
                b -= a;
                if (!b.isNil()) { b >>= ctz(b); }}
            a <<= shf + ext;
            return a;}
        a <<= shf;
        return a;}
    // T: O(n * log(n)^2)
    friend InfInt inv(InfInt a, InfInt mod) {
        assert(!a.is_inf && !mod.is_inf && !mod.isNil() &&
               "InfInt inv error: Inputs must be finite and modulus must be nonzero.");
        mod.sgn = 1;
        if (mod == 1) { return 0; }
        a %= mod;
        if (a.sgn == -1) { a += mod; }
        if (a.isNil()) { return -1; }
        if (a == 1) { return 1; }
        if (a.n.size() <= 2 && mod.n.size() <= 2) {
            ulng umod = mod.n[0] | (mod.n.size() == 2 ? ulng(mod.n[1]) << 32 : 0);
            ulng r0 = ulng(a), r1 = umod; lll x0 = 1, x1 = 0;
            while (r1 != 0) {
                ulng q = r0 / r1;
                r0 = std::exchange(r1, r0 - q * r1);
                x0 = std::exchange(x1, x0 - lll(q) * x1);}
            if (r0 != 1) { return -1; }
            x0 %= lll(umod);
            if (x0 < 0) { x0 += lll(umod); }
            return ulng(x0);}
        InfInt x, y, g = exGcd(std::move(a), mod, x, y);
        if (g != 1) { return -1; }
        x %= mod; 
        if (x.sgn == -1) { x += mod; }
        return x;}
    // T: O(n * log(n)^2)
    friend InfInt lcm(InfInt a, InfInt b) {
        if (a.isNil() || b.isNil()) { return 0; }
        if (a.is_inf || b.is_inf) { return InfInt("inf"); }
        InfInt g = gcd(a, b);
        InfInt res = (std::move(a) / std::move(g)) * b; res.sgn = 1;
        return res;}
    friend InfInt pow(InfInt a, lng b) {
        assert(b >= 0 && "InfInt pow error: Exponent must be >= 0.");
        if (b == 0) { return 1; }
        if (a.is_inf) {
            if (!(b & 1)) { a.sgn = 1; }
            return a;}
        if (b == 1 || a.isNil() || a == 1) { return a; }
        if (a == -1) { return b & 1 ? -1 : 1; }
        InfInt res = a;
        for (int i = std::bit_width(ulng(b)) - 2; i >= 0; i--) {
            res *= res;
            if ((b >> i) & 1) { res *= a; }}
        return res;}
    // T: O(n^2 * log(n))
    friend InfInt powMod(InfInt a, InfInt b, InfInt mod) {
        assert(!a.is_inf && !b.is_inf && !mod.is_inf && !mod.isNil() &&
               "InfInt powMod error: Inputs must be finite and modulus must be nonzero.");
        if (mod == 1 || mod == -1) { return 0; }
        if (b.isNil()) { return 1; }
        mod.sgn = 1; a %= mod;
        if (a.sgn == -1) { a += mod; }
        if (a == 1) { return 1; }
        if (b.sgn == -1) {
            a = inv(a, mod);
            if (a == -1) { return -1; }
            b.sgn = 1;}
        if (a.isNil() || b == 1) { return a; }
        int bits = 32 * int(b.n.size()) - __builtin_clz(b.n.back());
        auto getBit = [&](int i) -> int { return int((b.n[i >> 5] >> (i & 31)) & 1); };
        int len = 1, best = INT_MAX;
        for (int i = 1; i <= min(bits, 8); i++) {
            int l = max(bits - i, 0);
            while (!getBit(l)) { l++; }
            int cost = (1 << (i - 1)) - (i == 1) + l;
            for (int j = l - 1; j >= 0;) {
                if (!getBit(j)) { j--; continue; }
                int k = max(j - i + 1, 0);
                while (!getBit(k)) { k++; }
                cost++; j = k - 1;}
            if (cost < best) { len = i; best = cost; }}
        auto winPow = [&]<typename T>(T base, auto &&mulMod) -> T {
            if (len == 1) {
                T res = base;
                for (int i = bits - 2; i >= 0; i--) {
                    mulMod(res, res);
                    if (getBit(i)) { mulMod(res, base); }}
                return res;}
            vector<T> odd(1 << (len - 1)); odd[0] = std::move(base);
            if (odd.size() > 1) {
                T cur = odd[0]; mulMod(cur, cur);
                for (int i = 1; i < odd.size(); i++) { odd[i] = odd[i - 1]; mulMod(odd[i], cur); }}
            int l = max(bits - len, 0);
            while (!getBit(l)) { l++; }
            int k = 0;
            for (int j = bits - 1; j >= l; j--) { k = (k << 1) | getBit(j); }
            T res = odd[k >> 1];
            for (int i = l - 1; i >= 0;) {
                if (!getBit(i)) { mulMod(res, res); i--; continue; }
                l = max(i - len + 1, 0);
                while (!getBit(l)) { l++; }
                k = 0;
                for (int j = i; j >= l; j--) { k = (k << 1) | getBit(j); }
                for (int j = l; j <= i; j++) { mulMod(res, res); }
                mulMod(res, odd[k >> 1]); i = l - 1;}
            return res;};
        if (mod.n.size() <= 2) {
            ulng umod = mod.n[0] | (mod.n.size() == 2 ? ulng(mod.n[1]) << 32 : 0);
            auto mulMod = [&](ulng &x, const ulng &y) -> void { x = ulng(ulll(x) * y % umod); };
            return winPow(ulng(a), mulMod);}
        InfInt b_inv; const InfInt *b_inv_ptr = nullptr;
        if (mod.n.size() > 256) {
            InfInt b_norm = mod << __builtin_clz(mod.n.back());
            b_inv = newtonRaphsonInv(b_norm); b_inv_ptr = &b_inv;}
        auto mulMod = [&](InfInt &x, const InfInt &y) -> void {
            x *= y; x = divMod(std::move(x), mod, b_inv_ptr).second;};
        return winPow(std::move(a), mulMod);}
    // T: O(n * log(n)^2)
    friend InfInt sqrt(const InfInt &a) {
        assert(a.sgn == 1 && "InfInt sqrt error: Cannot compute square root of negative number.");
        if (a.isNil()) { return 0; }
        if (a.is_inf) { return a; }
        if (a.n.size() <= 2) {
            ulng ua = a.n[0] | (a.n.size() == 2 ? ulng(a.n[1]) << 32 : 0);
            ulng res = ulng(std::sqrt((long double)(ua)));
            while (ulll(res) * res > ua) { res--; }
            while (ulll(res + 1) * (res + 1) <= ua) { res++; }
            return res;}
        InfInt res = 1; res <<= (32 * int(a.n.size()) - __builtin_clz(a.n.back()) + 1) / 2;
        while (true) {
            InfInt nxt = a / res; nxt += res; nxt >>= 1;
            if (nxt >= res) { break; }
            res = std::move(nxt);}
        return res;}
    static InfInt rand(int bits) {
        if (bits <= 0) { return 0; }
        static std::mt19937 rng(uint(std::chrono::steady_clock::now().time_since_epoch().count()));
        int q = bits >> 5, r = bits & 31;
        InfInt res; res.sgn = 1; res.n.assign(q + (r > 0), 0);
        for (int i = 0; i < q; i++) { res.n[i] = uint(rng() & 0xFFFFFFFFU); }
        if (r > 0) { res.n.back() = uint(rng() & ((1U << r) - 1)) | (1U << (r - 1)); }
        else { res.n.back() |= (1U << 31); }
        res.trim();
        return res;}

    // T: O(1)
    bool isNil() const { return !is_inf && (n.empty() || (n.size() == 1 && n[0] == 0)); }
    // T: O(1)
    explicit operator bool() const { return !isNil(); }
    // T: O(1)
    bool operator!() const { return isNil(); }
    friend std::strong_ordering magCmp(const InfInt &a, const InfInt &b) {
        if (a.is_inf && b.is_inf) { return std::strong_ordering::equal; }
        if (a.is_inf || b.is_inf) { return a.is_inf ? std::strong_ordering::greater : std::strong_ordering::less; }
        if (a.isNil() || b.isNil()) { return int(!a.isNil()) <=> int(!b.isNil()); }
        if (a.n.size() != b.n.size()) { return a.n.size() <=> b.n.size(); }
        for (int i = int(a.n.size()) - 1; i >= 0; i--) {
            if (a.n[i] != b.n[i]) { return a.n[i] <=> b.n[i]; }}
        return std::strong_ordering::equal;}
    friend std::strong_ordering operator<=>(const InfInt &a, const InfInt &b) {
        if (a.is_inf && b.is_inf) { return a.sgn <=> b.sgn; }
        if (a.is_inf || b.is_inf) { return a.sgn * a.is_inf <=> b.sgn * b.is_inf; }
        if (a.isNil() || b.isNil()) { return int(!a.isNil()) * a.sgn <=> int(!b.isNil()) * b.sgn; }
        if (a.sgn != b.sgn) { return a.sgn <=> b.sgn; }
        return a.sgn == 1 ? magCmp(a, b) : magCmp(b, a);}
    friend bool operator==(const InfInt &a, const InfInt &b) {
        if (a.is_inf || b.is_inf) { return a.sgn == b.sgn && a.is_inf == b.is_inf; }
        if (a.isNil() && b.isNil()) { return true; }
        return a.sgn == b.sgn && a.n == b.n;}

    InfInt &operator<<=(int shf) {
        assert(shf >= 0 && "InfInt left shift error: Shift must be >= 0.");
        if (is_inf || isNil() || shf == 0) { return *this; }
        int len = int(n.size()), q = shf >> 5, r = shf & 31;
        if (r == 0) { n.insert(n.begin(), q, 0); return *this; }
        else {
            n.resize(len + q + 1, 0);
            for (int i = len - 1; i >= 0; i--) {
                ulng cur = ulng(n[i]) << r;
                n[i + q + 1] |= uint(cur >> 32);
                n[i + q] = uint(cur);}
            for (int i = 0; i < q; i++) { n[i] = 0; }}
        trim();
        return *this;}
    InfInt &operator>>=(int shf) {
        assert(shf >= 0 && "InfInt right shift error: Shift must be >= 0.");
        if (is_inf || isNil() || shf == 0) { return *this; }
        int len = int(n.size()), q = shf >> 5, r = shf & 31;
        if (q >= len) { return *this = sgn == -1 ? -1 : 0; }
        bool carry = false;
        if (sgn == -1) {
            for (int i = 0; i < q; i++) { if (n[i] > 0) { carry = true; break; } }
            carry |= ((r > 0) & ((n[q] & ((1U << r) - 1)) > 0));}
        if (r == 0) { n.erase(n.begin(), n.begin() + q); }
        else {
            for (int i = 0; i < len - q; i++) {
                ulng cur = i + q + 1 < len ? ulng(n[i + q + 1]) : 0;
                n[i] = uint((n[i + q] >> r) | (cur << (32 - r)));}
            n.resize(len - q);}
        if (sgn == -1 && carry) { --(*this); }
        if (r > 0) { trim(); }
        return *this;}
    vector<uint> convTwosComp() const {
        assert(!is_inf && "InfInt two's complement error: Conversion of infinity.");
        if (sgn == 1) { return n; }
        vector<uint> res; res.reserve(n.size() + 1); ulng carry = 1;
        for (uint x : n) {
            ulng cur = ulng(~x) + carry;
            res.push_back(uint(cur)); carry = cur >> 32;}
        if (carry) { res.push_back(uint(carry)); }
        return res;}
    static InfInt fromTwosComp(vector<uint> blks, int ext) {
        int sgn = 1;
        if (ext != 0) {
            sgn = -1; ulng carry = 1;
            for (int i = 0; i < blks.size(); i++) {
                ulng cur = ulng(~blks[i]) + carry;
                blks[i] = uint(cur); carry = cur >> 32;}
            if (carry) { blks.push_back(uint(carry)); }}
        InfInt res; res.sgn = sgn; res.n = std::move(blks); res.trim();
        return res;}
    template<typename F>
    static InfInt bitOpTwosComp(InfInt a, const InfInt &b, F ope) {
        bool a_neg = a.sgn == -1, b_neg = b.sgn == -1;
        bool res_neg = ope(a_neg ? 0xFFFFFFFFU : 0, b_neg ? 0xFFFFFFFFU : 0) != 0;
        int a_len = int(a.n.size()), b_len = int(b.n.size()), len = max(a_len, b_len) + 1;
        ulng a_carry = a_neg, b_carry = b_neg, res_carry = res_neg;
        a.n.resize(len);
        for (int i = 0; i < len; i++) {
            uint a_i = i < a_len ? a.n[i] : 0, b_i = i < b_len ? b.n[i] : 0;
            if (a_neg) { ulng cur = ulng(~a_i) + a_carry; a_i = uint(cur); a_carry = cur >> 32; }
            if (b_neg) { ulng cur = ulng(~b_i) + b_carry; b_i = uint(cur); b_carry = cur >> 32; }
            uint cur = ope(a_i, b_i);
            if (res_neg) {
                ulng mag = ulng(~cur) + res_carry; a.n[i] = uint(mag); res_carry = mag >> 32;}
            else { a.n[i] = cur; }}
        if (res_carry) { a.n.push_back(uint(res_carry)); }
        a.sgn = res_neg ? -1 : 1; a.trim();
        return a;}
    friend InfInt operator&(InfInt a, const InfInt &b) {
        assert(!a.is_inf && !b.is_inf && "InfInt bitwise AND error: Bitwise AND on infinity.");
        if (a.sgn == 1 && b.sgn == 1) {
            a.n.resize(min(a.n.size(), b.n.size()));
            for (int i = 0; i < a.n.size(); i++) { a.n[i] &= b.n[i]; }
            a.trim(); return a;}
        return bitOpTwosComp(std::move(a), b, std::bit_and<uint>());}
    friend InfInt operator^(InfInt a, const InfInt &b) {
        assert(!a.is_inf && !b.is_inf && "InfInt bitwise XOR error: Bitwise XOR on infinity.");
        if (a.sgn == 1 && b.sgn == 1) {
            a.n.resize(max(a.n.size(), b.n.size()), 0);
            for (int i = 0; i < b.n.size(); i++) { a.n[i] ^= b.n[i]; }
            a.trim(); return a;}
        return bitOpTwosComp(std::move(a), b, std::bit_xor<uint>());}
    friend InfInt operator|(InfInt a, const InfInt &b) {
        assert(!a.is_inf && !b.is_inf && "InfInt bitwise OR error: Bitwise OR on infinity.");
        if (a.sgn == 1 && b.sgn == 1) {
            a.n.resize(max(a.n.size(), b.n.size()), 0);
            for (int i = 0; i < b.n.size(); i++) { a.n[i] |= b.n[i]; }
            a.trim(); return a;}
        return bitOpTwosComp(std::move(a), b, std::bit_or<uint>());}

    InfInt operator~() const {
        assert(!is_inf && "InfInt bitwise NOT error: Bitwise NOT on infinity.");
        InfInt res = -(*this); --res;
        return res;}
    friend InfInt operator<<(InfInt a, int shf) { a <<= shf; return a; }
    friend InfInt operator>>(InfInt a, int shf) { a >>= shf; return a; }
    InfInt &operator&=(const InfInt &o) {
        assert(!is_inf && !o.is_inf && "InfInt bitwise AND error: Bitwise AND on infinity.");
        if (this == &o) { return *this; }
        return *this = std::move(*this) & o;}
    InfInt &operator^=(const InfInt &o) {
        assert(!is_inf && !o.is_inf && "InfInt bitwise XOR error: Bitwise XOR on infinity.");
        if (this == &o) { return *this = 0; }
        return *this = std::move(*this) ^ o;}
    InfInt &operator|=(const InfInt &o) {
        assert(!is_inf && !o.is_inf && "InfInt bitwise OR error: Bitwise OR on infinity.");
        if (this == &o) { return *this; }
        return *this = std::move(*this) | o;}

    friend string bin(const InfInt &a) {
        if (a.is_inf) { return (a.sgn == -1 ? "-inf" : "inf"); }
        if (a.isNil()) { return "0"; }
        int len = 32 * (int(a.n.size()) - 1) + 32 - __builtin_clz(a.n.back()) + (a.sgn == -1);
        string res(len, '0');
        if (a.sgn == -1) { res[0] = '-'; }
        for (int i = 0; i < a.n.size(); i++) {
            uint cur = a.n[i];
            int cnt = i + 1 == a.n.size() ? 32 - __builtin_clz(cur) : 32;
            for (int j = 0; j < cnt; j++) { res[--len] = char('0' + (cur & 1)); cur >>= 1; }}
        return res;}
    friend string oct(const InfInt &a) {
        if (a.is_inf) { return (a.sgn == -1 ? "-inf" : "inf"); }
        if (a.isNil()) { return "0"; }
        int len = (32 * (int(a.n.size()) - 1) + 34 - __builtin_clz(a.n.back())) / 3;
        string res(len + (a.sgn == -1), '0');
        if (a.sgn == -1) { res[0] = '-'; }
        for (int i = 0; i < len; i++) {
            int j = (3 * i) >> 5, shf = (3 * i) & 31;
            ulng cur = ulng(a.n[j]) >> shf;
            if (shf > 29 && j + 1 < a.n.size()) { cur |= ulng(a.n[j + 1]) << (32 - shf); }
            res[int(res.size()) - i - 1] = char('0' + (cur & 7));}
        return res;}
    friend string hex(const InfInt &a) {
        if (a.is_inf) { return (a.sgn == -1 ? "-inf" : "inf"); }
        if (a.isNil()) { return "0"; }
        string res; res.reserve(8 * a.n.size() + 1);
        int i = 0;
        #ifdef __AVX2__
        __m256i m16 = _mm256_set1_epi64x(0x0000FFFF0000FFFFULL);
        __m256i m8  = _mm256_set1_epi64x(0x00FF00FF00FF00FFULL);
        __m256i m4  = _mm256_set1_epi64x(0x0F0F0F0F0F0F0F0FULL);
        for (; i + 3 < a.n.size(); i += 4) {
            __m256i cur = _mm256_cvtepu32_epi64(_mm_loadu_si128((const __m128i*)(&a.n[i])));
            cur = _mm256_and_si256(_mm256_or_si256(cur, _mm256_slli_epi64(cur, 16)), m16);
            cur = _mm256_and_si256(_mm256_or_si256(cur, _mm256_slli_epi64(cur, 8)),  m8);
            cur = _mm256_and_si256(_mm256_or_si256(cur, _mm256_slli_epi64(cur, 4)),  m4);
            __m256i alpha = _mm256_cmpgt_epi8(cur, _mm256_set1_epi8(9));
            cur = _mm256_add_epi8(
                  _mm256_add_epi8(cur, _mm256_set1_epi8('0')),
                  _mm256_and_si256(alpha, _mm256_set1_epi8(7)));
            char buf[32]; _mm256_storeu_si256((__m256i*)(buf), cur);
            res.append(buf, 32);}
        #endif
        for (; i < a.n.size(); i++) {
            ulng cur = ulng(a.n[i]);
            cur = (cur | (cur << 16)) & 0x0000FFFF0000FFFFULL;
            cur = (cur | (cur << 8))  & 0x00FF00FF00FF00FFULL;
            cur = (cur | (cur << 4))  & 0x0F0F0F0F0F0F0F0FULL;
            cur = (cur + 0x3030303030303030ULL) +
                  7 * (((cur + 0x3636363636363636ULL) & 0x4040404040404040ULL) >> 6);
            char buf[8]; std::memcpy(buf, &cur, 8);
            res.append(buf, 8);}
        while (res.size() > 1 && res.back() == '0') { res.pop_back(); }
        if (a.sgn == -1) { res += '-'; }
        reverse(res.begin(), res.end());
        return res;}
    friend int ctz(const InfInt &a) {
        assert(!a.is_inf && "InfInt ctz error: ctz on infinity.");
        if (a.isNil()) { return 0; }
        int cnt = 0;
        for (uint x : a.n) {
            if (x == 0) { cnt += 32; }
            else { cnt += __builtin_ctz(x); break; }}
        return cnt;}
    friend int popcount(const InfInt &a) {
        assert(!a.is_inf && "InfInt popcount error: popcount on infinity.");
        int cnt = 0, i = 0;
        #ifdef __AVX2__
        __m256i lut = _mm256_broadcastsi128_si256(
                      _mm_setr_epi8(0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4));
        __m256i msk = _mm256_set1_epi8(0x0F), acc = _mm256_setzero_si256();
        for (; i + 7 < a.n.size(); i += 8) {
            __m256i x = _mm256_loadu_si256((const __m256i*)(&a.n[i]));
            __m256i cur = _mm256_add_epi8(
                          _mm256_shuffle_epi8(lut, _mm256_and_si256(x, msk)),
                          _mm256_shuffle_epi8(lut, _mm256_and_si256(_mm256_srli_epi16(x, 4), msk)));
            acc = _mm256_add_epi64(acc, _mm256_sad_epu8(cur, _mm256_setzero_si256()));}
        alignas(32) ulng res[4]; _mm256_store_si256((__m256i*)(res), acc);
        cnt = int(res[0] + res[1] + res[2] + res[3]);
        #endif
        for (; i < a.n.size(); i++) { cnt += __builtin_popcount(a.n[i]); }
        return cnt;}

    // T: O(1)
    struct SetBase { uint base; SetBase(uint Base) : base(Base) {} };
    // T: O(1)
    static int getBaseId() { static const int id = std::ios_base::xalloc(); return id; }
    // T: O(1)
    friend istream &operator>>(istream &is, SetBase sb) { is.iword(getBaseId()) = long(sb.base); return is; }
    // T: O(n) or O(n * log(n)^2)
    friend istream &operator>>(istream &is, InfInt &a) {
        is >> std::ws;
        if (is.peek() == EOF) { return is; }
        uint base = uint(is.iword(getBaseId()));
        base = base == 0 ? 10 : base;
        if (is.flags() & std::ios_base::oct) { base = 8; }
        else if (is.flags() & std::ios_base::hex) { base = 16; }
        static string s; s.clear();
        is >> s;
        if (s.find('[') != string::npos && s.back() != ']') {
            int c;
            while ((c = is.get()) != EOF) {
                s += char(c);
                if (c == ']') { break; }}}
        a.read(s, base);
        return is;}
    // T: O(1)
    friend ostream &operator<<(ostream &os, SetBase sb) { os.iword(getBaseId()) = long(sb.base); return os; }
    // T: O(n) or O(n * log(n)^2)
    friend ostream &operator<<(ostream &os, const InfInt &a) {
        uint base = uint(os.iword(getBaseId()));
        base = base == 0 ? 10 : base;
        if (os.flags() & std::ios_base::oct) { base = 8; }
        else if (os.flags() & std::ios_base::hex) { base = 16; }
        return os << a.toString(base);}
};
using iint = InfInt;
#line 2 "X-Tests Abbreviated\\Yosupo-BigInteger-04-AddHexBigInt.cpp"

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int t;
    cin >> t;
    while (t--) {
        iint a, b;
        cin >> iint::SetBase(16) >> a >> b;
        a += b;
        cout << iint::SetBase(16) << a << '\n';
    }

    return 0;
}
