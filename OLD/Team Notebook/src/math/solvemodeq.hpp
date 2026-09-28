#pragma once
#include "../1-Core/01-template.hpp"
#include "../5-Mathematics/02-exgcd.hpp"

// T: O(log(n)), M: O(1)
pair<lng, lng> solveModEq(lng a, lng b, lng m0) { // ax ≡ b (mod m0)
    a = (a % m0 + m0) % m0; b = (b % m0 + m0) % m0;
    lng x, y, g = exGcd(a, m0, x, y);
    if (b % g != 0) { return {-1, -1}; }
    lng m = m0 / g;
    x = lll(x) * (b / g) % m;
    if (x < 0) { x += m; }
    return {x, m};}