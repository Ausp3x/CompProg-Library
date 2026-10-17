#pragma once
#include "../01-Core/01-template.hpp"
#include "01-mod_arithmetic.hpp"
#include "07-primality_factorization.hpp"
#include "11-discrete_log_root.hpp"

namespace modroot_detail {
    using Poly = vector<ulng>;
    // T: O(1) for mul, O(e) for pw (wraps at 2^64), O(log(m)) for inv and val, M: O(1); x * y mod m, p^e, inverse of a unit mod m < 2^63, v_p(x) for x != 0.
    inline ulng mul(ulng x, ulng y, ulng m) { return ulng(ulll(x) * y % m); }
    inline ulng pw(ulng p, int e) {
        ulng r = 1;
        for (int i = 0; i < e; ++i) { r *= p; }
        return r;}
    inline ulng inv(ulng x, ulng m) { return ulng(modNorm(extendedGcd(lng(x % m), lng(m)).x, lng(m))); }
    inline int val(ulng x, ulng p) {
        int v = 0;
        for (; x && x % p == 0; x /= p) { ++v; }
        return v;}
    inline lng checkedPow(lng p, int e) {
        lll r = 1;
        for (int i = 0; i < e && r <= std::numeric_limits<lng>::max(); ++i) { r *= p; }
        return p < 2 || e < 1 || r > std::numeric_limits<lng>::max() ? -1 : lng(r);}

    // T: O(|x| * |y| * log(m)), M: O(|x| * |y|); CRT products of residue classes (r, m) with coprime moduli.
    inline vector<pair<lng, lng>> merge(const vector<pair<lng, lng>> &x, const vector<pair<lng, lng>> &y) {
        vector<pair<lng, lng>> res;
        for (auto [r, m] : x) {
            for (auto [s, n] : y) {
                lng t = modNorm((lll(s) - r) % n * modNorm(extendedGcd(m, n).x, n), n);
                res.pb({lng(r + lll(m) * t), m * n});}}
        return res;}

    // T: O(log(p^e) * log(p) + z * log(p)) expected, z = Cipolla candidates, O(e) for p = 2, M: O(1); odd prime p with p^e < 2^63, or p = 2 with e <= 64; roots of x^2 = a as sorted residues mod p^j, with j.
    inline pair<int, vector<ulng>> sqrtPP(ulng a, ulng p, int e) {
        ulng pe = pw(p, e);
        a = pe ? a % pe : a;
        if (!a) { return {(e + 1) / 2, {0}}; }
        int v = p == 2 ? std::countr_zero(a) : val(a, p), t = v / 2, f = e - v;
        ulng u = a / pw(p, v), y = 1;
        if (v % 2) { return {0, {}}; }
        if (p == 2) {
            if (f <= 2) { return f == 1 || u % 4 == 1 ? pair<int, vector<ulng>>{t + 1, {1ULL << t}} : pair<int, vector<ulng>>{0, {}}; }
            if (u % 8 != 1) { return {0, {}}; }
            for (int i = 3; i < f; ++i) { if (((y * y - u) >> i) & 1) { y += 1ULL << (i - 1); } }
            ulng q = 1ULL << (e - t - 1), r = (y << t) & (q - 1);
            return {e - t - 1, {min(r, q - r), max(r, q - r)}};}
        lng r0 = sqrtModPrimeCipolla(lng(u % p), lng(p));
        if (r0 < 0) { return {0, {}}; }
        ulng pf = pw(p, f), z = inv(ulng(r0), p), h = (pf + 1) / 2;
        for (int prec = 1; prec < f; prec *= 2) { z = mul(mul(z, (3 + pf - mul(u, mul(z, z, pf), pf)) % pf, pf), h, pf); }
        ulng q = pw(p, e - t), r = mul(u, z, pf) * pw(p, t);
        return {e - t, {min(r, q - r), max(r, q - r)}};}

    // T: O(p^(1/4) * log(p) + e * log(k) * log(p^e)) expected, O(log(p^e) + log(k)) for countPP, M: O(p^(1/4)); prime p, p^e < 2^63; some x with x^k = a mod p^e (unit w, k >= 1 for unitRoot) or -1; countPP: their number, 0^0 = 1.
    inline lng unitRoot(ulng w, ulng k, ulng p, int e) {
        ulng pe = pw(p, e), r0 = 1, kk = k;
        if (p > 2) {
            lng r = kthRootModPrime(lng(w % p), lng(k), lng(p));
            if (r < 0) { return -1; }
            r0 = ulng(r);}
        if (e == 1) { return lng(r0 % pe); }
        int s = 0, j0 = p == 2 ? 2 : 1;
        for (; kk % p == 0; kk /= p) { ++s; }
        ulng u = mul(w, inv(modPow64(r0, k, pe), pe), pe), ord = pw(p, e - 1), ps = pw(p, s);
        if (s && (u - 1) % pw(p, min(s + j0, e))) { return -1; }
        ulng tgt = modPow64(u, inv(kk % ord, ord), pe), z = s ? 1 : tgt;
        // z^(p^s) = tgt is lifted one p-adic digit at a time inside 1 + p^j0 Z, where (1 + p^j c)^(p^s) = 1 + p^(j + s) c mod p^(j + s + 1).
        for (int i = s + j0; s && i < e; ++i) {
            ulng c = mul(tgt, inv(modPow64(z, ps, pe), pe), pe);
            z = mul(z, (1 + mul(pw(p, i - s), (c - 1) / pw(p, i) % p, pe)) % pe, pe);}
        return lng(mul(r0, z, pe));}
    inline lng kthPP(ulng a, ulng k, ulng p, int e) {
        ulng pe = pw(p, e);
        a %= pe;
        if (!k) { return a == 1 % pe ? lng(1 % pe) : -1; }
        if (!a) { return 0; }
        int v = val(a, p);
        if (ulng(v) % k) { return -1; }
        lng y = unitRoot(a / pw(p, v), k, p, e - v);
        return y < 0 ? -1 : lng(mul(ulng(y), pw(p, int(ulng(v) / k)), pe));}
    inline ulng countPP(ulng a, ulng k, ulng p, int e) {
        ulng pe = pw(p, e);
        a %= pe;
        if (!k) { return a == 1 % pe ? pe : 0; }
        if (!a) { return pw(p, e - int((ulng(e) + k - 1) / k)); }
        int v = val(a, p), f = e - v;
        if (ulng(v) % k) { return 0; }
        ulng w = a / pw(p, v), pf = pw(p, f), n = 1;
        if (p > 2) {
            ulng phi = pf / p * (p - 1), g = gcd(k, phi);
            n = modPow64(w, phi / g, pf) == 1 ? g : 0;}
        else if (f > 1 && k % 2 == 0) {
            int s = std::countr_zero(k);
            n = (w - 1) % pw(2, min(s + 2, f)) ? 0 : pw(2, 1 + min(s, f - 2));}
        return n * pw(p, v - int(ulng(v) / k));}

    // T: O(|f| * |g|) for divmod and mulMod, times log(e) for powMod, O(|f| * |g|) for gcdPoly, M: O(|f| + |g|); polynomials over F_p, low to high, trimmed, g nonzero.
    inline void trim(Poly &f) { while (!f.empty() && !f.back()) { f.pop_back(); } }
    inline pair<Poly, Poly> divmod(Poly f, const Poly &g, ulng p) {
        int n = int(f.size()), d = int(g.size()) - 1;
        if (n <= d) { return {{}, f}; }
        Poly q(n - d);
        ulng iv = inv(g[d], p);
        for (int i = n - 1; i >= d; --i) {
            ulng c = q[i - d] = mul(f[i], iv, p);
            for (int j = 0; j <= d; ++j) { f[i - d + j] = (f[i - d + j] + p - mul(c, g[j], p)) % p; }}
        f.resize(d);
        trim(f);
        return {q, f};}
    inline Poly mulMod(const Poly &x, const Poly &y, const Poly &g, ulng p) {
        if (x.empty() || y.empty()) { return {}; }
        Poly r(x.size() + y.size() - 1);
        for (int i = 0; i < int(x.size()); ++i) {
            for (int j = 0; j < int(y.size()); ++j) { r[i + j] = (r[i + j] + mul(x[i], y[j], p)) % p; }}
        return divmod(r, g, p).se;}
    inline Poly powMod(Poly x, ulng e, const Poly &g, ulng p) {
        Poly r = divmod({1}, g, p).se;
        for (x = divmod(x, g, p).se; e; e >>= 1) {
            if (e & 1) { r = mulMod(r, x, g, p); }
            if (e > 1) { x = mulMod(x, x, g, p); }}
        return r;}
    inline Poly gcdPoly(Poly x, Poly y, ulng p) {
        while (!y.empty()) { x = std::exchange(y, divmod(x, y, p).se); }
        return x;}

    // T: O(p * n) when p <= 64 * n, else O(n^2 * log(p) * log(n)) expected (Cantor-Zassenhaus), n = |f|, M: O(n); prime p; sorted distinct roots of f mod p.
    inline vector<ulng> rootsModP(Poly f, ulng p, std::mt19937_64 &rng) {
        trim(f);
        vector<ulng> res;
        if (f.size() <= 1) { return res; }
        if (p <= 64 * f.size()) {
            for (ulng x = 0; x < p; ++x) {
                ulng v = 0;
                for (int i = int(f.size()) - 1; i >= 0; --i) { v = (mul(v, x, p) + f[i]) % p; }
                if (!v) { res.pb(x); }}
            return res;}
        Poly h = powMod({0, 1}, p, f, p);
        h.resize(max(h.size(), size_t(2)));
        h[1] = (h[1] + p - 1) % p;
        trim(h);
        vector<Poly> todo{gcdPoly(f, h, p)};
        while (!todo.empty()) {
            Poly g = todo.back();
            todo.pop_back();
            if (g.size() == 2) { res.pb(mul(p - g[0], inv(g[1], p), p)); }
            while (g.size() > 2) {
                Poly w = powMod({rng() % p, 1}, (p - 1) / 2, g, p);
                w.resize(max(w.size(), size_t(1)));
                w[0] = (w[0] + p - 1) % p;
                trim(w);
                Poly d = gcdPoly(g, w, p);
                if (d.size() > 1 && d.size() < g.size()) { todo.pb(divmod(g, d, p).fi); todo.pb(d); break; }}}
        sort(res.begin(), res.end());
        return res;}

    // T: O(n * log(K)) for newton, O(n * K) liftRoots nodes of O(n^2) plus rootsModP, M: O(n * K); g mod p^e; newton: the root above a simple root r mod p; liftRoots: balls (r, j), every x = r mod p^j a root.
    inline ulng newton(const Poly &g, ulng r, ulng pe, int e) {
        ulng x = r;
        for (int prec = 1; prec < e; prec *= 2) {
            ulng v = 0, d = 0;
            for (int i = int(g.size()) - 1; i >= 0; --i) {
                d = (mul(d, x, pe) + v) % pe;
                v = (mul(v, x, pe) + g[i]) % pe;}
            x = (x + pe - mul(v, inv(d, pe), pe)) % pe;}
        return x;}
    inline vector<pair<ulng, int>> liftRoots(Poly g, ulng p, int e, std::mt19937_64 &rng) {
        int v = e;
        for (ulng c : g) { if (c) { v = min(v, val(c, p)); } }
        if (v >= e) { return {{0, 0}}; }
        ulng pv = pw(p, v), pe = pw(p, e -= v);
        Poly bar;
        for (ulng &c : g) { c /= pv; bar.pb(c % p); }
        vector<pair<ulng, int>> res;
        for (ulng r : rootsModP(bar, p, rng)) {
            ulng d = 0, q = 1;
            for (int i = int(bar.size()) - 1; i >= 1; --i) { d = (mul(d, r, p) + mul(ulng(i) % p, bar[i], p)) % p; }
            if (d) { res.pb({newton(g, r, pe, e), e}); continue; }
            // A multiple root: x = r + p t, recurse on g(r + p t), whose coefficients all vanish mod p.
            Poly h = g;
            int n = int(h.size());
            for (int i = 0; i < n; ++i) {
                for (int j = n - 2; j >= i; --j) { h[j] = (h[j] + mul(r, h[j + 1], pe)) % pe; }}
            for (int i = 0; i < n; ++i, q = mul(q, p, pe)) { h[i] = mul(h[i], q, pe); }
            for (auto [t, j] : liftRoots(h, p, e, rng)) { res.pb({r + p * t, j + 1}); }}
        return res;}

    // T: O(log(p^e) * log(p) + z * log(p)) expected, M: O(1); prime p, p^e < 2^63; roots of a x^2 + b x + c = 0 mod p^e as sorted residues mod p^j, with j.
    inline pair<int, vector<ulng>> quadPP(ulng a, ulng b, ulng c, ulng p, int e) {
        int v = e;
        for (ulng x : {a, b, c}) { if (x) { v = min(v, val(x, p)); } }
        if (v >= e) { return {0, {0}}; }
        ulng pv = pw(p, v), pe = pw(p, e -= v);
        a /= pv; b /= pv; c /= pv;
        if (a % p == 0) {
            if (b % p == 0) { return {0, {}}; }
            return {e, {newton({c, b, a}, mul(pe - c % pe, inv(b, p), p), pe, e)}};}
        vector<ulng> res;
        if (p == 2) {
            // (2 a x + b)^2 = b^2 - 4 a c mod 2^(e + 2), and y = 2 a x + b has the parity of b.
            auto [j, ys] = sqrtPP(b * b - 4 * a * c, 2, e + 2);
            for (ulng y : ys) { res.pb((((y - b) & ((1ULL << j) - 1)) >> 1) * invMod2p64(a) & ((1ULL << (j - 1)) - 1)); }
            sort(res.begin(), res.end());
            return {j - 1, res};}
        auto [j, ys] = sqrtPP((mul(b, b, pe) + pe - mul(mul(4, a, pe), c, pe)) % pe, p, e);
        ulng q = pw(p, j), ia = inv(2 * a % pe, pe);
        for (ulng y : ys) { res.pb(mul((y + q - b % q) % q, ia, q)); }
        sort(res.begin(), res.end());
        return {j, res};}
} // namespace modroot_detail

// T: O(log(p) * log(p^e) + z * log(p)) expected, z = Cipolla candidates (O(k) for sqrtMod2k), M: O(1); prime p, e >= 1, p^e < 2^63, 0 <= k <= 64 (a taken mod 2^64);
// the least root of x^2 = a, or -1.
inline lng sqrtModPrimePower(lng a, lng p, int e) {
    lng pe = modroot_detail::checkedPow(p, e);
    assert(pe > 0);
    if (pe <= 0) { return -1; }
    auto r = modroot_detail::sqrtPP(ulng(modNorm(a, pe)), ulng(p), e).se;
    return r.empty() ? -1 : lng(r[0]);}
inline lng sqrtMod2k(lng a, int k) {
    assert(0 <= k && k <= 64);
    auto r = modroot_detail::sqrtPP(ulng(a), 2, min(max(k, 0), 64)).se;
    return r.empty() ? -1 : lng(r[0]);}

// T: O(m^(1/4) * log(m) + w * log(m)^2) expected for one root, plus O(out * w * log(m)) for all, w = distinct primes of m, M: O(out); m >= 1;
// sqrtModComposite: some root or -1; sqrtModAll: sorted classes (r, M), M | m, whose union is every root.
inline lng sqrtModComposite(lng a, lng m) {
    assert(m >= 1);
    vector<pair<lng, lng>> res{{0, 1}};
    for (auto [p, e] : factorize(ulng(max(m, lng(1))))) {
        lng pe = lng(modroot_detail::pw(p, e));
        auto r = modroot_detail::sqrtPP(ulng(modNorm(a, pe)), p, e).se;
        if (r.empty()) { return -1; }
        res = modroot_detail::merge(res, {{lng(r[0]), pe}});}
    return res[0].fi;}
inline vector<pair<lng, lng>> sqrtModAll(lng a, lng m) {
    assert(m >= 1);
    vector<pair<lng, lng>> res{{0, 1}};
    for (auto [p, e] : factorize(ulng(max(m, lng(1))))) {
        auto [j, r] = modroot_detail::sqrtPP(ulng(modNorm(a, lng(modroot_detail::pw(p, e)))), p, e);
        vector<pair<lng, lng>> cls;
        for (ulng x : r) { cls.pb({lng(x), lng(modroot_detail::pw(p, j))}); }
        res = modroot_detail::merge(res, cls);}
    sort(res.begin(), res.end());
    return res;}

// T: O(m^(1/4) * log(m) + w * (p^(1/4) * log(p) + log(k) * log(m)^2)) expected, M: O(m^(1/4)), rootCount O(m^(1/4) * log(m) + log(k) * log(m)); k >= 0, 0^0 = 1;
// some x with x^k = a (prime power p^e < 2^63 or any m >= 1), or -1; rootCount: the number of such x in [0, m).
inline lng kthRootModPrimePower(lng a, lng k, lng p, int e) {
    lng pe = modroot_detail::checkedPow(p, e);
    assert(k >= 0 && pe > 0);
    return pe > 0 ? modroot_detail::kthPP(ulng(modNorm(a, pe)), ulng(max(k, lng(0))), ulng(p), e) : -1;}
inline lng kthRootModComposite(lng a, lng k, lng m) {
    assert(k >= 0 && m >= 1);
    vector<pair<lng, lng>> res{{0, 1}};
    for (auto [p, e] : factorize(ulng(max(m, lng(1))))) {
        lng pe = lng(modroot_detail::pw(p, e)), r = modroot_detail::kthPP(ulng(modNorm(a, pe)), ulng(max(k, lng(0))), p, e);
        if (r < 0) { return -1; }
        res = modroot_detail::merge(res, {{r, pe}});}
    return res[0].fi;}
inline lng rootCount(lng a, lng k, lng m) {
    assert(k >= 0 && m >= 1);
    ulng res = 1;
    for (auto [p, e] : factorize(ulng(max(m, lng(1))))) { res *= modroot_detail::countPP(ulng(modNorm(a, lng(modroot_detail::pw(p, e)))), ulng(max(k, lng(0))), p, e); }
    return lng(res);}

// T: O(m^(1/4) * log(m) + w * n^3 * e * log(p) + out * w * log(m)) expected (O(p * n) root scans replace log(p) when p <= 64 * n), n = |f|, M: O(n * e + out); m >= 1, f low to high,
// seeded Las Vegas; sorted disjoint classes (r, M), M | m, whose union is every x with f(x) = 0 mod m.
inline vector<pair<lng, lng>> polynomialCongruenceRoots(const vector<lng> &f, lng m, ulng seed = 0) {
    assert(m >= 1);
    std::mt19937_64 rng(seed);
    vector<pair<lng, lng>> res{{0, 1}};
    for (auto [p, e] : factorize(ulng(max(m, lng(1))))) {
        lng pe = lng(modroot_detail::pw(p, e));
        modroot_detail::Poly g;
        for (lng c : f) { g.pb(ulng(modNorm(c, pe))); }
        vector<pair<lng, lng>> cls;
        for (auto [r, j] : modroot_detail::liftRoots(g, p, e, rng)) { cls.pb({lng(r), lng(modroot_detail::pw(p, j))}); }
        res = modroot_detail::merge(res, cls);}
    sort(res.begin(), res.end());
    return res;}

// T: O(m^(1/4) * log(m) + w * log(m)^2 + out * w * log(m)) expected, M: O(out); m >= 1, any a, b, c;
// sorted classes (r, M) with a common M | m whose union is every x with a x^2 + b x + c = 0 mod m.
inline vector<pair<lng, lng>> quadraticCongruenceMod(lng a, lng b, lng c, lng m) {
    assert(m >= 1);
    vector<pair<lng, lng>> res{{0, 1}};
    for (auto [p, e] : factorize(ulng(max(m, lng(1))))) {
        lng pe = lng(modroot_detail::pw(p, e));
        auto [j, r] = modroot_detail::quadPP(ulng(modNorm(a, pe)), ulng(modNorm(b, pe)), ulng(modNorm(c, pe)), p, e);
        vector<pair<lng, lng>> cls;
        for (ulng x : r) { cls.pb({lng(x), lng(modroot_detail::pw(p, j))}); }
        res = modroot_detail::merge(res, cls);}
    sort(res.begin(), res.end());
    return res;}
