#pragma once
#include "../01-Core/01-template.hpp"
#include "01-mod_arithmetic.hpp"
#include "03-equation_solvers.hpp"
#include "07-primality_factorization.hpp"

// T: O(1), M: O(1); ok: x = value mod modulus (value in [0, modulus)); !ok && overflow: lcm above LLONG_MAX; !ok && !overflow: inconsistent.
struct CrtResult {
    lng value = 0, modulus = 0;
    bool ok = false, overflow = false;
};

// T: O(log(min(m1, m2))), M: O(1); m1, m2 >= 1, all lng residues, any moduli.
inline CrtResult crt2(lng a1, lng m1, lng a2, lng m2) {
    assert(m1 >= 1 && m2 >= 1);
    a1 = modNorm(a1, m1); a2 = modNorm(a2, m2);
    auto [g, x, y] = extendedGcd(m1, m2);
    lng d = a2 - a1;
    if (d % g) { return {}; }
    lng m = lng(m2 / g), l;
    if (__builtin_mul_overflow(m1, m, &l)) { return {0, 0, false, true}; }
    lng t = modNorm(lll(d / g) * x, m);
    return {a1 + m1 * t, l, true, false};}

// T: O(k * log(max(m))), M: O(k); k congruences x = a_i or a_i * x = b_i mod m_i >= 1, empty gives {0, 1}, legacy {-1, -1} on failure.
inline CrtResult crt(const vector<pair<lng, lng>> &cong) {
    CrtResult res{0, 1, true, false};
    for (auto [a, m] : cong) {
        res = crt2(res.value, res.modulus, a, m);
        if (!res.ok) { break; }}
    return res;}
inline CrtResult crtScaled(const vector<tuple<lng, lng, lng>> &eqs) {
    vector<pair<lng, lng>> cong;
    for (auto [a, b, m] : eqs) {
        cong.pb(solveModEq(a, b, m));
        if (cong.back().se < 0) { return {}; }}
    return crt(cong);}
inline pair<lng, lng> superChiRemThm(const vector<tuple<lng, lng, lng>> &eqs) {
    auto r = crtScaled(eqs);
    return r.ok ? pair<lng, lng>{r.value, r.modulus} : pair<lng, lng>{-1, -1};}

// T: O(k^2 + k * log(max(m))), M: O(k); pairwise coprime m_i >= 1, digits d_i in [0, m_i) or false; garner: least solution mod `mod` >= 1 or -1.
inline bool garnerDigits(const vector<pair<lng, lng>> &cong, vector<lng> &digits) {
    vector<lng> res;
    for (auto [a, m] : cong) {
        assert(m >= 1);
        lng val = 0, pre = 1 % m;
        for (int j = 0; j < int(res.size()); ++j) {
            val = lng((val + lll(res[j]) * pre) % m);
            pre = lng(lll(pre) * cong[j].se % m);}
        auto [g, x, y] = extendedGcd(pre, m);
        if (g != 1) { return false; }
        res.pb(modNorm(lll(modNorm(lll(a) - val, m)) * x, m));}
    digits = std::move(res);
    return true;}
inline lng garner(const vector<pair<lng, lng>> &cong, lng mod) {
    assert(mod >= 1);
    vector<lng> d;
    if (!garnerDigits(cong, d)) { return -1; }
    lng res = 0, pre = 1 % mod;
    for (int i = 0; i < int(d.size()); ++i) {
        res = lng((res + lll(d[i]) * pre) % mod);
        pre = lng(lll(pre) * modNorm(cong[i].se, mod) % mod);}
    return res;}

// T: O(k^2 * w^3) for coprimeBase plus O(k * c * log(max(m))), c = base size, M: O(k + c); any m_i >= 1, mod >= 1, least solution mod `mod`, -1 when inconsistent.
inline lng crtMod(const vector<pair<lng, lng>> &cong, lng mod) {
    assert(mod >= 1);
    vector<ulng> ms;
    for (auto [a, m] : cong) { assert(m >= 1); ms.pb(ulng(m)); }
    vector<pair<lng, lng>> sys;
    for (ulng q : coprimeBase(ms)) {
        int best = -1; lng top = 1;
        vector<lng> part(cong.size(), 1);
        for (int i = 0; i < int(cong.size()); ++i) {
            for (lng m = cong[i].se; m % lng(q) == 0; m /= lng(q)) { part[i] *= lng(q); }
            if (part[i] > top) { top = part[i]; best = i; }}
        lng r = modNorm(cong[best].fi, top);
        for (int i = 0; i < int(cong.size()); ++i) { if (modNorm(cong[i].fi, part[i]) != r % part[i]) { return -1; } }
        sys.pb({r, top});}
    return garner(sys, mod);}

// S: O(1), U: O(log(max(m))) per add, Q: O(1), M: O(1); crt2 conventions, a failed state stays failed.
struct CrtIncremental {
    CrtResult res{0, 1, true, false};

    bool add(lng a, lng m) {
        if (res.ok) { res = crt2(res.value, res.modulus, a, m); }
        return res.ok;}
    lng value() const { return res.value; }
    lng modulus() const { return res.modulus; }
    bool ok() const { return res.ok; }
};

// T: O(1), M: O(1); m >= 1, the residue of x in [-(m - 1) / 2, m / 2].
inline lng signedRepresentative(lll x, lng m) {
    lng r = modNorm(x, m);
    return r > m / 2 ? r - m : r;}
