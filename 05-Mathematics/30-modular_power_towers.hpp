#pragma once
#include "../01-Core/01-template.hpp"
#include "01-mod_arithmetic.hpp"
#include "07-primality_factorization.hpp"
#include "08-multiplicative_functions.hpp"

namespace power_tower_detail {
    // T: O(log(cap)) capPow and capped (plus z), O(D * log(m) + z) towerMod, M: O(1); E_j = base(j)^E_(j+1), E_len = top, 0^0 = 1, CAP > every max(e_p).
    constexpr ulng CAP = 64;
    inline ulng capPow(ulng a, ulng e, ulng cap) {
        if (e == 0 || a == 1) { return 1; }
        if (a == 0) { return 0; }
        ulng r = 1;
        for (; e; --e) {
            if (r > (cap - 1) / a) { return cap; }
            r *= a;}
        return r;}
    inline auto listZeros(std::span<const ulng> a) {
        return [a](ulng j) {
            ulng z = 0;
            while (j + z < a.size() && a[j + z] == 0) { ++z; }
            return z;};}

    template<typename B, typename Z>
    ulng capped(ulng j, ulng len, ulng top, ulng need, B base, Z zeros) {
        array<pair<ulng, ulng>, 8> st;
        int sz = 0; ulng val;
        while (true) {
            if (j == len) { val = min(top, need); break; }
            ulng a = base(j);
            if (a == 0) {
                ulng z = zeros(j);
                val = ulng((j + z < len || top > 0) != bool(z & 1)); break;}
            if (a == 1 || need == 1) { val = 1; break; }
            ulng k = 0;
            for (ulng pw = 1; pw < need; ++k) { pw = pw > (need - 1) / a ? need : pw * a; }
            assert(sz < 8); st[sz++] = {a, need}; need = k; ++j;}
        while (sz) { auto [a, nd] = st[--sz]; val = capPow(a, val, nd); }
        return val;}

    template<typename B, typename Z>
    ulng towerMod(ulng len, ulng top, const vector<ulng> &chain, B base, Z zeros) {
        assert(!chain.empty() && chain.back() == 1);
        for (int i = 1; i < int(chain.size()); ++i) { assert(chain[i] < chain[i - 1]); }
        if (chain.empty()) { return 0; }
        ulng d = min<ulng>(len, chain.size() - 1), r, s;
        if (d == len) { r = top % chain[d]; s = min(top, CAP); }
        else { r = 0; s = capped(d, len, top, CAP, base, zeros); }
        for (ulng j = d; j-- > 0;) {
            ulng c = chain[j], c1 = chain[j + 1], a = base(j), e = s < CAP ? s : r;
            r = modPow64(a, e, c);
            // A huge exponent becomes its residue plus a multiple of c1 (lambda(c) | c1) reaching CAP, split to avoid overflow.
            if (s >= CAP && e < CAP) { r = modMul64(r, modPow64(a, c1 * ((CAP - 1) / c1 + 1), c), c); }
            s = capPow(a, s, CAP);}
        return r;}
} // namespace power_tower_detail

// T: O(L * m^(1/4) * log(m)) expected, M: O(L); m >= 1, chain m, f(m), ..., 1 of L + 1 = O(log(m)) values, f = phi or lambda.
inline vector<ulng> eulerChain(ulng m) {
    assert(m >= 1); vector<ulng> res{m};
    while (res.back() > 1) { res.pb(phi(res.back())); }
    return res;}
inline vector<ulng> carmichaelChain(ulng m) {
    assert(m >= 1); vector<ulng> res{m};
    while (res.back() > 1) { res.pb(carmichaelLambda(res.back())); }
    return res;}

// T: O(m^(1/4) * log(m)) expected, M: O(log(m)); m >= 1, least t with a^(t + e) = a^(t + e + lambda(m)) mod m for all a, e >= 0 (0 for m = 1).
inline int towerThreshold(ulng m) {
    assert(m >= 1); int res = 0;
    for (auto [p, e] : factorize(m)) { res = max(res, e); }
    return res;}

// T: O(min(n, L) * log(m) + z) Mod, O(log(cap) + z) Capped, M: O(1); z = zero run met, 0^0 = 1, empty tower 1, chain of m from eulerChain or carmichaelChain.
inline ulng powerTowerMod(std::span<const ulng> a, const vector<ulng> &chain) {
    return power_tower_detail::towerMod(a.size(), 1, chain, [&](ulng j) { return a[j]; }, power_tower_detail::listZeros(a));}
inline ulng powerTowerCapped(std::span<const ulng> a, ulng cap) {
    if (cap == 0) { return 0; }
    return power_tower_detail::capped(0, a.size(), 1, cap, [&](ulng j) { return a[j]; }, power_tower_detail::listZeros(a));}

// T: O(L * log(m)), M: O(1); a^a^...^a^x0 with any 64-bit count k of a modulo chain[0]; tetrationMod(a, b) = a↑↑b, a↑↑0 = 1.
inline ulng iteratedPowerMod(ulng a, ulng k, ulng x0, const vector<ulng> &chain) {
    return power_tower_detail::towerMod(k, x0, chain, [&](ulng) { return a; }, [&](ulng j) { return a == 0 ? k - j : 0; });}
inline ulng tetrationMod(ulng a, ulng b, const vector<ulng> &chain) { return iteratedPowerMod(a, b, 1, chain); }
