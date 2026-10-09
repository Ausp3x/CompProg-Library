#pragma once
#include "../01-Core/01-template.hpp"
#include "04-sieve_algorithms.hpp"

namespace transform_detail {
    // T: O(n * log(n)), M: O(1); n = a.size() power of two; f(a[low], a[high]) on every pair differing in one bit.
    template<typename T, typename F>
    void butterfly(vector<T> &a, F f) {
        int n = int(a.size());
        assert(std::has_single_bit(uint(n)));
        for (int h = 1; h < n; h *= 2) {
            for (int i = 0; i + 2 * h <= n; i += 2 * h) { for (int j = i; j < i + h; ++j) { f(a[j], a[j + h]); } }}}
    // T: O(n * log(log(n))), M: O(1); n = a.size() - 1, prms ascending covering [2, n]; divisor or multiple sums along each prime.
    template<typename T>
    void divisorStep(vector<T> &a, const vector<int> &prms, bool inverse) {
        int n = int(a.size()) - 1;
        for (int p : prms) {
            if (p > n) { break; }
            if (inverse) { for (int i = n / p; i >= 1; --i) { a[i * p] -= a[i]; } }
            else { for (int i = 1; i <= n / p; ++i) { a[i * p] += a[i]; } }}}
    template<typename T>
    void multipleStep(vector<T> &a, const vector<int> &prms, bool inverse) {
        int n = int(a.size()) - 1;
        for (int p : prms) {
            if (p > n) { break; }
            if (inverse) { for (int i = 1; i <= n / p; ++i) { a[i] -= a[i * p]; } }
            else { for (int i = n / p; i >= 1; --i) { a[i] += a[i * p]; } }}}
    inline vector<int> primes(lng n) { return n >= 2 ? SieveOfErath(int(n)).prms : vector<int>(); }
    // T: O(k^2 * n), M: O(1); n = f[0].size() = 2^k; f *= g as rank polynomials truncated at degree k, pointwise in S.
    template<typename T>
    void rankedMul(vector<vector<T>> &f, const vector<vector<T>> &g) {
        int k = int(f.size()) - 1, len = int(f[0].size());
        if (g.size() != f.size() || int(g[0].size()) != len) { return; }
        for (int r = k; r >= 0; --r) {
            for (int s = 0; s < len; ++s) { f[r][s] *= g[0][s]; }
            for (int i = 0; i < r; ++i) { for (int s = 0; s < len; ++s) { f[r][s] += f[i][s] * g[r - i][s]; } }}}
} // namespace transform_detail

// T: O(n * log(n)), M: O(1); in place, n = a.size() a power of two; zeta sums over subsets (supersets), Mobius inverts.
template<typename T> void subsetZeta(vector<T> &a) { transform_detail::butterfly(a, [](T &x, T &y) { y += x; }); }
template<typename T> void subsetMobius(vector<T> &a) { transform_detail::butterfly(a, [](T &x, T &y) { y -= x; }); }
template<typename T> void supersetZeta(vector<T> &a) { transform_detail::butterfly(a, [](T &x, T &y) { x += y; }); }
template<typename T> void supersetMobius(vector<T> &a) { transform_detail::butterfly(a, [](T &x, T &y) { x -= y; }); }
// T: O(n * log(n)), M: O(1); inverse divides by n: exact signed division for integral T (a WHT image), one inverse of n otherwise.
template<typename T>
void walshHadamard(vector<T> &a, bool inverse = false) {
    transform_detail::butterfly(a, [](T &x, T &y) { T u = x; x += y; y = u - y; });
    if (!inverse) { return; }
    if constexpr (std::is_integral_v<T>) {
        using S = std::make_signed_t<T>;
        for (auto &x : a) { x = T(S(x) / S(a.size())); }}
    else {
        T r = T(1) / T(int(a.size()));
        for (auto &x : a) { x *= r; }}}
// T: O(n * d * log(n) / log(d)), M: O(d); n = a.size() = d^k, d x d kernel; applies the kernel tensored k times.
template<typename T>
void kroneckerPowerTransform(vector<T> &a, const vector<vector<T>> &ker) {
    int n = int(a.size()), d = int(ker.size());
    bool square = std::all_of(ker.begin(), ker.end(), [d](const vector<T> &r) { return int(r.size()) == d; });
    assert(d >= 1 && square);
    vector<T> x(d);
    lng s = 1;
    for (; s < n && d > 1 && square; s *= d) {
        for (lng i = 0; i + s * d <= n; i += s * d) {
            for (lng j = i; j < i + s; ++j) {
                for (int t = 0; t < d; ++t) { x[t] = a[j + t * s]; }
                for (int r = 0; r < d; ++r) {
                    T v = T();
                    for (int t = 0; t < d; ++t) { v += ker[r][t] * x[t]; }
                    a[j + r * s] = v;}}}}
    assert(s == n);}

// T: O(n * log(n)), M: O(n); |a| = |b| = n a power of two; res[S] = sum over T op U = S of a[T] * b[U].
template<typename T>
vector<T> orConvolution(vector<T> a, vector<T> b) {
    assert(a.size() == b.size());
    b.resize(a.size());
    subsetZeta(a); subsetZeta(b);
    for (int i = 0; i < int(a.size()); ++i) { a[i] *= b[i]; }
    subsetMobius(a);
    return a;}
template<typename T>
vector<T> andConvolution(vector<T> a, vector<T> b) {
    assert(a.size() == b.size());
    b.resize(a.size());
    supersetZeta(a); supersetZeta(b);
    for (int i = 0; i < int(a.size()); ++i) { a[i] *= b[i]; }
    supersetMobius(a);
    return a;}
template<typename T>
vector<T> xorConvolution(vector<T> a, vector<T> b) {
    assert(a.size() == b.size());
    b.resize(a.size());
    walshHadamard(a); walshHadamard(b);
    for (int i = 0; i < int(a.size()); ++i) { a[i] *= b[i]; }
    walshHadamard(a, true);
    return a;}

// T: O(n * log(log(n))), M: O(n); n = a.size() - 1, in place on [1, n], a[0] untouched; divisor: sum over d | k, multiple: over k | m <= n.
template<typename T> void divisorZeta(vector<T> &a) { transform_detail::divisorStep(a, transform_detail::primes(lng(a.size()) - 1), false); }
template<typename T> void divisorMobius(vector<T> &a) { transform_detail::divisorStep(a, transform_detail::primes(lng(a.size()) - 1), true); }
template<typename T> void multipleZeta(vector<T> &a) { transform_detail::multipleStep(a, transform_detail::primes(lng(a.size()) - 1), false); }
template<typename T> void multipleMobius(vector<T> &a) { transform_detail::multipleStep(a, transform_detail::primes(lng(a.size()) - 1), true); }
// T: O(n * log(log(n))), M: O(n); indices from 1, res[0] = 0; gcd: size min(|a|, |b|), lcm: size max(|a|, |b|), lcm >= size dropped.
template<typename T>
vector<T> gcdConvolution(vector<T> a, vector<T> b) {
    using namespace transform_detail;
    auto prms = primes(lng(max(a.size(), b.size())) - 1);
    multipleStep(a, prms, false); multipleStep(b, prms, false);
    a.resize(min(a.size(), b.size()));
    for (int i = 0; i < int(a.size()); ++i) { a[i] *= b[i]; }
    multipleStep(a, prms, true);
    if (!a.empty()) { a[0] = T(); }
    return a;}
template<typename T>
vector<T> lcmConvolution(vector<T> a, vector<T> b) {
    using namespace transform_detail;
    a.resize(max(a.size(), b.size())); b.resize(a.size());
    auto prms = primes(lng(a.size()) - 1);
    divisorStep(a, prms, false); divisorStep(b, prms, false);
    for (int i = 0; i < int(a.size()); ++i) { a[i] *= b[i]; }
    divisorStep(a, prms, true);
    if (!a.empty()) { a[0] = T(); }
    return a;}

// T: O(k^2 * n), M: O(k * n); n = 2^k = a.size(); z[r][S] = sum of a[T] over T subset S with |T| = r; rankedMobius returns z[|S|][S] after Mobius.
template<typename T>
vector<vector<T>> rankedZeta(const vector<T> &a) {
    int n = int(a.size()), k = std::bit_width(uint(max(n, 1) - 1));
    assert(std::has_single_bit(uint(n)));
    vector<vector<T>> z(k + 1, vector<T>(n));
    for (int s = 0; s < n; ++s) { z[std::popcount(uint(s))][s] = a[s]; }
    for (auto &v : z) { subsetZeta(v); }
    return z;}
template<typename T>
vector<T> rankedMobius(vector<vector<T>> z) {
    int n = z.empty() ? 0 : int(z[0].size()), k = std::bit_width(uint(max(n, 1) - 1));
    assert(int(z.size()) == k + 1);
    z.resize(k + 1);
    for (auto &v : z) { v.resize(n); subsetMobius(v); }
    vector<T> res(n);
    for (int s = 0; s < n; ++s) { res[s] = z[std::popcount(uint(s))][s]; }
    return res;}
// T: O(q * k^2 * n), M: O(k * n); q >= 1 operands of size n = 2^k; res[S] = sum over ordered partitions S = T1 + ... + Tq of prod a_i[T_i].
template<typename T>
vector<T> disjointUnionConvolution(const vector<vector<T>> &a) {
    assert(!a.empty());
    auto f = rankedZeta(a[0]);
    for (int i = 1; i < int(a.size()); ++i) {
        assert(a[i].size() == a[0].size());
        transform_detail::rankedMul(f, rankedZeta(a[i]));}
    return rankedMobius(std::move(f));}
// T: O(k^2 * n), M: O(k * n); |a| = |b| = n = 2^k; res[S] = sum over T subset S of a[T] * b[S \ T].
template<typename T>
vector<T> subsetConvolution(const vector<T> &a, const vector<T> &b) {
    assert(a.size() == b.size());
    auto f = rankedZeta(a);
    transform_detail::rankedMul(f, rankedZeta(b));
    return rankedMobius(std::move(f));}

// S: NA, U: NA, Q: O(n * log(n)) bitwise, O(n * log(log(n))) with a sieve covering n, O(n * log(n)) Slow, M: O(1); legacy in-place names.
template<typename T>
struct FastConv {
    static void fctOr(vector<T> &a, bool is_inv) { is_inv ? subsetMobius(a) : subsetZeta(a); }
    static void fctAnd(vector<T> &a, bool is_inv) { is_inv ? supersetMobius(a) : supersetZeta(a); }
    static void fctXor(vector<T> &a, bool is_inv) { walshHadamard(a, is_inv); }
    static void fctGcdSlow(vector<T> &a, bool is_inv) {
        int n = int(a.size()) - 1;
        if (is_inv) { for (int i = n; i >= 1; --i) { for (int j = 2 * i; j <= n; j += i) { a[i] -= a[j]; } } }
        else { for (int i = 1; i <= n; ++i) { for (int j = 2 * i; j <= n; j += i) { a[i] += a[j]; } } }}
    static void fctGcd(vector<T> &a, bool is_inv, const LinearSieve &sv) {
        assert(lng(sv.n) >= lng(a.size()) - 1);
        transform_detail::multipleStep(a, sv.prms, is_inv);}
    static void fctLcmSlow(vector<T> &a, bool is_inv) {
        int n = int(a.size()) - 1;
        if (is_inv) { for (int i = 1; i <= n; ++i) { for (int j = 2 * i; j <= n; j += i) { a[j] -= a[i]; } } }
        else { for (int i = n; i >= 1; --i) { for (int j = 2 * i; j <= n; j += i) { a[j] += a[i]; } } }}
    static void fctLcm(vector<T> &a, bool is_inv, const LinearSieve &sv) {
        assert(lng(sv.n) >= lng(a.size()) - 1);
        transform_detail::divisorStep(a, sv.prms, is_inv);}
};
