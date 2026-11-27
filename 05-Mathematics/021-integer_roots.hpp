#pragma once
#include "../01-Core/01-template.hpp"

namespace integer_root_detail {
    // T: O(k), M: O(1); r^k when it is <= n, else n + 1; r^k <= n < 2^64 keeps every product below 2^128.
    inline ulll powCap(ulng r, int k, ulng n) {
        ulll res = 1;
        for (int i = 0; i < k; ++i) {
            res *= r;
            if (res > n) { return ulll(n) + 1; }}
        return res;}
} // namespace integer_root_detail

// T: O(1), M: O(1); floor(sqrt(n)) for every ulng (isqrt) and ulll (isqrt128); isqrtRemainder gives {s, n - s^2}.
inline ulng isqrt(ulng n) {
    ulng r = min(ulng(std::sqrt(double(n))), ulng(0xFFFFFFFF));
    if (r * r > n) { return r - 1; }
    return r + (r < 0xFFFFFFFF && (r + 1) * (r + 1) <= n);}
inline ulng isqrt128(ulll n) {
    if (!(n >> 64)) { return isqrt(ulng(n)); }
    double e = std::sqrt(double(n));
    ulll r = e >= 0x1p64 ? ~ulng(0) : ulng(e);
    r = min((r + n / r) / 2, ulll(~ulng(0)));
    while (r * r > n) { --r; }
    return ulng(r);}
inline pair<ulng, ulng> isqrtRemainder(ulng n) {
    ulng s = isqrt(n);
    return {s, n - s * s};}

// T: O(min(k, 64)) (kthRootRound: O(min(k, 128))), M: O(1); floor, ceil and round of n^(1/k) for every ulng and k >= 1; no ties.
inline ulng ikthRoot(ulng n, int k) {
    assert(k >= 1);
    if (k <= 1) { return n; }
    if (k == 2) { return isqrt(n); }
    if (k >= 64) { return n > 0; }
    ulng r = ulng(std::pow(double(n), 1.0 / k));
    while (r && integer_root_detail::powCap(r, k, n) > n) { --r; }
    while (integer_root_detail::powCap(r + 1, k, n) <= n) { ++r; }
    return r;}
inline ulng ceilKthRoot(ulng n, int k) {
    ulng r = ikthRoot(n, k);
    return r + (integer_root_detail::powCap(r, min(k, 64), n) != n);}
inline ulng kthRootRound(ulng n, int k) {
    ulng r = ikthRoot(n, k), q = 2 * r + 1;
    if (r == 0 || k <= 1) { return r; }
    ulll f = 1, rem = 0;
    for (int j = 0; j < k && f < n; ++j) {
        ulll y = q * rem, s = q * f + (y >> j);
        f = s >> 1; rem = (s & 1) << j | (y & ((ulll(1) << j) - 1));}
    return r + (f < n);}

// T: O(1), M: O(1); floor of the real cube root for every lng, rounding toward minus infinity.
inline lng icbrt(lng n) {
    ulng m = n < 0 ? 0 - ulng(n) : ulng(n), c = ikthRoot(m, 3);
    return n >= 0 ? lng(c) : -lng(c) - (c * c * c != m);}

// T: O(1) (isPerfectPower: O(log(n)^2)), M: O(1); n = s^2 (0, 1 included); {b, e} with n = b^e, e maximal, n <= 1 gives {n, 1}.
inline bool isPerfectSquare(ulng n) {
    constexpr ulng RES = [] {
        ulng res = 0;
        for (int i = 0; i < 64; ++i) { res |= ulng(1) << (i * i % 64); }
        return res;}();
    if (!(RES >> (n & 63) & 1)) { return false; }
    ulng s = isqrt(n);
    return s * s == n;}
inline pair<ulng, int> isPerfectPower(ulng n) {
    int e = 1;
    for (int p : {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61}) {
        while (n > 1 && p < int(std::bit_width(n))) {
            ulng r = ikthRoot(n, p);
            if (integer_root_detail::powCap(r, p, n) != n) { break; }
            n = r; e *= p;}}
    return {n, e};}
