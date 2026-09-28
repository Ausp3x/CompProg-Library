#pragma once
#include "../1-Core/01-template.hpp"
#include "../5-Mathematics/02-exgcd.hpp"

// T: O(log(n)), M: O(1)
lng getInvE(lng n, lng mod) {
    n = (n % mod + mod) % mod;
    if (n == 0) { return -1; }
    lng x, y, g = exGcd(n, mod, x, y);
    if (g != 1) { return -1; }
    return (x % mod + mod) % mod;}

// T: O(n^(1/3)), M: O(log(n))
lng getInvP(lng n, lng mod) { // Pierce Expansion, mod must be prime
    return n <= 1 ? n : mod - lll(mod) / n * getInvP(mod % n, mod) % mod;}