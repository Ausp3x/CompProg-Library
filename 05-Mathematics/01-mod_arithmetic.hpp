#pragma once
#include "../01-Core/01-template.hpp"

// All signed 64-bit inputs, including MIN. gcd(0,0)=lcm(0,b)=0.
// T: O(log(2 + max(|a|, |b|))), M: O(1)
inline ulng gcd64(lng a, lng b) {
    ulng x = a < 0 ? -ulng(a) : ulng(a), y = b < 0 ? -ulng(b) : ulng(b);
    return gcd(x, y); }
// T: O(log(2 + max(|a|, |b|))), M: O(1)
inline lll lcmWide(lng a, lng b) {
    if (!a || !b) { return 0; }
    lll x = a < 0 ? -lll(a) : a, y = b < 0 ? -lll(b) : b;
    return x / gcd64(a, b) * y; }

// g>=0, a*x+b*y=g; (0,0) returns (0,1,0). Wide fields hold g=2^63.
struct ExtendedGcd { lll g, x, y; };
// T: O(log(2 + max(|a|, |b|))), M: O(1)
inline ExtendedGcd extendedGcd(lng a, lng b) {
    lll u = a, v = b, x = 1, y = 0, xx = 0, yy = 1;
    while (v) {
        lll q = u / v;
        x = std::exchange(xx, x - q * xx); y = std::exchange(yy, y - q * yy);
        u = std::exchange(v, u % v); }
    if (u < 0) { u = -u; x = -x; y = -y; }
    return {u, x, y}; }
// Legacy adapter: requires g<=LLONG_MAX and distinct output references.
// T: O(log(2 + max(|a|, |b|))), M: O(1)
inline lng exGcd(lng a, lng b, lng &x, lng &y) {
    assert(&x != &y); auto r = extendedGcd(a, b);
    assert(r.g <= std::numeric_limits<lng>::max());
    x = lng(r.x); y = lng(r.y); return lng(r.g); }

// Nonzero b; quotient must fit signed 128-bit (excludes MIN/-1).
// T: O(1), M: O(1)
inline lll floorDiv(lll a, lll b) {
    assert(b && !(a == std::numeric_limits<lll>::min() && b == -1));
    lll q = a / b, r = a % b; return q - (r && ((r < 0) != (b < 0))); }
// T: O(1), M: O(1)
inline lll ceilDiv(lll a, lll b) {
    assert(b && !(a == std::numeric_limits<lll>::min() && b == -1));
    lll q = a / b, r = a % b; return q + (r && ((r < 0) == (b < 0))); }
// Canonical representative in [0,m); m>0. Wide signed input supported.
// T: O(1), M: O(1)
inline lng modNorm(lll a, lng m) {
    assert(m > 0); lng r = lng(a % m); return r < 0 ? r + m : r; }
// Arbitrary signed operands; m>0. Product evaluated after normalization.
// T: O(1), M: O(1)
inline lng modMul(lng a, lng b, lng m) {
    return lng(lll(modNorm(a, m)) * modNorm(b, m) % m); }
// Full unsigned 64-bit operands/modulus; m>0. No reduction backend setup.
// T: O(1), M: O(1)
inline ulng modMul64(ulng a, ulng b, ulng m) {
    assert(m); return ulng(ulll(a) * b % m); }

// Integer power, 0^0=1. False means signed 64-bit overflow; out unchanged.
// T: O(log(2 + e)), M: O(1)
inline bool intPow(lng a, ulng e, lng &out) {
    lng r = 1;
    while (e) {
        if ((e & 1) && __builtin_mul_overflow(r, a, &r)) { return false; }
        e >>= 1;
        if (e && __builtin_mul_overflow(a, a, &a)) { return false; }}
    out = r; return true; }
// m>0; negative b uses the unit inverse, or returns -1 for a nonunit.
// Modulus 1 is the zero ring, including negative powers: result 0.
// T: O(log(2 + |b|) + log(m)) for b<0, O(log(2 + b)) otherwise; M: O(1)
inline lng modPow(lng a, lng b, lng mod = INF64) {
    a = modNorm(a, mod); ulng e = b < 0 ? -ulng(b) : ulng(b);
    if (b < 0) {
        auto r = extendedGcd(a, mod);
        if (r.g != 1) { return -1; }
        a = modNorm(r.x, mod); }
    lng r = 1 % mod;
    while (e) {
        if (e & 1) { r = lng(lll(r) * a % mod); }
        e >>= 1; if (e) { a = lng(lll(a) * a % mod); }}
    return r; }
// Nonnegative full-width exponent and unsigned modulus; 0^0=1%m.
// T: O(log(2 + e)), M: O(1)
inline ulng modPow64(ulng a, ulng e, ulng m) {
    assert(m); a %= m; ulng r = 1 % m;
    while (e) {
        if (e & 1) { r = modMul64(r, a, m); }
        e >>= 1; if (e) { a = modMul64(a, a, m); }}
    return r; }
