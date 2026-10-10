#pragma once
#include "../01-Core/01-template.hpp"
#include "../01-Core/05-modint.hpp"
#include "13-convolution.hpp"

template<typename M> vector<M> fpsInverse(const vector<M> &a, int n);
template<typename M> pair<vector<M>, vector<M>> polyDivmod(vector<M> a, vector<M> b);

namespace polynomial_detail {
    // Largest sizes on the O(n^2) paths, for NTT primes and for other moduli (three-prime products).
    struct Cut { int ntt, other; };
    inline constexpr Cut DIV{256, 1536}, GCD{3072, 12288}, HGCD{128, 512}, INV{112, 1536}, LOG{112, 896}, EXP{224, 2048}, POW{144, 256}, SQRT{80, 768}, SHIFT{40, 112};
    template<typename M> constexpr int cut(Cut c) { return convolution_detail::nttPrime<M>() ? c.ntt : c.other; }
    template<typename M> using Mat = array<vector<M>, 4>;
    template<typename M> using Log = vector<pair<int, M>>;
    template<typename M>
    vector<M> &trim(vector<M> &a) {
        while (!a.empty() && a.back() == M(0)) { a.pop_back(); }
        return a;}
    template<typename M> int deg(const vector<M> &a) { return int(a.size()) - 1; }
    template<typename M> M coef(const vector<M> &a, int i) { return i < int(a.size()) ? a[i] : M(0); }
    template<typename M>
    vector<M> mul(const vector<M> &a, const vector<M> &b) {
        auto c = convolution(a, b);
        return trim(c);}
    template<typename M>
    vector<M> addSub(vector<M> a, const vector<M> &b, bool sub) {
        if (a.size() < b.size()) { a.resize(b.size()); }
        for (int i = 0; i < int(b.size()); ++i) { a[i] = sub ? a[i] - b[i] : a[i] + b[i]; }
        return trim(a);}
    template<typename M> vector<M> drop(const vector<M> &a, int k) { return k < int(a.size()) ? vector<M>(a.begin() + k, a.end()) : vector<M>(); }
    template<typename M>
    vector<M> cyclic(vector<M> a, vector<M> b, int len) {
        a.resize(len); b.resize(len);
        return cyclicConvolution(a, b);}
    template<typename M>
    vector<M> inverses(int n) {
        vector<M> r(max(n, 2), M(1));
        const ulng p = M::mod();
        for (int i = 2; i < n; ++i) { r[i] = -r[p % ulng(i)] * M(p / ulng(i)); }
        r.resize(n);
        return r;}
    template<typename M>
    pair<vector<M>, vector<M>> factorials(int n) {
        vector<M> f(n + 1, M(1)), g(n + 1);
        for (int i = 1; i <= n; ++i) { f[i] = f[i - 1] * M(i); }
        g[n] = inv(f[n]);
        for (int i = n; i > 0; --i) { g[i - 1] = g[i] * M(i); }
        return {f, g};}
    template<typename M>
    vector<pair<int, M>> terms(const vector<M> &a, int from, int n, M scale) {
        vector<pair<int, M>> res;
        for (int i = from + 1; i < min(int(a.size()), from + n); ++i) { if (a[i] != M(0)) { res.emplace_back(i - from, a[i] * scale); } }
        return res;}
    // c * u^k mod x^n for u = 1 + sum t_j x^j, from u f' = k u' f.
    template<typename M>
    vector<M> powUnit(const vector<pair<int, M>> &t, M k, M c, int n) {
        auto iv = inverses<M>(n);
        vector<M> f(n);
        if (n) { f[0] = c; }
        for (int i = 1; i < n; ++i) {
            M s = 0;
            for (auto &[j, x] : t) {
                if (j > i) { break; }
                s += (k * M(j) - M(i - j)) * x * f[i - j];}
            f[i] = s * iv[i];}
        return f;}
    template<typename M> Mat<M> eye() { return {vector<M>{1}, {}, {}, vector<M>{1}}; }
    template<typename M>
    void apply(const Mat<M> &t, vector<M> &a, vector<M> &b) {
        auto c = addSub(mul(t[0], a), mul(t[1], b), false);
        b = addSub(mul(t[2], a), mul(t[3], b), false);
        a = std::move(c);}
    template<typename M>
    Mat<M> compose(const Mat<M> &s, const Mat<M> &t) {
        return {addSub(mul(s[0], t[0]), mul(s[1], t[2]), false), addSub(mul(s[0], t[1]), mul(s[1], t[3]), false),
                addSub(mul(s[2], t[0]), mul(s[3], t[2]), false), addSub(mul(s[2], t[1]), mul(s[3], t[3]), false)};}
    // One Euclid step (a, b) -> (b, a mod b); t becomes [[0, 1], [1, -q]] * t and log receives (deg q, lc q).
    template<typename M>
    void step(vector<M> &a, vector<M> &b, Mat<M> *t, Log<M> *log) {
        auto [q, r] = polyDivmod(a, b);
        if (log) { log->emplace_back(deg(q), q.back()); }
        if (t) {
            auto &x = *t;
            auto u = addSub(x[0], mul(q, x[2]), true), v = addSub(x[1], mul(q, x[3]), true);
            x = {std::move(x[2]), std::move(x[3]), std::move(u), std::move(v)};}
        a = std::move(b); b = std::move(r);}
    // deg a > deg b; T(a, b) = consecutive remainders (c, d) with deg c >= ceil(deg a / 2) > deg d.
    template<typename M>
    Mat<M> hgcd(vector<M> a, vector<M> b, Log<M> *log, int base) {
        int m = (deg(a) + 1) / 2;
        if (deg(b) < m) { return eye<M>(); }
        if (deg(a) <= base) {
            Mat<M> t = eye<M>();
            while (deg(b) >= m) { step(a, b, &t, log); }
            return t;}
        auto r = hgcd(drop(a, m), drop(b, m), log, base);
        apply(r, a, b);
        if (deg(b) < m) { return r; }
        step(a, b, &r, log);
        if (deg(b) < m) { return r; }
        int k = 2 * m - deg(a);
        return compose(hgcd(drop(a, k), drop(b, k), log, base), r);}
    // deg a >= deg b; returns the last nonzero remainder, accumulating the transform and the quotient log; half-GCD steps above degree top.
    template<typename M>
    vector<M> euclid(vector<M> a, vector<M> b, Mat<M> *t, Log<M> *log, int top, int base) {
        while (!b.empty()) {
            if (deg(a) <= top) {
                // The naive tail builds its own small transform and composes it once.
                Mat<M> s = eye<M>();
                while (!b.empty()) { step(a, b, t ? &s : nullptr, log); }
                if (t) { *t = compose(s, *t); }
                break;}
            if (deg(a) > deg(b)) {
                auto s = hgcd(a, b, log, base);
                apply(s, a, b);
                if (t) { *t = compose(s, *t); }
                if (b.empty()) { break; }}
            step(a, b, t, log);}
        return a;}
    template<typename M>
    struct Reducer {
        vector<M> b, ib;
        explicit Reducer(vector<M> m) : b(std::move(trim(m))) {
            if (deg(b) > cut<M>(DIV)) { ib = fpsInverse(vector<M>(b.rbegin(), b.rend()), deg(b)); }}
        vector<M> operator()(vector<M> a) const {
            trim(a);
            int n = int(a.size()), d = deg(b);
            if (n <= d) { return a; }
            if (ib.empty() || n - d > d) { return polyDivmod(a, b).second; }
            auto q = truncatedConvolution(vector<M>(a.rbegin(), a.rbegin() + (n - d)), ib, n - d);
            reverse(q.begin(), q.end());
            auto bq = truncatedConvolution(b, q, d);
            a.resize(d);
            for (int i = 0; i < d; ++i) { a[i] -= bq[i]; }
            return trim(a);}
    };
    // f(g) by baby-step giant-step blocks; red maps a product to its canonical reduced form.
    template<typename M, typename R>
    vector<M> blockHorner(const vector<M> &f, const vector<M> &g, R red) {
        int n = int(f.size()), b = max(1, int(std::sqrt(double(n))));
        vector<vector<M>> pw{red(vector<M>{1})};
        for (int i = 0; i < b; ++i) { pw.pb(red(convolution(pw.back(), g))); }
        vector<M> res;
        for (int blk = (n - 1) / b; blk >= 0; --blk) {
            res = red(convolution(res, pw[b]));
            for (int i = 0; i < b && blk * b + i < n; ++i) {
                if (res.size() < pw[i].size()) { res.resize(pw[i].size()); }
                for (int j = 0; j < int(pw[i].size()); ++j) { res[j] += f[blk * b + i] * pw[i][j]; }}}
        return red(res);}
    template<typename T>
    vector<T> cyclotomic(int n) {
        assert(n >= 1);
        if (n == 1) { return {T(0) - T(1), T(1)}; }
        vector<int> ps;
        int m = n, rad = 1, phi = 1;
        for (int p = 2; p <= m / p; ++p) {
            if (m % p) { continue; }
            ps.pb(p); rad *= p; phi *= p - 1;
            while (m % p == 0) { m /= p; }}
        if (m > 1) { ps.pb(m); rad *= m; phi *= m - 1; }
        vector<T> s(phi + 1);
        s[0] = T(1);
        for (int mask = 0; mask < 1 << ps.size(); ++mask) {
            int d = rad;
            for (int i = 0; i < int(ps.size()); ++i) { if (mask >> i & 1) { d /= ps[i]; } }
            // Phi_rad = prod over d | rad of (1 - x^d)^mu(rad / d), as a series mod x^(phi + 1).
            if (std::popcount(uint(mask)) & 1) { for (int i = d; i <= phi; ++i) { s[i] = s[i] + s[i - d]; } }
            else { for (int i = phi; i >= d; --i) { s[i] = s[i] - s[i - d]; } }}
        int e = n / rad;
        vector<T> res(lng(phi) * e + 1);
        for (int i = 0; i <= phi; ++i) { res[lng(i) * e] = s[i]; }
        return res;}
    template<typename M>
    vector<M> expNewton(const vector<M> &a, int n) {
        auto iv = inverses<M>(n);
        // f = exp(a) mod x^m, g = 1 / f mod x^m; log f = a mod x^m makes r = f' - f a' vanish below x^(m - 1).
        vector<M> f{1}, g{1};
        for (int m = 1; m < n; m *= 2) {
            if (m > 1) {
                auto e = cyclic(f, g, m);
                auto u = cyclic(g, vector<M>(e.begin() + m / 2, e.end()), m);
                g.resize(m);
                for (int i = 0; i < m / 2; ++i) { g[m / 2 + i] = -u[i]; }}
            vector<M> q(m - 1), r(m - 1), s(m);
            for (int i = 0; i < m - 1; ++i) { q[i] = coef(a, i + 1) * M(i + 1); }
            auto c = cyclic(f, q, m);
            if (m > 1) { r[0] = -c[m - 1]; }
            for (int i = 0; i + 2 < m; ++i) { r[i + 1] = f[i + 1] * M(i + 1) - c[i]; }
            auto w = cyclic(g, r, 2 * m);
            for (int i = 0; i < m && m + i < n; ++i) { s[i] = coef(a, m + i) - w[i] * iv[m + i]; }
            auto t = cyclic(f, s, 2 * m);
            f.resize(2 * m);
            for (int i = 0; i < m; ++i) { f[m + i] = t[i]; }}
        f.resize(n);
        return f;}
    template<typename M>
    vector<M> invNewton(const vector<M> &a, int n) {
        vector<M> g{inv(a[0])};
        for (int m = 1; m < n; m *= 2) {
            // a * g = 1 + x^m * t (mod x^(2m)); the cyclic wrap only touches indices below m.
            auto e = cyclic(vector<M>(a.begin(), a.begin() + min(int(a.size()), 2 * m)), g, 2 * m);
            auto h = cyclic(g, vector<M>(e.begin() + m, e.end()), 2 * m);
            g.resize(2 * m);
            for (int i = 0; i < m; ++i) { g[m + i] = -h[i]; }}
        g.resize(n);
        return g;}
    template<typename M>
    vector<M> logNewton(const vector<M> &a, int n) {
        vector<M> d(n - 1);
        for (int i = 0; i + 1 < min(int(a.size()), n); ++i) { d[i] = a[i + 1] * M(i + 1); }
        auto q = truncatedConvolution(d, invNewton(a, n - 1), n - 1);
        auto iv = inverses<M>(n);
        vector<M> res(n);
        for (int i = 1; i < n; ++i) { res[i] = q[i - 1] * iv[i]; }
        return res;}
    // g = sqrt(u) with g[0] = c and h = 1 / g, both doubled mod x^m.
    template<typename M>
    vector<M> sqrtNewton(const vector<M> &u, M c, int len) {
        vector<M> g{c}, h{inv(c)};
        const M half = inv(M(2));
        for (int m = 1; m < len; m *= 2) {
            auto sq = cyclic(g, g, 2 * m);
            vector<M> d(m);
            for (int i = 0; i < m; ++i) { d[i] = coef(u, m + i) - sq[m + i]; }
            auto e = cyclic(h, d, 2 * m);
            g.resize(2 * m);
            for (int i = 0; i < m; ++i) { g[m + i] = e[i] * half; }
            if (2 * m >= len) { break; }
            auto f = cyclic(g, h, 2 * m);
            auto t = cyclic(h, vector<M>(f.begin() + m, f.end()), 2 * m);
            h.resize(2 * m);
            for (int i = 0; i < m; ++i) { h[m + i] = -t[i]; }}
        g.resize(len);
        return g;}
    template<typename M>
    vector<M> sqrtImpl(const vector<M> &a, int n, bool sparse) {
        assert(n >= 0 && M::mod() % 2);
        int v = 0;
        while (v < int(a.size()) && a[v] == M(0)) { ++v; }
        vector<M> res(n);
        if (v == int(a.size())) { return res; }
        M c;
        if (v % 2 || !trySqrt(a[v], c)) { return {}; }
        if (-c < c) { c = -c; }
        int len = n - v / 2;
        if (len <= 0) { return res; }
        assert(!sparse || ulng(len) <= M::mod());
        vector<M> u;
        if (sparse || (len <= cut<M>(SQRT) && ulng(len) <= M::mod())) { u = powUnit(terms(a, v, len, inv(a[v])), inv(M(2)), c, len); }
        else { u = sqrtNewton(vector<M>(a.begin() + v, a.begin() + min(int(a.size()), v + len)), c, len); }
        std::copy(u.begin(), u.end(), res.begin() + v / 2);
        return res;}
} // namespace polynomial_detail

// T: O(n), M: O(n); n = |a| + |b|; results trimmed of trailing zeros (the zero polynomial is empty).
template<typename M> vector<M> polyAdd(const vector<M> &a, const vector<M> &b) { return polynomial_detail::addSub(a, b, false); }
template<typename M> vector<M> polySub(const vector<M> &a, const vector<M> &b) { return polynomial_detail::addSub(a, b, true); }
// T: O(n * log(n)), M: O(n); n = |a| + |b|; any modint, trimmed.
template<typename M> vector<M> polyMul(const vector<M> &a, const vector<M> &b) { return polynomial_detail::mul(a, b); }
// T: O(n), M: O(n); derivative (any modint), integral with zero constant (|a| < mod, prime), both trimmed.
template<typename M>
vector<M> polyDerivative(const vector<M> &a) {
    vector<M> res(max(int(a.size()) - 1, 0));
    for (int i = 0; i < int(res.size()); ++i) { res[i] = a[i + 1] * M(i + 1); }
    return polynomial_detail::trim(res);}
template<typename M>
vector<M> polyIntegral(const vector<M> &a) {
    int n = int(a.size());
    assert(ulng(n) < M::mod());
    auto iv = polynomial_detail::inverses<M>(n + 1);
    vector<M> res(n + 1);
    for (int i = 0; i < n; ++i) { res[i + 1] = a[i] * iv[i + 1]; }
    return polynomial_detail::trim(res);}
// T: O(n), M: O(1); Horner evaluation a(x).
template<typename M>
M polyEval(const vector<M> &a, M x) {
    M res = 0;
    for (int i = int(a.size()) - 1; i >= 0; --i) { res = res * x + a[i]; }
    return res;}
// T: O(n + k), M: O(n + k); k >= 0 multiplies by x^k, k < 0 drops the -k lowest coefficients; trimmed.
template<typename M>
vector<M> polyShift(vector<M> a, int k) {
    polynomial_detail::trim(a);
    if (k < 0) { return polynomial_detail::drop(a, -k); }
    if (!a.empty()) { a.insert(a.begin(), k, M(0)); }
    return a;}
// T: O(n), M: O(n); truncate: exactly n >= 0 coefficients of a mod x^n, zero-padded; normalize: trailing zeros removed.
template<typename M>
vector<M> polyTruncate(vector<M> a, int n) {
    assert(n >= 0);
    a.resize(n);
    return a;}
template<typename M> vector<M> polyNormalize(vector<M> a) { return polynomial_detail::trim(a); }

// T: O(n * k), M: O(n); k = nonzero coefficients of a below x^n; dense contracts of fpsInverse, fpsLog, fpsExp, fpsPow (k >= 0), fpsSqrt.
template<typename M>
vector<M> fpsSparseInverse(const vector<M> &a, int n) {
    assert(n >= 0 && !a.empty() && a[0] != M(0));
    auto t = polynomial_detail::terms(a, 0, n, M(1));
    M c = inv(a[0]);
    vector<M> g(n);
    for (int i = 0; i < n; ++i) {
        M s = i ? M(0) : M(1);
        for (auto &[j, x] : t) {
            if (j > i) { break; }
            s -= x * g[i - j];}
        g[i] = s * c;}
    return g;}
template<typename M>
vector<M> fpsSparseLog(const vector<M> &a, int n) {
    assert(n >= 0 && ulng(n) <= M::mod() && !a.empty() && a[0] == M(1));
    auto t = polynomial_detail::terms(a, 0, n, M(1));
    auto iv = polynomial_detail::inverses<M>(n);
    vector<M> b(n);
    // i b_i = i a_i - sum_{j >= 1} a_j (i - j) b_{i - j}.
    for (int i = 1; i < n; ++i) {
        M s = polynomial_detail::coef(a, i) * M(i);
        for (auto &[j, x] : t) {
            if (j >= i) { break; }
            s -= x * M(i - j) * b[i - j];}
        b[i] = s * iv[i];}
    return b;}
template<typename M>
vector<M> fpsSparseExp(const vector<M> &a, int n) {
    assert(n >= 0 && ulng(n) <= M::mod() && (a.empty() || a[0] == M(0)));
    auto t = polynomial_detail::terms(a, 0, n, M(1));
    for (auto &[j, x] : t) { x *= M(j); }
    auto iv = polynomial_detail::inverses<M>(n);
    vector<M> f(n);
    if (n) { f[0] = 1; }
    for (int i = 1; i < n; ++i) {
        for (auto &[j, x] : t) {
            if (j > i) { break; }
            f[i] += x * f[i - j];}
        f[i] *= iv[i];}
    return f;}
template<typename M>
vector<M> fpsSparsePow(const vector<M> &a, lng k, int n) {
    assert(n >= 0 && k >= 0);
    vector<M> res(n);
    int v = 0;
    while (v < int(a.size()) && a[v] == M(0)) { ++v; }
    if (!n || k <= 0 || v == int(a.size())) {
        if (n && k == 0) { res[0] = 1; }
        return res;}
    if (v && k >= (n + v - 1) / v) { return res; }
    int len = n - int(v * k);
    assert(ulng(len) <= M::mod());
    auto u = polynomial_detail::powUnit(polynomial_detail::terms(a, v, len, inv(a[v])), M(k), pow(a[v], ulng(k)), len);
    std::copy(u.begin(), u.end(), res.begin() + (n - len));
    return res;}
template<typename M> vector<M> fpsSparseSqrt(const vector<M> &a, int n) { return polynomial_detail::sqrtImpl(a, n, true); }

// T: O(n * log(n)), M: O(n); n >= 0, a[0] a unit; exactly n coefficients of 1 / a.
template<typename M>
vector<M> fpsInverse(const vector<M> &a, int n) {
    assert(n >= 0 && !a.empty() && a[0] != M(0));
    return n <= polynomial_detail::cut<M>(polynomial_detail::INV) ? fpsSparseInverse(a, n) : polynomial_detail::invNewton(a, n);}
// T: O(n * log(n)), M: O(n); g[0] a unit; exactly n coefficients of f / g.
template<typename M> vector<M> fpsDiv(const vector<M> &f, const vector<M> &g, int n) { return truncatedConvolution(f, fpsInverse(g, n), n); }
// T: O(n * log(n)), M: O(n); a[0] = 1, n <= mod prime; exactly n coefficients of log(a).
template<typename M>
vector<M> fpsLog(const vector<M> &a, int n) {
    assert(n >= 0 && ulng(n) <= M::mod() && !a.empty() && a[0] == M(1));
    return n <= polynomial_detail::cut<M>(polynomial_detail::LOG) ? fpsSparseLog(a, n) : polynomial_detail::logNewton(a, n);}
// T: O(n * log(n)), M: O(n); a[0] = 0, n <= mod prime; exactly n coefficients of exp(a).
template<typename M>
vector<M> fpsExp(const vector<M> &a, int n) {
    assert(n >= 0 && ulng(n) <= M::mod() && (a.empty() || a[0] == M(0)));
    return n <= polynomial_detail::cut<M>(polynomial_detail::EXP) ? fpsSparseExp(a, n) : polynomial_detail::expNewton(a, n);}
// T: O(n * log(n)), M: O(n); a^k mod x^n, k < 0 needs a[0] != 0, 0^0 = 1, n - v * k <= mod prime for the valuation v.
template<typename M>
vector<M> fpsPow(const vector<M> &a, lng k, int n) {
    using namespace polynomial_detail;
    assert(n >= 0);
    if (!n) { return {}; }
    vector<M> res(n);
    int v = 0;
    while (v < int(a.size()) && a[v] == M(0)) { ++v; }
    if (k == 0) { res[0] = 1; return res; }
    if (v == int(a.size())) { assert(k > 0); return res; }
    assert(k > 0 || v == 0);
    if (v && (k < 0 || k >= (n + v - 1) / v)) { return res; }
    int len = n - int(v * k);
    assert(ulng(len) <= M::mod());
    M c = a[v], ic = inv(c), kk = M(k), ck = k > 0 ? pow(c, ulng(k)) : pow(ic, ulng(-(k + 1)) + 1);
    vector<M> u;
    if (len <= cut<M>(POW)) { u = powUnit(terms(a, v, len, ic), kk, ck, len); }
    else {
        u.assign(a.begin() + v, a.begin() + min(int(a.size()), v + len));
        for (auto &x : u) { x *= ic; }
        u = fpsLog(u, len);
        for (auto &x : u) { x *= kk; }
        u = fpsExp(u, len);
        for (auto &x : u) { x *= ck; }}
    std::copy(u.begin(), u.end(), res.begin() + (n - len));
    return res;}
// T: O(n * log(n)), M: O(n); a^(p / q) with q > 0 a unit, lowest nonzero coefficient 1; empty when no power series answer.
template<typename M>
vector<M> fpsPow(const vector<M> &a, lng p, lng q, int n) {
    assert(n >= 0 && q > 0 && M(q) != M(0));
    int v = 0;
    while (v < int(a.size()) && a[v] == M(0)) { ++v; }
    if (v == int(a.size())) { return p < 0 ? vector<M>() : fpsPow(a, p ? 1 : 0, n); }
    if (lll(v) * p % q || (v && p < 0)) { return {}; }
    assert(a[v] == M(1));
    vector<M> res(n);
    lll s = lll(v) * p / q;
    if (s >= n) { return res; }
    int len = n - int(s);
    auto u = fpsLog(vector<M>(a.begin() + v, a.begin() + min(int(a.size()), v + len)), len);
    M r = M(p) / M(q);
    for (auto &x : u) { x *= r; }
    u = fpsExp(u, len);
    std::copy(u.begin(), u.end(), res.begin() + int(s));
    return res;}
// T: O(n * log(n)), M: O(n); odd prime; root with the smaller constant val(); empty for odd valuation or a nonresidue.
template<typename M>
vector<M> fpsSqrt(const vector<M> &a, int n) { return polynomial_detail::sqrtImpl(a, n, false); }

// T: O(n * log(n)), M: O(n); n = |a|; b nonzero with a unit leading coefficient; (q, r) trimmed with deg r < deg b.
template<typename M>
pair<vector<M>, vector<M>> polyDivmod(vector<M> a, vector<M> b) {
    using namespace polynomial_detail;
    trim(a); trim(b);
    assert(!b.empty());
    int n = int(a.size()), m = int(b.size()), k = n - m + 1;
    if (k <= 0 || !m) { return {{}, a}; }
    vector<M> q(k);
    if (min(k, m) <= cut<M>(DIV)) {
        M c = inv(b.back());
        for (int i = k - 1; i >= 0; --i) {
            q[i] = a[i + m - 1] * c;
            for (int j = 0; j + 1 < m; ++j) { a[i + j] -= q[i] * b[j]; }}}
    else {
        q = truncatedConvolution(vector<M>(a.rbegin(), a.rbegin() + k), fpsInverse(vector<M>(b.rbegin(), b.rend()), k), k);
        reverse(q.begin(), q.end());
        auto bq = truncatedConvolution(b, q, m - 1);
        for (int i = 0; i + 1 < m; ++i) { a[i] -= bq[i]; }}
    a.resize(m - 1);
    return {q, trim(a)};}
template<typename M> vector<M> polyMod(const vector<M> &a, const vector<M> &b) { return polyDivmod(a, b).second; }
// T: O(n), M: O(n); a = (x - c) * q + r; returns (q trimmed, r = a(c)).
template<typename M>
pair<vector<M>, M> polyDivLinear(vector<M> a, M c) {
    polynomial_detail::trim(a);
    if (a.empty()) { return {{}, M(0)}; }
    for (int i = int(a.size()) - 2; i >= 0; --i) { a[i] += c * a[i + 1]; }
    M r = a[0];
    a.erase(a.begin());
    return {a, r};}

// T: O(n * log(n)^2), M: O(n); prime field; monic gcd, empty when both are zero.
template<typename M>
vector<M> polyGcd(vector<M> a, vector<M> b) {
    using namespace polynomial_detail;
    trim(a); trim(b);
    if (a.size() < b.size()) { swap(a, b); }
    if (a.empty()) { return {}; }
    auto g = euclid<M>(a, b, nullptr, nullptr, cut<M>(GCD), cut<M>(HGCD));
    M c = inv(g.back());
    for (auto &x : g) { x *= c; }
    return g;}
// T: NA, M: O(n); monic gcd g and Euclidean cofactors x, y with x a + y b = g.
template<typename M>
struct PolyXgcd {
    vector<M> g, x, y;
};
// T: O(n * log(n)^2), M: O(n); prime field; g = x a + y b monic with Euclidean cofactors, all empty when a = b = 0.
template<typename M>
PolyXgcd<M> polyXgcd(vector<M> a, vector<M> b) {
    using namespace polynomial_detail;
    trim(a); trim(b);
    bool sw = a.size() < b.size();
    if (sw) { swap(a, b); }
    if (a.empty()) { return {}; }
    Mat<M> t = eye<M>();
    auto g = euclid<M>(a, b, &t, nullptr, cut<M>(GCD), cut<M>(HGCD));
    M c = inv(g.back());
    for (auto *p : {&g, &t[0], &t[1]}) { for (auto &x : *p) { x *= c; } }
    if (sw) { swap(t[0], t[1]); }
    return {g, t[0], t[1]};}
// T: O(n * log(n)^2), M: O(n); deg a > deg b, prime field; T = {t00, t01, t10, t11} maps (a, b) to remainders (c, d), deg c >= ceil(deg a / 2) > deg d.
template<typename M>
array<vector<M>, 4> polyHalfGcd(vector<M> a, vector<M> b) {
    polynomial_detail::trim(a); polynomial_detail::trim(b);
    assert(a.size() > b.size());
    if (a.size() <= b.size()) { return polynomial_detail::eye<M>(); }
    return polynomial_detail::hgcd<M>(a, b, nullptr, polynomial_detail::cut<M>(polynomial_detail::HGCD));}
// T: O(n * log(n)^2), M: O(n); prime field; lc(a)^deg(b) * prod b(roots of a), 0 when a or b is zero.
template<typename M>
M polyResultant(vector<M> a, vector<M> b) {
    using namespace polynomial_detail;
    trim(a); trim(b);
    if (a.empty() || b.empty()) { return 0; }
    M res = 1;
    if (a.size() < b.size()) {
        if (deg(a) & deg(b) & 1) { res = -res; }
        swap(a, b);}
    if (b.size() == 1) { return res * pow(b[0], deg(a)); }
    Log<M> log;
    euclid<M>(a, b, nullptr, &log, cut<M>(GCD), cut<M>(HGCD));
    int k = int(log.size());
    vector<int> d(k + 1, deg(a));
    vector<M> l(k + 1, a.back());
    // r_{i-1} = q_i r_i + r_{i+1}: deg r_i = deg r_{i-1} - deg q_i and lc r_i = lc r_{i-1} / lc q_i.
    for (int i = 1; i <= k; ++i) { d[i] = d[i - 1] - log[i - 1].first; l[i] = l[i - 1] / log[i - 1].second; }
    if (d[k] > 0) { return 0; }
    for (int i = 1; i < k; ++i) {
        if (d[i - 1] & d[i] & 1) { res = -res; }
        res *= pow(l[i], d[i - 1] - d[i + 1]);}
    return res * pow(l[k], d[k - 1]);}
// T: O(n * log(n)^2), M: O(n); deg a >= 1, prime field; (-1)^(n (n - 1) / 2) / lc(a) * Res(a, a') with formal deg a' = n - 1.
template<typename M>
M polyDiscriminant(vector<M> a) {
    polynomial_detail::trim(a);
    int n = int(a.size()) - 1;
    assert(n >= 1);
    auto d = polyDerivative(a);
    if (d.empty()) { return 0; }
    M res = polyResultant(a, d) * pow(a.back(), n - int(d.size())) / a.back();
    return lng(n) * (n - 1) / 2 % 2 ? -res : res;}
// T: O(n * log(n) * log(e)), M: O(n); n = deg m, m nonzero with a unit leading coefficient; a^e mod m, 0^0 = 1.
template<typename M>
vector<M> polyPowMod(const vector<M> &a, ulng e, const vector<M> &m) {
    polynomial_detail::Reducer<M> red(m);
    assert(!red.b.empty());
    auto x = red(a), res = red(vector<M>{1});
    for (; e; e >>= 1) {
        if (e & 1) { res = red(convolution(res, x)); }
        if (e > 1) { x = red(convolution(x, x)); }}
    return res;}
// T: O(n * log(n)^2), M: O(n); prime field, deg m >= 1; inverse of a modulo m, empty when gcd(a, m) != 1.
template<typename M>
vector<M> polyInverseMod(const vector<M> &a, const vector<M> &m) {
    assert(polynomial_detail::deg(polyNormalize(m)) >= 1);
    auto r = polyXgcd(polyMod(a, m), m);
    return r.g == vector<M>{1} ? r.x : vector<M>();}
// T: O(k * n + sqrt(k) * n * log(n)), M: O(sqrt(k) * n); k = |f|, n = deg h, h nonzero with a unit leading coefficient; f(g) mod h.
template<typename M>
vector<M> polyComposeMod(const vector<M> &f, const vector<M> &g, const vector<M> &h) {
    polynomial_detail::Reducer<M> red(h);
    assert(!red.b.empty());
    return polynomial_detail::blockHorner(f, red(g), red);}

// T: O(k * n + sqrt(k) * n * log(n)), M: O(sqrt(k) * n); k = |f| (min(|f|, n) when g[0] = 0); exactly n coefficients of f(g).
template<typename M>
vector<M> fpsComposition(const vector<M> &f, const vector<M> &g, int n) {
    assert(n >= 0);
    bool low = g.empty() || g[0] == M(0);
    vector<M> fn(f.begin(), f.begin() + (low ? min(int(f.size()), n) : int(f.size())));
    auto red = [n](vector<M> a) {
        if (int(a.size()) > n) { a.resize(n); }
        return a;};
    auto res = polynomial_detail::blockHorner(fn, red(g), red);
    res.resize(n);
    return res;}
// T: O(n^2), M: O(n * sqrt(n)); f[0] = 0, f[1] != 0, prime field; exactly n coefficients of the g with f(g) = x.
template<typename M>
vector<M> fpsCompositionalInverse(const vector<M> &f, int n) {
    assert(n >= 0 && f.size() >= 2 && f[0] == M(0) && f[1] != M(0));
    if (f.size() < 2) { return vector<M>(max(n, 0)); }
    vector<M> g{0, inv(f[1])};
    auto df = polyDerivative(f);
    for (int m = 2; m < n; m = min(2 * m, n)) {
        int len = min(2 * m, n);
        auto e = fpsComposition(f, g, len);
        e[1] -= 1;
        auto q = fpsDiv(e, fpsComposition(df, g, len), len);
        g.resize(len);
        for (int i = 0; i < len; ++i) { g[i] -= q[i]; }}
    g.resize(n);
    return g;}
// T: O(n * log(n)), M: O(n); a(x + c), same length; any modint (O(n^2) below the cutoff, for n > mod or a composite modulus).
template<typename M>
vector<M> taylorShift(vector<M> a, M c) {
    int n = int(a.size());
    if (n <= polynomial_detail::cut<M>(polynomial_detail::SHIFT) || ulng(n) > M::mod() || !M::is_prime) {
        for (int i = 0; i + 1 < n; ++i) { for (int j = n - 2; j >= i; --j) { a[j] += c * a[j + 1]; } }
        return a;}
    auto [f, g] = polynomial_detail::factorials<M>(n - 1);
    vector<M> e(n);
    M w = 1;
    for (int i = 0; i < n; ++i, w *= c) { a[i] *= f[i]; e[i] = w * g[i]; }
    reverse(a.begin(), a.end());
    // b_k k! = sum_j a_{k + j} (k + j)! c^j / j!, a correlation.
    auto b = truncatedConvolution(a, e, n);
    reverse(b.begin(), b.end());
    for (int i = 0; i < n; ++i) { b[i] *= g[i]; }
    return b;}
// T: O(n * log(n)^2), M: O(n); n = total length; product of all polynomials ({1} for none), trimmed.
template<typename M>
vector<M> productTree(const vector<vector<M>> &ps) {
    if (ps.empty()) { return {1}; }
    auto rec = [&](auto &&self, int l, int r) -> vector<M> {
        if (r - l == 1) { return polyNormalize(ps[l]); }
        int mid = (l + r) / 2;
        return polynomial_detail::mul(self(self, l, mid), self(self, mid, r));};
    return rec(rec, 0, int(ps.size()));}
// T: O(n * log(n)^2), M: O(n); monic prod (x - r_i).
template<typename M>
vector<M> polyFromRoots(const vector<M> &rs) {
    vector<vector<M>> ps;
    for (auto &r : rs) { ps.pb({-r, M(1)}); }
    return productTree(ps);}
// T: O(n * log(n)), M: O(n); n = |a| + |b| - 1 <= mod prime; c_k = sum C(k, i) a_i b_{k - i}, empty if a or b is.
template<typename M>
vector<M> egfConvolution(vector<M> a, vector<M> b) {
    if (a.empty() || b.empty()) { return {}; }
    int n = int(a.size() + b.size()) - 1;
    assert(ulng(n) <= M::mod());
    auto [f, g] = polynomial_detail::factorials<M>(n - 1);
    for (int i = 0; i < int(a.size()); ++i) { a[i] *= g[i]; }
    for (int i = 0; i < int(b.size()); ++i) { b[i] *= g[i]; }
    auto c = convolution(a, b);
    for (int i = 0; i < n; ++i) { c[i] *= f[i]; }
    return c;}
// T: O(sqrt(n) + 2^w * phi(n)), M: O(phi(n)); n >= 1, w = distinct primes; integer version exact when every coefficient fits lng, Mod reduces modulo M::mod().
inline vector<lng> cyclotomicPolynomial(int n) {
    auto s = polynomial_detail::cyclotomic<ulng>(n);
    return vector<lng>(s.begin(), s.end());}
template<typename M> vector<M> cyclotomicPolynomialMod(int n) { return polynomial_detail::cyclotomic<M>(n); }
// T: O(n * log(n)), M: O(n); a nonzero with unit lc; p_k = sum of k-th powers of the roots (with multiplicity) for k < n, p_0 = deg a.
template<typename M>
vector<M> powerSumsOfRoots(vector<M> a, int n) {
    polynomial_detail::trim(a);
    assert(n >= 0 && !a.empty());
    if (!n) { return {}; }
    reverse(a.begin(), a.end());
    // sum_{k >= 1} p_k x^k = -x R' / R for R = rev(a) = lc prod (1 - r x).
    auto q = fpsDiv(polyDerivative(a), a, n - 1);
    vector<M> res(n);
    res[0] = M(int(a.size()) - 1);
    for (int k = 1; k < n; ++k) { res[k] = -q[k - 1]; }
    return res;}
// T: O(n * log(n)), M: O(n); n = |p| - 1 < mod prime; monic degree-n polynomial with power sums p_1..p_n (p_0 ignored).
template<typename M>
vector<M> polyFromPowerSums(const vector<M> &p) {
    int n = int(p.size()) - 1;
    assert(n >= 0 && ulng(n) < M::mod());
    auto iv = polynomial_detail::inverses<M>(n + 1);
    vector<M> e(n + 1);
    for (int k = 1; k <= n; ++k) { e[k] = -p[k] * iv[k]; }
    e = fpsExp(e, n + 1);
    reverse(e.begin(), e.end());
    return e;}
// T: O(n * log(n)), M: O(n); n = |f| < mod prime; g with g(x) = sum_{i < x} f(i), deg g = deg f + 1, trimmed.
template<typename M>
vector<M> prefixSumOfPolynomial(const vector<M> &f) {
    int n = int(f.size());
    if (!n) { return {}; }
    assert(ulng(n) < M::mod());
    auto [fa, g] = polynomial_detail::factorials<M>(n);
    // D g = B(D) f with B(z) = z / (e^z - 1) = 1 / sum z^j / (j + 1)!.
    vector<M> e(n), t(n);
    for (int j = 0; j < n; ++j) { e[j] = g[j + 1]; t[j] = f[n - 1 - j] * fa[n - 1 - j]; }
    auto c = truncatedConvolution(t, fpsInverse(e, n), n);
    vector<M> res(n + 1);
    for (int i = 0; i < n; ++i) { res[i + 1] = c[n - 1 - i] * g[i + 1]; }
    return polynomial_detail::trim(res);}

