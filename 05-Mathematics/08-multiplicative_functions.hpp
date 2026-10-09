#pragma once
#include "../01-Core/01-template.hpp"
#include "07-primality_factorization.hpp"

namespace multiplicative_detail {
    // T: O(log(2 + e)), M: O(1); a^e in a ring T, 0^0 = 1.
    template<typename T>
    T power(T a, ulng e) {
        T res(1);
        for (; e; e >>= 1) {
            if (e & 1) { res *= a; }
            a *= a;}
        return res;}
} // namespace multiplicative_detail

// T: O(n^(1/4) * log(n)) expected, M: O(log(n)); n >= 1, exact values.
inline ulng phi(ulng n) {
    ulng res = n;
    for (auto [p, e] : factorize(n)) { res -= res / p; }
    return res;}
inline int mobius(ulng n) {
    int res = 1;
    for (auto [p, e] : factorize(n)) { res = e > 1 ? 0 : -res; }
    return res;}
inline int omega(ulng n) { return int(factorize(n).size()); }
inline int bigOmega(ulng n) {
    int res = 0;
    for (auto [p, e] : factorize(n)) { res += e; }
    return res;}
inline int liouville(ulng n) { return bigOmega(n) % 2 ? -1 : 1; }
inline ulng carmichaelLambda(ulng n) {
    ulng res = 1;
    for (auto [p, e] : factorize(n)) {
        ulng v = p - 1;
        for (int k = 1; k < e; ++k) { v *= p; }
        res = lcm(res, p == 2 && e >= 3 ? v / 2 : v);}
    return res;}

// T: O(n^(1/4) * log(n) + log(n) * log(2 + k)) expected, M: O(log(n)); n >= 1, ring T (unsigned T wraps, exact below its range).
template<typename T = ulll>
T sigmaK(ulng n, ulng k) {
    T res(1);
    for (auto [p, e] : factorize(n)) {
        T q = multiplicative_detail::power(T(p), k), s(1), t(1);
        for (int i = 0; i < e; ++i) { t *= q; s += t; }
        res *= s;}
    return res;}
template<typename T = ulll>
T jordanTotient(ulng n, ulng k) {
    T res(1);
    for (auto [p, e] : factorize(n)) {
        T q = multiplicative_detail::power(T(p), k);
        res *= multiplicative_detail::power(q, ulng(e - 1)) * (q - T(1));}
    return res;}

// T: O(n) plus one f call per prime power, M: O(n); 0 <= n < INT_MAX, f(p, k, p^k) -> T, entry 0 is T(0), entry 1 is T(1).
template<typename T, typename F>
vector<T> multiplicativeTable(int n, F f) {
    assert(0 <= n && n < std::numeric_limits<int>::max());
    vector<T> res(n + 1, T(0));
    vector<int> prms, pw(n + 1, 0);
    vector<int8_t> ex(n + 1, 0);
    if (n >= 1) { res[1] = T(1); }
    for (int i = 2; i <= n; ++i) {
        if (!pw[i]) { prms.pb(i); pw[i] = i; ex[i] = 1; res[i] = f(i, 1, i); }
        for (int p : prms) {
            if (lng(i) * p > n) { break; }
            int j = i * p;
            if (i % p) { pw[j] = p; ex[j] = 1; res[j] = T(res[i] * res[p]); continue; }
            // pw[j] = p^(k+1) is the full p-part of j; a smaller prime power was built before j.
            pw[j] = pw[i] * p; ex[j] = int8_t(ex[i] + 1);
            res[j] = pw[j] == j ? f(p, int(ex[j]), j) : T(res[pw[j]] * res[j / pw[j]]);
            break;}}
    return res;}

// T: O(n), M: O(n); 0 <= n < INT_MAX, entry 0 is 0, prefix sums fit lng.
inline vector<int> phiTable(int n) { return multiplicativeTable<int>(n, [](int p, int, int pk) { return pk - pk / p; }); }
inline vector<int8_t> mobiusTable(int n) { return multiplicativeTable<int8_t>(n, [](int, int k, int) { return int8_t(k == 1 ? -1 : 0); }); }
inline vector<int> divisorCountTable(int n) { return multiplicativeTable<int>(n, [](int, int k, int) { return k + 1; }); }
inline vector<lng> divisorSumTable(int n) { return multiplicativeTable<lng>(n, [](int p, int, int pk) { return (lng(pk) * p - 1) / (p - 1); }); }
inline vector<lng> prefixPhiTable(int n) {
    vector<int> f = phiTable(n);
    vector<lng> res(n + 1, 0);
    for (int i = 1; i <= n; ++i) { res[i] = res[i - 1] + f[i]; }
    return res;}
inline vector<int> prefixMobiusTable(int n) {
    vector<int8_t> f = mobiusTable(n);
    vector<int> res(n + 1, 0);
    for (int i = 1; i <= n; ++i) { res[i] = res[i - 1] + f[i]; }
    return res;}

// T: O(n * log(n)), M: O(n); a.size() == b.size() == n + 1 >= 1, index 0 ignored and returned as T(0).
template<typename T>
vector<T> dirichletConvolutionTable(const vector<T> &a, const vector<T> &b) {
    assert(!a.empty() && a.size() == b.size());
    int n = int(a.size()) - 1;
    vector<T> res(n + 1, T(0));
    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= n / i; ++j) { res[i * j] += a[i] * b[j]; }}
    return res;}
// T: O(n * log(n)), M: O(n); a.size() == n + 1 >= 2, a[1] a unit with exact T(1) / a[1].
template<typename T>
vector<T> dirichletInverseTable(const vector<T> &a) {
    assert(a.size() >= 2);
    int n = int(a.size()) - 1;
    T inv = T(1) / a[1];
    vector<T> res(n + 1, T(0));
    for (int i = 1; i <= n; ++i) {
        // res[i] holds sum(a[d] * res[i / d]) over d > 1 until i is reached.
        res[i] = i == 1 ? inv : -inv * res[i];
        for (int j = 2; j <= n / i; ++j) { res[i * j] += a[j] * res[i]; }}
    return res;}

// S: O(n^(1/4) * log(n) + d) expected, U: O(d * o) per transform, Q: O(o + log(n)) for get, M: O(d); d = d(n), o = omega(n), mixed-radix divisor order.
template<typename T>
struct DivisorArray {
    vector<pair<ulng, int>> fac;
    vector<ulng> divs;
    vector<T> val;

    explicit DivisorArray(ulng n = 1) { build(n); }
    explicit DivisorArray(const vector<pair<ulng, int>> &f) { build(f); }
    void build(ulng n) { build(factorize(n)); }
    void build(const vector<pair<ulng, int>> &f) {
        fac = f; divs = {1};
        for (auto [p, e] : fac) {
            int len = int(divs.size());
            ulng pk = 1;
            for (int k = 1; k <= e; ++k) {
                pk *= p;
                for (int i = 0; i < len; ++i) { divs.pb(divs[i] * pk); }}}
        val.assign(divs.size(), T(0));}

    int size() const { return int(divs.size()); }
    int index(ulng d) const {
        assert(d >= 1);
        int res = 0, stride = 1;
        for (auto [p, e] : fac) {
            int k = 0;
            while (d % p == 0) { d /= p; ++k; }
            assert(k <= e);
            res += k * stride; stride *= e + 1;}
        assert(d == 1);
        return res;}
    T &get(ulng d) { return val[index(d)]; }
    T &operator[](int i) { return val[i]; }

    template<typename F>
    void setMultiplicative(F f) {
        val[0] = T(1);
        int len = 1;
        for (auto [p, e] : fac) {
            ulng pk = 1;
            for (int k = 1; k <= e; ++k) {
                pk *= p; T v = f(p, k, pk);
                for (int i = 0; i < len; ++i) { val[k * len + i] = val[i] * v; }}
            len *= e + 1;}}
    // dir 0 zeta over divisors, 1 its inverse, 2 zeta over multiples, 3 its inverse.
    void transform(int dir) {
        int s = 1;
        for (auto [p, e] : fac) {
            int block = s * (e + 1);
            for (int b = 0; b < size(); b += block) {
                if (dir == 0) { for (int i = b + s; i < b + block; ++i) { val[i] += val[i - s]; } }
                if (dir == 1) { for (int i = b + block - 1; i >= b + s; --i) { val[i] -= val[i - s]; } }
                if (dir == 2) { for (int i = b + block - s - 1; i >= b; --i) { val[i] += val[i + s]; } }
                if (dir == 3) { for (int i = b; i < b + block - s; ++i) { val[i] -= val[i + s]; } }}
            s = block;}}
    void zeta() { transform(0); }
    void mobius() { transform(1); }
    void multipleZeta() { transform(2); }
    void multipleMobius() { transform(3); }
};
