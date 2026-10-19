#pragma once
#include "../01-Core/01-template.hpp"
#include "01-mod_arithmetic.hpp"

namespace floorsum_detail {
    // T: O(log(m)) (tri, choose3: O(1)), M: O(1) (moments, minMod, firstMod: O(log(m)) recursion); reduced a, b in [0, m) except moments; moments: sum C(f + 1, 2) last.
    inline ulll reduced(ulng n, ulng m, ulng a, ulng b) {
        ulll res = 0;
        while (true) {
            if (a >= m) { res += ulll(n) * (n - 1) / 2 * (a / m); a %= m; }
            if (b >= m) { res += ulll(n) * (b / m); b %= m; }
            ulll y = ulll(a) * n + b;
            if (y < m) { return res; }
            if (y >> 64) { n = ulng(y / m); b = ulng(y % m); }
            else { n = ulng(y) / m; b = ulng(y) % m; }
            swap(m, a);}}

    // {(a n + b) / m, (a n + b) % m}, a, b < m < 2^127, n < 2^63: Knuth D for one quotient word below 2^63; hi > 0 forces m > 2^65, a shift in [1, 62].
    inline pair<ulng, ulll> divStep(ulll a, ulng n, ulll b, ulll m) {
        ulll low = ulll(ulng(a)) * n, high = (a >> 64) * n, lo = low + (high << 64), hi = (high >> 64) + (lo < low);
        lo += b; hi += lo < b;
        if (!hi) { return {ulng(lo / m), lo % m}; }
        int s = std::countl_zero(ulng(m >> 64));
        ulll d = m << s, rest = lo << s, top = (hi << s | (lo >> 1) >> (127 - s)) << 64 | rest >> 64;
        ulng d1 = ulng(d >> 64), d0 = ulng(d), u0 = ulng(rest), q = ulng(top / d1);
        ulll r = top - ulll(q) * d1;
        while (!(r >> 64) && ulll(q) * d0 > (r << 64 | u0)) { --q; r += d1; }
        return {q, ((r << 64 | u0) - ulll(q) * d0) >> s};}
    inline ulll reducedWide(ulng n, ulll m, ulll a, ulll b) {
        ulll res = 0;
        while (n) {
            if (a >= m) { res += ulll(n) * (n - 1) / 2 * (a / m); a %= m; }
            if (b >= m) { res += ulll(n) * (b / m); b %= m; }
            auto [q, r] = divStep(a, n, b, m);
            if (!q) { break; }
            n = q; b = r; swap(m, a);}
        return res;}
    template<typename T> T tri(lng x) { return x % 2 == 0 ? T(x / 2) * (T(x) + T(1)) : T(x) * T((x >> 1) + 1); }
    template<typename T> T choose3(lng n) {
        lng f[3] = {n, n - 1, n - 2}, d = 6;
        for (lng &x : f) { lng g = gcd(x, d); x /= g; d /= g; }
        return T(f[0]) * T(f[1]) * T(f[2]);}
    template<typename T> array<T, 3> moments(lng n, lng m, lng a, lng b) {
        lng qa = lng(floorDiv(a, m)), qb = lng(floorDiv(b, m));
        a = modNorm(a, m); b = modNorm(b, m);
        T f = 0, g = 0, h = 0;
        lng top = n == 0 ? 0 : lng((lll(a) * (n - 1) + b) / m);
        if (top > 0) {
            auto [fs, gs, hs] = moments<T>(top, a, m, m - 1 - b);
            f = T(top) * T(n - 1) - fs; g = T(top) * tri<T>(n - 1) - hs; h = T(n - 1) * tri<T>(top) - gs - fs;}
        T A = T(qa), B = T(qb), s1 = tri<T>(n - 1), c3 = choose3<T>(n);
        h += A * g + B * f + A * A * c3 + (tri<T>(qa) - A + A * (B + T(1))) * s1 + T(n) * tri<T>(qb);
        g += A * (c3 + c3 + s1) + B * s1;
        f += A * s1 + B * T(n);
        return {f, g, h};}

    inline pair<lng, lng> minMod(lng n, lng m, lng a, lng b) {
        if (a == 0 || n == 1) { return {b, 0}; }
        if (a <= m - a) {
            lng laps = lng((lll(a) * (n - 1) + b) / m);
            if (laps == 0) { return {b, 0}; }
            auto [v, j] = minMod(laps, a, modNorm(-lll(m), a), modNorm(lll(b) - m, a));
            if (b <= v) { return {b, 0}; }
            return {v, lng(ceilDiv(lll(j + 1) * m - b, a))};}
        lng d = m - a, ends = lng((lll(d) * n + m - 1 - b) / m);
        if (ends == 0) { return {lng((lll(a) * (n - 1) + b) % m), n - 1}; }
        auto [v, j] = minMod(ends, d, m % d, b % d);
        return {v, lng((b + lll(j) * m - v) / d)};}
    inline lng firstMod(lng m, lng a, lng b, lng l, lng r) {
        if (l <= b && b < r) { return 0; }
        if (a == 0) { return -1; }
        if (a > m - a) { return firstMod(m, m - a, m - 1 - b, m - r, m - l); }
        if (b < l) {
            lng x = lng(ceilDiv(l - b, a));
            if (lll(a) * x + b < r) { return x; }}
        lng j = firstMod(a, modNorm(-lll(m), a), modNorm(lll(b) - l - m, a), 0, min(r - l, a));
        return j < 0 ? -1 : lng(ceilDiv(lll(j + 1) * m + l - b, a));}
} // namespace floorsum_detail

// T: O(log(m)), M: O(1); n >= 0, m >= 1, |a|, |b| <= 2^126, 0 <= l <= r <= m; over i in [0, n): the sum of floor((a i + b) / m) (ceilSum: ceil) modulo 2^128,
// exact when it fits lll; modSum: the sum of (a i + b) mod m; countModInRange: the number of i with l <= (a i + b) mod m < r.
inline lll floorSum(lng n, lng m, lll a, lll b) {
    assert(n >= 0 && m >= 1);
    lll qa = ulll(a) >> 64 ? floorDiv(a, m) : 0, qb = ulll(b) >> 64 ? floorDiv(b, m) : 0;
    ulll res = ulll(qa) * (ulll(n) * ulng(n - 1) / 2) + ulll(qb) * ulng(n);
    return lll(res + floorsum_detail::reduced(ulng(n), ulng(m), ulng(a - qa * m), ulng(b - qb * m)));}
inline lll ceilSum(lng n, lng m, lll a, lll b) { return floorSum(n, m, a, b + m - 1); }
// T: O(log(m)), M: O(1); n >= 0, 1 <= m < 2^127, any lll a, b: floorSum with a 128-bit modulus, modulo 2^128, exact when it fits lll.
inline lll floorSumWide(lng n, lll m, lll a, lll b) {
    assert(n >= 0 && m >= 1);
    lll qa = floorDiv(a, m), qb = floorDiv(b, m);
    ulll res = ulll(qa) * (ulll(n) * ulng(max(n, lng(1)) - 1) / 2) + ulll(qb) * ulng(n);
    return lll(res + floorsum_detail::reducedWide(ulng(max(n, lng(0))), ulll(m), ulll(a) - ulll(qa) * ulll(m), ulll(b) - ulll(qb) * ulll(m)));}
inline lll modSum(lng n, lng m, lll a, lll b) {
    assert(n >= 0 && m >= 1);
    lng ra = modNorm(a, m), rb = modNorm(b, m);
    return lll(ulll(ra) * (ulll(n) * ulng(n - 1) / 2) + ulll(rb) * ulng(n) - ulll(m) * ulll(floorSum(n, m, ra, rb)));}
inline lng countModInRange(lng n, lng m, lll a, lll b, lng l, lng r) {
    assert(n >= 0 && m >= 1 && 0 <= l && l <= r && r <= m);
    lng ra = modNorm(a, m), rb = modNorm(b, m);
    return lng(ulll(floorSum(n, m, ra, rb - l)) - ulll(floorSum(n, m, ra, rb - r)));}

// T: O(log(m)) ring operations, M: O(log(m)); n >= 0, m >= 1, any lng a, b, T a ring from lng (modint, ulll modulo 2^128); {sum f, sum i f, sum f^2} over i in [0, n), f = floor((a i + b) / m).
template<typename T> array<T, 3> floorSumMoments(lng n, lng m, lng a, lng b) {
    assert(n >= 0 && m >= 1);
    auto [f, g, h] = floorsum_detail::moments<T>(n, m, a, b);
    return {f, g, h + h - f};}

// T: O(log(m)), M: O(log(m)); n >= 1, m >= 1, |a|, |b| <= 2^126, 0 <= l <= r <= m; {min (max) of (a x + b) mod m over x in [0, n), least x attaining it};
// firstModInRange: the least x >= 0 with l <= (a x + b) mod m < r, or -1.
inline pair<lng, lng> minOfModOfLinear(lng n, lng m, lll a, lll b) {
    assert(n >= 1 && m >= 1);
    return floorsum_detail::minMod(n, m, modNorm(a, m), modNorm(b, m));}
inline pair<lng, lng> maxOfModOfLinear(lng n, lng m, lll a, lll b) {
    assert(n >= 1 && m >= 1);
    auto [v, x] = floorsum_detail::minMod(n, m, modNorm(-a, m), m - 1 - modNorm(b, m));
    return {m - 1 - v, x};}
inline lng firstModInRange(lng m, lll a, lll b, lng l, lng r) {
    assert(m >= 1 && 0 <= l && l <= r && r <= m);
    return l == r ? -1 : floorsum_detail::firstMod(m, modNorm(a, m), modNorm(b, m), l, r);}

// T: O(sqrt(n)), O(k * B) for k values and B <= 2 * sum of sqrt(v) blocks, M: O(1) (O(out) for enumerateQuotients); 0 <= n, v <= 2^62; f(q, l, r) per maximal
// [l, r) in [1, n] with floor(n / x) = q, x ascending; enumerateQuotients: the distinct floor(n / x) ascending; joint: f(l, r) per maximal [l, r) in [1, max v] keeping each floor(v / x).
template<typename F> void forEachQuotientBlock(lng n, F f) {
    assert(n >= 0);
    for (lng l = 1, r; l <= n; l = r) {
        lng q = n / l; r = n / q + 1;
        f(q, l, r);}}
inline vector<lng> enumerateQuotients(lng n) {
    vector<lng> res;
    forEachQuotientBlock(n, [&](lng q, lng, lng) { res.pb(q); });
    reverse(res.begin(), res.end()); return res;}
template<typename F> void forEachJointQuotientBlock(const vector<lng> &vs, F f) {
    lng hi = 0;
    for (lng v : vs) { assert(v >= 0); hi = max(hi, v); }
    for (lng l = 1, r; l <= hi; l = r) {
        r = hi + 1;
        for (lng v : vs) { if (v >= l) { r = min(r, v / (v / l) + 1); }}
        f(l, r);}}
