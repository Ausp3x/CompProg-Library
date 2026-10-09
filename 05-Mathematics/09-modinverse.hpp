#pragma once
#include "../01-Core/01-template.hpp"
#include "../01-Core/05-modint.hpp"
#include "01-mod_arithmetic.hpp"

// T: O(log(m)), M: O(1); m >= 1 (prime for Fermat), all lng a, inverse in [0, m) or -1 for a nonunit (0 when m = 1).
inline lng inverseXgcd(lng a, lng m) {
    assert(m >= 1);
    auto [g, x, y] = extendedGcd(modNorm(a, m), m);
    return g == 1 ? modNorm(x, m) : -1;}
inline lng inverseFermat(lng a, lng m) {
    assert(m >= 2);
    a = modNorm(a, m);
    return a ? modPow(a, m - 2, m) : -1;}
// T: O(p^(1/3 + e)) steps for any e > 0, M: O(1); prime p, inv(a) = -(p / a) * inv(p mod a), -1 when p | a.
inline lng inversePierce(lng a, lng p) {
    assert(p >= 2);
    a = modNorm(a, p);
    if (!a) { return -1; }
    lng res = 1;
    for (; a > 1; a = p % a) { res = lng(lll(res) * (p - p / a) % p); }
    return res;}

// T: O(n), M: O(n); prime M::mod() > n >= 0, entry 0 is 0.
template<typename M = mint>
vector<M> inverseTable(int n) {
    auto p = M::mod();
    assert(n >= 0 && ulng(n) < ulng(p) && M::is_prime);
    vector<M> res(n + 1, 0);
    if (n >= 1) { res[1] = 1; }
    for (int i = 2; i <= n; ++i) { res[i] = -res[p % i] * M(p / i); }
    return res;}

// T: O(n + log(mod)), M: O(n); batch: any modulus, in place, false (a unchanged) on a nonunit; list: prime modulus, zeros map to 0.
template<typename M = mint>
bool inverseBatch(vector<M> &a) {
    int n = int(a.size());
    vector<M> pre(n + 1, 1);
    for (int i = 0; i < n; ++i) { pre[i + 1] = pre[i] * a[i]; }
    M r;
    if (!tryInv(pre[n], r)) { return false; }
    for (int i = n - 1; i >= 0; --i) {
        M t = a[i];
        a[i] = r * pre[i]; r *= t;}
    return true;}
template<typename M = mint>
vector<M> inverseList(const vector<M> &a) {
    assert(M::is_prime);
    int n = int(a.size());
    vector<M> res(n), pre(n + 1, 1);
    for (int i = 0; i < n; ++i) { pre[i + 1] = a[i] == 0 ? pre[i] : pre[i] * a[i]; }
    M r = inv(pre[n]);
    for (int i = n - 1; i >= 0; --i) {
        if (a[i] == 0) { continue; }
        res[i] = r * pre[i]; r *= a[i];}
    return res;}

// T: O(n + (n / log(n)) * log(2 + k)), M: O(n); 0 <= n < INT_MAX, any modulus, i^k for i in [0, n] with 0^0 = 1.
template<typename M = mint>
vector<M> powerTable(int n, ulng k) {
    assert(0 <= n && n < std::numeric_limits<int>::max());
    vector<M> res(n + 1, 0);
    vector<int> prms, spf(n + 1, 0);
    res[0] = k == 0 ? 1 : 0;
    if (n >= 1) { res[1] = 1; }
    for (int i = 2; i <= n; ++i) {
        if (!spf[i]) { prms.pb(i); spf[i] = i; res[i] = pow(M(i), k); }
        for (int p : prms) {
            if (p > spf[i] || lng(i) * p > n) { break; }
            spf[i * p] = p; res[i * p] = res[i] * res[p];}}
    return res;}
