#pragma once
#include "../01-Core/01-template.hpp"

// T: O(1), M: O(1); g >= 0 and a * x + b * y = g, (0, 0) gives (0, 1, 0); lll holds g = 2^63.
struct ExtendedGcd { lll g, x, y; };
// T: O(log(2 + max(|a|, |b|))), M: O(1); all lng inputs, gcd(0, 0) = lcm(0, b) = 0.
inline ulng gcd64(lng a, lng b) {
    ulng x = a < 0 ? -ulng(a) : ulng(a), y = b < 0 ? -ulng(b) : ulng(b);
    return gcd(x, y);}
inline lll lcmWide(lng a, lng b) {
    if (!a || !b) { return 0; }
    lll x = a < 0 ? -lll(a) : a, y = b < 0 ? -lll(b) : b;
    return x / gcd64(a, b) * y;}
inline ExtendedGcd extendedGcd(lng a, lng b) {
    lll u = a, v = b, x = 1, y = 0, xx = 0, yy = 1;
    while (v) {
        lll q = u / v;
        x = std::exchange(xx, x - q * xx); y = std::exchange(yy, y - q * yy);
        u = std::exchange(v, u % v);}
    if (u < 0) { u = -u; x = -x; y = -y; }
    return {u, x, y};}
inline lng exGcd(lng a, lng b, lng &x, lng &y) {
    assert(&x != &y); auto r = extendedGcd(a, b);
    assert(r.g <= std::numeric_limits<lng>::max());
    x = lng(r.x); y = lng(r.y); return lng(r.g);}

// T: O(1), M: O(1); b != 0 and quotient fits lll, m > 0; checked false leaves out unchanged.
inline lll floorDiv(lll a, lll b) {
    assert(b && !(a == std::numeric_limits<lll>::min() && b == -1));
    lll q = a / b, r = a % b; return q - (r && ((r < 0) != (b < 0)));}
inline lll ceilDiv(lll a, lll b) {
    assert(b && !(a == std::numeric_limits<lll>::min() && b == -1));
    lll q = a / b, r = a % b; return q + (r && ((r < 0) == (b < 0)));}
inline lng modNorm(lll a, lng m) {
    assert(m > 0); lng r = lng(a % m); return r < 0 ? r + m : r;}
inline lng modMul(lng a, lng b, lng m) { return lng(lll(modNorm(a, m)) * modNorm(b, m) % m); }
inline ulng modMul64(ulng a, ulng b, ulng m) { assert(m); return ulng(ulll(a) * b % m); }
inline bool checkedAdd(lng a, lng b, lng &out) {
    lng t; if (__builtin_add_overflow(a, b, &t)) { return false; }
    out = t; return true;}
inline bool checkedMul(lng a, lng b, lng &out) {
    lng t; if (__builtin_mul_overflow(a, b, &t)) { return false; }
    out = t; return true;}
inline lng saturatingAdd(lng a, lng b) {
    lng t; if (!__builtin_add_overflow(a, b, &t)) { return t; }
    return b < 0 ? std::numeric_limits<lng>::min() : std::numeric_limits<lng>::max();}
inline lng saturatingMul(lng a, lng b) {
    lng t; if (!__builtin_mul_overflow(a, b, &t)) { return t; }
    return (a < 0) != (b < 0) ? std::numeric_limits<lng>::min() : std::numeric_limits<lng>::max();}

// T: O(log(w)), M: O(1); a odd, returns x with a * x = 1 mod 2^64.
inline ulng invMod2p64(ulng a) {
    assert(a & 1); ulng x = (3 * a) ^ 2;
    for (int i = 0; i < 4; ++i) { x *= 2 - a * x; }
    return x;}

// T: O(log(2 + |e|)) plus O(log(m)) for modPow with e < 0, M: O(1); 0^0 = 1 % m, intPow false on overflow, modPow -1 for a nonunit.
inline bool intPow(lng a, ulng e, lng &out) {
    lng r = 1;
    while (e) {
        if ((e & 1) && __builtin_mul_overflow(r, a, &r)) { return false; }
        e >>= 1;
        if (e && __builtin_mul_overflow(a, a, &a)) { return false; }}
    out = r; return true;}
inline lng modPow(lng a, lng b, lng mod = INF64) {
    a = modNorm(a, mod); ulng e = b < 0 ? -ulng(b) : ulng(b);
    if (b < 0) {
        auto r = extendedGcd(a, mod);
        if (r.g != 1) { return -1; }
        a = modNorm(r.x, mod);}
    lng r = 1 % mod;
    while (e) {
        if (e & 1) { r = lng(lll(r) * a % mod); }
        e >>= 1; if (e) { a = lng(lll(a) * a % mod); }}
    return r;}
inline ulng modPow64(ulng a, ulng e, ulng m) {
    assert(m); a %= m; ulng r = 1 % m;
    while (e) {
        if (e & 1) { r = modMul64(r, a, m); }
        e >>= 1; if (e) { a = modMul64(a, a, m); }}
    return r;}
