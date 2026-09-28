#pragma once
#include "../1-Core/01-template.hpp"
#include "../5-Mathematics/02-exgcd.hpp"

// T: O(log(n)), M: O(1)  
lng modPow(lng a, lng b, lng mod = INF64)  {
    a = (a % mod + mod) % mod;
    if (b < 0) {
        lng x, y;
        if (exGcd(a, mod, x, y) != 1) { return -1; }
        a = (x % mod + mod) % mod; b = -b;}
    lng res = 1;
    while (b > 0) {
        if (b & 1) { res = lll(res) * a % mod; }
        a = lll(a) * a % mod; b >>= 1;}
    return res;}