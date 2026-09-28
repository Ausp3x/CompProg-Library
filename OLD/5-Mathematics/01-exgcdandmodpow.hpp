#pragma once
#include "../1-Core/01-template.hpp"

// T: O(log(n)), M: O(1)
lng exGcd(lng a, lng b, lng &x, lng &y) {
    x = 1; y = 0; lng X = 0, Y = 1;
    while (b != 0) {
        lng q = a / b;
        x = std::exchange(X, x - q * X);
        y = std::exchange(Y, y - q * Y);
        a = std::exchange(b, a % b);}
    if (a < 0) { a = -a; x = -x; y = -y; }
    return a;}

// T: O(log(n)), M: O(1)  
lng modPow(lng a, lng b, lng mod = INF64) {
    assert(mod > 0);
    a = (a % mod + mod) % mod;
    ulng ub;
    if (b < 0) {
        lng x, y;
        if (exGcd(a, mod, x, y) != 1) { return -1; }
        a = (x % mod + mod) % mod; ub = ulng(-(b + 1)) + 1;}
    else { ub = b; }
    lng res = 1 % mod;
    while (ub > 0) {
        if (ub & 1) { res = lll(res) * a % mod; }
        a = lll(a) * a % mod; ub >>= 1;}
    return res;}