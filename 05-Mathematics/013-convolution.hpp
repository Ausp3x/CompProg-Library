#pragma once
#include "../01-Core/01-template.hpp"
#include "../01-Core/05-modint.hpp"

// T: O(sqrt(c) + r * log(p)^2), M: O(1); c = odd part of p - 1, r = candidates tried (at most spf(p) for composite p).
// Smallest primitive root of prime p, 0 when p is detected composite.
constexpr ulng primitiveRootNtt(ulng p) {
    assert(p >= 2);
    if (p < 3) { return p == 2; }
    auto pw = [p](ulng a, ulng e) {
        ulng r = 1;
        for (; e; e >>= 1, a = ulng(ulll(a) * a % p)) { if (e & 1) { r = ulng(ulll(r) * a % p); } }
        return r;};
    ulng f[64] = {2}, m = (p - 1) >> std::countr_zero(p - 1);
    int k = 1;
    for (ulng d = 3; d <= m / d; d += 2) {
        if (m % d) { continue; }
        f[k++] = d;
        while (m % d == 0) { m /= d; }}
    if (m > 1) { f[k++] = m; }
    for (ulng g = 2; g < p; ++g) {
        if (pw(g, p - 1) != 1) { return 0; }
        bool ok = true;
        for (int i = 0; i < k && ok; ++i) { ok = pw(g, (p - 1) / f[i]) != 1; }
        if (ok) { return g; }}
    return 0;}

namespace convolution_detail {
    using cd = std::complex<double>;
    inline constexpr int NAIVE = 60, KARATSUBA = 32;
    inline constexpr uint P1 = 754974721, P2 = 167772161, P3 = 469762049;
    template<typename M> inline constexpr int RANK = std::countr_zero(ulng(M::mod()) - 1);
    template<typename M> inline constexpr ulng ROOT = primitiveRootNtt(ulng(M::mod()));
    template<typename M> inline constexpr lng LIMIT = lng(1) << min(RANK<M>, 30);
    // T: O(1), M: O(1); M is a Core static prime modint with transforms of length 2^20 or more.
    template<typename M>
    constexpr bool nttPrime() {
        if constexpr (StaticModularInt<M>) { return M::is_prime && RANK<M> >= 20; }
        else { return false; }}
    // T: O(log(e)), M: O(1).
    template<typename M>
    M power(M a, ulng e) {
        M r = 1;
        for (; e; e >>= 1, a *= a) { if (e & 1) { r *= a; } }
        return r;}
    // T: O(n * log(n)), M: O(1); n = a.size() power of two, w[h + j] = (2h-th root)^j; dif: natural to bit-reversed, dit: the transpose.
    template<typename T>
    void dif(vector<T> &a, const vector<T> &w) {
        int n = int(a.size());
        for (int h = n / 2; h >= 1; h /= 2) {
            for (int i = 0; i + 2 * h <= n; i += 2 * h) {
                for (int j = 0; j < h; ++j) {
                    T u = a[i + j], v = a[i + j + h];
                    a[i + j] = u + v; a[i + j + h] = (u - v) * w[h + j];}}}}
    template<typename T>
    void dit(vector<T> &a, const vector<T> &w) {
        int n = int(a.size());
        for (int h = 1; h < n; h *= 2) {
            for (int i = 0; i + 2 * h <= n; i += 2 * h) {
                for (int j = 0; j < h; ++j) {
                    T u = a[i + j], v = a[i + j + h] * w[h + j];
                    a[i + j] = u + v; a[i + j + h] = u - v;}}}}
    // S: O(n) on growth, Q: O(1), M: O(n); entry h + j = exp(-+i * pi * j / h), built in long double by halving.
    inline const vector<cd> &fftRootTable(int n, bool inverse) {
        static vector<cd> tab[2];
        auto &t = tab[inverse];
        if (int(t.size()) >= n) { return t; }
        int L = int(std::bit_ceil(uint(n)));
        vector<std::complex<long double>> r(L, 1);
        for (int h = 2; h < L; h *= 2) {
            auto z = std::polar(1.0L, (inverse ? 1 : -1) * std::numbers::pi_v<long double> / h);
            for (int j = 0; j < h; ++j) { r[h + j] = j & 1 ? r[(h + j) / 2] * z : r[(h + j) / 2]; }}
        t.resize(L);
        for (int i = 0; i < L; ++i) { t[i] = cd(double(r[i].real()), double(r[i].imag())); }
        return t;}
    // T: O(n * m^(log2(3) - 1)) for n >= m, M: O(n + m) per level; res[0, n + m - 1) += a * b over any ring.
    template<typename T>
    void karatsuba(const T *a, int n, const T *b, int m, T *res) {
        if (n < m) { std::swap(a, b); std::swap(n, m); }
        if (m <= KARATSUBA) {
            for (int i = 0; i < n; ++i) { for (int j = 0; j < m; ++j) { res[i + j] += a[i] * b[j]; } }
            return;}
        int h = (n + 1) / 2;
        if (m <= h) {
            karatsuba(a, h, b, m, res); karatsuba(a + h, n - h, b, m, res + h);
            return;}
        vector<T> s(a, a + h), t(b, b + h), lo(2 * h - 1), hi(n + m - 2 * h - 1), mid(2 * h - 1);
        for (int i = 0; i < n - h; ++i) { s[i] += a[h + i]; }
        for (int i = 0; i < m - h; ++i) { t[i] += b[h + i]; }
        karatsuba(a, h, b, h, lo.data()); karatsuba(a + h, n - h, b + h, m - h, hi.data()); karatsuba(s.data(), h, t.data(), h, mid.data());
        for (int i = 0; i < 2 * h - 1; ++i) { res[i] += lo[i]; mid[i] -= lo[i]; }
        for (int i = 0; i < int(hi.size()); ++i) { res[2 * h + i] += hi[i]; mid[i] -= hi[i]; }
        for (int i = 0; i < 2 * h - 1; ++i) { res[h + i] += mid[i]; }}
    // T: O(1), M: O(1); six primes below 2^31 with 2^24 | p - 1, whose product exceeds 2^173.
    inline constexpr uint PRIMES[6] = {P1, P2, P3, 2013265921, 1811939329, 2113929217};
    // S: O(1) at compile time, M: O(1); w[j][t] = P_1 ... P_t mod P_(j+1), inv[j] = (P_1 ... P_j)^-1 mod P_(j+1).
    struct GarnerTable {
        ulng w[6][6] = {}, inv[6] = {};
        constexpr GarnerTable() {
            for (int j = 0; j < 6; ++j) {
                ulng p = PRIMES[j], acc = 1, r = 1;
                for (int t = 0; t < j; ++t) { w[j][t] = acc; acc = acc * PRIMES[t] % p; }
                for (ulng e = p - 2; e; e >>= 1, acc = acc * acc % p) { if (e & 1) { r = r * acc % p; } }
                inv[j] = r;}}
    };
    inline constexpr GarnerTable GARNER{};
    // T: O(J), M: O(1); mixed-radix digit J of c = d_0 + P_1 d_1 + P_1 P_2 d_2 + ... from the lower digits and c mod P_(J+1).
    template<int J, int K>
    uint garnerDigit(const array<uint, K> &d, uint rj) {
        if constexpr (J == 0) { return rj; }
        else {
            constexpr ulng p = PRIMES[J];
            ulng s = 0;
            // Each product is below 2^62, so four fit a word before a reduction.
            for (int t = 0; t < J; ++t) { s += d[t] * GARNER.w[J][t]; if (t % 4 == 3) { s %= p; } }
            return uint((rj + p - s % p) * GARNER.inv[J] % p);}}
    // T: O(K * (n * m / B + (n + m) * log(n + m))), M: O(K * (n + m)); B = 2^23, K in [1, 6], any n + m - 1 < 2^31; digit j of every coefficient in res[j].
    template<int K, typename T>
    array<vector<uint>, K> garner(const vector<T> &a, const vector<T> &b);
    // T: O(1), M: O(1); least K with k x < P_1 ... P_K, exact through four primes (product below 2^117), by bit lengths for five (2^144 <= P_1 ... P_5).
    inline int primeCount(ulng k, ulll x) {
        ulll w = 1;
        for (int j = 1; j <= 4; ++j) {
            w *= PRIMES[j - 1];
            if (x <= (w - 1) / k) { return j; }}
        int bits = int(std::bit_width(k)) + (x >> 64 ? 128 - std::countl_zero(ulng(x >> 64)) : int(std::bit_width(ulng(x))));
        return bits <= 144 ? 5 : 6;}
    // T: O(1) dispatch, M: O(1); f.template operator()<K>() for K in [1, 6].
    template<typename F>
    auto withPrimes(int K, F f) {
        switch (K) {
            case 1: return f.template operator()<1>();
            case 2: return f.template operator()<2>();
            case 3: return f.template operator()<3>();
            case 4: return f.template operator()<4>();
            case 5: return f.template operator()<5>();
            default: return f.template operator()<6>();}}
} // namespace convolution_detail

// S: O(n) on growth, Q: O(1), M: O(n) per type and direction; static prime M::mod() with n | 2^RANK; entry h + j = w^j, w primitive 2h-th root (inverse: w^-1); growth invalidates references.
template<typename M = mint>
const vector<M> &nttRootTable(int n, bool inverse = false) {
    using namespace convolution_detail;
    static vector<M> tab[2];
    constexpr ulng p = M::mod();
    assert(n >= 0 && std::countr_zero(std::bit_ceil(ulng(max(n, 1)))) <= RANK<M>);
    auto &t = tab[inverse];
    if (int(t.size()) >= n) { return t; }
    t.assign(std::bit_ceil(uint(n)), M(0));
    for (int h = 1; h < int(t.size()); h *= 2) {
        M z = power(M(ROOT<M>), (p - 1) / (2 * ulng(h))), x = 1;
        if (inverse) { z = power(z, p - 2); }
        for (int j = 0; j < h; ++j, x *= z) { t[h + j] = x; }}
    return t;}

// T: O(n * log(n)), M: O(n) shared table; n = a.size() power of two dividing 2^RANK; ntt output and intt input bit-reversed, intt scaled by 1 / n, transposedNtt = ntt^T.
template<typename M = mint>
void ntt(vector<M> &a) {
    assert(std::has_single_bit(a.size()));
    convolution_detail::dif(a, nttRootTable<M>(int(a.size())));}
template<typename M = mint>
void intt(vector<M> &a) {
    assert(std::has_single_bit(a.size()));
    convolution_detail::dit(a, nttRootTable<M>(int(a.size()), true));
    M r = convolution_detail::power(M(a.size()), M::mod() - 2);
    for (auto &x : a) { x *= r; }}
template<typename M = mint>
void transposedNtt(vector<M> &a) {
    assert(std::has_single_bit(a.size()));
    convolution_detail::dit(a, nttRootTable<M>(int(a.size())));}
// T: O(n * log(n)), M: O(n); a = ntt of a length-n polynomial, 2n | 2^RANK; becomes its length-2n ntt.
template<typename M = mint>
void nttDoubling(vector<M> &a) {
    int n = int(a.size());
    assert(std::has_single_bit(a.size()) && std::countr_zero(ulng(2 * n)) <= convolution_detail::RANK<M>);
    vector<M> b = a;
    intt(b);
    M z = convolution_detail::power(M(convolution_detail::ROOT<M>), (M::mod() - 1) / (2 * ulng(max(n, 1)))), r = 1;
    for (auto &x : b) { x *= r; r *= z; }
    ntt(b);
    a.insert(a.end(), b.begin(), b.end());}

// T: O(n * log(n)), M: O(n) shared table; n = a.size() power of two, X[k] = sum x[j] * exp(-2 * pi * i * j * k / n), output (ifft input) bit-reversed, ifft scaled by 1 / n.
inline void fft(vector<std::complex<double>> &a) {
    assert(std::has_single_bit(a.size()));
    convolution_detail::dif(a, convolution_detail::fftRootTable(int(a.size()), false));}
inline void ifft(vector<std::complex<double>> &a) {
    assert(std::has_single_bit(a.size()));
    convolution_detail::dit(a, convolution_detail::fftRootTable(int(a.size()), true));
    for (auto &x : a) { x /= double(a.size()); }}

// T: O(n * m), M: O(n + m); any ring with T() = 0; empty input gives an empty result.
template<typename T>
vector<T> convolutionNaive(const vector<T> &a, const vector<T> &b) {
    int n = int(a.size()), m = int(b.size());
    if (!n || !m) { return {}; }
    vector<T> res(n + m - 1);
    for (int i = 0; i < n; ++i) { for (int j = 0; j < m; ++j) { res[i + j] += a[i] * b[j]; } }
    return res;}
// T: O(n * m^(log2(3) - 1)) for n >= m, M: O(n + m); any ring with T() = 0.
template<typename T>
vector<T> convolutionKaratsuba(const vector<T> &a, const vector<T> &b) {
    if (a.empty() || b.empty()) { return {}; }
    vector<T> res(a.size() + b.size() - 1);
    convolution_detail::karatsuba(a.data(), int(a.size()), b.data(), int(b.size()), res.data());
    return res;}

// T: O((n + m) * log(n + m)), M: O(n + m); rounding is exact for integers when (sum a^2 + sum b^2) * log2(L) < 9e14.
inline vector<double> convolutionFft(const vector<double> &a, const vector<double> &b) {
    int n = int(a.size()), m = int(b.size());
    if (!n || !m) { return {}; }
    int len = int(std::bit_ceil(uint(n + m - 1)));
    vector<std::complex<double>> x(len), y(len);
    for (int i = 0; i < n; ++i) { x[i].real(a[i]); }
    for (int i = 0; i < m; ++i) { x[i].imag(b[i]); }
    fft(x);
    for (int i = 0; i < len; ++i) {
        // Bit-reversed slot i holds index k; slot i ^ (bit_floor(i) - 1) holds -k.
        auto u = x[i], v = x[i ? i ^ (std::bit_floor(uint(i)) - 1) : 0];
        y[i] = (u * u - std::conj(v * v)) * std::complex<double>(0, -0.25);}
    ifft(y);
    vector<double> res(n + m - 1);
    for (int i = 0; i < n + m - 1; ++i) { res[i] = y[i].real(); }
    return res;}

// T: O((n + m) * log(n + m)), M: O(n + m); static prime M::mod() with n + m - 1 <= 2^RANK; naive when min(n, m) <= 60, one forward transform when &a == &b.
template<typename M = mint>
vector<M> convolutionNtt(const vector<M> &a, const vector<M> &b) {
    int n = int(a.size()), m = int(b.size());
    if (!n || !m) { return {}; }
    if (min(n, m) <= convolution_detail::NAIVE) { return convolutionNaive(a, b); }
    int len = int(std::bit_ceil(uint(n + m - 1)));
    vector<M> x(len), y;
    std::copy(a.begin(), a.end(), x.begin());
    ntt(x);
    if (&a == &b) { for (auto &v : x) { v *= v; } }
    else {
        y.resize(len);
        std::copy(b.begin(), b.end(), y.begin());
        ntt(y);
        for (int i = 0; i < len; ++i) { x[i] *= y[i]; }}
    intt(x);
    x.resize(n + m - 1);
    return x;}

// T: O(n * m / B + (n + m) * log(B)), M: O(n + m); B = 2^(min(RANK, 30) - 1); static prime M::mod() with RANK >= 1, any n + m - 1 < 2^31.
template<typename M = mint>
vector<M> convolutionLarge(const vector<M> &a, const vector<M> &b) {
    static_assert(convolution_detail::RANK<M> >= 1);
    constexpr int L = int(convolution_detail::LIMIT<M>), B = L / 2;
    int n = int(a.size()), m = int(b.size());
    if (!n || !m || n + m - 1 <= L) { return convolutionNtt(a, b); }
    auto blocks = [&](const vector<M> &v) {
        vector<vector<M>> res;
        for (int i = 0; i < int(v.size()); i += B) {
            vector<M> t(L);
            std::copy(v.begin() + i, v.begin() + min(i + B, int(v.size())), t.begin());
            ntt(t);
            res.pb(std::move(t));}
        return res;};
    vector<vector<M>> x = blocks(a), z;
    const vector<vector<M>> &y = &a == &b ? x : (z = blocks(b));
    int p = int(x.size()), q = int(y.size());
    vector<M> res(n + m - 1);
    for (int k = 0; k < p + q - 1; ++k) {
        vector<M> acc(L);
        for (int i = max(0, k - q + 1); i <= min(k, p - 1); ++i) { for (int j = 0; j < L; ++j) { acc[j] += x[i][j] * y[k - i][j]; } }
        intt(acc);
        for (int j = 0; j < L && lng(k) * B + j < n + m - 1; ++j) { res[k * B + j] += acc[j]; }}
    return res;}

template<int K, typename T>
array<vector<uint>, K> convolution_detail::garner(const vector<T> &a, const vector<T> &b) {
    array<vector<uint>, K> r;
    [&]<int... J>(std::integer_sequence<int, J...>) {
        ((r[J] = [&] {
            using MP = ModInt<PRIMES[J]>;
            vector<MP> x(a.begin(), a.end());
            vector<MP> z = &a == &b ? convolutionLarge(x, x) : convolutionLarge(x, vector<MP>(b.begin(), b.end()));
            vector<uint> v(z.size());
            for (int i = 0; i < int(z.size()); ++i) { v[i] = z[i].val(); }
            return v;}()), ...);}(std::make_integer_sequence<int, K>());
    array<uint, K> d;
    for (int i = 0; i < int(r[0].size()); ++i) {
        [&]<int... J>(std::integer_sequence<int, J...>) { ((d[J] = r[J][i] = garnerDigit<J, K>(d, r[J][i])), ...); }(std::make_integer_sequence<int, K>());}
    return r;}

// T: O(K * (n * m / B + (n + m) * log(n + m))), M: O(K * (n + m)); B = 2^23, K <= 6 primes chosen from the inputs, any n + m - 1 < 2^31; Long: exact when a coefficient fits lng, else modulo 2^64; U128: modulo 2^128.
inline vector<lng> convolutionLong(const vector<lng> &a, const vector<lng> &b) {
    using namespace convolution_detail;
    if (a.empty() || b.empty()) { return {}; }
    if (min(a.size(), b.size()) <= size_t(NAIVE)) {
        vector<lng> res;
        for (ulng x : convolutionNaive(vector<ulng>(a.begin(), a.end()), vector<ulng>(b.begin(), b.end()))) { res.pb(lng(x)); }
        return res;}
    auto mag = [](const vector<lng> &v) {
        ulng r = 0;
        for (lng x : v) { r = max(r, x < 0 ? 0 - ulng(x) : ulng(x)); }
        return r;};
    // Twice the coefficient bound keeps negative values in the upper half of [0, P_1 ... P_K).
    return withPrimes(primeCount(min(a.size(), b.size()), 2 * ulll(mag(a)) * mag(b)), [&]<int K>() {
        array<ulng, K> w;
        w[0] = 1;
        for (int j = 1; j < K; ++j) { w[j] = w[j - 1] * PRIMES[j - 1]; }
        ulng whole = w[K - 1] * PRIMES[K - 1];
        auto d = garner<K>(a, b);
        vector<lng> res(d[0].size());
        for (int i = 0; i < int(res.size()); ++i) {
            ulng c = 0;
            for (int j = 0; j < K; ++j) { c += d[j][i] * w[j]; }
            int j = K - 1;
            while (j > 0 && d[j][i] == (PRIMES[j] - 1) / 2) { --j; }
            res[i] = lng(d[j][i] > (PRIMES[j] - 1) / 2 ? c - whole : c);}
        return res;});}
inline vector<ulll> convolutionU128(const vector<ulng> &a, const vector<ulng> &b) {
    using namespace convolution_detail;
    if (a.empty() || b.empty()) { return {}; }
    if (min(a.size(), b.size()) <= size_t(NAIVE)) { return convolutionNaive(vector<ulll>(a.begin(), a.end()), vector<ulll>(b.begin(), b.end())); }
    ulll x = ulll(*std::max_element(a.begin(), a.end())) * *std::max_element(b.begin(), b.end());
    return withPrimes(primeCount(min(a.size(), b.size()), x), [&]<int K>() {
        array<ulll, K> w;
        w[0] = 1;
        for (int j = 1; j < K; ++j) { w[j] = w[j - 1] * PRIMES[j - 1]; }
        auto d = garner<K>(a, b);
        vector<ulll> res(d[0].size(), 0);
        for (int i = 0; i < int(res.size()); ++i) { for (int j = 0; j < K; ++j) { res[i] += d[j][i] * w[j]; } }
        return res;});}
// T: O(K * (n * m / B + (n + m) * log(n + m))), M: O(K * (n + m)); B = 2^23, any modint M (dynamic too), any n + m - 1 < 2^31, K <= 6 primes with min(n, m) (mod - 1)^2 < P_1 ... P_K; naive when min(n, m) <= 60.
template<typename M>
vector<M> convolutionArbitraryMod(const vector<M> &a, const vector<M> &b) {
    using namespace convolution_detail;
    if (min(a.size(), b.size()) <= size_t(NAIVE)) { return convolutionNaive(a, b); }
    auto val = [](const vector<M> &v) {
        vector<ulng> r(v.size());
        for (int i = 0; i < int(v.size()); ++i) { r[i] = v[i].val(); }
        return r;};
    vector<ulng> x = val(a), y = &a == &b ? vector<ulng>() : val(b);
    const vector<ulng> &z = &a == &b ? x : y;
    ulng top = M::mod() - 1;
    return withPrimes(primeCount(min(a.size(), b.size()), ulll(top) * top), [&]<int K>() {
        array<M, K> w;
        w[0] = 1;
        for (int j = 1; j < K; ++j) { w[j] = w[j - 1] * M(PRIMES[j - 1]); }
        auto d = garner<K>(x, z);
        vector<M> res(d[0].size(), 0);
        for (int i = 0; i < int(res.size()); ++i) { for (int j = 0; j < K; ++j) { res[i] += M(d[j][i]) * w[j]; } }
        return res;});}

// T: O((n + m) * log(n + m)), M: O(n + m); Core modint M: convolutionLarge for a static prime with RANK >= 20, else convolutionArbitraryMod.
template<typename M = mint>
vector<M> convolution(const vector<M> &a, const vector<M> &b) {
    if constexpr (convolution_detail::nttPrime<M>()) { return convolutionLarge(a, b); }
    else { return convolutionArbitraryMod(a, b); }}

// T: O(k * log(k)), M: O(k); first k >= 0 coefficients of a * b, zero-padded.
template<typename M = mint>
vector<M> truncatedConvolution(const vector<M> &a, const vector<M> &b, int k) {
    assert(k >= 0);
    vector<M> res = convolution(vector<M>(a.begin(), a.begin() + min(int(a.size()), k)), vector<M>(b.begin(), b.begin() + min(int(b.size()), k)));
    res.resize(k);
    return res;}
// T: O((n + m) * log(n + m)), M: O(n + m); res[k] = sum a[k - m + 1 + j] * b[j] over valid j, all n + m - 1 lags.
template<typename M = mint>
vector<M> crossCorrelation(const vector<M> &a, vector<M> b) {
    reverse(b.begin(), b.end());
    return convolution(a, b);}
// T: O(n * log(n)), M: O(n); m >= 1; coefficients [m - 1, n) of a * b (empty when m > n), one size-bit_ceil(n) cyclic transform for an NTT prime.
template<typename M = mint>
vector<M> middleProduct(const vector<M> &a, const vector<M> &b) {
    int n = int(a.size()), m = int(b.size());
    assert(m >= 1);
    if (m > n) { return {}; }
    if constexpr (convolution_detail::nttPrime<M>()) {
        if (m > convolution_detail::NAIVE && n <= convolution_detail::LIMIT<M>) {
            int len = int(std::bit_ceil(uint(n)));
            vector<M> x(len), y(len);
            std::copy(a.begin(), a.end(), x.begin()); std::copy(b.begin(), b.end(), y.begin());
            ntt(x); ntt(y);
            for (int i = 0; i < len; ++i) { x[i] *= y[i]; }
            intt(x);
            return vector<M>(x.begin() + m - 1, x.begin() + n);}}
    vector<M> res(n - m + 1);
    if (m <= convolution_detail::NAIVE) {
        for (int i = 0; i < n - m + 1; ++i) { for (int j = 0; j < m; ++j) { res[i] += a[i + m - 1 - j] * b[j]; } }
        return res;}
    auto c = convolution(a, b);
    return vector<M>(c.begin() + m - 1, c.begin() + n);}

// T: O(n * log(n)), M: O(n); |a| = |b| = n; res[k] = sum over i + j = k mod n (cyclic) or with sign -1 on wrap (negacyclic).
template<typename M = mint>
vector<M> cyclicConvolution(const vector<M> &a, vector<M> b) {
    int n = int(a.size());
    assert(int(b.size()) == n);
    b.resize(n);
    if constexpr (convolution_detail::nttPrime<M>()) {
        if (std::has_single_bit(uint(n)) && n <= convolution_detail::LIMIT<M>) {
            vector<M> x = a, y = b;
            ntt(x); ntt(y);
            for (int i = 0; i < n; ++i) { x[i] *= y[i]; }
            intt(x);
            return x;}}
    auto c = convolution(a, b);
    for (int i = n; i < int(c.size()); ++i) { c[i - n] += c[i]; }
    c.resize(n);
    return c;}
template<typename M = mint>
vector<M> negacyclicConvolution(const vector<M> &a, vector<M> b) {
    int n = int(a.size());
    assert(int(b.size()) == n);
    b.resize(n);
    if constexpr (convolution_detail::nttPrime<M>()) {
        if (std::has_single_bit(uint(n)) && 2 * lng(n) <= convolution_detail::LIMIT<M>) {
            // Twist by a primitive 2n-th root z: x^n + 1 becomes y^n - 1.
            M z = convolution_detail::power(M(convolution_detail::ROOT<M>), (M::mod() - 1) / (2 * ulng(n))), r = 1;
            vector<M> x = a;
            for (int i = 0; i < n; ++i, r *= z) { x[i] *= r; b[i] *= r; }
            auto c = cyclicConvolution(x, b);
            z = convolution_detail::power(z, 2 * ulng(n) - 1); r = 1;
            for (int i = 0; i < n; ++i, r *= z) { c[i] *= r; }
            return c;}}
    auto c = convolution(a, b);
    for (int i = n; i < int(c.size()); ++i) { c[i - n] -= c[i]; }
    c.resize(n);
    return c;}

// T: O(R * log(R)), M: O(R); R = prod(da[i] + db[i] - 1); row-major flat arrays, equal ranks, empty when some extent is 0; Kronecker substitution.
template<typename M = mint>
vector<M> convolutionTensor(const vector<M> &a, const vector<int> &da, const vector<M> &b, const vector<int> &db) {
    constexpr lng CAP = lng(1) << 31;
    int d = int(min(da.size(), db.size()));
    assert(da.size() == db.size());
    lng sa = 1, sb = 1;
    vector<lng> st(d + 1, 1);
    for (int i = d - 1; i >= 0; --i) {
        assert(da[i] >= 0 && db[i] >= 0);
        sa = min(sa * da[i], CAP); sb = min(sb * db[i], CAP);
        st[i] = min(st[i + 1] * max(da[i] + db[i] - 1, 1), CAP);}
    assert(lng(a.size()) == sa && lng(b.size()) == sb && st[0] < CAP);
    if (sa <= 0 || sb <= 0 || st[0] >= CAP) { return {}; }
    auto spread = [&](const vector<M> &v, const vector<int> &dv) {
        lng len = 1, pos = 0;
        for (int i = 0; i < d; ++i) { len += (dv[i] - 1) * st[i + 1]; }
        vector<M> res(len);
        vector<int> idx(d, 0);
        for (int t = 0; t < int(v.size()) && pos < len; ++t) {
            res[pos] = v[t];
            for (int i = d - 1; i >= 0; --i) {
                pos += st[i + 1];
                if (++idx[i] < dv[i]) { break; }
                pos -= dv[i] * st[i + 1]; idx[i] = 0;}}
        return res;};
    auto res = convolution(spread(a, da), spread(b, db));
    res.resize(st[0]);
    return res;}
// T: O(R * log(R)), M: O(R); R = (ha + hb - 1) * (wa + wb - 1), rectangular inputs; empty when a dimension is 0.
template<typename M = mint>
vector<vector<M>> convolution2d(const vector<vector<M>> &a, const vector<vector<M>> &b) {
    int ha = int(a.size()), hb = int(b.size()), wa = ha ? int(a[0].size()) : 0, wb = hb ? int(b[0].size()) : 0;
    if (!ha || !hb || !wa || !wb) { return {}; }
    vector<M> x, y;
    for (auto &r : a) { assert(int(r.size()) == wa); x.insert(x.end(), r.begin(), r.end()); }
    for (auto &r : b) { assert(int(r.size()) == wb); y.insert(y.end(), r.begin(), r.end()); }
    auto c = convolutionTensor(x, {ha, wa}, y, {hb, wb});
    int w = wa + wb - 1;
    vector<vector<M>> res(ha + hb - 1);
    for (int i = 0; i < ha + hb - 1; ++i) { res[i].assign(c.begin() + lng(i) * w, c.begin() + lng(i + 1) * w); }
    return res;}
