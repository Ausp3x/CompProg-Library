#pragma once
#include "../01-Core/01-template.hpp"
#include "../01-Core/05-modint.hpp"
#include "13-convolution.hpp"
#include "14-polynomial_algorithms.hpp"

namespace recurrence_detail {
    inline constexpr polynomial_detail::Cut BM{1280, 6144};
    template<typename M>
    vector<M> state(const vector<M> &q, const vector<M> &w, ulng l) {
        int d = int(q.size()) - 1;
        M c = inv(q[0]);
        vector<M> lam(d + 1);
        for (int j = 0; j <= d; ++j) { lam[d - j] = q[j] * c; }
        auto x = polyPowMod(vector<M>{0, 1}, l, lam);
        x.resize(d);
        reverse(x.begin(), x.end());
        return middleProduct(vector<M>(w.begin(), w.begin() + (2 * d - 1)), x);}
} // namespace recurrence_detail

// T: O(n^2), M: O(n); any modint; c with s_i = sum_{j=1}^{L} c_j s_{i-j} for L <= i < n (minimal over a field); false when a discrepancy to invert is not a unit.
template<typename M>
bool findLinearRecurrence(const vector<M> &s, vector<M> &c) {
    int n = int(s.size()), l = 0, m = 0;
    vector<M> cur(n + 1), prev(n + 1), t;
    cur[0] = prev[0] = 1;
    M b = 1;
    for (int i = 0; i < n; ++i) {
        ++m;
        M d = s[i], ib;
        for (int j = 1; j <= l; ++j) { d += cur[j] * s[i - j]; }
        if (d == M(0)) { continue; }
        if (!tryInv(b, ib)) { return false; }
        t = cur;
        M f = d * ib;
        for (int j = m; j <= n; ++j) { cur[j] -= f * prev[j - m]; }
        if (2 * l > i) { continue; }
        l = i + 1 - l; prev = t; b = d; m = 0;}
    c.resize(l);
    for (int j = 0; j < l; ++j) { c[j] = -cur[j + 1]; }
    return true;}
// T: O(n^2), M: O(n); prime field; minimal recurrence of findLinearRecurrence.
template<typename M>
vector<M> berlekampMassey(const vector<M> &s) {
    vector<M> c;
    [[maybe_unused]] bool ok = findLinearRecurrence(s, c);
    assert(ok);
    return c;}
// T: O(n * log(n)^2), M: O(n); prime field; a minimal recurrence of s by the Euclidean (Pade) form of Berlekamp-Massey.
template<typename M>
vector<M> findLinearRecurrenceHalfGcd(const vector<M> &s) {
    int n = int(s.size());
    vector<M> a = polyNormalize(vector<M>(s.rbegin(), s.rend())), x(n + 1);
    if (a.empty()) { return {}; }
    x[n] = 1;
    auto t = polyHalfGcd(x, a);
    auto r0 = polyAdd(polyMul(t[0], x), polyMul(t[1], a)), r1 = polyAdd(polyMul(t[2], x), polyMul(t[3], a));
    auto lam = t[3];
    if (r1.size() >= lam.size()) { lam = polySub(t[1], polyMul(polyDivmod(r0, r1).first, t[3])); }
    int l = int(lam.size()) - 1;
    M f = inv(lam.back());
    vector<M> c(l);
    for (int j = 1; j <= l; ++j) { c[j - 1] = -lam[l - j] * f; }
    return c;}

// T: O(d^2 * log(k)), M: O(d); commutative semiring (add, mul, zero, one), |a| >= d = |c|; a_k for a_i = add over j of mul(c_j, a_{i-j}), i >= d.
template<typename T, typename Add, typename Mul>
T linearRecurrenceSemiring(const vector<T> &c, const vector<T> &a, ulng k, Add add, Mul mul, T zero, T one) {
    int d = int(c.size());
    assert(int(a.size()) >= d);
    if (!d) { return zero; }
    vector<T> r(d, zero);
    r[0] = one;
    // r = x^k mod (x^d - sum c_j x^(d - j)); the reduction only adds, so it is valid in a semiring.
    for (int bit = 63 - std::countl_zero(k | 1); bit >= 0; --bit) {
        vector<T> s(2 * d, zero);
        int o = int(k >> bit & 1);
        for (int i = 0; i < d; ++i) { for (int j = 0; j < d; ++j) { s[i + j + o] = add(s[i + j + o], mul(r[i], r[j])); } }
        for (int i = 2 * d - 1; i >= d; --i) { for (int j = 1; j <= d; ++j) { s[i - j] = add(s[i - j], mul(s[i], c[j - 1])); } }
        r.assign(s.begin(), s.begin() + d);}
    T res = zero;
    for (int i = 0; i < min(d, int(a.size())); ++i) { res = add(res, mul(r[i], a[i])); }
    return res;}
// T: O(d^2 * log(k)), M: O(d); any modint, |a| >= d = |c|; Kitamasa a_k without transforms.
template<typename M>
M kitamasa(const vector<M> &c, const vector<M> &a, ulng k) { return linearRecurrenceSemiring(c, a, k, std::plus<M>(), std::multiplies<M>(), M(0), M(1)); }
// T: O(d * log(d) * log(k)), M: O(d); any modint; x^k mod (x^d - sum c_j x^(d - j)) as d coefficients, a_k = sum r_i a_i.
template<typename M>
vector<M> fiduccia(const vector<M> &c, ulng k) {
    int d = int(c.size());
    vector<M> lam(d + 1);
    lam[d] = 1;
    for (int j = 1; j <= d; ++j) { lam[d - j] = -c[j - 1]; }
    auto r = polyPowMod(vector<M>{0, 1}, k, lam);
    r.resize(d);
    return r;}
// T: O(n * log(n) * log(k)), M: O(n); n = |P| + |Q|, Q[0] a unit; [x^k] P / Q.
template<typename M>
M bostanMori(vector<M> p, vector<M> q, ulng k) {
    assert(!q.empty() && q[0] != M(0));
    while (k >= max(p.size(), q.size())) {
        int np = int(p.size()), nq = int(q.size()), len = int(std::bit_ceil(uint(max(np, nq) + nq - 1)));
        vector<M> u, v;
        if constexpr (convolution_detail::nttPrime<M>()) {
            if (len >= 2 && len <= convolution_detail::LIMIT<M>) {
                u = p; v = q;
                u.resize(len); v.resize(len);
                ntt(u); ntt(v);
                // In bit-reversed spectra Q(-x) at slot i is Q(x) at slot i ^ 1.
                for (int i = 0; i < len; i += 2) {
                    M b = v[i];
                    u[i] *= v[i + 1]; u[i + 1] *= b; v[i] = v[i + 1] = b * v[i + 1];}
                intt(u); intt(v);
                u.resize(max(np + nq - 1, 0)); v.resize(2 * nq - 1);}}
        if (v.empty()) {
            auto qm = q;
            for (int i = 1; i < nq; i += 2) { qm[i] = -qm[i]; }
            u = convolution(p, qm); v = convolution(q, qm);}
        p.clear(); q.clear();
        for (int i = int(k & 1); i < int(u.size()); i += 2) { p.pb(u[i]); }
        for (int i = 0; i < int(v.size()); i += 2) { q.pb(v[i]); }
        k >>= 1;}
    return fpsDiv(p, q, int(k) + 1)[k];}
// T: O(d * log(d)), M: O(d); any modint, |a| >= d = |c|; (P, Q) with sum a_i x^i = P / Q, Q = 1 - sum c_j x^j, |P| = d.
template<typename M>
pair<vector<M>, vector<M>> rationalGeneratingFunction(const vector<M> &c, const vector<M> &a) {
    int d = int(c.size());
    assert(int(a.size()) >= d);
    vector<M> q(d + 1);
    q[0] = 1;
    for (int j = 1; j <= d; ++j) { q[j] = -c[j - 1]; }
    return {truncatedConvolution(vector<M>(a.begin(), a.begin() + min(int(a.size()), d)), q, d), q};}
// T: O(d * log(d) * log(k)), M: O(d); any modint, |a| >= d = |c|; a_k, and the prefix sum of a_i for i < k, of the recurrence fixed by c and a_0 .. a_{d-1}.
template<typename M>
M nthTermLinearRecurrence(const vector<M> &c, const vector<M> &a, ulng k) {
    auto [p, q] = rationalGeneratingFunction(c, a);
    return bostanMori(p, q, k);}
template<typename M>
M prefixSumLinearRecurrence(const vector<M> &c, const vector<M> &a, ulng k) {
    if (!k) { return 0; }
    auto [p, q] = rationalGeneratingFunction(c, a);
    return bostanMori(p, convolution(q, vector<M>{1, -1}), k - 1);}
// T: O(n * log(n) + d * log(d) * log(l)), M: O(n); n = |P| + |Q| + r - l, Q[0] a unit, l <= r; coefficients [l, r) of P / Q.
template<typename M>
vector<M> sliceRationalFps(vector<M> p, vector<M> q, ulng l, ulng r) {
    polynomial_detail::trim(p); polynomial_detail::trim(q);
    assert(l <= r && !q.empty() && q[0] != M(0));
    int d = int(q.size()) - 1, e = max(int(p.size()) - d, 0);
    if (l <= ulng(e + d)) {
        auto u = fpsDiv(p, q, int(r));
        return vector<M>(u.begin() + int(l), u.end());}
    if (!d) { return vector<M>(r - l); }
    auto u = fpsDiv(p, q, e + 2 * d - 1);
    auto w = recurrence_detail::state(q, vector<M>(u.begin() + e, u.end()), l - ulng(e));
    return fpsDiv(truncatedConvolution(w, q, d), q, int(r - l));}
// T: O(d * log(d) * log(k) + m * log(m)), M: O(d + m); any modint, |a| >= d = |c|; a_k .. a_{k + m - 1}.
template<typename M>
vector<M> consecutiveTerms(const vector<M> &c, const vector<M> &a, ulng k, int m) {
    assert(m >= 0);
    auto [p, q] = rationalGeneratingFunction(c, a);
    return sliceRationalFps(p, q, k, k + ulng(m));}
// T: O(n * log(n)^2 + n * log(n) * log(k)), M: O(n); prime field; s_k from samples of a recurrence of order L with 2L <= n (Berlekamp-Massey below the cutoff).
template<typename M>
M interpolateLinearRecurrence(const vector<M> &s, ulng k) {
    if (k < s.size()) { return s[k]; }
    auto c = int(s.size()) <= polynomial_detail::cut<M>(recurrence_detail::BM) ? berlekampMassey(s) : findLinearRecurrenceHalfGcd(s);
    return nthTermLinearRecurrence(c, s, k);}
// T: O(n), M: O(n); any T with ==, b >= 1, preperiod and period at most b, n = |s| >= 3b; s_k.
template<typename T>
T interpolatePeriodicSequence(const vector<T> &s, int b, ulng k) {
    int n = int(s.size()) - b;
    assert(b >= 1 && n >= 2 * b);
    if (k < s.size()) { return s[k]; }
    if (b < 1 || n < 1) { return T(); }
    vector<int> pi(n);
    for (int i = 1; i < n; ++i) {
        int j = pi[i - 1];
        while (j && !(s[b + i] == s[b + j])) { j = pi[j - 1]; }
        pi[i] = j + (s[b + i] == s[b + j]);}
    ulng p = ulng(n - pi[n - 1]);
    return s[b + (k - ulng(b)) % p];}
// T: O(n + log(k)), M: O(n); n = |s| <= mod prime; s_i = f(i) r^i with deg f < n; s_k.
template<typename M>
M interpolatePolyExp(const vector<M> &s, M r, ulng k) {
    int n = int(s.size());
    assert(n >= 1 && ulng(n) <= M::mod());
    if (k < ulng(n)) { return s[k]; }
    if (r == M(0)) { return 0; }
    auto [f, g] = polynomial_detail::factorials<M>(n - 1);
    M x = M(k), ir = inv(r), w = 1, res = 0;
    vector<M> pre(n + 1, M(1)), suf(n + 1, M(1));
    for (int i = 0; i < n; ++i) { pre[i + 1] = pre[i] * (x - M(i)); }
    for (int i = n - 1; i >= 0; --i) { suf[i] = suf[i + 1] * (x - M(i)); }
    for (int i = 0; i < n; ++i, w *= ir) {
        M t = s[i] * w * pre[i] * suf[i + 1] * g[i] * g[n - 1 - i];
        res += (n - 1 - i) % 2 ? -t : t;}
    return res * pow(r, k);}
// T: O(d^3 + d * log(d) * log(k)), M: O(d^2); prime field, A d x d; A^k v via the Krylov annihilator of v and x^k modulo it.
template<typename M>
vector<M> matrixPowerVector(const vector<vector<M>> &a, const vector<M> &v, ulng k) {
    int d = int(v.size());
    assert(int(a.size()) == d);
    if (int(a.size()) != d) { return vector<M>(d); }
    vector<vector<M>> w, red, comb;
    vector<int> piv;
    vector<M> mu;
    for (int m = 0; m <= d; ++m) {
        vector<M> x = m ? vector<M>(d) : v, y(d + 1);
        if (m) { for (int i = 0; i < d; ++i) { for (int j = 0; j < d; ++j) { x[i] += a[i][j] * w[m - 1][j]; } } }
        w.pb(x);
        y[m] = 1;
        for (int t = 0; t < m && t < int(red.size()); ++t) {
            M f = x[piv[t]];
            if (f == M(0)) { continue; }
            for (int j = 0; j < d; ++j) { x[j] -= f * red[t][j]; }
            for (int j = 0; j <= d; ++j) { y[j] -= f * comb[t][j]; }}
        int p = 0;
        while (p < d && x[p] == M(0)) { ++p; }
        if (p == d) { mu.assign(y.begin(), y.begin() + m + 1); break; }
        M f = inv(x[p]);
        for (auto &z : x) { z *= f; }
        for (auto &z : y) { z *= f; }
        red.pb(x); comb.pb(y); piv.pb(p);}
    auto r = polyPowMod(vector<M>{0, 1}, k, mu);
    vector<M> res(d);
    for (int i = 0; i < int(r.size()); ++i) { for (int j = 0; j < d; ++j) { res[j] += r[i] * w[i][j]; } }
    return res;}
